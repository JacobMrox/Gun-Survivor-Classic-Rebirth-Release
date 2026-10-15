#include "config.h"

#include <windows.h>

#include <cwchar>

#include "log.h"

namespace sf {

Config g_config;
std::wstring g_dir;

namespace {

std::wstring g_iniPath;
FILETIME g_iniTime = {};
DWORD g_lastCheck = 0;

bool ReadBool(const wchar_t* section, const wchar_t* key, bool fallback) {
  return GetPrivateProfileIntW(section, key, fallback ? 1 : 0, g_iniPath.c_str()) != 0;
}

int ReadInt(const wchar_t* section, const wchar_t* key, int fallback) {
  return static_cast<int>(GetPrivateProfileIntW(section, key, fallback, g_iniPath.c_str()));
}

// Accetta anche esadecimale (0x2D).
int ReadCode(const wchar_t* section, const wchar_t* key, int fallback) {
  wchar_t text[32];
  GetPrivateProfileStringW(section, key, L"", text, 32, g_iniPath.c_str());
  wchar_t* end = nullptr;
  const long value = std::wcstol(text, &end, 0);
  return end != text && value > 0 && value < 256 ? static_cast<int>(value) : fallback;
}

float ReadFloat(const wchar_t* section, const wchar_t* key, float fallback,
                bool allowZero = false) {
  wchar_t text[64];
  GetPrivateProfileStringW(section, key, L"", text, 64, g_iniPath.c_str());
  wchar_t* end = nullptr;
  const float value = std::wcstof(text, &end);
  const bool valid = end != text && (value > 0.0f || (allowZero && value == 0.0f));
  return valid ? value : fallback;
}

FILETIME IniTime() {
  WIN32_FILE_ATTRIBUTE_DATA data = {};
  if (!GetFileAttributesExW(g_iniPath.c_str(), GetFileExInfoStandard, &data)) return {};
  return data.ftLastWriteTime;
}

Config Read() {
  const Config defaults;
  Config c;
  c.enabled = ReadBool(L"General", L"Enable", defaults.enabled);
  c.log = ReadBool(L"General", L"Log", defaults.log);
  c.animObserver = ReadBool(L"Features", L"AnimObserver", defaults.animObserver);
  c.roomObserver = ReadBool(L"Features", L"RoomObserver", defaults.roomObserver);
  c.ps1ZombieStumble = ReadBool(L"Features", L"Ps1ZombieStumble", defaults.ps1ZombieStumble);
  c.ps1ZombieStumbleAlways =
      ReadBool(L"Features", L"Ps1ZombieStumbleAlways", defaults.ps1ZombieStumbleAlways);
  c.ps1ZombieStumbleHotkey =
      ReadBool(L"Features", L"Ps1ZombieStumbleHotkey", defaults.ps1ZombieStumbleHotkey);
  c.ps1ZombieStumbleKey =
      ReadCode(L"Features", L"Ps1ZombieStumbleKey", defaults.ps1ZombieStumbleKey);
  c.fallSpeed = ReadFloat(L"Ps1ZombieStumble", L"FallSpeed", defaults.fallSpeed);
  c.getUp1Speed = ReadFloat(L"Ps1ZombieStumble", L"GetUp1Speed", defaults.getUp1Speed);
  c.getUp2Speed = ReadFloat(L"Ps1ZombieStumble", L"GetUp2Speed", defaults.getUp2Speed);
  c.exitFrame = ReadInt(L"Ps1ZombieStumble", L"ExitFrame", defaults.exitFrame);
  c.ps1ZombieBackFall = ReadBool(L"Features", L"Ps1ZombieBackFall", defaults.ps1ZombieBackFall);
  c.ps1ZombieBackFallAlways =
      ReadBool(L"Features", L"Ps1ZombieBackFallAlways", defaults.ps1ZombieBackFallAlways);
  c.backFallSpeed = ReadFloat(L"Ps1ZombieBackFall", L"BackFallSpeed", defaults.backFallSpeed);
  c.supineSpeed = ReadFloat(L"Ps1ZombieBackFall", L"SupineSpeed", defaults.supineSpeed);
  c.sitUpSpeed = ReadFloat(L"Ps1ZombieBackFall", L"SitUpSpeed", defaults.sitUpSpeed);
  c.backFallFrame = ReadInt(L"Ps1ZombieBackFall", L"TriggerFrame", defaults.backFallFrame);
  c.backFallPush = ReadFloat(L"Ps1ZombieBackFall", L"Push", defaults.backFallPush, true);
  c.ps1ZombieWalk = ReadBool(L"Features", L"Ps1ZombieWalk", defaults.ps1ZombieWalk);
  c.ps1ZombieNavigation =
      ReadBool(L"Features", L"Ps1ZombieNavigation", defaults.ps1ZombieNavigation);
  c.navProbeDistance =
      ReadFloat(L"Ps1ZombieNavigation", L"ProbeDistance", defaults.navProbeDistance);
  c.navTurnStep = ReadFloat(L"Ps1ZombieNavigation", L"TurnStep", defaults.navTurnStep);
  c.navRateScale = ReadFloat(L"Ps1ZombieNavigation", L"RateScale", defaults.navRateScale);
  c.navAimDistance = ReadFloat(L"Ps1ZombieNavigation", L"AimDistance", defaults.navAimDistance);
  c.navPlayerWallMargin = ReadFloat(L"Ps1ZombieNavigation", L"PlayerWallMargin",
                                    defaults.navPlayerWallMargin, true);
  return c;
}

// Scrive i valori diversi da `before` (tutti se `all`).
void LogValues(const Config& before, const Config& now, bool all) {
#define SF_LOG_VALUE(field, fmt) \
  if (all || before.field != now.field) Log("config: " #field " = " fmt, now.field)
  SF_LOG_VALUE(animObserver, "%d");
  SF_LOG_VALUE(roomObserver, "%d");
  SF_LOG_VALUE(ps1ZombieStumble, "%d");
  SF_LOG_VALUE(ps1ZombieStumbleAlways, "%d");
  SF_LOG_VALUE(ps1ZombieStumbleHotkey, "%d");
  SF_LOG_VALUE(ps1ZombieStumbleKey, "0x%02X");
  SF_LOG_VALUE(fallSpeed, "%.2f");
  SF_LOG_VALUE(getUp1Speed, "%.2f");
  SF_LOG_VALUE(getUp2Speed, "%.2f");
  SF_LOG_VALUE(exitFrame, "%d");
  SF_LOG_VALUE(ps1ZombieBackFall, "%d");
  SF_LOG_VALUE(ps1ZombieBackFallAlways, "%d");
  SF_LOG_VALUE(backFallSpeed, "%.2f");
  SF_LOG_VALUE(supineSpeed, "%.2f");
  SF_LOG_VALUE(sitUpSpeed, "%.2f");
  SF_LOG_VALUE(backFallFrame, "%d");
  SF_LOG_VALUE(backFallPush, "%.2f");
  SF_LOG_VALUE(ps1ZombieWalk, "%d");
  SF_LOG_VALUE(ps1ZombieNavigation, "%d");
  SF_LOG_VALUE(navProbeDistance, "%.0f");
  SF_LOG_VALUE(navTurnStep, "%.2f");
  SF_LOG_VALUE(navRateScale, "%.2f");
  SF_LOG_VALUE(navAimDistance, "%.0f");
  SF_LOG_VALUE(navPlayerWallMargin, "%.0f");
#undef SF_LOG_VALUE
}

}  // namespace

void LoadConfig(const std::wstring& ini) {
  g_iniPath = ini;
  g_iniTime = IniTime();
  g_config = Read();
}

void LogConfig() { LogValues(g_config, g_config, true); }

void ReloadConfigIfChanged() {
  const DWORD now = GetTickCount();
  if (now - g_lastCheck < 1000) return;
  g_lastCheck = now;
  const FILETIME time = IniTime();
  if (CompareFileTime(&time, &g_iniTime) == 0) return;
  g_iniTime = time;
  const Config before = g_config;
  Config next = Read();
  // Gli hook e il log si decidono all'avvio: questi restano quelli iniziali.
  next.enabled = before.enabled;
  next.log = before.log;
  g_config = next;
  Log("config: survivor_fix.ini cambiato, riletto");
  LogValues(before, g_config, false);
}

}  // namespace sf
