#pragma once

#include <vector>

#include "../ps1/nav_logic.h"

namespace sf::features {

// Ogni Install*() verifica i byte attesi, aggancia e ritorna true se attiva.

// Registra nel log ogni cambio di animazione (tipo, richiesta, selettore).
// Serve a vedere cosa fa il gioco senza debugger: es. se 0x23 compare mai.
bool InstallAnimObserver();

// Registra stage/stanza caricati (00475530) e gli spawn degli zombie.
bool InstallRoomObserver();
// Chiamata dall'hook dell'update nemici (ps1_zombie_stumble.cpp) per ogni S.
void RoomObserverOnEnemy(void* s);
// Muri di movimento della stanza corrente (vuoto se non disponibili).
const std::vector<ps1::Wall>& CurrentRoomWalls();

// Cadute PS1 dello zombie: inciampo (23 -> 10 -> 11, Ps1ZombieStumble) e
// caduta all'indietro (0x20 -> 08 -> 0E -> 0F, Ps1ZombieBackFall). Stessi hook.
bool InstallPs1ZombieStumble();

}  // namespace sf::features
