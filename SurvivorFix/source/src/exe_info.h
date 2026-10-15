#pragma once

#include <string>

namespace sf {

// SHA-256 esadecimale di un file, stringa vuota in caso di errore.
std::string Sha256File(const wchar_t* path);

}  // namespace sf
