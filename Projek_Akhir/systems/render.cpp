#include "render.h"
#include "core/config.h"
#include "data/weapons.h"
#include "data/animations.h"
#include "world/collision.h"
#include "world/background.h"
#include "entities/npc.h"
#include "entities/enemy.h"
#include "systems/pause_menu.h"
#include "raymath.h"
#include <math.h>

// Sprite karakter untuk masing-masing senjata (1=sword, 2=pistol, 3=keris).
static Texture2D playerSprites[3] = {};
static bool playerSpritesLoaded = false;

// Sprite sheet animasi (Pemgim1/2/3.png). Sheet aslinya 2048x2650 dengan banyak
// ruang kosong, jadi saat dimuat hanya area yang berisi frame yang diunggah ke
// GPU. sheetOffset = sudut kiri-atas area potong pada sheet asli.
static Texture2D animSheets[3] = {};
static Vector2   sheetOffset[3] = {};

// Posisi kaki & kepala sprite idle lama (dipakai agar frame animasi sejajar
// persis dengan pose idle, sehingga tidak ada "lompatan" saat berganti).
static const float CHAR_FOOT_Y = 34.0f;   // jarak kaki di bawah player.position.y
static const float CHAR_HEAD_DX = 1.0f;   // kepala idle bergeser 1 px ke belakang

static void LoadAnimSheet(int w) {
    const WeaponAnim* a = &WEAPON_ANIMS[w];
    Image img = LoadImage(a->sheetPath);
    if (img.data == NULL) return;

    // Gabungan (union) semua frame jalan + serang
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    const AnimClip* clips[2] = { &a->walk, &a->attack };
    for (int c = 0; c < 2; c++)
        for (int i = 0; i < clips[c]->count; i++) {
            Rectangle r = clips[c]->frames[i].src;
            x0 = fminf(x0, r.x); y0 = fminf(y0, r.y);
            x1 = fmaxf(x1, r.x + r.width); y1 = fmaxf(y1, r.y + r.height);
        }
    x0 = fmaxf(0.0f, x0 - 2.0f); y0 = fmaxf(0.0f, y0 - 2.0f);
    x1 = fminf((float)img.width, x1 + 2.0f); y1 = fminf((float)img.height, y1 + 2.0f);

    ImageCrop(&img, Rectangle{ x0, y0, x1 - x0, y1 - y0 });
    animSheets[w] = LoadTextureFromImage(img);
    sheetOffset[w] = Vector2{ x0, y0 };
    UnloadImage(img);
    if (animSheets[w].id != 0) SetTextureFilter(animSheets[w], TEXTURE_FILTER_POINT);
}

void RenderInit(void) {
    playerSprites[0] = LoadTexture("../Asset/player_weapon_1.png");
    playerSprites[1] = LoadTexture("../Asset/player_weapon_2.png");
    playerSprites[2] = LoadTexture("../Asset/player_weapon_3.png");
    for (int i = 0; i < 3; ++i) {
        if (playerSprites[i].id != 0)
            SetTextureFilter(playerSprites[i], TEXTURE_FILTER_POINT);
    }
    for (int i = 0; i < 3 && i < WEAPON_ANIM_COUNT; ++i) LoadAnimSheet(i);
    playerSpritesLoaded = true;
}

void RenderUnload(void) {
    if (!playerSpritesLoaded) return;
    for (int i = 0; i < 3; ++i) {
        if (playerSprites[i].id != 0) UnloadTexture(playerSprites[i]);
        if (animSheets[i].id != 0) UnloadTexture(animSheets[i]);
        animSheets[i] = Texture2D{};
    }
    playerSpritesLoaded = false;
}

// Gambar satu frame animasi dengan kaki & kepala sejajar pose idle.
static void DrawAnimFrame(int w, const AnimFrame* f, Vector2 pos, bool flip, Color tint) {
    const float sc = CHAR_SPRITE_SCALE;
    Rectangle src = f->src;
    src.x -= sheetOffset[w].x;
    src.y -= sheetOffset[w].y;
    float wpx = src.width, hpx = src.height;
    if (flip) src.width = -src.width;

    float anchor = flip ? (wpx - f->anchorX) : f->anchorX;
    float headDx = flip ? CHAR_HEAD_DX : -CHAR_HEAD_DX;
    Rectangle dst = { pos.x, pos.y, wpx * sc, hpx * sc };
    Vector2 origin = { anchor * sc - headDx, hpx * sc - CHAR_FOOT_Y };
    DrawTexturePro(animSheets[w], src, dst, origin, 0.0f, tint);
}

// Posisi moncong senjata di dunia (frame tembak), mengikuti arah hadap sprite.
static Vector2 MuzzleWorldPos(int w, Vector2 pos, bool flip) {
    const WeaponAnim* a = &WEAPON_ANIMS[w];
    const AnimFrame* f = &a->attack.frames[a->hitFrame];
    const float sc = CHAR_SPRITE_SCALE;
    float dx = (a->muzzle.x - f->anchorX) * sc;
    float headDx = flip ? CHAR_HEAD_DX : -CHAR_HEAD_DX;
    return Vector2{
        pos.x + headDx + (flip ? -dx : dx),
        pos.y + CHAR_FOOT_Y - (f->src.height - a->muzzle.y) * sc
    };
}

// Dipanggil di antara DisplayBeginFrame() / DisplayEndFrame() (lihat systems/display.h).
void RenderGame(const GameState* s) {
    ClearBackground(Color{ 24, 24, 32, 255 });

    BeginMode2D(s->cam);

    // Ground lingkungan (tileset seamless) + tembok batu + bayangan objek
    BackgroundDrawGround(&s->world);

    // Objek lingkungan yang berada DI BELAKANG player (alas di atas posisi player)
    BackgroundDrawDecor(s->player.position.y, false);

    // NPC (prompt [E] hanya saat bermain normal)
    int talkable = (s->mode == MODE_PLAY && s->player.hp > 0)
        ? NpcFindTalkable(s->npcs, s->npcCount, s->player.position) : -1;
    int speaking = (s->mode == MODE_DIALOGUE) ? s->dialogue.npcIndex : -1;
    NpcDrawAll(s->npcs, s->npcCount, talkable, speaking);

    // Enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy* e = &s->enemies[i];
        if (!e->alive) continue;

        // Animasi muncul: lingkaran sihir mengecil + badan memudar masuk
        if (e->spawnTimer > 0) {
            float t = 1.0f - Clamp(e->spawnTimer / ENEMY_SPAWN_DELAY, 0.0f, 1.0f);
            float ringR = e->radius + 26.0f * (1.0f - t);
            DrawRing(e->position, ringR - 3.0f, ringR, 0, 360, 32, Fade(Color{ 200, 60, 200, 255 }, 0.35f + 0.5f * t));
            DrawCircleV(e->position, e->radius * t, Fade(Color{ 200, 60, 60, 255 }, t));
            continue;
        }

        DrawCircleV(e->position, e->radius, e->hitFlash > 0 ? WHITE : Color{ 200, 60, 60, 255 });

        float ratio = Clamp(e->hp / e->maxHp, 0.0f, 1.0f);
        DrawRectangle((int)(e->position.x - 20), (int)(e->position.y - e->radius - 12), 40, 5, Color{ 60, 0, 0, 255 });
        DrawRectangle((int)(e->position.x - 20), (int)(e->position.y - e->radius - 12), (int)(40 * ratio), 5, Color{ 220, 50, 50, 255 });
    }

    // ------------------------------------------------------------------
    // Efek & animasi serangan
    // ------------------------------------------------------------------
    const Weapon* wp = &WEAPONS[s->player.weaponIndex];
    const Player* pl = &s->player;
    bool attacking = pl->swingTime > 0 && pl->attackWeapon >= 0 && pl->attackWeapon < WEAPON_ANIM_COUNT;
    float elapsed = pl->swingDuration - pl->swingTime;
    bool isGun = attacking && WEAPON_ANIMS[pl->attackWeapon].muzzle.x >= 0.0f;

    // Petunjuk area kena (melee). Sprite hanya menghadap kiri/kanan sedangkan
    // serangan mengikuti mouse 360 derajat, jadi area kena digambar tipis tepat
    // setelah frame hit agar pemain tahu arah tebasan/tusukan yang sebenarnya.
    if (attacking && !isGun && elapsed >= pl->hitTime) {
        const Weapon* aw = &WEAPONS[pl->attackWeapon];
        float span = pl->swingDuration - pl->hitTime;
        float a = span > 0 ? Clamp(1.0f - (elapsed - pl->hitTime) / span, 0.0f, 1.0f) : 0.0f;
        float ang = atan2f(pl->attackDir.y, pl->attackDir.x) * RAD2DEG;
        float half = aw->arc * RAD2DEG * 0.5f;
        Color c = aw->color;
        c.a = (unsigned char)(80 * a);
        DrawCircleSector(pl->position, aw->range, ang - half, ang + half, 32, c);
    }

    // Player: pose idle = sprite player_weapon_N.png (seperti sebelumnya);
    // jalan / serang = frame dari sprite sheet Pemgim1..3.png.
    // Semua sprite menghadap ke kanan, sehingga dibalik saat player menghadap kiri.
    int spriteIndex = pl->weaponIndex;
    if (spriteIndex < 0 || spriteIndex > 2) spriteIndex = 0;
    Color tint = (pl->iframes > 0 && ((int)(pl->iframes * 20) % 2))
        ? Color{ 200, 255, 200, 255 } : WHITE;

    if (attacking && animSheets[pl->attackWeapon].id != 0) {
        // Arah hadap dikunci ke arah serangan selama animasi berjalan
        int w = pl->attackWeapon;
        bool flip = pl->attackDir.x < 0.0f;
        int fi = AnimAttackFrameAt(w, elapsed);
        DrawAnimFrame(w, &WEAPON_ANIMS[w].attack.frames[fi], pl->position, flip, tint);
    }
    else if (pl->moving && pl->hp > 0 && animSheets[spriteIndex].id != 0 &&
             WEAPON_ANIMS[spriteIndex].walk.count > 0) {
        const AnimClip* walk = &WEAPON_ANIMS[spriteIndex].walk;
        int fi = (int)(pl->walkTime * WALK_ANIM_FPS) % walk->count;
        DrawAnimFrame(spriteIndex, &walk->frames[fi], pl->position, pl->facing.x < 0.0f, tint);
    }
    else if (playerSpritesLoaded && playerSprites[spriteIndex].id != 0) {
        Texture2D tex = playerSprites[spriteIndex];
        Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        if (pl->facing.x < 0.0f) {
            src.width = -src.width;
        }
        Rectangle dst = {
            pl->position.x,
            pl->position.y,
            54.0f,
            72.0f
        };
        DrawTexturePro(tex, src, dst, Vector2{ 27.0f, 36.0f }, 0.0f, tint);
    }
    else {
        DrawCircleV(pl->position, PLAYER_RADIUS, GREEN);
    }

    // Jejak peluru pistol: keluar dari moncong (sumber muzzle flash pada sprite)
    // menuju arah bidikan sepanjang jangkauan senjata. Tidak ada aset peluru di
    // folder Asset, jadi peluru digambar secara prosedural.
    if (pl->shotFx > 0 && pl->attackWeapon >= 0 && pl->attackWeapon < WEAPON_ANIM_COUNT &&
        WEAPON_ANIMS[pl->attackWeapon].muzzle.x >= 0.0f) {
        const Weapon* aw = &WEAPONS[pl->attackWeapon];
        bool flip = pl->attackDir.x < 0.0f;
        Vector2 from = MuzzleWorldPos(pl->attackWeapon, pl->position, flip);
        Vector2 to = Vector2Add(pl->position, Vector2Scale(pl->attackDir, aw->range));
        float t = 1.0f - Clamp(pl->shotFx / SHOT_FX_DURATION, 0.0f, 1.0f);   // 0 -> 1
        float head = Clamp(t * 1.6f, 0.0f, 1.0f);           // kepala peluru melesat cepat
        float tail = Clamp(head - 0.35f, 0.0f, 1.0f);
        Vector2 ph = Vector2Lerp(from, to, head);
        Vector2 pt = Vector2Lerp(from, to, tail);
        DrawLineEx(from, ph, 1.5f, Fade(Color{ 255, 230, 150, 255 }, 0.35f * (1.0f - t)));
        DrawLineEx(pt, ph, 3.0f, Fade(aw->color, 0.9f * (1.0f - t * 0.5f)));
        DrawCircleV(ph, 2.5f, Fade(Color{ 255, 250, 220, 255 }, 1.0f - t * 0.6f));
    }

    // Garis Aiming (dari player ke arah mouse)
    Vector2 aimLineEnd = Vector2Add(s->player.position, Vector2Scale(s->player.facing, 60.0f));
    DrawLineEx(s->player.position, aimLineEnd, 2.0f, Fade(LIME, 0.5f));

    // Objek lingkungan DI DEPAN player (pohon/gedung menutupi player dari belakang)
    BackgroundDrawDecor(s->player.position.y, true);

    EndMode2D();

    // --- CROSSHAIR (Digambar setelah EndMode2D agar menempel di layar) ---
    if (s->mode == MODE_PLAY) {
        Vector2 mouseScreen = GetMousePosition();
        DrawCircleLines((int)mouseScreen.x, (int)mouseScreen.y, 8.0f, LIGHTGRAY);
        DrawLine((int)mouseScreen.x - 12, (int)mouseScreen.y, (int)mouseScreen.x - 4, (int)mouseScreen.y, LIGHTGRAY);
        DrawLine((int)mouseScreen.x + 4, (int)mouseScreen.y, (int)mouseScreen.x + 12, (int)mouseScreen.y, LIGHTGRAY);
        DrawLine((int)mouseScreen.x, (int)mouseScreen.y - 12, (int)mouseScreen.x, (int)mouseScreen.y - 4, LIGHTGRAY);
        DrawLine((int)mouseScreen.x, (int)mouseScreen.y + 4, (int)mouseScreen.x, (int)mouseScreen.y + 12, LIGHTGRAY);
    }

    // HUD
    DrawRectangle(10, 10, 280, 112, Color{ 0, 0, 0, 180 });
    DrawText(TextFormat("HP: %.0f / %.0f", s->player.hp, s->player.maxHp), 20, 20, 20, RAYWHITE);
    DrawRectangle(20, 44, 240, 12, Color{ 60, 0, 0, 255 });
    DrawRectangle(20, 44, (int)(240 * Clamp(s->player.hp / s->player.maxHp, 0.0f, 1.0f)), 12, Color{ 220, 50, 50, 255 });
    DrawText(TextFormat("[%d] %s", s->player.weaponIndex + 1, wp->name), 20, 62, 18, wp->color);
    int enemiesHere = EnemyCountInRoom(s->enemies, MAX_ENEMIES, &s->world, s->world.currentRoom);
    DrawText(TextFormat("Musuh di ruangan: %d", enemiesHere), 20, 88, 16,
        enemiesHere > 0 ? Color{ 240, 120, 120, 255 } : Color{ 140, 220, 140, 255 });

    // Room title
    if (s->world.roomTitleTimer > 0 && s->world.currentRoom >= 0) {
        float fadeIn = Clamp((2.0f - s->world.roomTitleTimer) / 0.3f, 0.0f, 1.0f);
        float fadeOut = Clamp(s->world.roomTitleTimer / 0.4f, 0.0f, 1.0f);
        Color c = RAYWHITE;
        c.a = (unsigned char)(255 * fadeIn * fadeOut);
        const char* rn = s->world.rooms[s->world.currentRoom].name;
        int sz = 36;
        DrawText(rn, SCREEN_W / 2 - MeasureText(rn, sz) / 2, 40, sz, c);
    }

    if (s->player.hp <= 0) {
        DrawText("DEAD", SCREEN_W / 2 - 50, SCREEN_H / 2 - 20, 40, RED);
    }

    BackgroundDrawWarning();
    DrawFPS(SCREEN_W - 90, 10);

    // Kotak dialog NPC
    if (s->mode == MODE_DIALOGUE)
        DialogueDraw(&s->dialogue, &s->npcs[s->dialogue.npcIndex]);

    // Menu pause (di atas semuanya)
    if (s->mode == MODE_PAUSE)
        PauseMenuDraw(&s->pause);

    // Toast (pesan singkat, mis. "Game tersimpan")
    if (s->toastTimer > 0) {
        float a = Clamp(s->toastTimer / 0.4f, 0.0f, 1.0f);
        int fs = 20;
        int tw = MeasureText(s->toast, fs);
        int tx = SCREEN_W / 2 - tw / 2;
        int ty = (s->mode == MODE_PAUSE) ? SCREEN_H / 2 + 250 : 100;   // di bawah panel pause
        DrawRectangle(tx - 14, ty - 8, tw + 28, fs + 16, Fade(Color{ 20, 20, 30, 255 }, 0.85f * a));
        DrawText(s->toast, tx, ty, fs, Fade(Color{ 240, 210, 120, 255 }, a));
    }
}