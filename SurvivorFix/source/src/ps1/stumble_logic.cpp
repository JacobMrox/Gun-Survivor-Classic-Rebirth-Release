#include "stumble_logic.h"

#include <cmath>

namespace ps1 {

void ResetOnChaseStart(StumbleState& s) {
  s.band = 0;
  s.attemptConsumed = false;
}

void UpdateBand(StumbleState& s, float distance) {
  const int d = static_cast<int>(distance);
  if (d > kBand1Min && d <= kBand1Max) {
    s.band = 1;  // il bit 8 resta com'e'
  } else if (d > kBand2Min && d <= kBand2Max) {
    s.band = 2;
    s.attemptConsumed = false;
  } else if (d > kBand3Min && d <= kBand2Min) {
    s.band = 3;
    s.attemptConsumed = false;
  } else if (d <= kBand4Max) {
    s.band = 4;
    s.attemptConsumed = false;
  }
  // 1600 < d <= 3000 oppure d > 5000: nessun cambiamento (salto a 80102A60).
}

bool ShouldStartFall(StumbleState& s, int rngByte, bool playerInSector) {
  if (s.band != 1 || s.attemptConsumed) return false;
  s.attemptConsumed = true;
  if ((rngByte & 0xFF) >= kRngThreshold) return false;
  return playerInSector;
}

bool InFrontSector(float forwardX, float forwardZ, float toX, float toZ) {
  const float lengths = std::sqrt((forwardX * forwardX + forwardZ * forwardZ) *
                                  (toX * toX + toZ * toZ));
  if (lengths <= 0.0f) return false;
  const float halfAngle = 2.0f * 3.14159265f * kSectorHalfAngle / kAngleUnitsPerTurn;
  return (forwardX * toX + forwardZ * toZ) / lengths > std::cos(halfAngle);
}

FallExit DecideFallExit(float distance, bool playerFrontBit, bool gateOk) {
  if (distance < kContactDistance && playerFrontBit && gateOk) {
    return FallExit::Contact;
  }
  return FallExit::GetUp;
}

}  // namespace ps1
