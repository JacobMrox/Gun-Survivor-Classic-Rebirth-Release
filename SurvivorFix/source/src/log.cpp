#include "log.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>

namespace sf {

static FILE* g_file = nullptr;
static CRITICAL_SECTION g_lock;

void LogOpen(const wchar_t* path, bool enabled) {
  InitializeCriticalSection(&g_lock);
  if (!enabled) return;
  g_file = _wfopen(path, L"w");
}

void Log(const char* fmt, ...) {
  if (!g_file) return;
  EnterCriticalSection(&g_lock);
  std::fprintf(g_file, "[%10lu] ", GetTickCount());
  va_list args;
  va_start(args, fmt);
  std::vfprintf(g_file, fmt, args);
  va_end(args);
  std::fputc('\n', g_file);
  std::fflush(g_file);
  LeaveCriticalSection(&g_lock);
}

}  // namespace sf
