// Logica PS1 dell'inciampo/caduta dello zombie, tradotta dal codice MIPS originale.
//
// Fonte: PS1_ZOMBIE_FALL_TRIGGER_AUDIT.md (SLUS_010.87, banco zombie a 80100000).
//   - distanza:   8007F4F4 (E+868, planare, intera)
//   - decisione:  80102828 (stato 4, inseguimento)
//   - settore:    80100924 -> 8010078C
//   - esito:      80103C6C (stato C, dopo la sequenza 23)
//
// Questo modulo e' "puro": niente indirizzi PC, niente memoria del gioco.
// Si compila e si testa anche su Linux (tests/test_stumble_logic.cpp).
// Ogni punto non ancora verificato nel MIPS e' marcato ASSUNZIONE.
#pragma once

#include <cstdint>

namespace ps1 {

// Selettori di animazione PS1 (stessi indici nei dati ENEM00.NAB/NAC del PC).
constexpr int kSelFall = 0x23;     // braccia protese, caduta in avanti
constexpr int kSelContact = 0x24;  // esito ravvicinato (presa/attacco)
constexpr int kSelGetUp1 = 0x10;   // da prono a mani/ginocchia
constexpr int kSelGetUp2 = 0x11;   // rialzata fino a eretto

// 80102828 (verificato in Ghidra, decompilato e assembly), distanze intere:
//   fascia 1: 3000 < d < 0x1389  maschera E+35C &= ~0x17 (conserva il bit 8)
//   fascia 2: 0x578 < d < 0x641  maschera &= ~0x1B (riarma il bit 8)
//   fascia 3: 1000 < d < 0x579   maschera &= ~0x1D (riarma il bit 8)
//   fascia 4: d <= 1000          maschera &= ~0x1E (riarma il bit 8)
//   altrimenti (1600 < d <= 3000, d > 5000): fascia e bit invariati.
// Le fasce 2-4 avviano altre azioni (es. 0x1C da fascia 2) non ancora portate.
constexpr int kBand1Min = 3000;     // escluso
constexpr int kBand1Max = 5000;     // incluso (d < 0x1389)
constexpr int kBand2Min = 0x578;    // 1400, escluso
constexpr int kBand2Max = 0x640;    // 1600, incluso
constexpr int kBand3Min = 1000;     // escluso
constexpr int kBand4Max = 1000;     // incluso
// 80102AC4: slti v0,v0,0x0C  ->  (rng & 0xFF) < 0x0C
constexpr int kRngThreshold = 0x0C;
// 8010078C: raggi a heading +/- 0x28*0x0B unita' angolari (4096 per giro).
constexpr int kSectorHalfAngle = 0x28 * 0x0B;  // 440 -> 38,67 gradi
constexpr int kAngleUnitsPerTurn = 4096;
// 80103C6C: esito ravvicinato se distanza aggiornata < 0x380.
constexpr float kContactDistance = 0x380;
// 80103C6C: il controllo avviene quando il cursore Q1 raggiunge inizio+8.
constexpr int kExitCheckFrame = 8;

struct StumbleState {
  int band = 0;                  // E+358
  bool attemptConsumed = false;  // E+35C bit 8
};

// 80102828, E+31C == 0: all'ingresso nello stato 4 fascia e bit si azzerano.
void ResetOnChaseStart(StumbleState& s);

// Aggiorna la fascia memorizzata con la distanza planare (8007F4F4 la tronca a int).
void UpdateBand(StumbleState& s, float distance);

// Decisione per un update in stato di inseguimento.
//   rngByte:        byte casuale 0..255 (PS1: 80011570() & 0xFF)
//   playerInSector: esito di InFrontSector (80100924(E,0x28) -> 8010078C)
// Ritorna true se lo zombie deve richiedere la sequenza kSelFall.
// Come sul PS1, il tentativo viene consumato anche se rng/settore falliscono.
bool ShouldStartFall(StumbleState& s, int rngByte, bool playerInSector);

// 8010078C: il punto (toX,toZ), relativo allo zombie, cade nel cono di
// +/- kSectorHalfAngle attorno alla direzione (forwardX,forwardZ).
// Il MIPS usa i segni di due prodotti vettoriali con i raggi ruotati; per un
// semiangolo < 90 gradi equivale al confronto con il coseno usato qui.
bool InFrontSector(float forwardX, float forwardZ, float toX, float toZ);

enum class FallExit { Contact, GetUp };

// Esito al frame kExitCheckFrame della caduta (80103C6C).
//   playerFrontBit: E+320 bit 0x40 (zombie nel settore davanti al giocatore)
//   gateOk:         80100628 (script/stato giocatore/altri nemici)
FallExit DecideFallExit(float distance, bool playerFrontBit, bool gateOk);

}  // namespace ps1
