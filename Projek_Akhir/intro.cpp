#include "intro.h"

bool RunIntroDialogue() {
    std::vector<DialogueData> dialogues = {
        { "Orang A", 'A', "Halo! Selamat datang di proyek game akhir kita." },
        { "Orang B", 'B', "Halo juga! Apakah semua sistem permainannya sudah siap?" },
        { "Orang A", 'A', "Tentu saja, kodenya sudah terhubung dengan baik." },
        { "Orang B", 'B', "Mantap! Kalau begitu, mari kita mulai permainannya sekarang." }
    };
    size_t currentDialogue = 0;

    Font font = LoadFont("C:/Windows/Fonts/arial.ttf");

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            currentDialogue++;
            if (currentDialogue >= dialogues.size()) {
                UnloadFont(font);
                return true; // Intro selesai, lanjut ke game utama!
            }
        }

        const DialogueData& current = dialogues[currentDialogue];

        BeginDrawing();
        ClearBackground(RAYWHITE); // Background putih bersih

        Color colorA = (current.speakerId == 'A') ? Color{ 20, 20, 20, 255 } : Color{ 160, 160, 160, 255 };
        Color colorB = (current.speakerId == 'B') ? Color{ 20, 20, 20, 255 } : Color{ 160, 160, 160, 255 };

        // Gambar Avatar Bulatan Hitam
        DrawCircle(165, 265, 45, colorA);
        DrawCircleLines(165, 265, 45, LIGHTGRAY);
        DrawText("Orang A", 135, 325, 14, DARKGRAY);

        DrawCircle(625, 265, 45, colorB);
        DrawCircleLines(625, 265, 45, LIGHTGRAY);
        DrawText("Orang B", 595, 325, 14, DARKGRAY);

        // Kotak Percakapan
        DrawRectangle(60, 410, 680, 140, Color{ 245, 245, 245, 255 });
        DrawRectangleLines(60, 410, 680, 140, LIGHTGRAY);

        // Teks Percakapan
        DrawTextEx(font, (current.speakerName + " :").c_str(), Vector2{ 80.f, 425.f }, 18.f, 1.f, BLACK);
        DrawTextEx(font, current.text.c_str(), Vector2{ 80.f, 455.f }, 20.f, 1.f, DARKGRAY);
        DrawText("[ Klik Kiri untuk Lanjut ]", 80, 525, 12, GRAY);

        EndDrawing();
    }

    UnloadFont(font);
    return false;
}