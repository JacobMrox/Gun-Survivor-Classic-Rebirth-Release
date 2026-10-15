// Indirizzi e layout di gun_bio.exe (PC, base 0x00400000).
//
// Regola: qui entra solo cio' che e' verificato in assembly nei report della
// radice del repo. Ogni voce cita il documento. Prima di agganciare una
// funzione, game::CheckBytes() confronta i byte attesi in memoria: se l'exe e'
// diverso (altra build, patch) la correzione resta spenta invece di crashare.
#pragma once

#include <cstddef>
#include <cstdint>

namespace game {

constexpr uintptr_t kImageBase = 0x00400000;

// SHA-256 dell'exe originale usato per la ricerca (STATUS.md).
// Solo informativo: la patch inglese di Reikaz potrebbe cambiare l'hash.
constexpr const char* kOriginalSha256 =
    "2e5b7a3d4d53a68499b36be5ec45fffa30ccfaaca24e66650cccdda8651be3f9";

struct ByteSignature {
  uintptr_t address;
  const uint8_t* bytes;
  size_t size;
};

bool CheckBytes(const ByteSignature& sig);

// ---------------------------------------------------------------------------
// FUN_00436B40: richiesta e applicazione animazione.
// ABI verificata (FUNCTION_MAP.md, "Convenzioni di chiamata e ABI"):
//   ECX = S, due argomenti su stack (richiesta, flag), RET 8  -> __thiscall
// Byte attesi (PS1_PC_RESTORATION_AUDIT.md, "Punti assembly"):
//   00436B61: 89 46 1C        mov [esi+1C], eax   (scrive S+1C)
//   00436B64: E8 D7 02 00 00  call 00436E40       (mapper)
constexpr uintptr_t kRequestAnim = 0x00436B40;
inline constexpr uint8_t kRequestAnimStore[] = {0x89, 0x46, 0x1C};
inline constexpr uint8_t kRequestAnimCall[] = {0xE8, 0xD7, 0x02, 0x00, 0x00};

// ---------------------------------------------------------------------------
// FUN_00436E40: mapper richiesta -> selettore.
// Verifica Ghidra 2026-10-15: ECX = S (MOV ESI,ECX), un argomento su stack
// (richiesta), tutti i 211 RET sono RET 4 -> __thiscall. Ritorna il selettore,
// che 00436B40 applica (S+28, poi 00412B90 per ogni traccia) solo se diverso
// dal corrente o se S+24 != 0. Per il tipo 0 con richieste 2/3/4 "mantiene"
// il selettore finche' la sequenza non e' completa (ANIMATIONS.md).
constexpr uintptr_t kMapAnim = 0x00436E40;
inline constexpr uint8_t kMapAnimPrologue[] = {0x83, 0xEC, 0x40, 0x53,
                                               0x55, 0x56, 0x8B, 0xF1};

// FUN_00475530: costruisce il percorso dell'HMD di stanza
// PSX_DATA/stage/STAGEss/rrrr/rrrr_A.HMD (o _B) da (stage, stanza, variante).
// Verifica Ghidra 2026-10-15: __cdecl (out, stage, stanza, variante), RET semplice.
constexpr uintptr_t kRoomPath = 0x00475530;
inline constexpr uint8_t kRoomPathPrologue[] = {0x83, 0xEC, 0x14, 0xA1,
                                                0x28, 0xF6, 0x5B, 0x00};

// FUN_0045FD70: accumulo del movimento locale di O.
// Verifica Ghidra 2026-10-15: ECX = O, tre float su stack (x, y, z), RET 0xC;
// accum(O+D4/D8/DC) += xyz * (1.0 / O+120), azzera O+104/O+105.
constexpr uintptr_t kAccumulate = 0x0045FD70;
inline constexpr uint8_t kAccumulatePrologue[] = {0x83, 0xEC, 0x0C, 0xD9,
                                                  0x81, 0x20, 0x01, 0x00, 0x00};

// FUN_0043A8A0: update dell'AI per un S.
// Verifica Ghidra 2026-10-15: ECX = S, nessun argomento, tutti i 35 RET sono
// C3; ritorna in EAX la richiesta passata a 00436B40 oppure -1.
constexpr uintptr_t kUpdateEnemy = 0x0043A8A0;
inline constexpr uint8_t kUpdateEnemyPrologue[] = {0x83, 0xEC, 0x44, 0x53,
                                                   0x55, 0x56, 0x8B, 0xF1};

// ---------------------------------------------------------------------------
// Blocco S (ANIMATIONS.md, "Campi del blocco S rilevanti").
namespace S {
constexpr size_t kCollection = 0x0C;  // C*
constexpr size_t kIndex = 0x10;       // int, indice di O in C+0C
constexpr size_t kType = 0x14;        // int, "tipo" che pilota la conversione
constexpr size_t kRequest = 0x1C;     // int, richiesta piu' recente
constexpr size_t kPhase = 0x20;       // byte, fase 0/1/2
constexpr size_t kSelector = 0x28;    // int, selettore corrente (-1 = riavvio)
// byte, completamenti richiesti prima che 00436B40 accetti un altro selettore.
constexpr size_t kThreshold = 0x2C;
// Proposti dal mapper e copiati da 00436B40 all'applicazione: +3C -> +2C
// (byte, completamenti prima di poter cambiare), +40 -> +30 (float, velocita').
constexpr size_t kNextThreshold = 0x3C;
constexpr size_t kNextSpeed = 0x40;
// int, azzerato all'applicazione, +1 a ogni completamento della traccia 0
// (ciclo iniziale di 0043A8A0).
constexpr size_t kLoops = 0x34;
}  // namespace S

// Collezione C (0043A8A0: O = [[C+0C] + 4*S+10], T = [[C+0C] + 4*C+14]).
namespace C {
constexpr size_t kObjects = 0x0C;  // O**
constexpr size_t kTarget = 0x14;   // int, indice del designato T
}  // namespace C

// Oggetto O (stesso layout per T). FUNCTION_MAP.md e 0043A8A0.
namespace O {
constexpr size_t kPos = 0x08;      // float x, y, z
constexpr size_t kModel = 0x7C;    // M*
constexpr size_t kAccum = 0xD4;    // float x, y, z: movimento accumulato da 0045FD70
// Matrice di rotazione 3x3 a +3C (righe da 12 byte; 0045E9D0 usa "matrice+0x3C").
// La terza riga, +5C/+60/+64, e' la direzione in avanti: letta in gioco il
// 2026-10-15 coincide con la direzione zombie->giocatore durante l'inseguimento
// (0,809/0,588 contro 0,82/0,57; 0,156/0,988 contro 0,14/0,99).
// +E0 invece valeva (0,10,0) sullo zombie: non e' l'heading.
// Matrice 4x4 a +3C, righe da 16 byte (3 float + 1), traslazione a +6C (= O.pos):
// riga 0 = (cos, 0, -sin), riga 1 = (0, 1, 0), riga 2 = avanti (letta in gioco).
constexpr size_t kMatrix = 0x3C;
constexpr size_t kMatrixRowFloats = 4;
constexpr size_t kForward = 0x5C;  // float x, y, z
// float x, y, z: punto mirato da chi insegue O (0043A8A0 lo porta nel locale dello
// zombie con 0045E9D0 e decide rotazione/avanzamento). FUNCTION_MAP.md, ENEMY_AI.md.
constexpr size_t kAimPoint = 0xB8;
}  // namespace O

// Modello M e tracce Q (record da 0x60 byte). ANIMATIONS.md e 0043A8A0.
namespace M {
constexpr size_t kTracks = 0x2C;      // Q*
constexpr size_t kTrackCount = 0x30;  // unsigned
constexpr size_t kTrackSize = 0x60;
}  // namespace M
namespace Q {
constexpr size_t kIndex = 0x10;   // int, indice effettivo della sequenza
constexpr size_t kCursor = 0x44;  // int, cursore nello stream dei comandi
constexpr size_t kPairs = 0x5C;   // coppie (controllo u32, cursore iniziale u32)
}  // namespace Q

template <typename T>
inline T Read(const void* base, size_t offset) {
  return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(base) + offset);
}

template <typename T>
inline T* Field(void* base, size_t offset) {
  return reinterpret_cast<T*>(static_cast<uint8_t*>(base) + offset);
}

}  // namespace game
