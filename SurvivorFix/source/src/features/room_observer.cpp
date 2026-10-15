// RoomObserver: scrive nel log stage e stanza caricati e la posizione di spawn
// di ogni zombie (servita a verificare che le coordinate PC coincidono con quelle
// PS1 di ROOMENV.DAT), e carica i muri della stanza per Ps1ZombieNavigation.
// Non cambia il gioco.
//
// 00475530 costruisce "PSX_DATA\stage\STAGEss\rrrr\rrrr_A/B.HMD" (verifica
// Ghidra 2026-10-15: __cdecl, quattro argomenti, RET semplice; usata da
// 004740C0 e da altri 18 call site del caricamento stanza).
#include <windows.h>

#include <cstdio>
#include <unordered_set>
#include <vector>

#include "../config.h"
#include "../game.h"
#include "../hooks.h"
#include "../log.h"
#include "../ps1/nav_logic.h"
#include "../roomenv_index.h"
#include "features.h"

namespace sf::features {

namespace {

using RoomPathFn = void(__cdecl*)(char* out, int stage, int room, int variant);
RoomPathFn g_roomPath = nullptr;

int g_stage = -1;
int g_room = -1;
std::unordered_set<void*> g_seen;  // S gia' registrati nella stanza corrente
std::vector<ps1::Wall> g_walls;      // muri di movimento della stanza corrente

// Muri di movimento (800B60CC sul PS1) dal record della stanza nel ROOMENV.DAT del PC:
// sezione all'offset +4 dell'intestazione: n1 (byte), poi a +8 n1 muri da 16 byte.
void LoadWalls(int stage, int room) {
  g_walls.clear();
  uint32_t offset = 0;
  bool found = false;
  for (const auto& r : game::roomenv::kRooms) {
    if (r.stage == stage && r.room == room) {
      offset = r.offset;
      found = true;
      break;
    }
  }
  if (!found) {
    Log("muri: stanza %02X/%02X senza muri in ROOMENV", stage, room);
    return;
  }
  FILE* f = _wfopen((g_dir + L"Resource\\ROOMENV.DAT").c_str(), L"rb");
  if (!f) {
    Log("muri: Resource\\ROOMENV.DAT non trovato");
    return;
  }
  uint32_t header[7] = {};
  uint8_t counts[8] = {};
  bool ok = std::fseek(f, static_cast<long>(offset), SEEK_SET) == 0 &&
            std::fread(header, sizeof(header), 1, f) == 1 && header[0] == 6 &&
            header[1] == 0x1C &&
            std::fseek(f, static_cast<long>(offset + header[1]), SEEK_SET) == 0 &&
            std::fread(counts, sizeof(counts), 1, f) == 1;
  if (ok) {
    g_walls.resize(counts[0]);
    ok = g_walls.empty() ||
         std::fread(g_walls.data(), sizeof(ps1::Wall), g_walls.size(), f) == g_walls.size();
  }
  std::fclose(f);
  if (!ok) {
    g_walls.clear();
    Log("muri: record della stanza %02X/%02X non valido a 0x%X", stage, room, offset);
    return;
  }
  Log("muri: stanza %02X/%02X, %u muri da ROOMENV (offset 0x%X)", stage, room,
      static_cast<unsigned>(g_walls.size()), offset);
}

void __cdecl RoomPathDetour(char* out, int stage, int room, int variant) {
  g_roomPath(out, stage, room, variant);
  stage &= 0xFF;
  room &= 0xFF;
  if (stage == g_stage && room == g_room) return;
  g_stage = stage;
  g_room = room;
  g_seen.clear();
  Log("stanza: stage %02X stanza %02X (%s)", stage, room, out);
  LoadWalls(stage, room);
}

}  // namespace

const std::vector<ps1::Wall>& CurrentRoomWalls() { return g_walls; }

void RoomObserverOnEnemy(void* s) {
  if (!g_config.roomObserver || !g_roomPath) return;
  if (!g_seen.insert(s).second) return;
  auto* c = game::Read<uint8_t*>(s, game::S::kCollection);
  if (!c) return;
  auto** objects = game::Read<uint8_t**>(c, game::C::kObjects);
  if (!objects) return;
  const uint8_t* o = objects[game::Read<int>(s, game::S::kIndex)];
  const uint8_t* t = objects[game::Read<int>(c, game::C::kTarget)];
  if (!o || !t) return;
  const float* op = reinterpret_cast<const float*>(o + game::O::kPos);
  const float* of = reinterpret_cast<const float*>(o + game::O::kForward);
  const float* tp = reinterpret_cast<const float*>(t + game::O::kPos);
  Log("spawn: stage %02X stanza %02X S=%p tipo=%02X pos=(%.0f, %.0f, %.0f) avanti=(%.3f, %.3f) "
      "giocatore=(%.0f, %.0f, %.0f)",
      g_stage, g_room, s, game::Read<int>(s, game::S::kType), op[0], op[1], op[2], of[0], of[2],
      tp[0], tp[1], tp[2]);
}

bool InstallRoomObserver() {
  const game::ByteSignature check = {game::kRoomPath, game::kRoomPathPrologue,
                                     sizeof(game::kRoomPathPrologue)};
  if (!game::CheckBytes(check)) {
    Log("RoomObserver: byte inattesi a %08X, exe diverso? Funzione spenta.",
        static_cast<unsigned>(game::kRoomPath));
    return false;
  }
  return Hook("RoomPath", game::kRoomPath, reinterpret_cast<void*>(&RoomPathDetour),
              reinterpret_cast<void**>(&g_roomPath));
}

}  // namespace sf::features
