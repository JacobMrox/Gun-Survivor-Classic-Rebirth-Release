#include "backfall_logic.h"

namespace ps1 {

bool ShouldStartBackFall(int rngByte) { return (rngByte & 0xFF) < kBackFallRngThreshold; }

void StartBackPush(BackPush& p) { p.strength = kBackPushStart; }

float NextBackPush(BackPush& p) {
  if (p.strength <= 0) return 0.0f;
  const float push = kBackPushMagnitude * static_cast<float>(p.strength);
  p.strength -= kBackPushStep;
  if (p.strength < 1) p.strength = 0;
  return push;
}

}  // namespace ps1
