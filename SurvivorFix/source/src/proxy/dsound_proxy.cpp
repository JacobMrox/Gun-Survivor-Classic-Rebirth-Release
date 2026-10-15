// dsound.dll proxy: Survivor Fix si carica da solo quando il gioco importa DSOUND.dll,
// senza launcher. gun_bio.exe importa da DSOUND.dll solo l'ordinale 11
// (DirectSoundCreate8); Classic Rebirth non usa dsound (ROADMAP passo 1).
//
// Tutte le 12 esportazioni della dsound di sistema, con gli stessi ordinali
// (dsound_proxy.def), si inoltrano alla dsound.dll della cartella di sistema. La DLL vera
// si carica alla prima chiamata, mai in DllMain (loader lock). In un processo a 32 bit
// GetSystemDirectory restituisce System32 e WOW64 lo reindirizza a SysWOW64.
#include <windows.h>

#include <cstdarg>

namespace {

// Diagnostica del proxy, indipendente da Survivor Fix (puo' essere chiamato prima che
// l'inizializzazione apra survivor_fix.log): survivor_fix_dsound.log accanto alla DLL,
// solo API Win32, niente CRT ne' lock condivisi.
void Trace(const char* fmt, ...) {
  static wchar_t path[MAX_PATH] = {};
  if (!path[0]) {
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&Trace), &self);
    const DWORD len = GetModuleFileNameW(self, path, MAX_PATH);
    DWORD i = len;
    while (i > 0 && path[i - 1] != L'\\') --i;
    path[i] = 0;
    lstrcatW(path, L"survivor_fix_dsound.log");
  }
  char line[512];
  int n = wsprintfA(line, "[%10lu tid %5lu] ", GetTickCount(), GetCurrentThreadId());
  va_list args;
  va_start(args, fmt);
  n += wvsprintfA(line + n, fmt, args);
  va_end(args);
  line[n++] = 13;
  line[n++] = 10;
  HANDLE f = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) return;
  DWORD written = 0;
  WriteFile(f, line, n, &written, nullptr);
  CloseHandle(f);
}

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
  Trace("LoadLibraryW %ls ...", path);
  HMODULE module = LoadLibraryW(path);
  const DWORD error = module ? 0 : GetLastError();
  Trace("caricamento %s %ls -> %p (errore %lu)%s", label, path, module, error,
        module && module == SelfModule() ? ", e' il proxy stesso: scartata" : "");
  if (module == SelfModule()) return nullptr;
  return module;
}

UINT WINAPI Wow64Dir(LPWSTR buffer, UINT size) { return GetSystemWow64DirectoryW(buffer, size); }

// Una sola volta per processo, anche se piu' thread chiamano insieme (es. il thread
// principale per l'audio e DirectShow per i filmati): prima i thread potevano vedere il
// caricamento "gia' tentato" ma non finito e memorizzare per sempre un puntatore nullo.
BOOL CALLBACK LoadOnce(PINIT_ONCE, PVOID, PVOID* context) {
  // Processo a 32 bit su Windows a 64 bit: SysWOW64. Su Windows a 32 bit la funzione
  // fallisce e si usa la cartella di sistema.
  HMODULE module = TryLoad(&Wow64Dir, "SysWOW64");
  if (!module) module = TryLoad(&GetSystemDirectoryW, "sistema");
  *context = module;
  return TRUE;
}

HMODULE RealDsound() {
  static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
  void* module = nullptr;
  InitOnceExecuteOnce(&once, &LoadOnce, nullptr, &module);
  return static_cast<HMODULE>(module);
}

template <typename Fn>
Fn Real(const char* name) {
  HMODULE module = RealDsound();
  FARPROC proc = module ? GetProcAddress(module, name) : nullptr;
  Trace("%s -> %p", name, proc);
  return reinterpret_cast<Fn>(reinterpret_cast<void*>(proc));
}

// Risolve la funzione al primo uso; un risultato nullo non viene memorizzato.
#define SF_RESOLVE(Type, name)                                     static Type volatile cached = nullptr;                            Type fn = cached;                                                 if (!fn) {                                                          fn = Real<Type>(name);                                            cached = fn;                                                    }

constexpr HRESULT kNoDsound = E_FAIL;

}  // namespace

extern "C" {

HRESULT WINAPI Proxy_DirectSoundCreate(const GUID* device, void** ds, IUnknown* outer) {
  using Fn = HRESULT(WINAPI*)(const GUID*, void**, IUnknown*);
  SF_RESOLVE(Fn, "DirectSoundCreate")
  return fn ? fn(device, ds, outer) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundEnumerateA(void* callback, void* context) {
  using Fn = HRESULT(WINAPI*)(void*, void*);
  SF_RESOLVE(Fn, "DirectSoundEnumerateA")
  return fn ? fn(callback, context) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundEnumerateW(void* callback, void* context) {
  using Fn = HRESULT(WINAPI*)(void*, void*);
  SF_RESOLVE(Fn, "DirectSoundEnumerateW")
  return fn ? fn(callback, context) : kNoDsound;
}

HRESULT WINAPI Proxy_DllCanUnloadNow() {
  using Fn = HRESULT(WINAPI*)();
  Trace("DllCanUnloadNow (precaricamento o COM)");
  SF_RESOLVE(Fn, "DllCanUnloadNow")
  return fn ? fn() : S_FALSE;
}

HRESULT WINAPI Proxy_DllGetClassObject(const CLSID& clsid, const IID& iid, void** out) {
  using Fn = HRESULT(WINAPI*)(const CLSID&, const IID&, void**);
  SF_RESOLVE(Fn, "DllGetClassObject")
  return fn ? fn(clsid, iid, out) : CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI Proxy_DirectSoundCaptureCreate(const GUID* device, void** dsc, IUnknown* outer) {
  using Fn = HRESULT(WINAPI*)(const GUID*, void**, IUnknown*);
  SF_RESOLVE(Fn, "DirectSoundCaptureCreate")
  return fn ? fn(device, dsc, outer) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundCaptureEnumerateA(void* callback, void* context) {
  using Fn = HRESULT(WINAPI*)(void*, void*);
  SF_RESOLVE(Fn, "DirectSoundCaptureEnumerateA")
  return fn ? fn(callback, context) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundCaptureEnumerateW(void* callback, void* context) {
  using Fn = HRESULT(WINAPI*)(void*, void*);
  SF_RESOLVE(Fn, "DirectSoundCaptureEnumerateW")
  return fn ? fn(callback, context) : kNoDsound;
}

HRESULT WINAPI Proxy_GetDeviceID(const GUID* src, GUID* dst) {
  using Fn = HRESULT(WINAPI*)(const GUID*, GUID*);
  SF_RESOLVE(Fn, "GetDeviceID")
  return fn ? fn(src, dst) : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundFullDuplexCreate(const GUID* capture, const GUID* render,
                                                 const void* captureDesc, const void* renderDesc,
                                                 HWND window, DWORD level, void** duplex,
                                                 void** captureBuffer, void** renderBuffer,
                                                 IUnknown* outer) {
  using Fn = HRESULT(WINAPI*)(const GUID*, const GUID*, const void*, const void*, HWND, DWORD,
                              void**, void**, void**, IUnknown*);
  SF_RESOLVE(Fn, "DirectSoundFullDuplexCreate")
  return fn ? fn(capture, render, captureDesc, renderDesc, window, level, duplex, captureBuffer,
                 renderBuffer, outer)
            : kNoDsound;
}

HRESULT WINAPI Proxy_DirectSoundCreate8(const GUID* device, void** ds, IUnknown* outer) {
  using Fn = HRESULT(WINAPI*)(const GUID*, void**, IUnknown*);
  Trace("DirectSoundCreate8 chiamata");
  SF_RESOLVE(Fn, "DirectSoundCreate8")
  const HRESULT hr = fn ? fn(device, ds, outer) : kNoDsound;
  Trace("DirectSoundCreate8 -> 0x%08lX", static_cast<unsigned long>(hr));
  return hr;
}

HRESULT WINAPI Proxy_DirectSoundCaptureCreate8(const GUID* device, void** dsc, IUnknown* outer) {
  using Fn = HRESULT(WINAPI*)(const GUID*, void**, IUnknown*);
  SF_RESOLVE(Fn, "DirectSoundCaptureCreate8")
  return fn ? fn(device, dsc, outer) : kNoDsound;
}

}  // extern "C"
