#pragma once
#pragma once
#include "raylib.h"
#include <string>
#include <vector>

struct DialogueData {
    std::string speakerName;
    char speakerId;
    std::string text;
};

// Deklarasi fungsi intro
bool RunIntroDialogue();