#pragma once
// Which aircraft the radar may skip, by the ADS-B emitter category the aircraft
// sends itself (A1 light ... A7 rotorcraft, B1 glider, B2 balloon, B4 ultralight,
// B6 drone, C1-C3 surface vehicles/obstacles) and the ground flag. Aircraft
// without a category are never hidden by the category filters.
#include <stdint.h>
#include <string.h>

enum : uint8_t {
  RADAR_HIDE_HELI   = 1,   // A7
  RADAR_HIDE_LIGHT  = 2,   // A1
  RADAR_HIDE_SPORT  = 4,   // B1, B2, B4, B6
  RADAR_HIDE_GROUND = 8,   // on the ground and surface vehicles (C*)
  RADAR_HIDE_ALL    = 15
};

inline bool radarHidden(const char* category, bool onGround, uint8_t mask) {
  if (!mask) return false;
  const char* c = category ? category : "";
  if ((mask & RADAR_HIDE_GROUND) && (onGround || c[0] == 'C')) return true;
  if ((mask & RADAR_HIDE_HELI) && !strcmp(c, "A7")) return true;
  if ((mask & RADAR_HIDE_LIGHT) && !strcmp(c, "A1")) return true;
  if ((mask & RADAR_HIDE_SPORT) && (!strcmp(c, "B1") || !strcmp(c, "B2") || !strcmp(c, "B4") || !strcmp(c, "B6"))) return true;
  return false;
}

// Emergency squawks: 7500 unlawful interference, 7600 radio failure, 7700 general emergency.
inline bool radarEmergencySquawk(const char* squawk) {
  return squawk && (!strcmp(squawk, "7500") || !strcmp(squawk, "7600") || !strcmp(squawk, "7700"));
}

// Climb/descent indicator: +1 climbing, -1 descending, 0 level or unknown.
static const float RADAR_TREND_FPM = 300.0f;
inline int8_t radarTrend(bool hasRate, float rateFpm) {
  if (!hasRate) return 0;
  return rateFpm >= RADAR_TREND_FPM ? 1 : (rateFpm <= -RADAR_TREND_FPM ? -1 : 0);
}
