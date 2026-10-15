// Logica PS1 dell'inseguimento dello zombie (stato 4, 801027C0), direzione e passo.
//
// Fonte: docs/ps1_zombie_navigation.md (banco zombie PS1 a 80100000, letto in Ghidra).
//   - linea di vista:  80100AE8 (segmento a->b contro i muri di ROOMENV, un lato attivo)
//   - linea libera:    8007F210 (cammina e sterza verso il giocatore, 6..0x28 unita' per frame)
//   - aggiramento:     80100E24 (sonde davanti e a +/-90 gradi, rotazione da fermo di 0x16)
//   - movimento:       8007F404 (la velocita' si azzera a ogni frame: senza passo, fermo)
// Tutto e' nello spazio PS1 di ROOMENV.DAT: chi chiama converte le coordinate PC
// (x_PS1 = x_PC, z_PS1 = -z_PC, misura in gioco nella stanza 7/0).
//
// Modulo puro, testato in tests/test_nav_logic.cpp. I vettori sono sul piano XZ.
#pragma once

#include <cstdint>
#include <vector>

namespace ps1 {

struct Wall {
  int32_t x1, z1, x2, z2;
};

struct Vec2 {
  float x, z;
};

// 80100AE8: indice del primo muro che il segmento a->b attraversa partendo dal lato
// attivo del muro (prodotto vettoriale <= 0), oppure -1. Con ignoreNear > 0 salta i
// muri a meno di ignoreNear da b.
int FirstBlockingWall(const std::vector<Wall>& walls, Vec2 a, Vec2 b, float ignoreNear = 0.0f);

// Costanti di 80100E24 / 80100C90 / 80100924 / 8007F210. Angoli in radianti.
constexpr float kAngleUnit = 2.0f * 3.14159265f / 4096.0f;
constexpr float kTurnStep = 0x16 * kAngleUnit;                     // 1,93 gradi
constexpr float kSideProbeAngle = 0x400 * kAngleUnit;              // 90 gradi
constexpr float kForwardConeHalfAngle = 0x5A * 0x0B * kAngleUnit;  // 87 gradi
// 8008286C(rot, out, len) ruota (0,0,len<<8) e scala >>8: vettore lungo len.
// La sonda di 80100C90 e' quindi 100 * 0x14.
constexpr float kProbeDistance = 100.0f * 0x14;
constexpr float kNearWallDistance = 500.0f;  // 80100E24: 80100DC8 < 500

// 8007F210: unita' angolari per frame secondo cos(velocita', verso il giocatore) * 10.
// Indice 0 (e coseno negativo): 0x28; 9: 6.
int SteerUnits(float cosine);

enum class NavMode { Straight = 0, TurnLeft = 1, TurnRight = 2 };

struct NavState {
  NavMode mode = NavMode::Straight;  // P+0
};

struct NavInput {
  Vec2 pos;      // zombie
  Vec2 forward;  // direzione attuale dello zombie (E+1C6)
  Vec2 player;   // bersaglio
};

struct NavOutput {
  bool lineOfSight;  // nessun muro fra zombie e giocatore
  Vec2 heading;      // nuova direzione (normalizzata)
  bool walk;         // true: avanza (E+87C = avanti * E+866); false: gira fermo
  bool turnLeft;     // con walk == false: verso della rotazione (+90 gradi = sinistra)
};

struct NavParams {
  float probeDistance = kProbeDistance;
  float turnStep = kTurnStep;
  // Moltiplica tutte le rotazioni per update: il PS1 lavora per frame, il PC per update AI.
  float rateScale = 1.0f;
  // Adattamento PC. Sul PS1 anche il giocatore si muove contro questi muri (8007CB9C,
  // chiamata da 800785C0/8008369C), quindi il suo centro non entra mai nel margine fra
  // la linea e la parete vera. Sul PC il giocatore collide con le mesh e ci entra: nella
  // linea di vista si ignorano i muri a meno di questa distanza dal giocatore.
  float playerWallMargin = 1000.0f;
};

// Un update dello stato 4 (801027C0 -> 80100AE8 -> 8007F210 oppure 80100E24).
// "Sinistra" e' la rotazione di +90 gradi in XZ; il verso delle rotazioni segue le scelte
// di 8007F210/80100E24 (verso il lato del giocatore) indipendentemente dalla convenzione
// degli angoli PS1.
NavOutput NavStep(NavState& state, const std::vector<Wall>& walls, const NavInput& in,
                  const NavParams& params = NavParams());

// Utilita' esposte per i test.
Vec2 Rotate(Vec2 v, float radians);
float DistanceToWall(Vec2 p, const Wall& w);

}  // namespace ps1
