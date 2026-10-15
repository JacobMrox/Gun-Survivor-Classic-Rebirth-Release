#pragma once

#include <string>

namespace sf {

// Letta da survivor_fix.ini, nella stessa cartella della DLL.
// Ogni correzione ha il suo interruttore: si prova on/off senza ricompilare.
// Il file viene riletto a gioco aperto quando cambia (ReloadConfigIfChanged);
// gli hook pero' si installano solo all'avvio.
struct Config {
  bool enabled = true;
  bool log = true;
  bool animObserver = true;
  // Registra stage/stanza caricati e gli spawn degli zombie. Non cambia il gioco.
  bool roomObserver = true;
  bool ps1ZombieStumble = false;
  // Solo per le prove: ogni tentativo in fascia 1 e nel settore riesce (niente tiro 4,7%).
  bool ps1ZombieStumbleAlways = false;
  // Solo per le prove: un tasto fa cadere lo zombie in inseguimento piu' vicino.
  bool ps1ZombieStumbleHotkey = false;
  // Codice virtual-key del tasto. Default Ins: F9 apre il menu di uscita del gioco
  // (letto via DirectInput, che la DLL non aggancia).
  int ps1ZombieStumbleKey = 0x2D;
  // [Ps1ZombieStumble]: velocita' (S+40) di 23/10/11 e comando di uscita della 23.
  // Default: velocita' delle cadute/rialzate PC e inizio+8 del PS1 (80103C6C).
  float fallSpeed = 2.0f;
  float getUp1Speed = 2.0f;
  float getUp2Speed = 2.0f;
  int exitFrame = 8;
  // Caduta all'indietro PS1 (0x20 -> 08 -> 0E -> 0F). [Ps1ZombieBackFall] nel .ini.
  bool ps1ZombieBackFall = false;
  // Solo per le prove: ogni 0x20 finisce in caduta all'indietro (niente tiro 80%).
  bool ps1ZombieBackFallAlways = false;
  float backFallSpeed = 2.0f;
  float supineSpeed = 2.0f;
  float sitUpSpeed = 2.0f;
  int backFallFrame = 10;     // comando della 0x20 a cui si tira (PS1: 10)
  float backFallPush = 0.0f;  // moltiplica la spinta PS1; 0 = cade sul posto
  // I mostri mordono anche alle spalle (il PC filtrava i colpi da dietro).
  bool biteFromBehind = false;
  // Una sola camminata per zombie (02 o 03) come sul PS1, senza i cambi a scatti del PC.
  bool ps1ZombieWalk = false;
  // Inseguimento PS1 con i muri di ROOMENV (stato 4). [Ps1ZombieNavigation] nel .ini.
  bool ps1ZombieNavigation = false;
  float navProbeDistance = 2000.0f;  // sonda davanti e ai lati (PS1: 100 * 0x14)
  float navTurnStep = 1.93f;         // gradi per frame PS1 nell'aggiramento (PS1: 0x16)
  float navRateScale = 1.0f;         // frame PS1 per update AI PC (~30 / 29,4)
  float navAimDistance = 3000.0f;    // distanza del punto di mira passato all'AI PC
  float navPlayerWallMargin = 1000.0f;  // muri ignorati se il giocatore e' piu' vicino
};

extern Config g_config;
extern std::wstring g_dir;  // cartella della DLL, con '\' finale

void LoadConfig(const std::wstring& iniPath);
// Scrive nel log tutti i valori letti.
void LogConfig();
// Al massimo una volta al secondo controlla la data del .ini; se e' cambiata
// lo rilegge e scrive nel log i valori nuovi.
void ReloadConfigIfChanged();

}  // namespace sf
