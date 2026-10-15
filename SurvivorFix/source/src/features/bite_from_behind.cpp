// BiteFromBehind: gli zombie (e gli altri mostri) mordono anche alle spalle.
//
// In 0043A8A0 (update AI, comune a tutti i tipi) l'evento d'attacco 0043BC70 parte
// solo se, oltre alla distanza, la direzione giocatore->mostro sta nel semipiano
// davanti al giocatore: 0045E870 con ECX = oggetto 0 (il giocatore) da' il suo
// (0,0,-1) locale, cioe' il davanti (verificato in gioco), e se il prodotto scalare
// e' < 0 il salto a 0043AB69 (JNZ +7) scavalca la CALL 0043BC70. Alle spalle quindi
// non si viene mai colpiti. Sul PS1 l'attacco frontale richiede anch'esso il mostro
// davanti, ma alle spalle c'e' la presa da dietro (80101948, stato 0x0E): il
// giocatore viene colpito comunque.
// Qui il JNZ diventa due NOP: resta il controllo della distanza.
// TODO: portare la presa da dietro del PS1 al posto di questo morso generico.
#include <windows.h>

#include <cstring>

#include "../config.h"
#include "../game.h"
#include "../log.h"
#include "features.h"

namespace sf::features {

namespace {

bool g_ready = false;    // byte verificati
bool g_applied = false;  // patch in memoria

void Write(const uint8_t* bytes) {
  void* target = reinterpret_cast<void*>(game::kBiteFacingJump);
  DWORD old = 0;
  if (!VirtualProtect(target, 2, PAGE_EXECUTE_READWRITE, &old)) return;
  std::memcpy(target, bytes, 2);
  VirtualProtect(target, 2, old, &old);
  FlushInstructionCache(GetCurrentProcess(), target, 2);
}

}  // namespace

bool InstallBiteFromBehind() {
  const game::ByteSignature check = {game::kBiteFacingCheck, game::kBiteFacingCheckBytes,
                                     sizeof(game::kBiteFacingCheckBytes)};
  if (!game::CheckBytes(check)) {
    Log("BiteFromBehind: byte inattesi a %08X, exe diverso? Funzione spenta.",
        static_cast<unsigned>(game::kBiteFacingCheck));
    return false;
  }
  g_ready = true;
  SyncBiteFromBehind();
  return true;
}

void SyncBiteFromBehind() {
  if (!g_ready || g_applied == g_config.biteFromBehind) return;
  static const uint8_t kNops[2] = {0x90, 0x90};
  Write(g_config.biteFromBehind ? kNops : game::kBiteFacingJumpBytes);
  g_applied = g_config.biteFromBehind;
  Log("BiteFromBehind: %s (0043AB69)", g_applied ? "attivo, morso anche alle spalle"
                                                 : "spento, morso solo davanti (originale)");
}

}  // namespace sf::features
