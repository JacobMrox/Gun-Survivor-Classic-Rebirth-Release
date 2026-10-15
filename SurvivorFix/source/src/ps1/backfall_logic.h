// Logica PS1 della caduta all'indietro dello zombie: 0x20 -> 08 -> 0E -> 0F.
//
// Fonte: banco zombie PS1 a 80100000 (ENEMY_SLOT40), letto in Ghidra il 2026-10-15.
//   - avvio, 80102D74 (stato 6), ramo LAB_801030C0: con l'animazione corrente
//     0x20 e il cursore Q1 esattamente a inizio(0x20)+10, se rng < 0xCC parte
//     80080334(E,08,4) e P+14 = 0x14 (spinta).
//   - spinta, sottostato 3: a ogni update E+87C += E+378 * P+14, poi P+14 -= 2,
//     fino a P+14 <= 0. Per la 0x20, il caso 0x1A mette P+10 = 500 e
//     E+378 = avanti(E+1C4) * (P+10/100) con segno secondo +0x32B: modulo 5.
//   - fine della 08 (80100728): 800802C8(E,4,0E); fine della 0E (80104108,
//     E+320 bit 8): 80080334(E,0F,2); fine della 0F: locomozione.
// Modulo puro, testato in tests/test_backfall_logic.cpp.
#pragma once

namespace ps1 {

constexpr int kSelBackFallTrigger = 0x20;  // gesto durante il quale si decide
constexpr int kSelBackFall = 0x08;         // caduta all'indietro
constexpr int kSelSupine = 0x0E;           // da supino solleva il busto
constexpr int kSelSitUp = 0x0F;            // rialzata da seduto

// LAB_801030C0: ...+0x80 (voce 0x20) + 10 == cursore Q1.
constexpr int kBackFallCheckFrame = 10;
// LAB_801030C0: (rng & 0xFF) < 0xCC, circa 80%.
constexpr int kBackFallRngThreshold = 0xCC;
// P+14 iniziale e decremento per update.
constexpr int kBackPushStart = 0x14;
constexpr int kBackPushStep = 2;
// |E+378| per la 0x20: P+10 / 100 = 500 / 100.
constexpr float kBackPushMagnitude = 5.0f;

bool ShouldStartBackFall(int rngByte);

struct BackPush {
  int strength = 0;  // P+14
};

// Avvia la spinta (80080334(E,08,4) mette P+14 = 0x14).
void StartBackPush(BackPush& p);

// Spostamento di questo update in unita' PS1 (|E+378| * P+14), poi P+14 -= 2.
// 0 quando la spinta e' finita.
float NextBackPush(BackPush& p);

}  // namespace ps1
