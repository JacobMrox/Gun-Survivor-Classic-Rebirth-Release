// dsound.dll proxy: Survivor Fix si carica da solo quando il gioco importa DSOUND.dll,
// senza launcher. gun_bio.exe importa da DSOUND.dll solo l'ordinale 11
// (DirectSoundCreate8); Classic Rebirth non usa dsound (ROADMAP passo 1).
//
// Tutte le 12 esportazioni della dsound di sistema, con gli stessi ordinali
// (dsound_proxy.def), si inoltrano alla dsound.dll della cartella di sistema. La DLL vera
// si carica alla prima chiamata, mai in DllMain (loader lock). In un processo a 32 bit
// GetSystemDirectory restituisce System32 e WOW64 lo reindirizza a SysWOW64.
#include <windows.h>

#include "../log.h"

namespace {

HMODULE SelfModule() {
  HMODULE self = nullptr;
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     reinterpret_cast<LPCWSTR>(&SelfModule), &self);
  return self;
}

// Carica la dsound.dll di sistema da una cartella; nullptr se fallisce o se il loader
// restituisce il proxy stesso (stesso nome di modulo gia' caricato).
HMODULE TryLoad(UINT (WINAPI* dirFn)(LPWSTR, UINT), const char* label) {
  wchar_t path[MAX_PATH];
  const UINT len = dirFn(path, MAX_PATH);
  if (len == 0 || len > MAX_PATH - 12) return nullptr;
  lstrcatW(path, L"\\dsound.dll");
  HMODULE module = LoadLibraryW(path);
  const DWORD error = module ? 0 : GetLastError();
  sf::Log("dsound: %s %ls -> %p (errore %lu)%s", label, path, module, error,
          module && module == SelfModule() ? ", e' il proxy stesso: scartata" : "");
  if (module == SelfModule()) return nullptr;
  return module;
}

UINT WINAPI Wow64Dir(LPWSTR buffer, UINT size) { return GetSystemWow64DirectoryW(buffer, size); }

HMODULE RealDsound() {
  static HMODULE module = nullptr;
  static bool tried = false;
  if (!module && !tried) {
    tried = true;
    // Processo a 32 bit su Windows a 64 bit: SysWOW64. Su Windows a 32 bit la funzione
    // fallisce e si usa la cartella di sistema.
    module = TryLoad(&Wow64Dir, "SysWOW64");
    if (!module) module = TryLoad(&GetSystemDirectoryW, "sistema");
  }
  return module;
}

template <typename Fn>
Fn Real(const char* name) {
  HMODULE module = RealDsound();
  FARPROC proc = module ? GetProcAddress(module, name) : nullptr;
  if (!proc) sf::Log("dsound: %s non trovata nella dll di sistema", name);
  return reinterpret_cast<Fn>(reinterpret_cast<void*>(proc));
}

constexpr HRESULT kNoDsound = E_FAIL;

}  // namespace

extern "C" {

HRESULT WINAPI Proxy_DirectSoundCreate(const GUID* device, void** ds, IUnknown* outer) {
  using Fn = HRESULT(WINAPI*)(const GUID*, void**, IUnknown*);
  static Fn fn = Real<Fn>("DirectSoundCreate");
  return fn ? fn(device, ds, outer) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundEnumerateA(void* callback, void* context) {
  using Fn = HRESULT(WINAPI*)(void*, void*);
  static Fn fn = Real<Fn>("DirectSoundEnumerateA");
  return fn ? fn(callback, context) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundEnumerateW(void* callback, void* context) {
  using Fn = HRESULT(WINAPI*)(void*, void*);
  static Fn fn = Real<Fn>("DirectSoundEnumerateW");
  return fn ? fn(callback, context) : kNoDsound;
}

HRESULT WINAPI Proxy_DllCanUnloadNow() {
  using Fn = HRESULT(WINAPI*)();
  static Fn fn = Real<Fn>("DllCanUnloadNow");
  return fn ? fn() : S_FALSE;
}

HRESULT WINAPI Proxy_DllGetClassObject(const CLSID& clsid, const IID& iid, void** out) {
  using Fn = HRESULT(WINAPI*)(const CLSID&, const IID&, void**);
  static Fn fn = Real<Fn>("DllGetClassObject");
  return fn ? fn(clsid, iid, out) : CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI Proxy_DirectSoundCaptureCreate(const GUID* device, void** dsc, IUnknown* outer) {
  using Fn = HRESULT(WINAPI*)(const GUID*, void**, IUnknown*);
  static Fn fn = Real<Fn>("DirectSoundCaptureCreate");
  return fn ? fn(device, dsc, outer) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundCaptureEnumerateA(void* callback, void* context) {
  using Fn = HRESULT(WINAPI*)(void*, void*);
  static Fn fn = Real<Fn>("DirectSoundCaptureEnumerateA");
  return fn ? fn(callback, context) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundCaptureEnumerateW(void* callback, void* context) {
  using Fn = HRESULT(WINAPI*)(void*, void*);
  static Fn fn = Real<Fn>("DirectSoundCaptureEnumerateW");
  return fn ? fn(callback, context) : kNoDsound;
}

HRESULT WINAPI Proxy_GetDeviceID(const GUID* src, GUID* dst) {
  using Fn = HRESULT(WINAPI*)(const GUID*, GUID*);
  static Fn fn = Real<Fn>("GetDeviceID");
  return fn ? fn(src, dst) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundFullDuplexCreate(const GUID* capture, const GUID* render,
                                                 const void* captureDesc, const void* renderDesc,
                                                 HWND window, DWORD level, void** duplex,
                                                 void** captureBuffer, void** renderBuffer,
                                                 IUnknown* outer) {
  using Fn = HRESULT(WINAPI*)(const GUID*, const GUID*, const void*, const void*, HWND, DWORD,
                              void**, void**, void**, IUnknown*);
  static Fn fn = Real<Fn>("DirectSoundFullDuplexCreate");
  return fn ? fn(capture, render, captureDesc, renderDesc, window, level, duplex, captureBuffer,
                 renderBuffer, outer)
            : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundCreate8(const GUID* device, void** ds, IUnknown* outer) {
  using Fn = HRESULT(WINAPI*)(const GUID*, void**, IUnknown*);
  static Fn fn = Real<Fn>("DirectSoundCreate8");
  return fn ? fn(device, ds, outer) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundCaptureCreate8(const GUID* device, void** dsc, IUnknown* outer) {
  using Fn = HRESULT(WINAPI*)(const GUID*, void**, IUnknown*);
  static Fn fn = Real<Fn>("DirectSoundCaptureCreate8");
  return fn ? fn(device, dsc, outer) : kNoDsound;
}

}  // extern "C"
