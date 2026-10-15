#pragma once

namespace sf {

void LogOpen(const wchar_t* path, bool enabled);
void Log(const char* fmt, ...);

}  // namespace sf
