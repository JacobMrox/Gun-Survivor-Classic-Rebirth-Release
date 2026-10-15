#include "game.h"

#include <windows.h>

#include <cstring>

namespace game {

bool CheckBytes(const ByteSignature& sig) {
  MEMORY_BASIC_INFORMATION mbi;
  if (!VirtualQuery(reinterpret_cast<void*>(sig.address), &mbi, sizeof(mbi)) ||
      mbi.State != MEM_COMMIT) {
    return false;
  }
  return std::memcmp(reinterpret_cast<const void*>(sig.address), sig.bytes,
                     sig.size) == 0;
}

}  // namespace game
