// entities/npc.cpp
// NPC yang bisa diajak bicara dengan tombol E + kotak dialog.
#include "npc.h"
#include "core/config.h"
#include "world/collision.h"
#include "world/background.h"
#include "raymath.h"
#include <math.h>
#include <string.h>
#include <string>
#include <vector>

// -----------------------------------------------------
// Data NPC. Ubah / tambah di sini.
// offset = posisi yang diinginkan relatif ke pusat ruangan. Jika tempat itu
// tertutup pohon/rumah/air, posisi digeser otomatis ke tempat kosong terdekat.
// -----------------------------------------------------
struct NpcDef {
    const char* name;
    int         room;
    Vector2     offset;
    Color       color;
    const char* lines[NPC_MAX_LINES];
};

static const NpcDef NPC_DEFS[] = {
    // ---- Ruang 0: Desa Pesisir (aman, tidak ada musuh) ----
    { "Kepala Desa", 0, { -170.0f, -80.0f }, { 230, 190, 80, 255 }, {
        "Selamat datang di Desa Pesisir, pengembara.",
        "Desa ini masih aman. Tapi ruangan lain dipenuhi monster sejak segel kuno retak.",
        "Monster akan muncul begitu kamu melangkah masuk ke ruangan mereka. Bersiaplah!",
        "Klik kiri atau J untuk menyerang, tombol 1 sampai 4 untuk mengganti senjata.",
    } },
    { "Nelayan Tua", 0, { 180.0f, -50.0f }, { 90, 170, 230, 255 }, {
        "Ombak hari ini tenang... terlalu tenang.",
        "Dulu aku sering berlayar ke timur, melewati Kota Arunika.",
        "Kalau kamu butuh istirahat, tekan ESC. Jangan lupa simpan perjalananmu.",
    } },
    { "Pedagang", 0, { 30.0f, 190.0f }, { 200, 120, 200, 255 }, {
        "Mau tahu rahasia senjata? Dengarkan baik-baik.",
        "Tombak menjangkau jauh tapi sempit. Cocok untuk menjaga jarak.",
        "Pedang besar lambat, tapi sabetannya lebar dan pukulannya keras.",
        "Belati cepat. Pilih yang sesuai dengan gaya bertarungmu!",
    } },
    // ---- Ruang 1: Kota Arunika ----
    { "Penjaga Kota", 1, { -260.0f, -160.0f }, { 170, 170, 190, 255 }, {
        "Kota Arunika jatuh dalam satu malam.",
        "Aku sudah berjaga di sini sejak itu. Monster-monster itu tidak pernah lelah.",
        "Bersihkan jalanan ini, dan mungkin warga berani kembali.",
    } },
    // ---- Ruang 2: Hutan Terlarang ----
    { "Pertapa Hutan", 2, { 240.0f, -150.0f }, { 110, 200, 120, 255 }, {
        "Hutan ini tidak suka tamu.",
        "Pohon-pohon di sini menyimpan suara orang-orang yang tersesat.",
        "Jika kamu terluka, mundurlah. Keberanian tanpa akal hanya mengantar ke kubur.",
    } },
    // ---- Ruang 3: Reruntuhan ----
    { "Arkeolog", 3, { -230.0f, 140.0f }, { 210, 150, 90, 255 }, {
        "Hati-hati melangkah! Reruntuhan ini berumur ribuan tahun.",
        "Ukiran di dinding menyebut sebuah segel yang menahan para monster.",
        "Kalau segel itu benar-benar retak... kita semua dalam bahaya.",
    } },
};
static const int NPC_DEF_COUNT = (int)(sizeof(NPC_DEFS) / sizeof(NPC_DEFS[0]));

// -----------------------------------------------------
// Penempatan
// -----------------------------------------------------
static bool NpcSpotFree(Vector2 p, float r, Rectangle room, const Npc* placed, int placedCount, Vector2 playerSpawn)
{
    const float inset = WALL_T + r + 12.0f;
    if (p.x < room.x + inset || p.x > room.x + room.width - inset ||
        p.y < room.y + inset || p.y > room.y + room.height - inset) return false;

    for (int i = 0; i < GetWallCount(); i++)
        if (CircleVsAABB(p, r + 10.0f, GetWallRect(i), nullptr)) return false;

    int t = BackgroundTerrainAt(p);
    if (t == WATER || t == WALLSTONE) return false;

    for (int i = 0; i < placedCount; i++)
        if (Vector2Distance(p, placed[i].position) < r * 2.0f + 50.0f) return false;

    if (Vector2Distance(p, playerSpawn) < r + PLAYER_RADIUS + 60.0f) return false;
    return true;
}

void NpcInitAll(Npc* npcs, int* outCount, const World* w, Vector2 playerSpawn)
{
    int count = 0;
    for (int d = 0; d < NPC_DEF_COUNT && count < MAX_NPCS; d++) {
        const NpcDef* def = &NPC_DEFS[d];
        if (def->room < 0 || def->room >= w->roomCount) continue;

        Rectangle b = w->rooms[def->room].bounds;
        Vector2 want = { b.x + b.width * 0.5f + def->offset.x, b.y + b.height * 0.5f + def->offset.y };

        // Cari tempat kosong secara melingkar mulai dari posisi yang diinginkan.
        Vector2 pos = want;
        bool found = NpcSpotFree(want, NPC_RADIUS, b, npcs, count, playerSpawn);
        for (int ring = 1; ring <= 40 && !found; ring++) {
            float rad = ring * 14.0f;
            int steps = 8 + ring * 2;
            for (int k = 0; k < steps && !found; k++) {
                float a = (float)k / steps * 2.0f * PI;
                Vector2 c = { want.x + cosf(a) * rad, want.y + sinf(a) * rad };
                if (NpcSpotFree(c, NPC_RADIUS, b, npcs, count, playerSpawn)) { pos = c; found = true; }
            }
        }

        Npc* n = &npcs[count];
        memset(n, 0, sizeof(*n));
        n->position = pos;
        n->radius = NPC_RADIUS;
        n->room = def->room;
        n->name = def->name;
        n->color = def->color;
        for (int i = 0; i < NPC_MAX_LINES && def->lines[i]; i++) {
            n->lines[i] = def->lines[i];
            n->lineCount++;
        }
        n->talked = false;
        count++;
    }
    *outCount = count;
}

void NpcResolveCircle(const Npc* npcs, int count, Vector2* pos, float radius)
{
    for (int i = 0; i < count; i++) {
        Vector2 to = Vector2Subtract(*pos, npcs[i].position);
        float d = Vector2Length(to);
        float minD = radius + npcs[i].radius;
        if (d < minD) {
            Vector2 n = (d > 0.0001f) ? Vector2Scale(to, 1.0f / d) : Vector2{ 1, 0 };
            *pos = Vector2Add(npcs[i].position, Vector2Scale(n, minD));
        }
    }
}

int NpcFindTalkable(const Npc* npcs, int count, Vector2 playerPos)
{
    int best = -1;
    float bestD = 1e9f;
    for (int i = 0; i < count; i++) {
        float d = Vector2Distance(playerPos, npcs[i].position);
        if (d <= npcs[i].radius + PLAYER_RADIUS + NPC_TALK_RANGE && d < bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

// -----------------------------------------------------
// Render NPC (dunia)
// -----------------------------------------------------
static Color Darker(Color c, float f)
{
    return Color{ (unsigned char)(c.r * f), (unsigned char)(c.g * f), (unsigned char)(c.b * f), c.a };
}

void NpcDrawAll(const Npc* npcs, int count, int talkableIndex, int speakingIndex)
{
    float time = (float)GetTime();
    for (int i = 0; i < count; i++) {
        const Npc* n = &npcs[i];
        float bob = sinf(time * 2.2f + i * 1.7f) * 1.5f;
        Vector2 body = { n->position.x, n->position.y + bob };

        // bayangan
        DrawEllipse((int)n->position.x, (int)(n->position.y + n->radius - 2), n->radius * 0.95f, n->radius * 0.4f, Color{ 0, 0, 0, 70 });

        // badan + kepala
        DrawCircleV(body, n->radius, Darker(n->color, 0.55f));
        DrawCircleV(body, n->radius - 2.5f, n->color);
        DrawCircleV(Vector2{ body.x - n->radius * 0.3f, body.y - n->radius * 0.35f }, n->radius * 0.28f, Fade(WHITE, 0.35f));

        // nama
        int fs = 14;
        int tw = MeasureText(n->name, fs);
        int tx = (int)(n->position.x - tw / 2);
        int ty = (int)(n->position.y - n->radius - 24);
        DrawRectangle(tx - 4, ty - 2, tw + 8, fs + 4, Color{ 0, 0, 0, 140 });
        DrawText(n->name, tx, ty, fs, RAYWHITE);

        // tanda "!" untuk NPC yang belum pernah diajak bicara
        if (!n->talked && i != speakingIndex) {
            float hop = fabsf(sinf(time * 4.0f + i)) * 4.0f;
            DrawText("!", (int)(n->position.x - 3), (int)(ty - 24 - hop), 22, GOLD);
        }

        // prompt [E] Bicara
        if (i == talkableIndex) {
            const char* p = "[E] Bicara";
            int pfs = 16;
            int pw = MeasureText(p, pfs);
            int px = (int)(n->position.x - pw / 2);
            int py = (int)(n->position.y + n->radius + 10);
            DrawRectangleRounded(Rectangle{ (float)px - 8, (float)py - 4, (float)pw + 16, (float)pfs + 8 }, 0.4f, 6, Color{ 20, 20, 30, 210 });
            DrawRectangleRoundedLinesEx(Rectangle{ (float)px - 8, (float)py - 4, (float)pw + 16, (float)pfs + 8 }, 0.4f, 6, 1.5f, GOLD);
            DrawText(p, px, py, pfs, RAYWHITE);
        }
    }
}

// -----------------------------------------------------
// Dialog
// -----------------------------------------------------
void DialogueStart(DialogueState* d, Npc* npc, int npcIndex)
{
    d->npcIndex = npcIndex;
    d->line = 0;
    d->revealTime = 0.0f;
    npc->talked = true;
}

static int VisibleChars(const DialogueState* d)
{
    return (int)(d->revealTime * DIALOGUE_CHARS_PER_SEC);
}

bool DialogueUpdate(DialogueState* d, const Npc* npc, bool advancePressed, float dt)
{
    if (npc->lineCount <= 0) return false;
    d->revealTime += dt;

    if (advancePressed) {
        int total = (int)strlen(npc->lines[d->line]);
        if (VisibleChars(d) < total) {
            // tampilkan seluruh baris dulu
            d->revealTime = (float)total / DIALOGUE_CHARS_PER_SEC + 0.01f;
        }
        else {
            d->line++;
            d->revealTime = 0.0f;
            if (d->line >= npc->lineCount) return false;
        }
    }
    return true;
}

// Pecah teks menjadi beberapa baris sesuai lebar maksimum (font default raylib).
static std::vector<std::string> WrapText(const char* text, int fontSize, int maxWidth)
{
    std::vector<std::string> out;
    std::string cur, word;
    auto flushWord = [&]() {
        if (word.empty()) return;
        std::string test = cur.empty() ? word : cur + " " + word;
        if (!cur.empty() && MeasureText(test.c_str(), fontSize) > maxWidth) {
            out.push_back(cur);
            cur = word;
        }
        else cur = test;
        word.clear();
    };
    for (const char* c = text; *c; c++) {
        if (*c == ' ') flushWord();
        else word += *c;
    }
    flushWord();
    if (!cur.empty()) out.push_back(cur);
    return out;
}

void DialogueDraw(const DialogueState* d, const Npc* npc)
{
    if (npc->lineCount <= 0) return;

    const float boxW = 980.0f, boxH = 160.0f;
    Rectangle box = { (SCREEN_W - boxW) * 0.5f, SCREEN_H - boxH - 30.0f, boxW, boxH };

    DrawRectangleRounded(box, 0.08f, 8, Color{ 18, 18, 28, 235 });
    DrawRectangleRoundedLinesEx(box, 0.08f, 8, 2.0f, Color{ 200, 170, 90, 255 });

    // potret
    Vector2 portrait = { box.x + 70.0f, box.y + boxH * 0.5f };
    DrawCircleV(portrait, 44.0f, Darker(npc->color, 0.5f));
    DrawCircleV(portrait, 40.0f, npc->color);
    DrawCircleV(Vector2{ portrait.x - 12.0f, portrait.y - 14.0f }, 11.0f, Fade(WHITE, 0.35f));

    // nama
    float textX = box.x + 140.0f;
    DrawText(npc->name, (int)textX, (int)box.y + 16, 22, Color{ 240, 210, 120, 255 });

    // teks dengan efek mesin ketik
    const int fs = 20;
    std::vector<std::string> lines = WrapText(npc->lines[d->line], fs, (int)(box.width - 170.0f));
    int budget = VisibleChars(d);
    float y = box.y + 52.0f;
    for (size_t i = 0; i < lines.size() && budget > 0; i++) {
        std::string part = lines[i].substr(0, (size_t)budget < lines[i].size() ? (size_t)budget : lines[i].size());
        DrawText(part.c_str(), (int)textX, (int)y, fs, RAYWHITE);
        budget -= (int)lines[i].size() + 1;   // +1 untuk spasi yang dipotong saat wrap
        y += fs + 8;
    }

    // petunjuk + nomor baris
    int total = (int)strlen(npc->lines[d->line]);
    bool lineDone = VisibleChars(d) >= total;
    const char* hint = !lineDone ? "[E / Spasi] Percepat"
                     : (d->line + 1 < npc->lineCount ? "[E / Spasi] Lanjut" : "[E / Spasi] Selesai");
    int hw = MeasureText(hint, 14);
    float blink = lineDone ? (0.6f + 0.4f * sinf((float)GetTime() * 5.0f)) : 0.6f;
    DrawText(hint, (int)(box.x + box.width - hw - 18), (int)(box.y + box.height - 26), 14, Fade(LIGHTGRAY, blink));
    DrawText(TextFormat("%d/%d", d->line + 1, npc->lineCount), (int)textX, (int)(box.y + box.height - 26), 14, GRAY);
}
