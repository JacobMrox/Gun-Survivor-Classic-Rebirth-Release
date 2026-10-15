// survivor_fix.dll: correzioni di Resident Evil Survivor PC basate sul PS1.
// Non modifica gun_bio.exe su disco: aggancia le funzioni in memoria.
// Convive con il d3d8.dll di Classic Rebirth (non tocca D3D, input, audio, video).
#include <windows.h>

#include <string>

#include "config.h"
#include "exe_info.h"
#include "features/features.h"
#include "game.h"
#include "hooks.h"
#include "log.h"

#ifndef SF_VERSION
#define SF_VERSION "dev"
#endif

namespace {

std::wstring ModuleDir(HMODULE module) {
  wchar_t path[MAX_PATH];
  DWORD len = GetModuleFileNameW(module, path, MAX_PATH);
  std::wstring dir(path, len);
  return dir.substr(0, dir.find_last_of(L"\\/") + 1);
}

DWORD WINAPI Init(LPVOID module) {
  using namespace sf;
  // Una sola copia per processo: dsound.dll (proxy) e survivor_fix.dll (launcher)
  // insieme installerebbero due volte gli stessi hook.
  wchar_t mutexName[64];
  wsprintfW(mutexName, L"Local\\SurvivorFix-%lu", GetCurrentProcessId());
  CreateMutexW(nullptr, FALSE, mutexName);
  if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

  // Come proxy (dsound.dll) carica subito la dsound di sistema, fuori dal loader lock:
  // col launcher il gioco la aveva gia' caricata all'avvio, e chi la cerca per nome
  // (es. DirectShow per i filmati) prima del primo suono troverebbe solo il proxy.
  // DllCanUnloadNow esiste solo nel proxy e non crea oggetti.
  if (auto preload = reinterpret_cast<HRESULT(WINAPI*)()>(reinterpret_cast<void*>(
          GetProcAddress(static_cast<HMODULE>(module), "DllCanUnloadNow")))) {
    preload();
  }

  g_dir = ModuleDir(static_cast<HMODULE>(module));
  LoadConfig(g_dir + L"survivor_fix.ini");
  LogOpen((g_dir + L"survivor_fix.log").c_str(), g_config.log);
  wchar_t modulePath[MAX_PATH];
  GetModuleFileNameW(static_cast<HMODULE>(module), modulePath, MAX_PATH);
  Log("survivor_fix %s (%ls)", SF_VERSION, modulePath);
  LogConfig();

  if (!g_config.enabled) {
    Log("disattivata da survivor_fix.ini ([General] Enable=0)");
    return 0;
  }

  wchar_t exePath[MAX_PATH];
  GetModuleFileNameW(nullptr, exePath, MAX_PATH);
  const std::string sha = Sha256File(exePath);
  Log("exe: %ls", exePath);
  Log("sha256: %s (%s)", sha.c_str(),
      sha == game::kOriginalSha256 ? "originale riconosciuto"
                                   : "diverso dall'originale della ricerca");

  const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
  if (base != game::kImageBase) {
    Log("base %08X inattesa (attesa %08X): nessuna correzione attivata",
        static_cast<unsigned>(base), static_cast<unsigned>(game::kImageBase));
    return 0;
  }
  if (!HooksInit()) return 0;

  if (g_config.animObserver) features::InstallAnimObserver();
  // Sempre: verifica i byte e applica la modifica solo se BiteFromBehind=1, cosi'
  // si puo' accendere e spegnere anche a gioco aperto.
  features::InstallBiteFromBehind();
  // La navigazione ha bisogno dell'aggancio della stanza (muri di ROOMENV) anche senza
  // il log degli spawn di RoomObserver.
  if (g_config.roomObserver || g_config.ps1ZombieNavigation) features::InstallRoomObserver();
  if (g_config.ps1ZombieStumble || g_config.ps1ZombieBackFall ||
      g_config.ps1ZombieNavigation || g_config.ps1ZombieWalk) {
    features::InstallPs1ZombieStumble();
  }
  Log("inizializzazione completata");
  return 0;
}

}  // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(instance);
    HANDLE thread = CreateThread(nullptr, 0, Init, instance, 0, nullptr);
    if (thread) CloseHandle(thread);
  }
  return TRUE;
}
