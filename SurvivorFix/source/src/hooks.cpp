#include "hooks.h"

#include <MinHook.h>

#include "log.h"

namespace sf {

bool HooksInit() {
  MH_STATUS status = MH_Initialize();
  if (status != MH_OK) {
    Log("MinHook: inizializzazione fallita (%s)", MH_StatusToString(status));
    return false;
  }
  return true;
}

void HooksShutdown() {
  MH_DisableHook(MH_ALL_HOOKS);
  MH_Uninitialize();
}

bool Hook(const char* name, uintptr_t target, void* detour, void** original) {
  void* address = reinterpret_cast<void*>(target);
  MH_STATUS status = MH_CreateHook(address, detour, original);
  if (status == MH_OK) status = MH_EnableHook(address);
  if (status != MH_OK) {
    Log("hook %s @ %08X: ERRORE %s", name, static_cast<unsigned>(target),
        MH_StatusToString(status));
    return false;
  }
  Log("hook %s @ %08X: attivo", name, static_cast<unsigned>(target));
  return true;
}

}  // namespace sf
