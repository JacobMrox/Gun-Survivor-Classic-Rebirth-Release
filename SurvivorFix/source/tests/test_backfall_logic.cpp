// Test della logica PS1 della caduta all'indietro. Gira su Linux in CI, senza il gioco.
#include <cstdio>
#include <cstdlib>

#include "../src/ps1/backfall_logic.h"

static int failures = 0;

#define CHECK(cond)                                              \
  do {                                                           \
    if (!(cond)) {                                               \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
      ++failures;                                                \
    }                                                            \
  } while (0)

static void TestRoll() {
  CHECK(ps1::ShouldStartBackFall(0x00));
  CHECK(ps1::ShouldStartBackFall(0xCB));
  CHECK(!ps1::ShouldStartBackFall(0xCC));  // soglia esclusa
  CHECK(!ps1::ShouldStartBackFall(0xFF));
}

static void TestPush() {
  ps1::BackPush p;
  CHECK(ps1::NextBackPush(p) == 0.0f);  // non avviata
  ps1::StartBackPush(p);
  float total = 0.0f;
  int updates = 0;
  float first = ps1::NextBackPush(p);
  total += first;
  ++updates;
  for (float v; (v = ps1::NextBackPush(p)) > 0.0f;) {
    total += v;
    ++updates;
  }
  CHECK(first == 100.0f);  // 5 * 20
  CHECK(updates == 10);    // 20, 18, ..., 2
  CHECK(total == 550.0f);  // 5 * 110
  CHECK(ps1::NextBackPush(p) == 0.0f);
}

int main() {
  TestRoll();
  TestPush();
  if (failures) {
    std::printf("%d test falliti\n", failures);
    return EXIT_FAILURE;
  }
  std::printf("OK\n");
  return EXIT_SUCCESS;
}
