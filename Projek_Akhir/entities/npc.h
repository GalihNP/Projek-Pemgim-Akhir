// entities/npc.h
#pragma once
#include "core/types.h"

// Panggil SETELAH WorldInit() + BackgroundInit() (posisi NPC dicari di tempat
// yang tidak menabrak dinding / objek lingkungan / air).
void NpcInitAll(Npc* npcs, int* outCount, const World* w, Vector2 playerSpawn);
void NpcAssetsUnload(void);

// Dorong lingkaran (player / musuh) keluar dari badan NPC.
void NpcResolveCircle(const Npc* npcs, int count, Vector2* pos, float radius);

// Index NPC terdekat yang bisa diajak bicara, atau -1.
int  NpcFindTalkable(const Npc* npcs, int count, Vector2 playerPos);

// Digambar di dalam BeginMode2D. talkableIndex = NPC yang menampilkan prompt [E].
void NpcDrawAll(const Npc* npcs, int count, int talkableIndex, int speakingIndex);

// --- Dialog ---
void DialogueStart(DialogueState* d, Npc* npc, int npcIndex);
// Mengembalikan false jika dialog sudah selesai.
bool DialogueUpdate(DialogueState* d, const Npc* npc, bool advancePressed, float dt);
// Digambar di layar (di luar BeginMode2D).
void DialogueDraw(const DialogueState* d, const Npc* npc);
