// Test della logica PS1 di inseguimento con i muri. Gira su Linux in CI, senza il gioco.
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "../src/ps1/nav_logic.h"

static int failures = 0;

#define CHECK(cond)                                              \
  do {                                                           \
    if (!(cond)) {                                               \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
      ++failures;                                                \
    }                                                            \
  } while (0)

using ps1::NavMode;
using ps1::Vec2;
using ps1::Wall;

// Muro verticale x=0 da z=1000 a z=-1000. Con (x1,z1)=(0,1000), (x2,z2)=(0,-1000)
// il lato attivo (prodotto <= 0) e' x >= 0.
static const std::vector<Wall> kWall = {{0, 1000, 0, -1000}};

static void TestLineOfSight() {
  CHECK(ps1::FirstBlockingWall(kWall, {500, 0}, {-500, 0}) == 0);   // dal lato attivo
  CHECK(ps1::FirstBlockingWall(kWall, {-500, 0}, {500, 0}) == -1);  // dal lato opposto
  CHECK(ps1::FirstBlockingWall(kWall, {500, 2000}, {-500, 2000}) == -1);  // oltre l'estremo
  CHECK(ps1::FirstBlockingWall(kWall, {500, 0}, {200, 0}) == -1);  // non arriva al muro
}

static void TestClearChase() {
  // Giocatore dietro a sinistra: avanza e sterza di 0x28 unita' (coseno < 0,1).
  ps1::NavState s;
  const auto out = ps1::NavStep(s, kWall, {{500, 2000}, {0, 1}, {-3000, 2000}});
  CHECK(out.lineOfSight);
  CHECK(out.walk);
  const float turned = std::atan2(-out.heading.x, out.heading.z);  // verso sinistra
  CHECK(std::fabs(turned - 0x28 * ps1::kAngleUnit) < 1e-3f);
}

static void TestSteerTable() {
  CHECK(ps1::SteerUnits(-1.0f) == 0x28);
  CHECK(ps1::SteerUnits(0.05f) == 0x28);
  CHECK(ps1::SteerUnits(0.15f) == 0x24);
  CHECK(ps1::SteerUnits(0.55f) == 0x14);
  CHECK(ps1::SteerUnits(0.95f) == 6);
  CHECK(ps1::SteerUnits(1.0f) == 6);
}

static void TestNoOvershoot() {
  // Quasi allineato: non ruota oltre il giocatore.
  ps1::NavState s;
  const auto out = ps1::NavStep(s, kWall, {{500, 2000}, {0, 1}, {505, 9000}});
  CHECK(out.lineOfSight);
  const float err = std::fabs(std::atan2(out.heading.x, out.heading.z) -
                              std::atan2(5.0f, 7000.0f));
  CHECK(err < 1e-3f);
}

static void TestTurnsWhenBlocked() {
  // Zombie a x=500 rivolto verso il muro, giocatore dietro il muro un po' a "sinistra"
  // (+90 gradi da (-1,0) e' (0,-1)): gira da fermo verso sinistra finche' davanti e' libero.
  ps1::NavState s;
  ps1::NavInput in = {{500, 0}, {-1, 0}, {-3000, -300}};
  auto out = ps1::NavStep(s, kWall, in);
  CHECK(!out.lineOfSight);
  CHECK(!out.walk);
  CHECK(out.turnLeft);
  int updates = 1;
  while (s.mode != NavMode::Straight && updates < 200) {
    in.forward = out.heading;
    out = ps1::NavStep(s, kWall, in);
    CHECK(!out.walk);
    ++updates;
  }
  CHECK(s.mode == NavMode::Straight);
  // Esce quando la sonda di 2000 non tocca piu' il muro: heading quasi parallelo al muro.
  CHECK(out.heading.z < -0.2f);
  CHECK(updates > 10);
  // Dall'update seguente avanza dritto.
  in.forward = out.heading;
  out = ps1::NavStep(s, kWall, in);
  CHECK(out.walk);
}

static void TestStraightWhenAheadClear() {
  // Linea bloccata ma davanti libero e giocatore nel cono: prosegue dritto.
  const std::vector<Wall> walls = {{-200, 1000, -200, -1000}};
  ps1::NavState s;
  const auto out = ps1::NavStep(s, walls, {{500, 500}, {0, -1}, {-3000, -800}});
  CHECK(!out.lineOfSight);
  CHECK(s.mode == NavMode::Straight);
  CHECK(out.walk);
  CHECK(std::fabs(out.heading.z + 1.0f) < 1e-4f);
}

static void TestRateScale() {
  ps1::NavState s;
  ps1::NavParams p;
  p.rateScale = 1.5f;
  const auto out = ps1::NavStep(s, kWall, {{500, 0}, {-1, 0}, {-3000, -300}}, p);
  const float turned = std::fabs(std::atan2(out.heading.z, -out.heading.x));
  CHECK(std::fabs(turned - 1.5f * ps1::kTurnStep) < 1e-3f);
}

static void TestPlayerAgainstWall() {
  // Giocatore appena dietro la linea (a 300 da essa): sul PS1 non puo' starci,
  // quindi il muro non conta; piu' lontano dietro il muro, invece, si.
  ps1::NavState s;
  auto out = ps1::NavStep(s, kWall, {{500, 0}, {-1, 0}, {-300, 0}});
  CHECK(out.lineOfSight);
  out = ps1::NavStep(s, kWall, {{500, 0}, {-1, 0}, {-3000, 0}});
  CHECK(!out.lineOfSight);
}

static void TestDistance() {
  CHECK(std::fabs(ps1::DistanceToWall({300, 0}, kWall[0]) - 300.0f) < 1e-3f);
  CHECK(std::fabs(ps1::DistanceToWall({0, 1300}, kWall[0]) - 300.0f) < 1e-3f);
}

int main() {
  TestLineOfSight();
  TestClearChase();
  TestSteerTable();
  TestNoOvershoot();
  TestTurnsWhenBlocked();
  TestStraightWhenAheadClear();
  TestRateScale();
  TestPlayerAgainstWall();
  TestDistance();
  if (failures) {
    std::printf("%d test falliti\n", failures);
    return EXIT_FAILURE;
  }
  std::printf("OK\n");
  return EXIT_SUCCESS;
}
