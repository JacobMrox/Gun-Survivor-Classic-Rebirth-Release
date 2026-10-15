// survivor_fix_launcher.exe: avvia il gioco e carica survivor_fix.dll.
//
// Usato perche' Classic Rebirth occupa gia' d3d8.dll e non ha un caricatore
// di plugin. Quando sapremo quali DLL importa gun_bio.exe si potra' passare a
// un proxy (es. dsound.dll) e lanciare direttamente il gioco (ROADMAP passo 1).
#include <windows.h>

#include <string>

namespace {

std::wstring LauncherDir() {
  wchar_t path[MAX_PATH];
  DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
  std::wstring dir(path, len);
  return dir.substr(0, dir.find_last_of(L"\\/") + 1);
}

int Fail(const wchar_t* message) {
  MessageBoxW(nullptr, message, L"Survivor Fix", MB_ICONERROR);
  return 1;
}

bool InjectDll(HANDLE process, const std::wstring& dllPath) {
  const SIZE_T size = (dllPath.size() + 1) * sizeof(wchar_t);
  void* remote = VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE,
                                PAGE_READWRITE);
  if (!remote) return false;
  bool ok = false;
  bool threadDone = true;
  if (WriteProcessMemory(process, remote, dllPath.c_str(), size, nullptr)) {
    auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(reinterpret_cast<void*>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW")));
    HANDLE thread =
        CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
    if (thread) {
      // Classic Rebirth mostra la sua finestra di configurazione durante il caricamento di
      // d3d8.dll: WaitForInputIdle ritorna gia' li', ma il thread remoto resta fermo sul
      // loader lock finche' l'utente non preme avvia. Si aspetta senza timeout (o la
      // chiusura del gioco) e il buffer si libera solo a thread finito.
      HANDLE waits[] = {thread, process};
      threadDone = WaitForMultipleObjects(2, waits, FALSE, INFINITE) == WAIT_OBJECT_0;
      DWORD module = 0;
      ok = threadDone && GetExitCodeThread(thread, &module) && module != 0 &&
           module != STILL_ACTIVE;
      CloseHandle(thread);
    }
  }
  if (threadDone) VirtualFreeEx(process, remote, 0, MEM_RELEASE);
  return ok;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  const std::wstring dir = LauncherDir();
  const std::wstring ini = dir + L"survivor_fix.ini";

  wchar_t exeName[MAX_PATH];
  GetPrivateProfileStringW(L"Launcher", L"Exe", L"gun_bio.exe", exeName, MAX_PATH,
                           ini.c_str());
  std::wstring exePath = dir + exeName;
  const std::wstring dllPath = dir + L"survivor_fix.dll";

  if (GetFileAttributesW(exePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
    return Fail(L"gun_bio.exe non trovato: copia il launcher nella cartella del gioco.");
  }
  if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
    return Fail(L"survivor_fix.dll non trovata accanto al launcher.");
  }

  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {};
  if (!CreateProcessW(exePath.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr,
                      dir.c_str(), &si, &pi)) {
    return Fail(L"Impossibile avviare gun_bio.exe.");
  }

  // Attende che il gioco abbia caricato le sue DLL (incluso Classic Rebirth).
  // Le correzioni riguardano il gameplay, quindi non serve agganciarsi prima.
  WaitForInputIdle(pi.hProcess, 30000);
  if (!InjectDll(pi.hProcess, dllPath)) {
    Fail(L"Il gioco e' partito ma survivor_fix.dll non e' stata caricata.");
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return 0;
}
