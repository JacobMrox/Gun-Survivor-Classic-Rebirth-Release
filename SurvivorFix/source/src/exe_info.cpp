#include "exe_info.h"

#include <windows.h>
#include <bcrypt.h>

#include <vector>

namespace sf {

std::string Sha256File(const wchar_t* path) {
  HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                            OPEN_EXISTING, 0, nullptr);
  if (file == INVALID_HANDLE_VALUE) return {};

  std::string result;
  BCRYPT_ALG_HANDLE alg = nullptr;
  BCRYPT_HASH_HANDLE hash = nullptr;
  if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0 &&
      BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) == 0) {
    std::vector<unsigned char> buffer(1 << 16);
    DWORD read = 0;
    while (ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read,
                    nullptr) &&
           read > 0) {
      BCryptHashData(hash, buffer.data(), read, 0);
    }
    unsigned char digest[32];
    if (BCryptFinishHash(hash, digest, sizeof(digest), 0) == 0) {
      static const char kHex[] = "0123456789abcdef";
      for (unsigned char b : digest) {
        result += kHex[b >> 4];
        result += kHex[b & 0xF];
      }
    }
  }
  if (hash) BCryptDestroyHash(hash);
  if (alg) BCryptCloseAlgorithmProvider(alg, 0);
  CloseHandle(file);
  return result;
}

}  // namespace sf
