#include "nav_logic.h"

#include <cmath>

namespace ps1 {

namespace {

Vec2 Normalize(Vec2 v) {
  const float len = std::sqrt(v.x * v.x + v.z * v.z);
  return len > 0.0f ? Vec2{v.x / len, v.z / len} : Vec2{0.0f, 0.0f};
}

int Probe(const std::vector<Wall>& walls, Vec2 pos, Vec2 dir, float distance) {
  return FirstBlockingWall(walls, pos, {pos.x + dir.x * distance, pos.z + dir.z * distance});
}

// 80100924(E, 0x5A) -> 8010078C: giocatore nel cono di +/-87 gradi davanti.
bool PlayerInCone(Vec2 heading, Vec2 toPlayer) {
  const Vec2 t = Normalize(toPlayer);
  return heading.x * t.x + heading.z * t.z > std::cos(kForwardConeHalfAngle);
}

}  // namespace

int FirstBlockingWall(const std::vector<Wall>& walls, Vec2 a, Vec2 b, float ignoreNear) {
  // Stesse operazioni di 80100AE8, in doppia precisione (sul PS1 int a 32 bit).
  const double ax = a.x, az = a.z, bx = b.x, bz = b.z;
  for (size_t i = 0; i < walls.size(); ++i) {
    const double x1 = walls[i].x1, z1 = walls[i].z1, x2 = walls[i].x2, z2 = walls[i].z2;
    const double sideA = (z2 - z1) * (ax - x1) - (x2 - x1) * (az - z1);
    const int sa = sideA > 0 ? 1 : -1;
    const int sb = (z2 - z1) * (bx - x1) - (x2 - x1) * (bz - z1) < 1 ? -1 : 1;
    const int s1 = (bz - az) * (x1 - ax) - (bx - ax) * (z1 - az) < 1 ? -1 : 1;
    const int s2 = (bz - az) * (x2 - ax) - (bx - ax) * (z2 - az) < 1 ? -1 : 1;
    if (sa * sb < 0 && s1 * s2 < 0 && sideA < 1) {
      if (ignoreNear > 0.0f && DistanceToWall(b, walls[i]) < ignoreNear) continue;
      return static_cast<int>(i);
    }
  }
  return -1;
}

Vec2 Rotate(Vec2 v, float radians) {
  const float c = std::cos(radians), s = std::sin(radians);
  return {v.x * c - v.z * s, v.x * s + v.z * c};
}

float DistanceToWall(Vec2 p, const Wall& w) {
  const float dx = static_cast<float>(w.x2 - w.x1), dz = static_cast<float>(w.z2 - w.z1);
  const float len2 = dx * dx + dz * dz;
  float t = len2 > 0.0f ? ((p.x - w.x1) * dx + (p.z - w.z1) * dz) / len2 : 0.0f;
  t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
  const float cx = w.x1 + t * dx - p.x, cz = w.z1 + t * dz - p.z;
  return std::sqrt(cx * cx + cz * cz);
}

int SteerUnits(float cosine) {
  // 8007F210: switch (cos * 4096 * 10 >> 12); coseno negativo o < 0,1 -> 0x28.
  static const int kUnits[10] = {0x28, 0x24, 0x20, 0x1C, 0x18, 0x14, 0x10, 0x0C, 8, 6};
  if (cosine < 0.1f) return kUnits[0];
  const int index = static_cast<int>(cosine * 10.0f);
  return kUnits[index > 9 ? 9 : index];
}

NavOutput NavStep(NavState& state, const std::vector<Wall>& walls, const NavInput& in,
                  const NavParams& params) {
  const Vec2 toPlayer = {in.player.x - in.pos.x, in.player.z - in.pos.z};
  const Vec2 h = Normalize(in.forward);
  const Vec2 left = Rotate(h, kSideProbeAngle);
  // 8007C04C / 80100D40: lato del giocatore rispetto alla direzione (>= 0: a sinistra).
  const bool playerLeft = toPlayer.x * left.x + toPlayer.z * left.z >= 0.0f;
  NavOutput out;
  out.heading = h;
  out.walk = true;
  out.turnLeft = false;
  out.lineOfSight = FirstBlockingWall(walls, in.pos, in.player, params.playerWallMargin) < 0;

  if (out.lineOfSight) {
    // 801027C0 -> 8007F210: avanza e sterza verso il giocatore. Il PS1 non limita la
    // rotazione all'angolo mancante (vicino all'allineamento oscilla di 6 unita');
    // qui la si limita, per non far tremolare la direzione sul PC.
    state.mode = NavMode::Straight;
    const Vec2 t = Normalize(toPlayer);
    const float cosine = h.x * t.x + h.z * t.z;
    float step = SteerUnits(cosine) * kAngleUnit * params.rateScale;
    const float missing = std::acos(cosine > 1.0f ? 1.0f : (cosine < -1.0f ? -1.0f : cosine));
    if (step > missing) step = missing;
    out.heading = Normalize(Rotate(h, playerLeft ? step : -step));
    return out;
  }

  if (state.mode == NavMode::Straight) {
    // 80100E24, P+0 == 0: prosegue dritto oppure sceglie da che parte girare.
    const int ahead = Probe(walls, in.pos, h, params.probeDistance);  // iVar5
    if (!(ahead == -1 && PlayerInCone(h, toPlayer))) {
      const int leftHit = Probe(walls, in.pos, left, params.probeDistance);  // iVar6
      const int rightHit = Probe(walls, in.pos, Rotate(h, -kSideProbeAngle),
                                 params.probeDistance);                      // iVar7
      NavMode next = NavMode::Straight;
      bool decided = false;
      if (!playerLeft) {
        if (rightHit == -1) next = NavMode::TurnRight, decided = true;
      } else if (leftHit == -1) {
        next = NavMode::TurnLeft, decided = true;
      }
      if (!decided) {  // LAB_80100EFC
        if (ahead == leftHit && ahead != -1) {
          next = NavMode::TurnRight, decided = true;
        } else if (ahead == rightHit && ahead != -1) {
          next = NavMode::TurnLeft, decided = true;
        } else if (leftHit == -1) {  // LAB_80100F3C
          if (rightHit != -1 && playerLeft) {
            next = NavMode::TurnLeft, decided = true;
          } else if (rightHit == -1) {
            next = playerLeft ? NavMode::TurnLeft : NavMode::TurnRight, decided = true;
          }
        } else if (rightHit == -1 && !playerLeft) {
          next = NavMode::TurnRight, decided = true;
        }
      }
      if (!decided && ahead != -1 && DistanceToWall(in.pos, walls[ahead]) < kNearWallDistance) {
        next = playerLeft ? NavMode::TurnLeft : NavMode::TurnRight;
      }
      state.mode = next;
    }
    if (state.mode == NavMode::Straight) return out;  // LAB_80101014 modo 0: avanza dritto
  }

  // LAB_80101014, modi 1/2: E+1C6 += / -= 0x16 senza velocita' (8007F404 la azzera:
  // gira da fermo), poi sonda davanti; libera -> modo 0 dal prossimo update.
  const float step = params.turnStep * params.rateScale;
  out.turnLeft = state.mode == NavMode::TurnLeft;
  out.heading = Normalize(Rotate(h, out.turnLeft ? step : -step));
  out.walk = false;
  if (Probe(walls, in.pos, out.heading, params.probeDistance) == -1) {
    state.mode = NavMode::Straight;
  }
  return out;
}

}  // namespace ps1
