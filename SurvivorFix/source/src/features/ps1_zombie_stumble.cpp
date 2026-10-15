// Inciampo/caduta PS1 dello zombie (tipo 0): 23 -> 10 -> 11, come in 80102828/80103C6C.
//
// Logica PS1: src/ps1/stumble_logic.* (fasce, riarmo e settore verificati nel MIPS).
// Aggancio PC (indirizzi e ABI in game.h, verificati in Ghidra):
//   - mapper 00436E40: durante l'inseguimento (richieste 2/3/4) valuta il
//     tentativo PS1 e, se riesce, restituisce 0x23 al posto della camminata.
//     Finche' la sequenza e' in corso restituisce il selettore voluto: per il
//     tipo 0 il gioco "mantiene" lo stesso selettore senza riapplicarlo.
//     Qui si decidono anche i passaggi (uscita a inizio+8, fine di 10 e 11),
//     nello stesso update in cui il gioco registra il completamento.
//   - update 0043A8A0: riserva per i passaggi quando l'AI non chiede
//     animazioni, e azzera il movimento accumulato, cosi' lo zombie non
//     scivola mentre e' a terra.
//
// ASSUNZIONE: le unita' di posizione PC equivalgono a quelle PS1 (scala 1:1).
//   Le soglie dell'AI PC hanno lo stesso ordine di grandezza (es. 1500 in
//   ANIMATIONS.md); il log stampa la distanza di ogni tentativo per verificarlo.
// Direzione in avanti: terza riga della matrice di O (game.h, O::kForward),
//   verificata leggendo la memoria in gioco. Il log stampa l'angolo di ogni tentativo.
// ASSUNZIONE: T (C+14) e' il giocatore.
// ASSUNZIONE: il cursore Q+44 conta i comandi come il cursore PS1 Q1+16
//   (stessi stream e coppie, PS1_ZOMBIE_FALL_TRIGGER_AUDIT.md); il log stampa
//   l'inizio letto, atteso 1335 per la traccia 1.
// TODO: esito ravvicinato 24 (presa) e gate 80100628: per ora l'esito vicino
//   restituisce lo zombie all'AI del PC, che decide l'attacco.
// TODO: le fasce 2-4 del PS1 avviano altre azioni (0x1C, presa): non portate.
// TODO: fra inizio+2 e inizio+7 della 23 il PS1 da' allo zombie una velocita'
//   in avanti (80103C6C: 8008286C(E+1C4, E+87C, E+866<<3)), uno slancio.
//   Qui lo zombie resta fermo finche' non si chiarisce E+866.
//
// Velocita' e soglia: di default le stesse che il mapper PC usa per le sue
// cadute e rialzate (0C/13 a 00437021, 10/11 a 0043721C): S+40 = 2.0, S+3C = 1.
// Per la 23 la soglia e' 0, altrimenti la 10 non partirebbe a inizio+8.
// Velocita' e comando di uscita si regolano da [Ps1ZombieStumble] nel .ini.
//
// Prove: Ps1ZombieStumbleHotkey=1 fa cadere con un tasto (Ps1ZombieStumbleKey,
// default Ins) lo zombie in inseguimento piu' vicino (GetAsyncKeyState letto
// nell'hook dell'update, solo con la finestra del gioco in primo piano, sul
// fronte di salita). F9 no: il gioco lo legge via DirectInput e apre il menu
// di uscita.
//
// Caduta all'indietro (Ps1ZombieBackFall, src/ps1/backfall_logic.*): mentre
// gira la 0x20 (sul PC uno dei gesti 1B-21 della richiesta 08), al comando
// inizio+10 un tiro all'80% passa a 08 -> 0E -> 0F come in 80102D74/80104108.
// La spinta all'indietro dei primi 10 update passa da 0045FD70, l'accumulatore
// locale del gioco, cosi' restano le collisioni.
// ASSUNZIONE: nell'accumulatore +Z locale e' "avanti" (la camminata accumula
//   (0,0,S+74) in 0043A8A0): la spinta usa -Z.
// ASSUNZIONE: la scala della spinta PC non e' nota (0045FD70 divide per O+120):
//   Push nel .ini moltiplica le unita' PS1, default 0 = cade sul posto.
// ASSUNZIONE: sul PS1 il segno della spinta dipende da +0x32B (direzione);
//   qui e' sempre all'indietro, coerente con la caduta all'indietro.
#include <windows.h>

#include <cmath>
#include <cstdint>
#include <unordered_map>

#include "../config.h"
#include "../game.h"
#include "../hooks.h"
#include "../log.h"
#include "../ps1/backfall_logic.h"
#include "../ps1/stumble_logic.h"
#include "features.h"

namespace sf::features {

namespace {

using MapAnimFn = int(__fastcall*)(void* s, void* edx, int request);
using UpdateEnemyFn = int(__fastcall*)(void* s, void* edx);
using AccumulateFn = void(__fastcall*)(void* o, void* edx, float x, float y, float z);
MapAnimFn g_mapAnim = nullptr;
UpdateEnemyFn g_updateEnemy = nullptr;
// Non agganciata: la si chiama soltanto (ABI in game.h).
const auto g_accumulate = reinterpret_cast<AccumulateFn>(game::kAccumulate);

constexpr int kZombieType = 0x00;
// Oltre questo numero di update in una fase lo zombie torna all'AI (S riusato,
// sequenza mai applicata, cursore mai raggiunto).
constexpr int kPhaseTimeout = 900;
// Uno zombie e' "vivo" per il tasto se il suo update e' passato da poco: le voci di
// stanze precedenti restano nella mappa ma i loro S non vanno piu' letti.
constexpr DWORD kAliveMs = 500;

enum class Phase { Idle, Fall, GetUp1, GetUp2, BackFall, Supine, SitUp };

struct Entry {
  ps1::StumbleState state;
  int prevRequest = -1;
  Phase phase = Phase::Idle;
  int updates = 0;
  bool applied = false;
  bool forceFall = false;  // richiesta dal tasto, parte alla prossima locomozione
  DWORD lastSeen = 0;      // GetTickCount dell'ultimo update
  bool backRolled = false;  // tiro della 0x20 gia' fatto per questa applicazione
  ps1::BackPush push;
  ps1::NavState nav;
  bool navBlocked = false;  // ultimo stato registrato nel log
  int navMode = 0;          // ultimo modo registrato nel log
};

std::unordered_map<void*, Entry> g_entries;
uint32_t g_rng = 0;
bool g_hotkeyDown = false;

int NextRngByte() {
  g_rng = g_rng * 0x015A4E35u + 1u;  // stesso LCG del gioco (00407230)
  return static_cast<int>((g_rng >> 16) & 0xFF);
}

bool IsLocomotion(int request) { return request >= 2 && request <= 4; }
// 0043A8A0 con O+11C (vita) <= 0 chiede 0x10 o 0x0F.
bool IsDeath(int request) { return request == 0x10 || request == 0x0F; }
bool IsBackChain(Phase phase) {
  return phase == Phase::BackFall || phase == Phase::Supine || phase == Phase::SitUp;
}

float PhaseSpeed(Phase phase) {
  switch (phase) {
    case Phase::Fall: return g_config.fallSpeed;
    case Phase::GetUp1: return g_config.getUp1Speed;
    case Phase::GetUp2: return g_config.getUp2Speed;
    case Phase::BackFall: return g_config.backFallSpeed;
    case Phase::Supine: return g_config.supineSpeed;
    default: return g_config.sitUpSpeed;
  }
}

int PhaseSelector(Phase phase) {
  switch (phase) {
    case Phase::Fall: return ps1::kSelFall;
    case Phase::GetUp1: return ps1::kSelGetUp1;
    case Phase::GetUp2: return ps1::kSelGetUp2;
    case Phase::BackFall: return ps1::kSelBackFall;
    case Phase::Supine: return ps1::kSelSupine;
    case Phase::SitUp: return ps1::kSelSitUp;
    default: return -1;
  }
}

// O e T dello zombie, o false se la catena di puntatori non e' valida.
bool GetObjects(void* s, uint8_t*& o, uint8_t*& t) {
  auto* c = game::Read<uint8_t*>(s, game::S::kCollection);
  if (!c) return false;
  auto** objects = game::Read<uint8_t**>(c, game::C::kObjects);
  if (!objects) return false;
  o = objects[game::Read<int>(s, game::S::kIndex)];
  t = objects[game::Read<int>(c, game::C::kTarget)];
  return o && t && o != t;
}

struct Geometry {
  float distance;  // planare, come 8007F4F4
  float toX, toZ;  // da O a T
};

Geometry Measure(const uint8_t* o, const uint8_t* t) {
  const float* op = reinterpret_cast<const float*>(o + game::O::kPos);
  const float* tp = reinterpret_cast<const float*>(t + game::O::kPos);
  Geometry g;
  g.toX = tp[0] - op[0];
  g.toZ = tp[2] - op[2];
  g.distance = std::sqrt(g.toX * g.toX + g.toZ * g.toZ);
  return g;
}

bool InSectorOf(const uint8_t* viewer, float toX, float toZ) {
  const float* f = reinterpret_cast<const float*>(viewer + game::O::kForward);
  return ps1::InFrontSector(f[0], f[2], toX, toZ);
}

float AngleDegrees(const uint8_t* viewer, float toX, float toZ) {
  const float* f = reinterpret_cast<const float*>(viewer + game::O::kForward);
  const float len = std::sqrt((f[0] * f[0] + f[2] * f[2]) * (toX * toX + toZ * toZ));
  if (len <= 0.0f) return -1.0f;
  return std::acos((f[0] * toX + f[2] * toZ) / len) * 180.0f / 3.14159265f;
}

// Traccia Q usata per l'uscita: la 1 come sul PS1 (Q1), altrimenti la 0.
uint8_t* ExitTrack(const uint8_t* o) {
  auto* m = game::Read<uint8_t*>(o, game::O::kModel);
  if (!m) return nullptr;
  auto* tracks = game::Read<uint8_t*>(m, game::M::kTracks);
  const auto count = game::Read<unsigned>(m, game::M::kTrackCount);
  if (!tracks || count == 0) return nullptr;
  return tracks + (count > 1 ? game::M::kTrackSize : 0);
}

int TrackStartCursor(const uint8_t* q) {
  auto* pairs = game::Read<const uint32_t*>(q, game::Q::kPairs);
  if (!pairs) return -1;
  return static_cast<int>(pairs[game::Read<int>(q, game::Q::kIndex) * 2 + 1]);
}

void SetPhase(void* s, Entry& e, Phase phase, const char* why) {
  Log("Ps1ZombieStumble: S=%p %s", s, why);
  e.phase = phase;
  e.updates = 0;
  e.applied = false;
}

// Passaggi di fase: uscita della 23 a inizio+8, completamenti di 10 e 11.
// Si valuta nel mapper, che in 0043A8A0 viene dopo il ciclo dei completamenti:
// nello stesso update il cursore e' gia' tornato all'inizio della sequenza
// finita, quindi la successiva va applicata subito, altrimenti per un frame si
// vede la posa iniziale (lo scatto a fine rialzata). L'update fa da riserva
// quando l'AI non chiede animazioni.
void Step(void* s, Entry& e) {
  uint8_t *o, *t;
  if (!GetObjects(s, o, t)) {
    e.phase = Phase::Idle;
    return;
  }
  const int selector = game::Read<int>(s, game::S::kSelector);
  if (selector != PhaseSelector(e.phase)) {
    if (e.applied) SetPhase(s, e, Phase::Idle, "selettore cambiato dal gioco, torna all'AI");
    return;  // non ancora applicato: il mapper continua a chiederlo
  }
  if (!e.applied) {
    e.applied = true;
    if (e.phase == Phase::Fall) {
      if (const uint8_t* q = ExitTrack(o)) {
        Log("Ps1ZombieStumble: S=%p caduta applicata, cursore %d inizio %d", s,
            game::Read<int>(q, game::Q::kCursor), TrackStartCursor(q));
      }
    }
    return;  // appena applicata: niente completamenti da valutare
  }

  const bool completed = game::Read<int>(s, game::S::kLoops) > 0;
  if (IsBackChain(e.phase)) {
    if (!completed) return;
    if (e.phase == Phase::BackFall) {
      SetPhase(s, e, Phase::Supine, "a terra: 0E");
    } else if (e.phase == Phase::Supine) {
      SetPhase(s, e, Phase::SitUp, "rialzata 0F");
    } else {
      SetPhase(s, e, Phase::Idle, "in piedi dopo la caduta all'indietro, torna all'AI");
    }
    return;
  }
  if (e.phase == Phase::Fall) {
    const uint8_t* q = ExitTrack(o);
    const int start = q ? TrackStartCursor(q) : -1;
    const bool reached =
        q && start >= 0 &&
        game::Read<int>(q, game::Q::kCursor) >= start + g_config.exitFrame;
    if (!reached && !completed) return;
    const Geometry g = Measure(o, t);
    // E+320 bit 0x40: zombie nel settore davanti al giocatore (800824B0).
    const bool front = InSectorOf(t, -g.toX, -g.toZ);
    // ASSUNZIONE: gate 80100628 sempre superato (vedi TODO in testa).
    if (ps1::DecideFallExit(g.distance, front, true) == ps1::FallExit::Contact) {
      SetPhase(s, e, Phase::Idle, "esito ravvicinato, torna all'AI (24 non portato)");
    } else {
      SetPhase(s, e, Phase::GetUp1, "esito: rialzata 10");
    }
  } else if (completed) {
    if (e.phase == Phase::GetUp1) {
      SetPhase(s, e, Phase::GetUp2, "rialzata 11");
    } else {
      // Sul PS1 la 10 riporta allo stato 4 (800802C8(E,4,10)): il commit
      // azzera E+31C e 80102828 riparte da fascia 0 con il tentativo riarmato.
      // In modalita' prova niente riarmo: altrimenti lo zombie ricade appena in
      // piedi e le cadute sembrano accodate. Si riarma avvicinandosi sotto 1600.
      if (!g_config.ps1ZombieStumbleAlways) ps1::ResetOnChaseStart(e.state);
      SetPhase(s, e, Phase::Idle, "in piedi, torna all'AI");
    }
  }
}

void HoldSequence(void* s, Entry& e, int& result) {
  result = PhaseSelector(e.phase);
  *game::Field<uint8_t>(s, game::S::kNextThreshold) = e.phase == Phase::Fall ? 0 : 1;
  *game::Field<float>(s, game::S::kNextSpeed) = PhaseSpeed(e.phase);
}

// 80102D74: con la 0x20 in corso, al cursore Q1 inizio+10 tira per la 08.
bool TryStartBackFall(void* s, Entry& e, int& result) {
  if (game::Read<int>(s, game::S::kSelector) != ps1::kSelBackFallTrigger) {
    e.backRolled = false;
    return false;
  }
  if (e.backRolled) return false;
  uint8_t *o, *t;
  if (!GetObjects(s, o, t)) return false;
  const uint8_t* q = ExitTrack(o);
  const int start = q ? TrackStartCursor(q) : -1;
  if (start < 0 || game::Read<int>(q, game::Q::kCursor) < start + g_config.backFallFrame) {
    return false;
  }
  e.backRolled = true;
  const int rng = g_config.ps1ZombieBackFallAlways ? 0 : NextRngByte();
  if (!ps1::ShouldStartBackFall(rng)) {
    Log("Ps1ZombieBackFall: S=%p 0x20 senza caduta (rng=%02X)", s, rng);
    return false;
  }
  Log("Ps1ZombieBackFall: S=%p caduta all'indietro 08 (d=%.0f rng=%02X)", s,
      Measure(o, t).distance, rng);
  SetPhase(s, e, Phase::BackFall, "inizio caduta all'indietro");
  ps1::StartBackPush(e.push);
  // La 0x20 puo' avere una soglia di completamenti (S+2C): sul PS1 la 08
  // parte comunque a inizio+10, quindi la si rende interrompibile.
  *game::Field<uint8_t>(s, game::S::kThreshold) = 0;
  HoldSequence(s, e, result);
  return true;
}

int MapAnimInner(void* s, void* edx, int request) {
  int result = g_mapAnim(s, edx, request);
  if (!g_config.ps1ZombieStumble && !g_config.ps1ZombieBackFall &&
      !g_config.ps1ZombieNavigation) {
    return result;
  }
  if (game::Read<int>(s, game::S::kType) != kZombieType) return result;

  Entry& e = g_entries[s];
  const bool locomotion = IsLocomotion(request);
  if (IsBackChain(e.phase)) {
    if (IsDeath(request)) {
      Log("Ps1ZombieBackFall: S=%p richiesta %02X, sequenza interrotta", s, request);
      e.phase = Phase::Idle;
    } else {
      // Durante la caduta all'indietro l'AI chiede ancora 08 (azione vicina) o
      // locomozione: si tiene la sequenza, come sul PS1 nello stato 6.
      Step(s, e);
      if (e.phase != Phase::Idle) HoldSequence(s, e, result);
    }
    e.prevRequest = request;
    return result;
  }
  if (e.phase == Phase::Idle && g_config.ps1ZombieBackFall && TryStartBackFall(s, e, result)) {
    e.prevRequest = request;
    return result;
  }
  if (!g_config.ps1ZombieStumble) {
    e.prevRequest = request;
    return result;
  }
  if (e.phase != Phase::Idle) {
    if (locomotion) {
      Step(s, e);
      // Tornato Idle: resta il risultato del mapper (per il tipo 0, con la
      // sequenza completa, una camminata 01-04 scelta dal gioco).
      if (e.phase != Phase::Idle) HoldSequence(s, e, result);
    } else {
      // Danno, morte o altra azione: decide il gioco.
      Log("Ps1ZombieStumble: S=%p richiesta %02X, sequenza interrotta", s, request);
      e.phase = Phase::Idle;
    }
  } else if (locomotion && e.forceFall) {
    e.forceFall = false;
    uint8_t *o, *t;
    if (GetObjects(s, o, t)) {
      Log("Ps1ZombieStumble: S=%p caduta 23 da tasto (d=%.0f)", s, Measure(o, t).distance);
      SetPhase(s, e, Phase::Fall, "inizio caduta");
      HoldSequence(s, e, result);
    }
  } else if (locomotion) {
    uint8_t *o, *t;
    if (GetObjects(s, o, t)) {
      if (!IsLocomotion(e.prevRequest)) ps1::ResetOnChaseStart(e.state);
      const Geometry g = Measure(o, t);
      ps1::UpdateBand(e.state, g.distance);
      const bool attempt = e.state.band == 1 && !e.state.attemptConsumed;
      // Ps1ZombieStumbleAlways (solo prove): rng 0 supera sempre la soglia 0x0C.
      const int rng = g_config.ps1ZombieStumbleAlways ? 0 : NextRngByte();
      const bool inSector = InSectorOf(o, g.toX, g.toZ);
      if (ps1::ShouldStartFall(e.state, rng, inSector)) {
        Log("Ps1ZombieStumble: S=%p caduta 23 (d=%.0f angolo=%.0f rng=%02X)", s,
            g.distance, AngleDegrees(o, g.toX, g.toZ), rng);
        SetPhase(s, e, Phase::Fall, "inizio caduta");
        HoldSequence(s, e, result);
      } else if (attempt) {
        Log("Ps1ZombieStumble: S=%p tentativo fallito (d=%.0f angolo=%.0f rng=%02X)",
            s, g.distance, AngleDegrees(o, g.toX, g.toZ), rng);
      }
    }
  }
  e.prevRequest = request;
  return result;
}

// Ps1ZombieWalk: una sola camminata, come sul PS1.
// Il PS1 sceglie la locomozione all'ingresso nell'inseguimento (801019F0 / 80101D44:
// 80101CE4(E, rng & 1 | 2), cioe' 02 o 03 col preset del primo zombie) e la ripete;
// la fine si controlla sulla traccia 1 (80100728(E,1,1)). Il PC per il tipo 0 sorteggia
// (R & 3) + 1 a ogni "completamento", che 0043A8A0 misura sulla traccia 0: nelle
// camminate 01 e 02 quella traccia fa un ciclo di 4 tick (contro 120/124 delle altre),
// quindi il gioco ne sceglie un'altra dopo pochi tick e riparte dalla posa iniziale:
// lo scatto. Qui il mapper restituisce sempre la camminata corrente (le tracce girano
// in ciclo da sole) e, entrando nella camminata, sceglie 02 o 03.
int ApplyWalkHold(void* s, int request, int result) {
  if (!g_config.ps1ZombieWalk || !IsLocomotion(request) || result < 1 || result > 4) {
    return result;
  }
  if (game::Read<int>(s, game::S::kType) != kZombieType) return result;
  auto it = g_entries.find(s);
  if (it != g_entries.end() && it->second.phase != Phase::Idle) return result;
  const int current = game::Read<int>(s, game::S::kSelector);
  if (current >= 1 && current <= 4) return current;
  const int pick = 2 + (NextRngByte() & 1);
  Log("Ps1ZombieWalk: S=%p camminata %02X (il PC aveva scelto %02X)", s, pick, result);
  return pick;
}

int __fastcall MapAnimDetour(void* s, void* edx, int request) {
  return ApplyWalkHold(s, request, MapAnimInner(s, edx, request));
}

bool GameHasFocus() {
  DWORD pid = 0;
  GetWindowThreadProcessId(GetForegroundWindow(), &pid);
  return pid == GetCurrentProcessId();
}

// Tasto: lo zombie tipo 0 piu' vicino al giocatore, in inseguimento e Idle.
void RequestHotkeyFall() {
  const DWORD now = GetTickCount();
  void* best = nullptr;
  float bestDistance = 0.0f;
  for (auto& [s, e] : g_entries) {
    if (now - e.lastSeen > kAliveMs || e.phase != Phase::Idle ||
        !IsLocomotion(e.prevRequest) ||
        game::Read<int>(s, game::S::kType) != kZombieType) {
      continue;
    }
    uint8_t *o, *t;
    if (!GetObjects(s, o, t)) continue;
    const float d = Measure(o, t).distance;
    if (!best || d < bestDistance) {
      best = s;
      bestDistance = d;
    }
  }
  if (!best) {
    Log("Ps1ZombieStumble: tasto, nessuno zombie in inseguimento");
    return;
  }
  g_entries[best].forceFall = true;
  Log("Ps1ZombieStumble: tasto, caduta richiesta per S=%p (d=%.0f)", best, bestDistance);
}

void PollHotkey() {
  if (!g_config.ps1ZombieStumbleHotkey) {
    g_hotkeyDown = false;
    return;
  }
  const bool down = (GetAsyncKeyState(g_config.ps1ZombieStumbleKey) & 0x8000) != 0 && GameHasFocus();
  const bool pressed = down && !g_hotkeyDown;  // tenere premuto non ripete
  g_hotkeyDown = down;
  if (pressed) RequestHotkeyFall();
}

// Ps1ZombieNavigation: direzione e passo dello zombie decisi dalla logica PS1 dello
// stato 4 (src/ps1/nav_logic.*), sia a linea libera (8007F210) sia con un muro in mezzo
// (80100E24). Al PC resta solo l'avanzamento con la sua velocita' (S+74).
//   - La logica lavora nello spazio PS1, dove z = -z del PC (stessa x; misurato nella
//     stanza 7/0): posizioni e direzioni si convertono in ingresso e in uscita.
//   - La direzione si scrive nella matrice di O (game.h, O::kMatrix) prima dell'AI PC e
//     di nuovo dopo, per annullare i +/-3 gradi che 0043A8A0 aggiunge da se'.
//   - Per far camminare lo zombie l'AI PC mira dritto davanti (scarto laterale < 200: va
//     dritto, richiesta 2); per farlo girare da fermo mira di lato (ruota, richieste 3/4,
//     con la loro animazione). T+B8 si ripristina subito dopo; gli attacchi usano T+8.
// ASSUNZIONE: T+B8 durante l'update dello zombie e' letto solo per mirare.
// RateScale 1,0: il PC fa ~29,4 update/s (T6, limitatore da 34 ms, misura senza strumentazione
// pesante; i ~20/s della prima misura erano falsati dal debugger) e il PS1 ~30 frame/s
// (DAT_80062E1C = 2, da confermare).
void SetHeading(uint8_t* o, float fx, float fz) {
  float* m = game::Field<float>(o, game::O::kMatrix);
  // Rotazione attorno a Y: riga 0 = (fz, 0, -fx), riga 2 = (fx, 0, fz); riga 1 invariata.
  m[0] = fz;
  m[1] = 0.0f;
  m[2] = -fx;
  float* row2 = m + 2 * game::O::kMatrixRowFloats;
  row2[0] = fx;
  row2[1] = 0.0f;
  row2[2] = fz;
}

int UpdateWithNavigation(void* s, void* edx) {
  if (!g_config.ps1ZombieNavigation || game::Read<int>(s, game::S::kType) != kZombieType) {
    return g_updateEnemy(s, edx);
  }
  auto it = g_entries.find(s);
  const auto& walls = CurrentRoomWalls();
  if (it == g_entries.end() || it->second.phase != Phase::Idle || walls.empty() ||
      !IsLocomotion(game::Read<int>(s, game::S::kRequest))) {
    return g_updateEnemy(s, edx);
  }
  uint8_t *o, *t;
  if (!GetObjects(s, o, t)) return g_updateEnemy(s, edx);
  Entry& e = it->second;
  const float* op = reinterpret_cast<const float*>(o + game::O::kPos);
  const float* of = reinterpret_cast<const float*>(o + game::O::kForward);
  const float* tp = reinterpret_cast<const float*>(t + game::O::kPos);
  ps1::NavParams params;
  params.probeDistance = g_config.navProbeDistance;
  params.turnStep = g_config.navTurnStep * 3.14159265f / 180.0f;
  params.rateScale = g_config.navRateScale;
  params.playerWallMargin = g_config.navPlayerWallMargin;
  const ps1::NavOutput nav =
      ps1::NavStep(e.nav, walls, {{op[0], -op[2]}, {of[0], -of[2]}, {tp[0], -tp[2]}}, params);

  if (nav.lineOfSight != !e.navBlocked) {
    e.navBlocked = !nav.lineOfSight;
    Log("Ps1ZombieNavigation: S=%p %s (pos PC %.0f, %.0f; giocatore PC %.0f, %.0f)", s,
        e.navBlocked ? "muro fra zombie e giocatore, aggira" : "linea libera, insegue",
        op[0], op[2], tp[0], tp[2]);
  }
  const int mode = static_cast<int>(e.nav.mode) + (nav.walk ? 0 : 10);
  if (!nav.lineOfSight && mode != e.navMode) {
    const char* what = !nav.walk ? (nav.turnLeft ? "gira da fermo a sinistra"
                                                 : "gira da fermo a destra")
                                 : "avanza dritto";
    Log("Ps1ZombieNavigation: S=%p %s, direzione PC (%.2f, %.2f), pos PC %.0f, %.0f, "
        "giocatore PC %.0f, %.0f",
        s, what, nav.heading.x, -nav.heading.z, op[0], op[2], tp[0], tp[2]);
  }
  e.navMode = nav.lineOfSight ? -1 : mode;

  // Direzione in coordinate PC e punto di mira per l'AI PC.
  const float hx = nav.heading.x, hz = -nav.heading.z;
  float ax = hx, az = hz;
  if (!nav.walk) {
    // Di lato, dalla parte in cui gira (in spazio PS1 +90 = sinistra), poi in PC.
    const ps1::Vec2 side = ps1::Rotate(nav.heading, nav.turnLeft ? ps1::kSideProbeAngle
                                                                 : -ps1::kSideProbeAngle);
    ax = side.x;
    az = -side.z;
  }
  SetHeading(o, hx, hz);
  float* aimPoint = game::Field<float>(t, game::O::kAimPoint);
  const float saved[3] = {aimPoint[0], aimPoint[1], aimPoint[2]};
  aimPoint[0] = op[0] + ax * g_config.navAimDistance;
  aimPoint[2] = op[2] + az * g_config.navAimDistance;
  const int result = g_updateEnemy(s, edx);
  aimPoint[0] = saved[0];
  aimPoint[1] = saved[1];
  aimPoint[2] = saved[2];
  SetHeading(o, hx, hz);
  return result;
}

int __fastcall UpdateEnemyDetour(void* s, void* edx) {
  const int result = UpdateWithNavigation(s, edx);
  ReloadConfigIfChanged();
  if (game::Read<int>(s, game::S::kType) == kZombieType) RoomObserverOnEnemy(s);
  if (!g_config.ps1ZombieStumble && !g_config.ps1ZombieBackFall &&
      !g_config.ps1ZombieNavigation) {
    g_entries.clear();
    return result;
  }
  if (game::Read<int>(s, game::S::kType) != kZombieType) return result;
  PollHotkey();
  auto it = g_entries.find(s);
  if (it == g_entries.end()) return result;
  Entry& e = it->second;
  e.lastSeen = GetTickCount();
  if (e.phase == Phase::Idle) return result;
  if (++e.updates > kPhaseTimeout) {
    SetPhase(s, e, Phase::Idle, "tempo scaduto, torna all'AI");
    return result;
  }
  Step(s, e);
  if (e.phase == Phase::Idle) return result;
  // Lo zombie non avanza mentre cade o si rialza.
  uint8_t *o, *t;
  if (GetObjects(s, o, t)) {
    float* accum = game::Field<float>(o, game::O::kAccum);
    accum[0] = accum[1] = accum[2] = 0.0f;
    // Caduta all'indietro: spinta PS1 (5 x 20, 18, ..., 2) scalata da Push.
    if (e.phase == Phase::BackFall) {
      const float push = ps1::NextBackPush(e.push) * g_config.backFallPush;
      if (push > 0.0f) g_accumulate(o, nullptr, 0.0f, 0.0f, -push);
    }
  }
  return result;
}

}  // namespace

bool InstallPs1ZombieStumble() {
  const game::ByteSignature checks[] = {
      {game::kAccumulate, game::kAccumulatePrologue, sizeof(game::kAccumulatePrologue)},
      {game::kMapAnim, game::kMapAnimPrologue, sizeof(game::kMapAnimPrologue)},
      {game::kUpdateEnemy, game::kUpdateEnemyPrologue,
       sizeof(game::kUpdateEnemyPrologue)},
  };
  for (const auto& check : checks) {
    if (!game::CheckBytes(check)) {
      Log("Ps1ZombieStumble: byte inattesi a %08X, exe diverso? Funzione spenta.",
          static_cast<unsigned>(check.address));
      return false;
    }
  }
  g_rng = GetTickCount();
  if (g_config.ps1ZombieStumbleAlways) {
    Log("Ps1ZombieStumble: modalita' prova, ogni tentativo riesce (Ps1ZombieStumbleAlways=1)");
  }
  // Prima l'update: da solo non cambia nulla (nessuna fase attiva).
  return Hook("UpdateEnemy", game::kUpdateEnemy,
              reinterpret_cast<void*>(&UpdateEnemyDetour),
              reinterpret_cast<void**>(&g_updateEnemy)) &&
         Hook("MapAnim", game::kMapAnim, reinterpret_cast<void*>(&MapAnimDetour),
              reinterpret_cast<void**>(&g_mapAnim));
}

}  // namespace sf::features
