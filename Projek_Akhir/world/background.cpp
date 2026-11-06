// =====================================================
// world/background.cpp
// ADVENTURE WORLD - ARUNIKA : background lingkungan pixel art
//
// Sistem ini memindahkan generator "Seamless Ground" dari arunika_fixed.cpp
// ke dalam game (Projek_Akhir) dan menyesuaikannya dengan dunia 2x2 ruangan:
//
//  1. Ground dibangun per piksel dari patch tekstur tileset.png (koordinat
//     patch sama persis dengan arunika_fixed.cpp), wrap + dithered offset-blend,
//     tanpa resampling -> satu texture utuh, tidak ada kotak/garis grid.
//  2. Tiap ruangan punya tema (bioma):
//        ruang 0 Desa Pesisir   : laut, pantai pasir, sungai, alun-alun tanah
//        ruang 1 Kota Arunika   : trotoar ubin, jalan aspal + marka
//        ruang 2 Hutan Terlarang: lantai hutan gelap, lumut, kolam
//        ruang 3 Reruntuhan     : batu cobble berlumut
//     Batas alami memakai domain warping noise sehingga organik.
//  3. Jalan setapak menghubungkan pusat ruangan ke setiap pintu, sehingga
//     jalan menyambung mulus antar-ruangan melewati celah pintu.
//  4. Dinding ruangan (collision yang sudah ada) dirender sebagai tembok batu
//     dari patch cobble tileset, lengkap dengan highlight dan bayangan.
//  5. Objek lingkungan (pohon, semak, rumah, perabot kota, batu, reruntuhan)
//     diekstrak OTOMATIS per-objek dari panel atlas (connected component pada
//     alpha setelah CleanPanelBackground), sehingga yang ditempel adalah objek
//     satuan, bukan potongan panel berbentuk kotak. Objek disebar per bioma
//     dengan aturan terrain, diurutkan menurut Y (depth) dan diberi bayangan.
//  6. Objek besar mendapat collision kecil di alasnya (memakai AddWall yang
//     sudah ada). Matikan dengan DECOR_COLLISION = false.
// =====================================================

#include "background.h"
#include "collision.h"
#include "core/config.h"
#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <queue>
#include <string>
#include <algorithm>

using namespace std;

// -----------------------------------------------------
// Konfigurasi
// -----------------------------------------------------
static const float GS = 2.0f;          // 1 piksel tileset = 2 unit dunia (skala bulat -> piksel tajam)
static const int   T  = 44;            // ukuran tile (piksel gambar) sebagai satuan frekuensi noise
static const bool  DECOR_COLLISION = true;
static const bool  DECOR_SHADOW    = false;   // true = bayangan elips tambahan di bawah objek

static const float WORLD_W = (float)(ROOM_COLS * ROOM_W);
static const float WORLD_H = (float)(ROOM_ROWS * ROOM_H);

enum Biome { BIOME_COAST = 0, BIOME_CITY, BIOME_FOREST, BIOME_RUIN };

enum DecorCat {
    CAT_COAST = 0,     // jembatan, dermaga, perahu
    CAT_ROCK,          // pulau batu & tebing pantai
    CAT_WATERROCK,     // karang pemecah di air
    CAT_TREE,          // pepohonan & jamur
    CAT_VILLAGE,       // objek desa pesisir
    CAT_FORESTOBJ,     // objek hutan terlarang
    CAT_CITY,          // gedung & perabot kota
    CAT_MISC,          // objek lainnya
    CAT_RUIN,          // reruntuhan
    CAT_PLANT,         // semak, bunga, rumput, tunggul (objek hijau kecil)
    CAT_HOUSE,         // rumah & kios desa
    CAT_BUILDING,      // gedung kota (disusun berderet di tepi jalan)
    CAT_COUNT
};

static const char* CAT_NAME[CAT_COUNT] = {
    "coast", "rock", "waterrock", "tree", "village", "forestobj", "city", "misc", "ruin", "plant", "house", "building"
};

// -----------------------------------------------------
// State modul
// -----------------------------------------------------
struct DecorSprite { Rectangle src; int cat; float fill; Rectangle atlas; Color mean; };
struct DecorInst   { int sprite; Vector2 base; bool flip; bool shadow; };

static bool               s_ready = false;
static bool               s_failed = false;
static Texture2D          s_ground = { 0 };
static Texture2D          s_decorTex = { 0 };
static int                s_imgW = 0, s_imgH = 0;
static vector<uint8_t>    s_terr;
static vector<DecorSprite> s_sprites;
static vector<DecorInst>  s_decor;

// =====================================================
// BAGIAN 1 - Fungsi dasar dari arunika_fixed.cpp
// =====================================================

// Hash deterministik untuk variasi tile
static unsigned Hash(int x, int y)
{
    unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Hash dengan seed, hasil 0..1
static float Hash01(int x, int y, unsigned seed)
{
    unsigned h = Hash(x + (int)(seed * 7919u), y - (int)(seed * 104729u));
    h ^= seed * 0x9E3779B1u;
    h = (h ^ (h >> 15)) * 0x2C1B3C6Du;
    h ^= h >> 12;
    return (float)(h & 0xFFFFFFu) / 16777215.0f;
}

static float SmoothFade(float t) { return t * t * (3.0f - 2.0f * t); }

static float ValueNoise(float x, float y, unsigned seed)
{
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float tx = SmoothFade(x - (float)xi), ty = SmoothFade(y - (float)yi);
    float a = Hash01(xi, yi, seed),     b = Hash01(xi + 1, yi, seed);
    float c = Hash01(xi, yi + 1, seed), d = Hash01(xi + 1, yi + 1, seed);
    return Lerp(Lerp(a, b, tx), Lerp(c, d, tx), ty);
}

static float Fbm(float x, float y, unsigned seed, int octaves = 3)
{
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    for (int i = 0; i < octaves; i++)
    {
        sum += ValueNoise(x, y, seed + (unsigned)i * 31u) * amp;
        norm += amp;
        x *= 2.03f; y *= 2.03f; amp *= 0.5f;
    }
    return sum / norm;
}

static float Smoothstep(float e0, float e1, float x)
{
    float t = Clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Medan noise frekuensi rendah di-cache pada grid kasar lalu diinterpolasi bilinear.
struct NoiseField
{
    int gw = 0, gh = 0, step = 4;
    vector<float> v;

    void Build(int w, int h, int stepPx, float tileSize, float freq, unsigned seed, int octaves)
    {
        step = stepPx;
        gw = w / step + 2;
        gh = h / step + 2;
        v.resize((size_t)gw * gh);
        for (int gy = 0; gy < gh; gy++)
            for (int gx = 0; gx < gw; gx++)
            {
                float u = (gx * step) / tileSize, t = (gy * step) / tileSize;
                v[(size_t)gy * gw + gx] = Fbm(u * freq, t * freq, seed, octaves);
            }
    }

    float At(int px, int py) const
    {
        float fx = (float)px / step, fy = (float)py / step;
        int x0 = (int)fx, y0 = (int)fy;
        float tx = fx - x0, ty = fy - y0;
        x0 = min(max(x0, 0), gw - 2); y0 = min(max(y0, 0), gh - 2);
        float a = v[(size_t)y0 * gw + x0],       b = v[(size_t)y0 * gw + x0 + 1];
        float c = v[(size_t)(y0 + 1) * gw + x0], d = v[(size_t)(y0 + 1) * gw + x0 + 1];
        return Lerp(Lerp(a, b, tx), Lerp(c, d, tx), ty);
    }
};

// Patch tekstur seamless dari tileset (sama dengan arunika_fixed.cpp)
struct TexPatch
{
    int w = 0, h = 0;
    unsigned salt = 0;
    bool blend = true;
    vector<Color> px;

    void Load(const Image& img, Rectangle r, unsigned s, float flatten = 0.0f, bool removeFoam = false)
    {
        salt = s;
        w = (int)r.width; h = (int)r.height;
        px.resize((size_t)w * h);
        const Color* src = (const Color*)img.data;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                int sx = std::clamp((int)r.x + x, 0, img.width - 1);
                int sy = std::clamp((int)r.y + y, 0, img.height - 1);
                px[(size_t)y * w + x] = src[(size_t)sy * img.width + sx];
            }

        if (flatten <= 0.0f && !removeFoam) return;

        const int R = max(2, min(w, h) / 4);
        vector<float> a((size_t)w * h * 3), b((size_t)w * h * 3);
        for (size_t i = 0; i < px.size(); i++)
        {
            a[i * 3] = px[i].r; a[i * 3 + 1] = px[i].g; a[i * 3 + 2] = px[i].b;
        }
        auto blurPass = [&](const vector<float>& in, vector<float>& o, bool horiz)
        {
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                {
                    float s3[3] = { 0, 0, 0 };
                    for (int k = -R; k <= R; k++)
                    {
                        int xx = horiz ? (((x + k) % w) + w) % w : x;
                        int yy = horiz ? y : (((y + k) % h) + h) % h;
                        const float* p = &in[((size_t)yy * w + xx) * 3];
                        s3[0] += p[0]; s3[1] += p[1]; s3[2] += p[2];
                    }
                    float* q = &o[((size_t)y * w + x) * 3];
                    q[0] = s3[0] / (2 * R + 1); q[1] = s3[1] / (2 * R + 1); q[2] = s3[2] / (2 * R + 1);
                }
        };
        blurPass(a, b, true);
        vector<float> blur((size_t)w * h * 3);
        blurPass(b, blur, false);

        float mean[3] = { 0, 0, 0 };
        for (size_t i = 0; i < px.size(); i++)
            for (int c = 0; c < 3; c++) mean[c] += a[i * 3 + c];
        for (int c = 0; c < 3; c++) mean[c] /= (float)px.size();
        float meanLum = (mean[0] + mean[1] + mean[2]) / 3.0f;

        for (size_t i = 0; i < px.size(); i++)
        {
            float c3[3] = { a[i * 3], a[i * 3 + 1], a[i * 3 + 2] };
            if (removeFoam && (c3[0] + c3[1] + c3[2]) / 3.0f > meanLum + 26.0f)
            {
                float j = (Hash01((int)i, 3, s) - 0.5f) * 8.0f;
                c3[0] = blur[i * 3] + j; c3[1] = blur[i * 3 + 1] + j; c3[2] = blur[i * 3 + 2] + j;
            }
            for (int c = 0; c < 3; c++)
                c3[c] += (mean[c] - blur[i * 3 + c]) * flatten;
            px[i] = { (unsigned char)Clamp(c3[0], 0.0f, 255.0f),
                      (unsigned char)Clamp(c3[1], 0.0f, 255.0f),
                      (unsigned char)Clamp(c3[2], 0.0f, 255.0f), 255 };
        }
    }

    Color GetV(int x, int y, float n) const
    {
        if (n < 0.36f) return Get(x, y);
        if (n < 0.68f) return Get(-x + 13, y + 7);
        return Get(x + 5, -y + 11);
    }

    Color Get(int x, int y) const
    {
        int ix = ((x % w) + w) % w;
        int iy = ((y % h) + h) % h;

        if (!blend) return px[(size_t)iy * w + ix];

        float mx = fabsf(2.0f * ix / (float)(w - 1) - 1.0f);
        float my = fabsf(2.0f * iy / (float)(h - 1) - 1.0f);
        float m = Smoothstep(0.45f, 1.0f, max(mx, my));

        if (Hash01(x, y, salt) < m)
        {
            ix = (ix + w / 2) % w;
            iy = (iy + h / 2) % h;
        }
        return px[(size_t)iy * w + ix];
    }
};

static float ColorDistance(Color a, Color b)
{
    float dr = (float)a.r - (float)b.r;
    float dg = (float)a.g - (float)b.g;
    float db = (float)a.b - (float)b.b;
    return sqrtf(dr * dr + dg * dg + db * db);
}

static Color Shade(Color c, float r, float g, float b)
{
    return {
        (unsigned char)Clamp(c.r * r, 0.0f, 255.0f),
        (unsigned char)Clamp(c.g * g, 0.0f, 255.0f),
        (unsigned char)Clamp(c.b * b, 0.0f, 255.0f),
        255
    };
}

// Menghapus background kotak abu-abu gelap panel atlas (flood-fill dari tepi)
static void CleanPanelBackground(Image* img, Rectangle rect, float tolerance = 34.0f)
{
    int x0 = (int)Clamp(rect.x, 0, (float)img->width - 1);
    int y0 = (int)Clamp(rect.y, 0, (float)img->height - 1);
    int x1 = (int)Clamp(rect.x + rect.width - 1, 0, (float)img->width - 1);
    int y1 = (int)Clamp(rect.y + rect.height - 1, 0, (float)img->height - 1);

    int w = x1 - x0 + 1;
    int h = y1 - y0 + 1;
    if (w <= 0 || h <= 0) return;

    Color* pixels = (Color*)img->data;

    Color bgSamples[4] = {
        pixels[y0 * img->width + x0],
        pixels[y0 * img->width + x1],
        pixels[y1 * img->width + x0],
        pixels[y1 * img->width + x1]
    };

    Color defaultBg = { 61, 64, 71, 255 };

    auto IsNeutral = [](Color c) -> bool {
        int mx = max((int)c.r, max((int)c.g, (int)c.b));
        int mn = min((int)c.r, min((int)c.g, (int)c.b));
        return (mx - mn) <= 16;
    };

    auto IsBgColor = [&](Color c) -> bool {
        if (c.a == 0) return true;
        if (!IsNeutral(c)) return false;
        if (ColorDistance(c, defaultBg) <= tolerance) return true;
        for (int i = 0; i < 4; i++)
        {
            if (bgSamples[i].a > 0 && bgSamples[i].r < 95 && bgSamples[i].g < 95 && bgSamples[i].b < 105)
            {
                if (ColorDistance(c, bgSamples[i]) <= tolerance) return true;
            }
        }
        return false;
    };

    vector<uint8_t> visited((size_t)w * h, 0);
    queue<pair<int, int>> q;

    for (int x = x0; x <= x1; x++)
    {
        if (IsBgColor(pixels[y0 * img->width + x])) { q.push({ x, y0 }); visited[(size_t)(x - x0)] = 1; }
        if (IsBgColor(pixels[y1 * img->width + x])) { q.push({ x, y1 }); visited[(size_t)(h - 1) * w + (x - x0)] = 1; }
    }
    for (int y = y0; y <= y1; y++)
    {
        if (IsBgColor(pixels[y * img->width + x0])) { q.push({ x0, y }); visited[(size_t)(y - y0) * w] = 1; }
        if (IsBgColor(pixels[y * img->width + x1])) { q.push({ x1, y }); visited[(size_t)(y - y0) * w + (w - 1)] = 1; }
    }

    const int dx[8] = { -1, 1, 0, 0, -1, -1, 1, 1 };
    const int dy[8] = { 0, 0, -1, 1, -1, 1, -1, 1 };

    while (!q.empty())
    {
        auto [cx, cy] = q.front();
        q.pop();
        pixels[cy * img->width + cx] = BLANK;

        for (int d = 0; d < 8; d++)
        {
            int nx = cx + dx[d];
            int ny = cy + dy[d];
            if (nx >= x0 && nx <= x1 && ny >= y0 && ny <= y1)
            {
                size_t idx = (size_t)(ny - y0) * w + (nx - x0);
                if (!visited[idx] && IsBgColor(pixels[ny * img->width + nx]))
                {
                    visited[idx] = 1;
                    q.push({ nx, ny });
                }
            }
        }
    }

    // Celah tertutup (mis. di bawah lengkungan jembatan) hanya dihapus jika
    // AREANYA BESAR. Area abu-abu kecil di dalam objek (jendela, pintu, bayangan
    // gedung) adalah bagian gambar dan dipertahankan agar style tileset tidak berubah.
    // Garis outline gelap objek juga tidak lagi dikikis.
    {
        vector<uint8_t> seen((size_t)w * h, 0);
        vector<int> comp;
        for (int sy = y0; sy <= y1; sy++)
            for (int sx = x0; sx <= x1; sx++)
            {
                size_t s0 = (size_t)(sy - y0) * w + (sx - x0);
                Color c0 = pixels[sy * img->width + sx];
                if (seen[s0] || c0.a == 0 || !IsNeutral(c0) || ColorDistance(c0, defaultBg) > 18.0f) continue;
                comp.clear();
                comp.push_back((int)s0); seen[s0] = 1;
                for (size_t k = 0; k < comp.size(); k++)
                {
                    int cx = comp[k] % w + x0, cy = comp[k] / w + y0;
                    for (int d = 0; d < 4; d++)
                    {
                        int nx = cx + dx[d], ny = cy + dy[d];
                        if (nx < x0 || nx > x1 || ny < y0 || ny > y1) continue;
                        size_t n = (size_t)(ny - y0) * w + (nx - x0);
                        Color cn = pixels[ny * img->width + nx];
                        if (!seen[n] && cn.a > 0 && IsNeutral(cn) && ColorDistance(cn, defaultBg) <= 18.0f)
                        { seen[n] = 1; comp.push_back((int)n); }
                    }
                }
                if (comp.size() >= 400)
                    for (int id : comp) pixels[(id / w + y0) * img->width + (id % w + x0)] = BLANK;
            }
    }
}

// Menghapus petak air gelap di panel "Karang Pemecah"
static void KeyOutPanelWater(Image* img, Rectangle rect)
{
    int x0 = (int)Clamp(rect.x, 0, (float)img->width - 1);
    int y0 = (int)Clamp(rect.y, 0, (float)img->height - 1);
    int x1 = (int)Clamp(rect.x + rect.width - 1, 0, (float)img->width - 1);
    int y1 = (int)Clamp(rect.y + rect.height - 1, 0, (float)img->height - 1);

    Color* p = (Color*)img->data;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
        {
            Color& c = p[y * img->width + x];
            if (c.a == 0) continue;
            int r = c.r, g = c.g, b = c.b;
            bool bluish = (b - r > 16) && (g - r > 6);
            bool notFoam = (r + g + b) < 420;
            bool darkTeal = r < 115;
            if (bluish && notFoam && darkTeal) c = BLANK;
        }
}

// Elevasi visual untuk bayangan tepi
static int TerrainRank(int t)
{
    switch (t)
    {
    case WATER:   return 0;
    case ASPHALT: return 1;
    case SAND: case DIRT: case ROAD: case RUIN: return 2;
    case CITY:    return 3;
    case WALLSTONE: return 5;
    default:      return 4;
    }
}

// Distance transform chamfer 3-4 (hasil /3 ~ jarak piksel)
static void ChamferDistance(vector<int>& d, int W, int H)
{
    const int INF = 1 << 28;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
        {
            int& c = d[(size_t)y * W + x];
            if (c == 0) continue;
            int best = INF;
            if (x > 0)               best = min(best, d[(size_t)y * W + x - 1] + 3);
            if (y > 0)               best = min(best, d[(size_t)(y - 1) * W + x] + 3);
            if (x > 0 && y > 0)      best = min(best, d[(size_t)(y - 1) * W + x - 1] + 4);
            if (x < W - 1 && y > 0)  best = min(best, d[(size_t)(y - 1) * W + x + 1] + 4);
            c = min(c, best);
        }
    for (int y = H - 1; y >= 0; y--)
        for (int x = W - 1; x >= 0; x--)
        {
            int& c = d[(size_t)y * W + x];
            if (c == 0) continue;
            int best = INF;
            if (x < W - 1)               best = min(best, d[(size_t)y * W + x + 1] + 3);
            if (y < H - 1)               best = min(best, d[(size_t)(y + 1) * W + x] + 3);
            if (x < W - 1 && y < H - 1)  best = min(best, d[(size_t)(y + 1) * W + x + 1] + 4);
            if (x > 0 && y < H - 1)      best = min(best, d[(size_t)(y + 1) * W + x - 1] + 4);
            c = min(c, best);
        }
}

// =====================================================
// BAGIAN 2 - Tata letak dunia per ruangan
// =====================================================
struct Polyline
{
    vector<Vector2> pts;
    Rectangle box;      // bounding box (diperbesar) untuk early-out
};

struct RoomLayout
{
    int   biome = BIOME_COAST;
    int   rx = 0, ry = 0;
    float x0 = 0, y0 = 0;           // pojok kiri atas ruangan (dunia)
    Vector2 hub = { 0, 0 };         // pusat ruangan, titik temu jalan
    bool  doorN = false, doorS = false, doorW = false, doorE = false;
    vector<Polyline> paths;         // jalan tanah (non-kota)
    Polyline river;                 // sungai (bioma pesisir)
    bool  hasRiver = false;
    // Kota
    Rectangle pave = { 0 };         // area trotoar
    float roadHx0 = 0, roadHx1 = 0; // bentang jalan horizontal
    float roadVy0 = 0, roadVy1 = 0; // bentang jalan vertikal
};

static vector<RoomLayout> s_layout;

static const float PATH_HW  = 50.0f;   // setengah lebar jalan tanah (dunia)
static const float ROAD_HW  = 70.0f;   // setengah lebar jalan aspal (= setengah pintu)

static int RoomIndexAt(float x, float y)
{
    int rx = std::clamp((int)floorf(x / ROOM_W), 0, ROOM_COLS - 1);
    int ry = std::clamp((int)floorf(y / ROOM_H), 0, ROOM_ROWS - 1);
    return ry * ROOM_COLS + rx;
}

static Polyline MakeCurve(Vector2 a, Vector2 b, float bend, Vector2 extendDir)
{
    Polyline p;
    Vector2 d = Vector2Subtract(b, a);
    float len = Vector2Length(d);
    Vector2 n = (len > 0.001f) ? Vector2{ -d.y / len, d.x / len } : Vector2{ 0, 0 };
    const int N = 16;
    for (int i = 0; i <= N; i++)
    {
        float t = (float)i / N;
        float off = sinf(PI * t) * bend * len;
        // bentuk S halus agar tidak seperti busur sempurna
        off += sinf(2.0f * PI * t) * bend * 0.35f * len;
        p.pts.push_back(Vector2Add(Vector2Add(a, Vector2Scale(d, t)), Vector2Scale(n, off)));
    }
    // diperpanjang menembus celah pintu agar menyambung dengan ruangan sebelah
    p.pts.push_back(Vector2Add(b, Vector2Scale(extendDir, WALL_T * 1.5f)));

    float mnx = 1e9f, mny = 1e9f, mxx = -1e9f, mxy = -1e9f;
    for (auto& q : p.pts) { mnx = min(mnx, q.x); mny = min(mny, q.y); mxx = max(mxx, q.x); mxy = max(mxy, q.y); }
    const float pad = 140.0f;
    p.box = { mnx - pad, mny - pad, (mxx - mnx) + 2 * pad, (mxy - mny) + 2 * pad };
    return p;
}

static float DistToPolyline(const Polyline& p, float x, float y)
{
    if (x < p.box.x || y < p.box.y || x > p.box.x + p.box.width || y > p.box.y + p.box.height) return 1e9f;
    float best = 1e18f;
    for (size_t i = 0; i + 1 < p.pts.size(); i++)
    {
        Vector2 a = p.pts[i], b = p.pts[i + 1];
        float vx = b.x - a.x, vy = b.y - a.y;
        float wx = x - a.x,  wy = y - a.y;
        float l2 = vx * vx + vy * vy;
        float t = (l2 > 0.0001f) ? Clamp((wx * vx + wy * vy) / l2, 0.0f, 1.0f) : 0.0f;
        float dx = wx - vx * t, dy = wy - vy * t;
        best = min(best, dx * dx + dy * dy);
    }
    return sqrtf(best);
}

static void BuildLayout()
{
    s_layout.clear();
    for (int i = 0; i < ROOM_COLS * ROOM_ROWS; i++)
    {
        RoomLayout L;
        L.rx = i % ROOM_COLS; L.ry = i / ROOM_COLS;
        L.biome = i % 4;
        L.x0 = (float)(L.rx * ROOM_W); L.y0 = (float)(L.ry * ROOM_H);
        L.hub = { L.x0 + ROOM_W * 0.5f, L.y0 + ROOM_H * 0.5f };
        L.doorN = L.ry > 0;  L.doorS = L.ry < ROOM_ROWS - 1;
        L.doorW = L.rx > 0;  L.doorE = L.rx < ROOM_COLS - 1;

        // Jalan tanah: pusat ruangan -> tiap pintu (melengkung)
        float sgn = (Hash(i, 7) & 1) ? 1.0f : -1.0f;
        if (L.doorE) L.paths.push_back(MakeCurve(L.hub, { L.x0 + ROOM_W, L.hub.y }, 0.10f * sgn, { 1, 0 }));
        if (L.doorW) L.paths.push_back(MakeCurve(L.hub, { L.x0, L.hub.y }, -0.10f * sgn, { -1, 0 }));
        if (L.doorS) L.paths.push_back(MakeCurve(L.hub, { L.hub.x, L.y0 + ROOM_H }, -0.12f * sgn, { 0, 1 }));
        if (L.doorN) L.paths.push_back(MakeCurve(L.hub, { L.hub.x, L.y0 }, 0.12f * sgn, { 0, -1 }));

        if (L.biome == BIOME_COAST)
        {
            // Sungai dari tepi atas mengalir ke laut di sisi kiri
            Polyline r;
            r.pts = {
                { L.x0 + ROOM_W * 0.40f, L.y0 - 40.0f },
                { L.x0 + ROOM_W * 0.37f, L.y0 + ROOM_H * 0.12f },
                { L.x0 + ROOM_W * 0.31f, L.y0 + ROOM_H * 0.25f },
                { L.x0 + ROOM_W * 0.23f, L.y0 + ROOM_H * 0.36f },
                { L.x0 + ROOM_W * 0.12f, L.y0 + ROOM_H * 0.45f },
                { L.x0 - 60.0f,          L.y0 + ROOM_H * 0.50f },
            };
            r.box = { L.x0 - 200, L.y0 - 200, ROOM_W * 0.6f + 400, ROOM_H * 0.6f + 400 };
            L.river = r;
            L.hasRiver = true;
        }

        if (L.biome == BIOME_CITY)
        {
            L.pave = { L.x0 + ROOM_W * 0.07f, L.y0 + ROOM_H * 0.09f, ROOM_W * 0.86f, ROOM_H * 0.82f };
            L.roadHx0 = L.doorW ? L.x0 : L.pave.x;
            L.roadHx1 = L.doorE ? L.x0 + ROOM_W : L.pave.x + L.pave.width;
            L.roadVy0 = L.doorN ? L.y0 : L.pave.y;
            L.roadVy1 = L.doorS ? L.y0 + ROOM_H : L.pave.y + L.pave.height;
        }
        s_layout.push_back(L);
    }
}

// Klasifikasi terrain di posisi dunia.
//  (x, y)   : koordinat asli   -> elemen buatan manusia (kota, jalan)
//  (wx, wy) : koordinat ter-warp -> batas alami (laut, sungai, kolam, hutan)
//  (px, py) : koordinat ter-warp halus untuk tepi jalan tanah
static int TerrainAt(float x, float y, float wx, float wy, float px, float py, float pathNoise)
{
    const RoomLayout& L = s_layout[RoomIndexAt(x, y)];
    float wu = (wx - L.x0) / ROOM_W, wv = (wy - L.y0) / ROOM_H;

    // ---- Laut di sepanjang tepi kiri dunia ----
    if (L.rx == 0)
    {
        float coast = ROOM_W * 0.165f + sinf(wy * 0.0029f) * 55.0f + sinf(wy * 0.0110f + 1.3f) * 22.0f;
        if (wx < coast) return WATER;
    }

    // ---- Kota: tepi lurus (buatan manusia) ----
    if (L.biome == BIOME_CITY)
    {
        bool inH = fabsf(y - L.hub.y) <= ROAD_HW && x >= L.roadHx0 && x <= L.roadHx1;
        bool inV = fabsf(x - L.hub.x) <= ROAD_HW && y >= L.roadVy0 && y <= L.roadVy1;
        if (inH || inV) return ASPHALT;
        if (CheckCollisionPointRec({ x, y }, L.pave)) return CITY;
        return GRASS;
    }

    // ---- Sungai ----
    if (L.hasRiver)
    {
        float d = DistToPolyline(L.river, wx, wy);
        float t = Clamp((wy - L.y0) / (ROOM_H * 0.5f), 0.0f, 1.0f);
        if (d <= 34.0f + t * 22.0f) return WATER;
    }

    // ---- Kolam di hutan ----
    if (L.biome == BIOME_FOREST)
    {
        float ex = (wu - 0.75f) / 0.11f, ey = (wv - 0.79f) / 0.10f;
        if (ex * ex + ey * ey <= 1.0f) return WATER;
    }

    // ---- Jalan tanah menuju pintu ----
    for (const Polyline& p : L.paths)
        if (DistToPolyline(p, px, py) <= PATH_HW + pathNoise) return ROAD;

    // ---- Area khusus bioma ----
    if (L.biome == BIOME_COAST)
    {
        // alun-alun desa (tanah) di sekitar pusat
        float ex = (wu - 0.52f) / 0.15f, ey = (wv - 0.48f) / 0.19f;
        if (ex * ex + ey * ey <= 1.0f) return DIRT;
        return GRASS;
    }
    if (L.biome == BIOME_RUIN)
    {
        float ex = (wu - 0.50f) / 0.33f, ey = (wv - 0.52f) / 0.31f;
        if (ex * ex + ey * ey <= 1.0f) return RUIN;
        return GRASS;
    }
    if (L.biome == BIOME_FOREST) return FOREST;
    return GRASS;
}

// =====================================================
// BAGIAN 3 - Generator ground (render-to-image, seamless)
// =====================================================
struct GroundPatches
{
    TexPatch sand, sand2, dirt, dirt2;
    TexPatch grassBig, grassTop, grassL, grassM, grassD;
    TexPatch moss, cobble, cobbleL;
    TexPatch waterShallow, waterMid, waterDeep;
    TexPatch sidewalk[4], asphalt;
    TexPatch forest[3];          // lantai hutan berdaun (Hutan Terlarang)
};

struct GroundData
{
    vector<int> distToWater, distToLand;   // dalam satuan chamfer (/3 = piksel)
};

static Image BuildSeamlessGround(const GroundPatches& P, int W, int H, const World* world, int structuralWalls, GroundData& gd)
{
    const float fT = (float)T;

    // ---- 1. Medan noise ----
    NoiseField warpX, warpY, grassVar, accent, shore, detailX, detailY, mixB, orient;
    warpX.Build(W, H, 4, fT, 0.42f, 11u, 3);
    warpY.Build(W, H, 4, fT, 0.42f, 12u, 3);
    grassVar.Build(W, H, 4, fT, 0.20f, 21u, 3);
    accent.Build(W, H, 4, fT, 0.55f, 22u, 3);
    orient.Build(W, H, 4, fT, 0.30f, 23u, 2);
    shore.Build(W, H, 4, fT, 0.60f, 24u, 3);
    detailX.Build(W, H, 2, fT, 2.20f, 25u, 2);
    detailY.Build(W, H, 2, fT, 2.20f, 26u, 2);
    mixB.Build(W, H, 4, fT, 0.50f, 27u, 3);

    const float WARP = 95.0f;       // amplitudo warp batas alami (dunia)

    // ---- 2. Klasifikasi terrain per piksel ----
    vector<uint8_t>& terr = s_terr;
    terr.assign((size_t)W * H, GRASS);
    for (int py = 0; py < H; py++)
        for (int px = 0; px < W; px++)
        {
            float x = (px + 0.5f) * GS, y = (py + 0.5f) * GS;
            float jit = (Hash01(px, py, 77u) - 0.5f) * 2.5f * GS;
            float dX = (detailX.At(px, py) - 0.5f), dY = (detailY.At(px, py) - 0.5f);
            float wx = x + (warpX.At(px, py) - 0.5f) * 2.0f * WARP + dX * 30.0f + jit;
            float wy = y + (warpY.At(px, py) - 0.5f) * 2.0f * WARP + dY * 30.0f + jit;
            float pX = x + dX * 26.0f + jit * 0.6f;
            float pY = y + dY * 26.0f + jit * 0.6f;
            float pathNoise = (shore.At(px, py) - 0.5f) * 22.0f;
            terr[(size_t)py * W + px] = (uint8_t)TerrainAt(x, y, wx, wy, pX, pY, pathNoise);
        }

    // ---- 3. Jarak ke air / daratan ----
    const int INF = 1 << 28;
    gd.distToWater.assign((size_t)W * H, 0);
    gd.distToLand.assign((size_t)W * H, 0);
    for (size_t i = 0; i < terr.size(); i++)
    {
        bool w = terr[i] == WATER;
        gd.distToWater[i] = w ? 0 : INF;
        gd.distToLand[i]  = w ? INF : 0;
    }
    ChamferDistance(gd.distToWater, W, H);
    ChamferDistance(gd.distToLand, W, H);

    // ---- 4. Pantai pasir otomatis sepanjang air ----
    for (int py = 0; py < H; py++)
        for (int px = 0; px < W; px++)
        {
            size_t i = (size_t)py * W + px;
            int t = terr[i];
            if (t == WATER || t == CITY || t == ASPHALT || t == SAND) continue;
            float dw = gd.distToWater[i] / 3.0f;
            if (dw > 40.0f) continue;
            float x = px * GS;
            bool seaSide = (x < ROOM_W * 0.40f);
            float width = seaSide ? (10.0f + shore.At(px, py) * 18.0f)
                                  : (2.0f + shore.At(px, py) * 6.0f);
            float jitter = (Hash01(px, py, 91u) - 0.5f) * 3.0f;
            if (dw + jitter < width) terr[i] = SAND;
        }

    // ---- 5. Pewarnaan ----
    Image img = GenImageColor(W, H, BLACK);
    Color* out = (Color*)img.data;

    const Color FOAM      = { 226, 240, 238, 255 };
    const Color FOAM_SOFT = { 168, 214, 222, 255 };
    const Color ROAD_MARK = { 222, 222, 212, 255 };

    for (int py = 0; py < H; py++)
    {
        for (int px = 0; px < W; px++)
        {
            size_t i = (size_t)py * W + px;
            int t = terr[i];
            float d01 = Hash01(px, py, 5u);
            float dith = (d01 - 0.5f) * 0.10f;

            int tx = px + (int)lroundf((warpY.At(px, py) - 0.5f) * 14.0f);
            int ty = py + (int)lroundf((warpX.At(px, py) - 0.5f) * 14.0f);

            float wxW = px * GS, wyW = py * GS;
            const RoomLayout& L = s_layout[RoomIndexAt(wxW, wyW)];

            // Kegelapan lantai hutan: penuh di tengah ruangan hutan, memudar
            // ke dinding/pintu sehingga tidak ada garis batas di celah pintu.
            float forestK = 0.0f;
            if (L.biome == BIOME_FOREST)
            {
                float dEdge = min(min(wxW - L.x0, L.x0 + ROOM_W - wxW), min(wyW - L.y0, L.y0 + ROOM_H - wyW));
                forestK = Smoothstep(60.0f, 330.0f, dEdge + (warpX.At(px, py) - 0.5f) * 160.0f);
            }

            float on = orient.At(px, py) + (d01 - 0.5f) * 0.08f;
            Color c = BLACK;

            switch (t)
            {
            case GRASS:
            case GRASS_ACCENT:
            case FOREST:
            {
                float gv = grassVar.At(px, py) + dith;
                if (gv < 0.46f)      c = P.grassBig.GetV(tx, ty, on);
                else if (gv < 0.58f) c = P.grassTop.GetV(tx + 17, ty + 9, on);
                else if (gv < 0.72f) c = P.grassM.GetV(tx + 5, ty + 31, on);
                else                 c = P.grassL.GetV(tx + 23, ty + 3, on);

                float ac = accent.At(px, py) + dith;
                if (ac > 0.64f) c = P.grassD.GetV(tx + 11, ty + 13, on);

                // Lantai hutan memakai tile asli "Hutan Terlarang" (tanpa recolor),
                // muncul berkelompok dan makin rapat menuju tengah hutan.
                if (forestK > 0.0f && ac + forestK * 0.30f > 0.70f)
                {
                    float fv = mixB.At(px, py) + dith;
                    int fi = fv < 0.45f ? 0 : (fv < 0.62f ? 2 : 1);
                    c = P.forest[fi].GetV(tx + 7 * fi, ty + 3 * fi, on);
                }
                // naungan kanopi sangat tipis, palet tileset tetap terjaga
                float k = forestK * 0.12f;
                c = Shade(c, 1.0f - k, 1.0f - k * 0.7f, 1.0f - k * 0.7f);
                break;
            }
            case SAND:
            {
                c = (mixB.At(px, py) + dith < 0.5f) ? P.sand.GetV(tx, ty, on) : P.sand2.GetV(tx + 21, ty + 5, on);
                float dw = gd.distToWater[i] / 3.0f;
                if (dw <= 3.0f) c = Shade(c, 0.86f, 0.86f, 0.88f);
                else if (dw <= 5.0f && d01 < 0.5f) c = Shade(c, 0.93f, 0.93f, 0.94f);
                break;
            }
            case ROAD:
            case DIRT:
            {
                c = (mixB.At(px, py) + dith < 0.5f) ? P.dirt.GetV(tx, ty, on) : P.dirt2.GetV(tx + 9, ty + 4, on);
                if (forestK > 0.3f) c = Shade(c, 0.88f, 0.88f, 0.88f);
                break;
            }
            case CITY:
            {
                int cw = P.sidewalk[0].w, ch = P.sidewalk[0].h;
                int cellX = px / cw, cellY = py / ch;
                int pick = (int)(Hash01(cellX, cellY, 61u) * 3.999f);
                c = P.sidewalk[pick].Get(px - cellX * cw, py - cellY * ch);
                break;
            }
            case ASPHALT:
            {
                c = P.asphalt.Get(px, py);
                // marka putus-putus di tengah jalan
                int cyH = (int)(L.hub.y / GS), cxV = (int)(L.hub.x / GS);
                int hw  = (int)(ROAD_HW / GS);
                bool inH = abs(py - cyH) <= hw;
                bool inV = abs(px - cxV) <= hw;
                bool nearX = abs(px - cxV) <= hw + 6;   // marka berhenti sebelum persimpangan
                bool nearY = abs(py - cyH) <= hw + 6;
                if (inH && !nearX && (py == cyH || py == cyH - 1) && (px / 14) % 2 == 0) c = ROAD_MARK;
                if (inV && !nearY && (px == cxV || px == cxV - 1) && (py / 14) % 2 == 0) c = ROAD_MARK;
                break;
            }
            case RUIN:
            {
                float rv = mixB.At(px, py) + dith;
                if (rv < 0.40f)      c = P.cobble.Get(tx, ty);
                else if (rv < 0.55f) c = P.cobbleL.Get(tx + 7, ty + 5);
                else if (rv < 0.68f) c = P.dirt.Get(tx, ty);
                else                 c = P.moss.Get(tx + 3, ty + 9);
                break;
            }
            case WATER:
            {
                float dl = gd.distToLand[i] / 3.0f;
                float sn = shore.At(px, py);

                float shallowLimit = 12.0f + sn * 14.0f + (d01 - 0.5f) * 5.0f;
                if (dl < shallowLimit)
                    c = P.waterShallow.GetV(tx, ty, on);
                else
                    c = (grassVar.At(px, py) + dith < 0.5f) ? P.waterMid.GetV(tx, ty, on) : P.waterDeep.GetV(tx + 13, ty + 7, on);

                float deep = Smoothstep(30.0f, 90.0f, dl);
                c = Shade(c, 1.0f - deep * 0.18f, 1.0f - deep * 0.14f, 1.0f - deep * 0.08f);

                if (dl <= 1.5f) c = FOAM;
                else if (dl <= 3.5f && d01 < 0.6f) c = FOAM_SOFT;
                else
                {
                    float band = fmodf(dl + sn * 9.0f, 15.0f);
                    if (dl < 26.0f && band > 7.0f && band < 8.4f && d01 < 0.7f) c = FOAM_SOFT;
                }
                break;
            }
            }
            out[i] = c;
        }
    }

    // ---- 6. Bayangan tepi: terrain lebih tinggi menaungi yang lebih rendah ----
    for (int py = 2; py < H; py++)
        for (int px = 1; px < W; px++)
        {
            size_t i = (size_t)py * W + px;
            int r = TerrainRank(terr[i]);
            if (r == 0) continue;
            int up1 = TerrainRank(terr[i - W]);
            int up2 = TerrainRank(terr[i - 2 * (size_t)W]);
            int lf1 = TerrainRank(terr[i - 1]);
            if (up1 > r || lf1 > r)  out[i] = Shade(out[i], 0.74f, 0.74f, 0.76f);
            else if (up2 > r)        out[i] = Shade(out[i], 0.87f, 0.87f, 0.88f);
        }

    // ---- 6b. Kerb trotoar ----
    for (int py = 2; py < H - 2; py++)
        for (int px = 2; px < W - 2; px++)
        {
            size_t i = (size_t)py * W + px;
            if (terr[i] != CITY) continue;
            bool edge1 = false, edge2 = false;
            for (int k = 1; k <= 2; k++)
            {
                bool e = terr[i - k] != CITY || terr[i + k] != CITY ||
                         terr[i - k * (size_t)W] != CITY || terr[i + k * (size_t)W] != CITY;
                if (e) { if (k == 1) edge1 = true; else edge2 = true; }
            }
            if (edge1)      out[i] = Shade(out[i], 0.70f, 0.70f, 0.72f);
            else if (edge2) out[i] = Shade(out[i], 1.08f, 1.08f, 1.06f);
        }

    // ---- 7. Tembok batu dari dinding collision ruangan ----
    for (int k = 0; k < structuralWalls; k++)
    {
        Rectangle r = GetWallRect(k);
        int x0 = std::clamp((int)floorf(r.x / GS), 0, W - 1);
        int y0 = std::clamp((int)floorf(r.y / GS), 0, H - 1);
        int x1 = std::clamp((int)ceilf((r.x + r.width) / GS) - 1, 0, W - 1);
        int y1 = std::clamp((int)ceilf((r.y + r.height) / GS) - 1, 0, H - 1);

        // bayangan jatuh di bawah dan kanan tembok
        for (int y = y1 + 1; y <= min(H - 1, y1 + 5); y++)
            for (int x = x0 + 2; x <= min(W - 1, x1 + 3); x++)
            {
                size_t i = (size_t)y * W + x;
                if (terr[i] == WALLSTONE) continue;
                float f = 0.62f + 0.06f * (y - y1);
                out[i] = Shade(out[i], f, f, f + 0.02f);
            }
        for (int y = y0 + 2; y <= y1; y++)
            for (int x = x1 + 1; x <= min(W - 1, x1 + 3); x++)
            {
                size_t i = (size_t)y * W + x;
                if (terr[i] == WALLSTONE) continue;
                out[i] = Shade(out[i], 0.72f, 0.72f, 0.74f);
            }

        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
            {
                size_t i = (size_t)y * W + x;
                Color c = Shade(P.cobble.Get(x * 1 + 3, y * 1 + 5), 0.92f, 0.92f, 0.95f);
                if ((Hash01(x, y, 131u) < 0.10f)) c = Shade(P.cobbleL.Get(x, y), 0.95f, 0.95f, 0.95f);
                // lumut tipis di kaki tembok
                if (y >= y1 - 1 && Hash01(x, y, 133u) < 0.35f) c = Shade(P.moss.Get(x, y), 0.95f, 0.95f, 0.95f);
                if (y <= y0 + 1)      c = Shade(c, 1.18f, 1.18f, 1.15f);   // sisi atas terkena cahaya
                if (y == y1)          c = Shade(c, 0.55f, 0.55f, 0.58f);   // sisi bawah
                if (x == x0 || x == x1) c = Shade(c, 0.75f, 0.75f, 0.78f);
                out[i] = c;
                terr[i] = WALLSTONE;
            }
    }

    (void)world;
    return img;
}

// =====================================================
// BAGIAN 4 - Ekstraksi objek dari panel atlas
// =====================================================
// Panel atlas berisi banyak objek bersebelahan. Menempel panel apa adanya
// menghasilkan "kotak tempelan". Di sini setiap panel dipecah menjadi objek
// satuan: alpha setelah CleanPanelBackground -> connected component (8-arah,
// dengan dilasi 1 px agar bagian objek yang hampir menempel tetap satu).
// Komponen yang berbentuk kotak penuh (tile ground), terpotong tepi panel,
// terlalu kecil, atau berbentuk baris teks dibuang.
struct Packer
{
    Image* dst; int penX = 2, penY = 2, rowH = 0;
    bool Place(int w, int h, int& ox, int& oy)
    {
        if (w + 4 > dst->width) return false;
        if (penX + w + 2 > dst->width) { penX = 2; penY += rowH + 3; rowH = 0; }
        if (penY + h + 2 > dst->height) return false;
        ox = penX; oy = penY;
        penX += w + 3; rowH = max(rowH, h);
        return true;
    }
};

static void ExtractSprites(const Image& img, Rectangle panel, int cat, Packer& pk)
{
    int x0 = std::clamp((int)panel.x, 0, img.width - 1);
    int y0 = std::clamp((int)panel.y, 0, img.height - 1);
    int x1 = std::clamp((int)(panel.x + panel.width) - 1, 0, img.width - 1);
    int y1 = std::clamp((int)(panel.y + panel.height) - 1, 0, img.height - 1);
    int w = x1 - x0 + 1, h = y1 - y0 + 1;
    if (w <= 2 || h <= 2) return;

    const Color* src = (const Color*)img.data;
    const size_t N = (size_t)w * h;
    vector<uint8_t> mask(N, 0), dil(N, 0);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            mask[(size_t)y * w + x] = src[(size_t)(y0 + y) * img.width + (x0 + x)].a > 40;

    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            uint8_t m = 0;
            for (int dy = -1; dy <= 1 && !m; dy++)
                for (int dx = -1; dx <= 1 && !m; dx++)
                {
                    int xx = x + dx, yy = y + dy;
                    if (xx >= 0 && yy >= 0 && xx < w && yy < h && mask[(size_t)yy * w + xx]) m = 1;
                }
            dil[(size_t)y * w + x] = m;
        }

    // ---- 1. Label awal: connected component 8-arah pada mask terdilasi ----
    vector<int> label(N, -1);
    int nextLabel = 0;
    vector<int> stack;
    for (size_t s0 = 0; s0 < N; s0++)
    {
        if (!dil[s0] || label[s0] >= 0) continue;
        int lab = nextLabel++;
        stack.clear(); stack.push_back((int)s0); label[s0] = lab;
        while (!stack.empty())
        {
            int id = stack.back(); stack.pop_back();
            int cx = id % w, cy = id / w;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                {
                    int nx = cx + dx, ny = cy + dy;
                    if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
                    size_t n = (size_t)ny * w + nx;
                    if (dil[n] && label[n] < 0) { label[n] = lab; stack.push_back((int)n); }
                }
        }
    }
    for (size_t i = 0; i < N; i++) if (!mask[i]) label[i] = -1;

    // ---- 2. Komponen terlalu besar biasanya beberapa objek yang tersambung
    //         rumput/tali tipis (mis. deretan pohon). Dipecah dengan erosi:
    //         inti objek dicari pada mask yang dikikis, lalu ditumbuhkan kembali
    //         ke bentuk aslinya sehingga piksel tidak ada yang hilang. ----
    struct Stat { int bx0, by0, bx1, by1, count; };
    auto computeStats = [&](int labels) {
        vector<Stat> st(labels, Stat{ w, h, -1, -1, 0 });
        for (size_t i = 0; i < N; i++)
        {
            int l = label[i]; if (l < 0) continue;
            int x = (int)(i % w), y = (int)(i / w);
            Stat& t = st[l];
            t.bx0 = min(t.bx0, x); t.by0 = min(t.by0, y); t.bx1 = max(t.bx1, x); t.by1 = max(t.by1, y); t.count++;
        }
        return st;
    };
    vector<Stat> st = computeStats(nextLabel);
    int firstPassLabels = nextLabel;
    for (int lab = 0; lab < firstPassLabels; lab++)
    {
        const Stat t = st[lab];
        if (t.count == 0) continue;
        int bw = t.bx1 - t.bx0 + 1, bh = t.by1 - t.by0 + 1;
        if (bw <= 130 && bh <= 150) continue;

        vector<uint8_t> sub((size_t)bw * bh, 0);
        for (int y = 0; y < bh; y++)
            for (int x = 0; x < bw; x++)
                sub[(size_t)y * bw + x] = label[(size_t)(t.by0 + y) * w + (t.bx0 + x)] == lab;

        for (int radius = 2; radius <= 5; radius++)
        {
            vector<uint8_t> core = sub, tmp;
            for (int it = 0; it < radius; it++)
            {
                tmp = core;
                for (int y = 0; y < bh; y++)
                    for (int x = 0; x < bw; x++)
                    {
                        if (!tmp[(size_t)y * bw + x]) continue;
                        bool keep = x > 0 && y > 0 && x < bw - 1 && y < bh - 1 &&
                                    tmp[(size_t)y * bw + x - 1] && tmp[(size_t)y * bw + x + 1] &&
                                    tmp[(size_t)(y - 1) * bw + x] && tmp[(size_t)(y + 1) * bw + x];
                        if (!keep) core[(size_t)y * bw + x] = 0;
                    }
            }
            // label inti
            vector<int> cl((size_t)bw * bh, -1);
            vector<int> coreSize;
            for (size_t s0 = 0; s0 < cl.size(); s0++)
            {
                if (!core[s0] || cl[s0] >= 0) continue;
                int l = (int)coreSize.size(); coreSize.push_back(0);
                stack.clear(); stack.push_back((int)s0); cl[s0] = l;
                while (!stack.empty())
                {
                    int id = stack.back(); stack.pop_back(); coreSize[l]++;
                    int cx = id % bw, cy = id / bw;
                    for (int dy = -1; dy <= 1; dy++)
                        for (int dx = -1; dx <= 1; dx++)
                        {
                            int nx = cx + dx, ny = cy + dy;
                            if (nx < 0 || ny < 0 || nx >= bw || ny >= bh) continue;
                            size_t n = (size_t)ny * bw + nx;
                            if (core[n] && cl[n] < 0) { cl[n] = l; stack.push_back((int)n); }
                        }
                }
            }
            int big = 0;
            for (int c : coreSize) if (c >= 250) big++;
            if (big <= 1) continue;          // belum terpisah, coba erosi lebih kuat

            // hanya inti besar yang menjadi objek; tumbuhkan kembali (BFS multi-sumber)
            vector<int> newLab(coreSize.size(), -1);
            for (size_t c = 0; c < coreSize.size(); c++) if (coreSize[c] >= 250) newLab[c] = nextLabel++;
            vector<int> grow((size_t)bw * bh, -1);
            std::queue<int> q;
            for (size_t i = 0; i < grow.size(); i++)
                if (cl[i] >= 0 && newLab[cl[i]] >= 0) { grow[i] = newLab[cl[i]]; q.push((int)i); }
            while (!q.empty())
            {
                int id = q.front(); q.pop();
                int cx = id % bw, cy = id / bw;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++)
                    {
                        int nx = cx + dx, ny = cy + dy;
                        if (nx < 0 || ny < 0 || nx >= bw || ny >= bh) continue;
                        size_t n = (size_t)ny * bw + nx;
                        if (sub[n] && grow[n] < 0) { grow[n] = grow[id]; q.push((int)n); }
                    }
            }
            for (int y = 0; y < bh; y++)
                for (int x = 0; x < bw; x++)
                {
                    size_t i = (size_t)y * bw + x;
                    if (sub[i]) label[(size_t)(t.by0 + y) * w + (t.bx0 + x)] = grow[i];
                }
            break;
        }
    }

    // ---- 3. Saring & salin setiap objek ke atlas objek ----
    st = computeStats(nextLabel);
    Color* dpx = (Color*)pk.dst->data;
    for (int lab = 0; lab < nextLabel; lab++)
    {
        const Stat t = st[lab];
        int count = t.count;
        if (count == 0) continue;
        int bx0 = t.bx0, by0 = t.by0, bx1 = t.bx1, by1 = t.by1;
        int bw = bx1 - bx0 + 1, bh = by1 - by0 + 1;
        float fill = (float)count / (float)(bw * bh);
        bool touchesEdge = bx0 <= 1 || by0 <= 1 || bx1 >= w - 2 || by1 >= h - 2;
        bool tooSmall    = count < 140 || bw < 4 || bh < 4 || max(bw, bh) < 14;
        bool textLike    = bh <= 30 && bw >= bh * 3.0f;
        // tile ground contoh (kotak penuh seukuran tile) atau area ground besar -> bukan objek.
        float aspect     = (float)bw / (float)bh;
        bool tileSized   = bw >= 24 && bw <= 95 && bh >= 24 && bh <= 95 && aspect > 0.45f && aspect < 1.7f;
        bool solidBlock  = (fill >= 0.86f && tileSized) ||
                           (fill >= 0.985f && bw >= 150 && bh >= 150);
        bool tooBig      = bw > 420 || bh > 420;
        if (touchesEdge || tooSmall || textLike || solidBlock || tooBig) continue;

        int ox, oy;
        if (!pk.Place(bw, bh, ox, oy))
        {
            TraceLog(LOG_WARNING, "BACKGROUND: atlas objek penuh, objek dilewati");
            continue;
        }
        double sumR = 0, sumG = 0, sumB = 0;
        for (int y = 0; y < bh; y++)
            for (int x = 0; x < bw; x++)
            {
                size_t id = (size_t)(by0 + y) * w + (bx0 + x);
                Color c = BLANK;
                if (label[id] == lab)
                {
                    c = src[(size_t)(y0 + by0 + y) * img.width + (x0 + bx0 + x)];
                    sumR += c.r; sumG += c.g; sumB += c.b;
                }
                dpx[(size_t)(oy + y) * pk.dst->width + (ox + x)] = c;
            }
        Color mean = { (unsigned char)(sumR / count), (unsigned char)(sumG / count), (unsigned char)(sumB / count), 255 };
        Rectangle atlasR = { (float)(x0 + bx0), (float)(y0 + by0), (float)bw, (float)bh };
        s_sprites.push_back({ { (float)ox, (float)oy, (float)bw, (float)bh }, cat, fill, atlasR, mean });
    }
}

// Sub-kategori berdasarkan posisi di panel dan bentuk/warna objek
static void ClassifySprites()
{
    for (DecorSprite& sp : s_sprites)
    {
        const Rectangle a = sp.atlas;
        bool green = sp.mean.g > sp.mean.r + 6 && sp.mean.g >= sp.mean.b;
        switch (sp.cat)
        {
        case CAT_TREE:          // baris atas panel = pohon besar, sisanya semak & bunga
            sp.cat = (a.height >= 90) ? CAT_TREE : CAT_PLANT;
            break;
        case CAT_FORESTOBJ:
            if (green) sp.cat = (a.height >= 60) ? CAT_TREE : CAT_PLANT;
            break;
        case CAT_VILLAGE:
            if (a.y >= 315 && (a.x < 330 || a.height < 50)) sp.cat = CAT_COAST;      // dermaga & perahu -> tepi air
            else if (a.y < 200 && a.width >= 55 && a.height >= 55) sp.cat = CAT_HOUSE; // rumah & kios
            break;
        case CAT_CITY:
            if (a.height >= 75) sp.cat = CAT_BUILDING;
            break;
        default: break;
        }
    }
}

// =====================================================
// BAGIAN 5 - Penyebaran objek secara natural
// =====================================================
static int TerrAtImg(int px, int py)
{
    if (px < 0 || py < 0 || px >= s_imgW || py >= s_imgH) return WALLSTONE;
    return s_terr[(size_t)py * s_imgW + px];
}

static bool IsGreen(int t) { return t == GRASS || t == GRASS_ACCENT || t == FOREST; }

static bool TerrainOkFor(int cat, int t)
{
    switch (cat)
    {
    case CAT_TREE: case CAT_FORESTOBJ: case CAT_MISC: case CAT_PLANT: return IsGreen(t);
    case CAT_VILLAGE: case CAT_HOUSE: return IsGreen(t) || t == DIRT || t == SAND;
    case CAT_CITY: case CAT_BUILDING: return t == CITY;
    case CAT_RUIN:      return t == RUIN || IsGreen(t);
    case CAT_COAST:     return t == SAND || t == WATER;   // dermaga/perahu boleh menjorok ke air
    case CAT_ROCK:      return t == SAND || t == WATER;
    case CAT_WATERROCK: return t == WATER;
    }
    return false;
}

struct PlaceRule { int cat; int count; float nearHub; };   // nearHub: radius maks dari pusat (x ROOM_W, 0 = bebas)

struct DecorPlacer
{
    const GroundData* gd = nullptr;
    Vector2 spawn = { 0, 0 };
    vector<Rectangle> boxes;

    // Validasi satu objek di (bx, by) = titik tengah alas. Jika lolos, objek
    // ditambahkan beserta collision alasnya.
    bool TryPlace(int si, float bx, float by, int cat, Rectangle room, bool flipOk)
    {
        const DecorSprite& sp = s_sprites[si];
        float sw = sp.src.width * GS, sh = sp.src.height * GS;
        bx = floorf(bx / GS) * GS; by = floorf(by / GS) * GS;
        Rectangle box = { bx - floorf(sp.src.width * 0.5f) * GS, by - sh, sw, sh };

        // seluruh objek di dalam ruangan (tidak menimpa tembok)
        const float inset = WALL_T + 10.0f;
        if (box.x < room.x + inset || box.y < room.y + inset * 0.5f ||
            box.x + box.width > room.x + room.width - inset ||
            box.y + box.height > room.y + room.height - inset) return false;

        if (Vector2Distance({ bx, by }, spawn) < 170.0f + sw * 0.5f) return false;

        // terrain di alas objek
        int ipy0 = (int)(by / GS) - 1, ipy1 = (int)((by - max(6.0f, sh * 0.12f)) / GS);
        for (int sxi = -3; sxi <= 3; sxi++)
            for (int syi = 0; syi < 2; syi++)
            {
                int t = TerrAtImg((int)((bx + sxi * sw * 0.12f) / GS), syi == 0 ? ipy0 : ipy1);
                if (!TerrainOkFor(cat, t)) return false;
            }

        // jarak aman dari jalan / pintu di samping dan depan alas
        {
            float m = 22.0f;
            int checks[3][2] = {
                { (int)((bx - sw * 0.5f - m) / GS), ipy0 }, { (int)((bx + sw * 0.5f + m) / GS), ipy0 },
                { (int)(bx / GS), (int)((by + m) / GS) } };
            for (auto& c : checks)
            {
                int t = TerrAtImg(c[0], c[1]);
                if (t == ROAD || t == ASPHALT || t == WALLSTONE) return false;
            }
        }

        // badan objek tidak menimpa jalan/tembok. Gedung & objek buatan diperiksa
        // penuh; tajuk pohon & tanaman boleh sedikit menaungi jalan.
        bool strict = !(cat == CAT_TREE || cat == CAT_PLANT || cat == CAT_FORESTOBJ || cat == CAT_MISC);
        float yStart = strict ? 0.0f : 0.55f;
        for (int gy = 0; gy <= 6; gy++)
            for (int gx = 0; gx <= 6; gx++)
            {
                float fy = yStart + (1.0f - yStart) * gy / 6.0f;
                int t = TerrAtImg((int)((box.x + box.width * gx / 6.0f) / GS), (int)((box.y + box.height * fy) / GS));
                if (t == ROAD || t == ASPHALT || t == WALLSTONE) return false;
            }

        // aturan khusus air
        int ipx = std::clamp((int)(bx / GS), 0, s_imgW - 1), ipy = std::clamp(ipy0, 0, s_imgH - 1);
        size_t gi = (size_t)ipy * s_imgW + ipx;
        float dLand = gd->distToLand[gi] / 3.0f, dWater = gd->distToWater[gi] / 3.0f;
        if (cat == CAT_WATERROCK && dLand < 14.0f) return false;
        if (cat == CAT_ROCK && TerrAtImg(ipx, ipy) == WATER && dLand > 26.0f) return false;
        if (cat == CAT_COAST && (TerrAtImg(ipx, ipy) == WATER ? dLand > 30.0f : dWater > 20.0f)) return false;

        // tumpang tindih dengan objek lain
        bool vegetation = cat == CAT_TREE || cat == CAT_PLANT;
        for (size_t k = 0; k < s_decor.size(); k++)
        {
            const DecorInst& o = s_decor[k];
            const DecorSprite& os = s_sprites[o.sprite];
            float osw = os.src.width * GS;
            if (fabsf(o.base.x - bx) < (sw + osw) * 0.34f && fabsf(o.base.y - by) < max(28.0f, min(sh, os.src.height * GS) * 0.28f))
                return false;
            const Rectangle& ob = boxes[k];
            if (CheckCollisionRecs(box, ob))
            {
                Rectangle in = GetCollisionRec(box, ob);
                float ratio = (in.width * in.height) / min(box.width * box.height, ob.width * ob.height);
                bool bothVeg = vegetation && (os.cat == CAT_TREE || os.cat == CAT_PLANT);
                if (ratio > (bothVeg ? 0.45f : 0.15f)) return false;
            }
        }

        DecorInst inst;
        inst.sprite = si;
        inst.base = { bx, by };
        inst.flip = flipOk;
        inst.shadow = cat != CAT_WATERROCK;
        s_decor.push_back(inst);
        boxes.push_back(box);

        // collision kecil di alas objek besar
        if (DECOR_COLLISION && sh >= 60.0f && sw >= 30.0f)
        {
            bool wide = cat == CAT_HOUSE || cat == CAT_BUILDING || sp.fill > 0.62f;
            float fw = sw * (wide ? 0.80f : 0.38f);
            float fh = Clamp(sh * (wide ? 0.22f : 0.14f), 12.0f, wide ? 70.0f : 36.0f);
            AddWall(bx - fw * 0.5f, by - fh, fw, fh);
        }
        return true;
    }
};

static void PlaceDecor(const World* w, Vector2 spawn, const GroundData& gd)
{
    vector<int> byCat[CAT_COUNT];
    for (int i = 0; i < (int)s_sprites.size(); i++) byCat[s_sprites[i].cat].push_back(i);
    for (int c = 0; c < CAT_COUNT; c++)
        TraceLog(LOG_INFO, "BACKGROUND: kategori %-9s : %d jenis objek", CAT_NAME[c], (int)byCat[c].size());

    DecorPlacer pl; pl.gd = &gd; pl.spawn = spawn;
    unsigned rng = 12345u;
    auto Rand01 = [&]() { rng = rng * 1664525u + 1013904223u; return (float)((rng >> 8) & 0xFFFFFF) / 16777215.0f; };
    int structural = GetWallCount();

    for (int ri = 0; ri < w->roomCount && ri < (int)s_layout.size(); ri++)
    {
        const RoomLayout& L = s_layout[ri];
        Rectangle room = w->rooms[ri].bounds;

        // ---- Kota: gedung berderet menghadap jalan (diletakkan lebih dulu) ----
        if (L.biome == BIOME_CITY && !byCat[CAT_BUILDING].empty())
        {
            const vector<int>& pool = byCat[CAT_BUILDING];
            float roadTop = L.hub.y - ROAD_HW;
            float paveBot = L.pave.y + L.pave.height;
            // garis alas setiap deret (dunia)
            // blok atas: 2 deret (menghadap jalan); blok bawah: 1 deret di belakang plaza
            float rows[3] = { roadTop - 14.0f, roadTop - 250.0f, paveBot - 16.0f };
            int placedB = 0, pick = (int)(Rand01() * pool.size());
            for (float baseY : rows)
            {
                float x = L.pave.x + 24.0f + Rand01() * 30.0f;
                float xEnd = L.pave.x + L.pave.width - 24.0f;
                while (x < xEnd)
                {
                    int si = pool[pick % pool.size()];
                    float sw = s_sprites[si].src.width * GS;
                    float bx = x + sw * 0.5f;
                    // lompati jalan vertikal
                    if (fabsf(bx - L.hub.x) < ROAD_HW + sw * 0.5f + 20.0f) { x = L.hub.x + ROAD_HW + 24.0f; continue; }
                    if (Rand01() < 0.18f) { x += 70.0f + Rand01() * 60.0f; continue; }   // celah: taman kecil / plaza
                    if (pl.TryPlace(si, bx, baseY, CAT_BUILDING, room, false)) { placedB++; pick++; x += sw + 14.0f + Rand01() * 26.0f; }
                    else { pick++; x += 24.0f; }
                }
            }
            TraceLog(LOG_INFO, "BACKGROUND: ruang %d gedung berderet -> %d", ri, placedB);
        }

        vector<PlaceRule> rules;
        switch (L.biome)
        {
        case BIOME_COAST:  rules = { { CAT_HOUSE, 7, 0.46f }, { CAT_VILLAGE, 12, 0.46f }, { CAT_TREE, 8, 0 },
                                     { CAT_PLANT, 14, 0 }, { CAT_MISC, 3, 0 } }; break;
        case BIOME_CITY:   rules = { { CAT_CITY, 18, 0 }, { CAT_TREE, 4, 0 }, { CAT_PLANT, 6, 0 }, { CAT_MISC, 3, 0 } }; break;
        case BIOME_FOREST: rules = { { CAT_TREE, 40, 0 }, { CAT_FORESTOBJ, 7, 0 }, { CAT_PLANT, 34, 0 }, { CAT_MISC, 2, 0 } }; break;
        case BIOME_RUIN:   rules = { { CAT_RUIN, 16, 0 }, { CAT_TREE, 7, 0 }, { CAT_PLANT, 14, 0 }, { CAT_MISC, 4, 0 } }; break;
        }
        if (L.rx == 0)
        {
            rules.push_back({ CAT_COAST, 5, 0 });
            rules.push_back({ CAT_ROCK, 3, 0 });
            rules.push_back({ CAT_WATERROCK, 9, 0 });
        }

        for (const PlaceRule& rule : rules)
        {
            const vector<int>& pool = byCat[rule.cat];
            if (pool.empty()) continue;

            vector<int> order = pool;
            for (int k = (int)order.size() - 1; k > 0; k--) swap(order[k], order[(int)(Rand01() * (k + 1)) % (k + 1)]);

            bool flippable = rule.cat == CAT_TREE || rule.cat == CAT_PLANT || rule.cat == CAT_FORESTOBJ ||
                             rule.cat == CAT_ROCK || rule.cat == CAT_WATERROCK;
            int placed = 0, cursor = 0, lastOk = 0;
            for (int attempt = 0; attempt < rule.count * 200 && placed < rule.count; attempt++)
            {
                // ganti jenis objek setiap 10 percobaan gagal (objek besar yang tidak muat)
                int si = order[(size_t)(cursor + (attempt - lastOk) / 10) % order.size()];
                const DecorSprite& sp = s_sprites[si];
                float sw = sp.src.width * GS, sh = sp.src.height * GS;
                float bx = room.x + sw * 0.5f + Rand01() * max(1.0f, room.width - sw);
                float by = room.y + sh + Rand01() * max(1.0f, room.height - sh);
                if (rule.nearHub > 0 && Vector2Distance({ bx, by }, L.hub) > rule.nearHub * ROOM_W) continue;

                if (pl.TryPlace(si, bx, by, rule.cat, room, flippable && Rand01() < 0.5f))
                {
                    placed++;
                    cursor += 1 + (attempt - lastOk) / 10;
                    lastOk = attempt + 1;
                }
            }
            TraceLog(LOG_INFO, "BACKGROUND: ruang %d kategori %s -> %d objek", ri, CAT_NAME[rule.cat], placed);
        }
    }

    sort(s_decor.begin(), s_decor.end(), [](const DecorInst& a, const DecorInst& b) { return a.base.y < b.base.y; });
    TraceLog(LOG_INFO, "BACKGROUND: total %d objek, %d collision tambahan", (int)s_decor.size(), GetWallCount() - structural);
}

// =====================================================
// BAGIAN 6 - API publik
// =====================================================
static string FindAsset(const char* name)
{
    const char* appDir = GetApplicationDirectory();
    string cands[] = {
        string(name),
        string("assets/") + name,
        string("../") + name,
        string("../assets/") + name,
        string(appDir) + name,
        string(appDir) + "assets/" + name,
        string(appDir) + "../../Projek_Akhir/" + name,
        string(appDir) + "../../Projek_Akhir/assets/" + name,
    };
    for (auto& c : cands) if (FileExists(c.c_str())) return c;
    return string();
}

bool BackgroundInit(const World* w, Vector2 playerSpawn)
{
    s_ready = false; s_failed = false;
    s_sprites.clear(); s_decor.clear();

    // layar "memuat" (generator butuh beberapa detik pada build Debug)
    BeginDrawing();
    ClearBackground(Color{ 24, 24, 32, 255 });
    DrawText("Membangun dunia Arunika...", SCREEN_W / 2 - MeasureText("Membangun dunia Arunika...", 28) / 2, SCREEN_H / 2 - 14, 28, RAYWHITE);
    EndDrawing();

    string tilesetPath = FindAsset("tileset.png");
    if (tilesetPath.empty())
    {
        TraceLog(LOG_ERROR, "BACKGROUND: tileset.png tidak ditemukan (letakkan di folder Projek_Akhir atau assets/)");
        s_failed = true;
        return false;
    }

    Image atlasImage = LoadImage(tilesetPath.c_str());
    if (atlasImage.data == nullptr) { s_failed = true; return false; }
    ImageFormat(&atlasImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    string alphaPath = FindAsset("atlas_alpha.png");
    Image objImage = alphaPath.empty() ? ImageCopy(atlasImage) : LoadImage(alphaPath.c_str());
    if (objImage.data == nullptr) objImage = ImageCopy(atlasImage);
    ImageFormat(&objImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    // ---------- Patch tekstur ground (koordinat tileset 1393x929, dari arunika_fixed.cpp) ----------
    GroundPatches gp;
    unsigned saltId = 100;
    auto PatchFrom = [&](TexPatch& tp, Rectangle r, float flatten = 0.8f, bool removeFoam = false)
    {
        tp.Load(atlasImage, r, saltId++, flatten, removeFoam);
    };

    // Variasi Ground
    PatchFrom(gp.sand,     {   9,  36, 54, 38 }, 0.9f);
    PatchFrom(gp.grassL,   { 533, 731, 48, 50 });
    PatchFrom(gp.dirt,     { 600, 731, 47, 50 });
    PatchFrom(gp.cobble,   { 732, 731, 43, 50 });
    PatchFrom(gp.grassM,   { 533, 799, 48, 32 });
    PatchFrom(gp.grassD,   { 600, 799, 47, 32 });
    PatchFrom(gp.cobbleL,  { 667, 799, 43, 32 });
    PatchFrom(gp.moss,     { 732, 799, 43, 32 });
    PatchFrom(gp.dirt2,    { 600, 851, 47, 34 });
    // Hutan Terlarang
    PatchFrom(gp.grassBig,     { 499, 138, 48, 72 });
    PatchFrom(gp.grassTop,     { 499,  38, 48, 34 });
    PatchFrom(gp.waterShallow, { 499, 296, 48, 70 }, 0.8f, true);
    PatchFrom(gp.forest[0],    { 500,  91, 45, 32 }, 0.5f);   // lantai daun lebat
    PatchFrom(gp.forest[1],    { 562, 384, 38, 48 }, 0.5f);   // semak rendah
    PatchFrom(gp.forest[2],    { 617, 358, 46, 32 }, 0.5f);   // daun + tanah
    // Desa Pesisir
    PatchFrom(gp.sand2,        {   9, 139, 54, 74 }, 0.9f);
    PatchFrom(gp.waterMid,     {  10, 303, 50, 46 }, 0.8f, true);
    PatchFrom(gp.waterDeep,    { 124, 288, 84, 30 }, 0.8f, true);
    // Kota Arunika
    PatchFrom(gp.sidewalk[0],  {  979, 257, 16, 15 }, 0.0f);
    PatchFrom(gp.sidewalk[1],  {  995, 257, 16, 15 }, 0.0f);
    PatchFrom(gp.sidewalk[2],  {  979, 272, 16, 15 }, 0.0f);
    PatchFrom(gp.sidewalk[3],  {  995, 241, 16, 15 }, 0.0f);
    for (auto& sw : gp.sidewalk) sw.blend = false;
    PatchFrom(gp.asphalt,      { 1104, 92, 30, 78 }, 0.6f);

    // ---------- Ground seukuran world ----------
    BuildLayout();
    s_imgW = (int)ceilf(WORLD_W / GS);
    s_imgH = (int)ceilf(WORLD_H / GS);
    int structuralWalls = GetWallCount();       // dinding ruangan dari WorldInit

    GroundData gd;
    Image groundImg = BuildSeamlessGround(gp, s_imgW, s_imgH, w, structuralWalls, gd);
    s_ground = LoadTextureFromImage(groundImg);
    UnloadImage(groundImg);
    SetTextureFilter(s_ground, TEXTURE_FILTER_POINT);
    SetTextureWrap(s_ground, TEXTURE_WRAP_CLAMP);

    // ---------- Panel objek (rect dari arunika_fixed.cpp) ----------
    struct PanelDef { Rectangle src; int cat; };
    PanelDef panels[] = {
        // Rect diukur ulang dari tileset.png 1393x929 agar tidak memotong atap rumah,
        // puncak pohon, tenda, tiang dermaga, dan lengkungan reruntuhan paling kiri.
        // Label judul panel sengaja terpotong tepi rect sehingga otomatis dibuang.
        { {    4, 711, 452, 196 }, CAT_COAST     },  // Jembatan & Dermaga
        { {  328, 479, 264, 199 }, CAT_ROCK      },  // Pulau Batu
        { {    4, 489, 314, 180 }, CAT_WATERROCK },  // Karang Pemecah
        { {  600, 450, 530, 245 }, CAT_TREE      },  // Pepohonan, semak, bunga
        { {  216,  16, 270, 432 }, CAT_VILLAGE   },  // Desa Pesisir (rumah, kios, sumur, perahu)
        { {  676,  14, 276, 434 }, CAT_FORESTOBJ },  // Hutan Terlarang (pohon, semak, gerbang batu)
        { { 1180,  16, 210, 434 }, CAT_CITY      },  // Kota Arunika (gedung & perabot)
        { { 1132, 486, 256, 212 }, CAT_MISC      },  // Objek Lainnya
        { {  852, 718, 536, 182 }, CAT_RUIN      },  // Reruntuhan & Dekorasi
    };

    KeyOutPanelWater(&objImage, panels[2].src);
    CleanPanelBackground(&objImage, { 0, 0, (float)objImage.width, (float)objImage.height }, 34.0f);
    for (auto& p : panels) CleanPanelBackground(&objImage, p.src, 36.0f);

    Image decorImg = GenImageColor(2048, 2048, BLANK);
    Packer pk; pk.dst = &decorImg;
    for (auto& p : panels)
    {
        size_t before = s_sprites.size();
        ExtractSprites(objImage, p.src, p.cat, pk);
        TraceLog(LOG_INFO, "BACKGROUND: panel %s -> %d objek", CAT_NAME[p.cat], (int)(s_sprites.size() - before));
    }
    ClassifySprites();
    for (auto& sp : s_sprites)
        TraceLog(LOG_DEBUG, "BACKGROUND SPRITE %s atlas=%d,%d %dx%d fill=%.2f rgb=%d,%d,%d", CAT_NAME[sp.cat],
                 (int)sp.atlas.x, (int)sp.atlas.y, (int)sp.atlas.width, (int)sp.atlas.height, sp.fill, sp.mean.r, sp.mean.g, sp.mean.b);
    s_decorTex = LoadTextureFromImage(decorImg);
    UnloadImage(decorImg);
    SetTextureFilter(s_decorTex, TEXTURE_FILTER_POINT);

    UnloadImage(objImage);
    UnloadImage(atlasImage);

    PlaceDecor(w, playerSpawn, gd);

    s_ready = true;
    return true;
}

void BackgroundUnload(void)
{
    if (s_ground.id) UnloadTexture(s_ground);
    if (s_decorTex.id) UnloadTexture(s_decorTex);
    s_ground = { 0 }; s_decorTex = { 0 };
    s_terr.clear(); s_sprites.clear(); s_decor.clear(); s_layout.clear();
    s_ready = false;
}

void BackgroundDrawGround(const World* w)
{
    if (!s_ready)
    {
        // Cadangan jika tileset tidak ada: tampilan lama agar game tetap jalan
        for (int i = 0; i < w->roomCount; i++) DrawRectangleRec(w->rooms[i].bounds, w->rooms[i].floor);
        for (int i = 0; i < GetWallCount(); i++) DrawRectangleRec(GetWallRect(i), Color{ 60, 65, 80, 255 });
        return;
    }

    DrawTextureEx(s_ground, { 0, 0 }, 0.0f, GS, WHITE);

    // bayangan lembut di bawah objek agar menapak di tanah
    for (const DecorInst& d : s_decor)
    {
        if (!DECOR_SHADOW || !d.shadow) continue;
        const DecorSprite& sp = s_sprites[d.sprite];
        float rx = sp.src.width * GS * 0.36f;
        float ry = Clamp(rx * 0.26f, 5.0f, 22.0f);
        DrawEllipse((int)d.base.x, (int)(d.base.y - ry * 0.35f), rx, ry, Color{ 0, 0, 0, 58 });
    }
}

void BackgroundDrawDecor(float splitY, bool front)
{
    if (!s_ready) return;
    for (const DecorInst& d : s_decor)
    {
        bool isFront = d.base.y > splitY;
        if (isFront != front) continue;
        const DecorSprite& sp = s_sprites[d.sprite];
        Rectangle src = sp.src;
        if (d.flip) src.width = -src.width;
        float w = sp.src.width * GS, h = sp.src.height * GS;
        Rectangle dst = { d.base.x - floorf(sp.src.width * 0.5f) * GS, d.base.y - h, w, h };
        DrawTexturePro(s_decorTex, src, dst, { 0, 0 }, 0.0f, WHITE);
    }
}

void BackgroundDrawWarning(void)
{
    if (!s_failed) return;
    const char* msg = "tileset.png tidak ditemukan - salin ke folder Projek_Akhir (atau Projek_Akhir/assets)";
    int tw = MeasureText(msg, 18);
    DrawRectangle(SCREEN_W / 2 - tw / 2 - 10, SCREEN_H - 70, tw + 20, 32, Color{ 120, 20, 20, 220 });
    DrawText(msg, SCREEN_W / 2 - tw / 2, SCREEN_H - 63, 18, RAYWHITE);
}

int BackgroundTerrainAt(Vector2 p)
{
    if (s_terr.empty()) return GRASS;
    return TerrAtImg((int)(p.x / GS), (int)(p.y / GS));
}
