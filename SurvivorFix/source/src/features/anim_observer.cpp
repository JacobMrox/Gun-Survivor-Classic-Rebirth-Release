#include "../config.h"
#include "../game.h"
#include "../hooks.h"
#include "../log.h"
#include "features.h"

namespace sf::features {

namespace {

// __thiscall emulato con __fastcall: ECX = S, EDX inutilizzato.
using RequestAnimFn = int(__fastcall*)(void* s, void* edx, int request, int flag);
RequestAnimFn g_requestAnim = nullptr;

int __fastcall RequestAnimDetour(void* s, void* edx, int request, int flag) {
  const int before = game::Read<int>(s, game::S::kSelector);
  const int result = g_requestAnim(s, edx, request, flag);
  const int after = game::Read<int>(s, game::S::kSelector);
  ReloadConfigIfChanged();
  if (after != before && g_config.animObserver) {
    Log("anim S=%p tipo=%02X richiesta=%02X selettore %02X -> %02X", s,
        game::Read<int>(s, game::S::kType), request & 0xFF, before & 0xFF,
        after & 0xFF);
  }
  return result;
}

}  // namespace

bool InstallAnimObserver() {
  const game::ByteSignature checks[] = {
      {game::kRequestAnim + 0x21, game::kRequestAnimStore,
       sizeof(game::kRequestAnimStore)},
      {game::kRequestAnim + 0x24, game::kRequestAnimCall,
       sizeof(game::kRequestAnimCall)},
  };
  for (const auto& check : checks) {
    if (!game::CheckBytes(check)) {
      Log("AnimObserver: byte inattesi a %08X, exe diverso? Funzione spenta.",
          static_cast<unsigned>(check.address));
      return false;
    }
  }
  return Hook("RequestAnim", game::kRequestAnim,
              reinterpret_cast<void*>(&RequestAnimDetour),
              reinterpret_cast<void**>(&g_requestAnim));
}

}  // namespace sf::features
