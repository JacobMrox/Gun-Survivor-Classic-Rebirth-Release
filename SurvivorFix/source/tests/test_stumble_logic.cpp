// Test della logica PS1 pura. Gira su Linux in CI, senza il gioco.
#include <cstdio>
#include <cstdlib>

#include "../src/ps1/stumble_logic.h"

static int failures = 0;

#define CHECK(cond)                                              \
  do {                                                           \
    if (!(cond)) {                                               \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
      ++failures;                                                \
    }                                                            \
  } while (0)

static void TestBand() {
  ps1::StumbleState s;
  ps1::UpdateBand(s, 3000.0f);
  CHECK(s.band == 0);  // estremo inferiore escluso (intervallo vuoto, invariata)
  ps1::UpdateBand(s, 3001.0f);  // distanza troncata a int come in 8007F4F4
  CHECK(s.band == 1);
  ps1::UpdateBand(s, 5000.0f);
  CHECK(s.band == 1);  // estremo superiore incluso
  ps1::UpdateBand(s, 5001.0f);
  CHECK(s.band == 1);  // oltre 5000 la fascia resta quella memorizzata
}

static void TestAttemptConsumedEvenOnFailure() {
  ps1::StumbleState s;
  ps1::UpdateBand(s, 4000.0f);
  CHECK(!ps1::ShouldStartFall(s, 0xFF, true));  // rng alto: fallisce
  CHECK(s.attemptConsumed);
  CHECK(!ps1::ShouldStartFall(s, 0x00, true));  // gia' consumato
}

static void TestTrigger() {
  ps1::StumbleState s;
  ps1::UpdateBand(s, 4000.0f);
  CHECK(ps1::ShouldStartFall(s, 0x0B, true));
  ps1::StumbleState t;
  ps1::UpdateBand(t, 4000.0f);
  CHECK(!ps1::ShouldStartFall(t, 0x0C, true));  // soglia esclusa
  ps1::StumbleState u;
  ps1::UpdateBand(u, 4000.0f);
  CHECK(!ps1::ShouldStartFall(u, 0x00, false));  // fuori settore
}

static void TestOutOfBand() {
  ps1::StumbleState s;
  ps1::UpdateBand(s, 1000.0f);
  CHECK(s.band == 4);
  CHECK(!ps1::ShouldStartFall(s, 0x00, true));
  CHECK(!s.attemptConsumed);
}

static void TestBandsFromMips() {
  ps1::StumbleState s;
  ps1::UpdateBand(s, 1500.0f);
  CHECK(s.band == 2);
  ps1::UpdateBand(s, 1600.9f);  // troncata a 1600: ancora fascia 2
  CHECK(s.band == 2);
  ps1::UpdateBand(s, 1200.0f);
  CHECK(s.band == 3);
  ps1::UpdateBand(s, 1400.0f);
  CHECK(s.band == 3);
  ps1::UpdateBand(s, 999.0f);
  CHECK(s.band == 4);
  ps1::UpdateBand(s, 2000.0f);  // intervallo vuoto: fascia invariata
  CHECK(s.band == 4);
  ps1::UpdateBand(s, 4000.0f);
  CHECK(s.band == 1);
  ps1::UpdateBand(s, 9000.0f);  // oltre 5000: fascia invariata
  CHECK(s.band == 1);
}

static void TestRearm() {
  ps1::StumbleState s;
  ps1::UpdateBand(s, 4000.0f);
  ps1::ShouldStartFall(s, 0xFF, true);
  // Allontanarsi oltre 5000 o restare fra 1600 e 3000 non riarma.
  ps1::UpdateBand(s, 6000.0f);
  ps1::UpdateBand(s, 4000.0f);
  CHECK(!ps1::ShouldStartFall(s, 0x00, true));
  ps1::UpdateBand(s, 2500.0f);
  ps1::UpdateBand(s, 4000.0f);
  CHECK(!ps1::ShouldStartFall(s, 0x00, true));
  // Avvicinarsi sotto 1600 (fasce 2-4) riarma.
  ps1::UpdateBand(s, 1500.0f);
  ps1::UpdateBand(s, 4000.0f);
  CHECK(ps1::ShouldStartFall(s, 0x00, true));
  // Il reset all'ingresso nell'inseguimento riarma.
  ps1::ResetOnChaseStart(s);
  CHECK(s.band == 0 && !s.attemptConsumed);
}

static void TestSector() {
  CHECK(ps1::InFrontSector(0.0f, 1.0f, 0.0f, 10.0f));      // dritto davanti
  CHECK(ps1::InFrontSector(0.0f, 1.0f, 7.0f, 10.0f));      // 35 gradi
  CHECK(!ps1::InFrontSector(0.0f, 1.0f, 9.0f, 10.0f));     // 42 gradi
  CHECK(!ps1::InFrontSector(0.0f, 1.0f, 0.0f, -10.0f));    // dietro
  CHECK(!ps1::InFrontSector(0.0f, 0.0f, 0.0f, 10.0f));     // heading nullo
}

static void TestExit() {
  CHECK(ps1::DecideFallExit(100.0f, true, true) == ps1::FallExit::Contact);
  CHECK(ps1::DecideFallExit(0x380, true, true) == ps1::FallExit::GetUp);
  CHECK(ps1::DecideFallExit(100.0f, false, true) == ps1::FallExit::GetUp);
  CHECK(ps1::DecideFallExit(100.0f, true, false) == ps1::FallExit::GetUp);
}

int main() {
  TestBand();
  TestAttemptConsumedEvenOnFailure();
  TestTrigger();
  TestOutOfBand();
  TestBandsFromMips();
  TestRearm();
  TestSector();
  TestExit();
  if (failures) {
    std::printf("%d test falliti\n", failures);
    return EXIT_FAILURE;
  }
  std::printf("OK\n");
  return EXIT_SUCCESS;
}
