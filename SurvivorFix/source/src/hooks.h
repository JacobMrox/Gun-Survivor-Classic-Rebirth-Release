#pragma once

#include <cstdint>

namespace sf {

bool HooksInit();
void HooksShutdown();

// Aggancia la funzione all'indirizzo `target` e la abilita.
// `original` riceve il trampolino per chiamare la funzione originale.
bool Hook(const char* name, uintptr_t target, void* detour, void** original);

}  // namespace sf
