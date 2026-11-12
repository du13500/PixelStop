#pragma once
// Continues a radar position between two requests: distance and altitude are
// estimated from ground speed, track and vertical rate of the last answer.
// Pure functions without Arduino dependencies so they can be tested on the host.
#include <math.h>
#include <stdint.h>

struct RadarMotion {
  float lat, lon;       // aircraft position of the last answer
  float speedKt;        // ground speed
  float trackDeg;       // true track, 0 = north
  float rateFpm;        // vertical rate (barometric, else geometric)
  uint32_t sampleMs;    // millis() of the position (request time minus position age)
  bool hasMotion;       // speed and track usable
  bool hasRate;         // vertical rate usable
};

// Estimates stop after this time at the latest; slower polls freeze the values.
static const uint32_t RADAR_MOTION_MIN_MS = 60000UL;
static const uint32_t RADAR_MOTION_MAX_MS = 600000UL;  // up to 10 min (quota intervals)
// The altitude follows the vertical rate for a shorter time: aircraft level off.
static const uint32_t RADAR_MOTION_ALT_MS = 45000UL;
// Grace for request duration before a value counts as overdue.
static const uint32_t RADAR_MOTION_GRACE_MS = 15000UL;

inline float radarFlatDistanceKm(float lat1, float lon1, float lat2, float lon2) {
  float dLat = (lat2 - lat1) * 111.0f;
  float dLon = (lon2 - lon1) * 111.0f * cosf(lat1 * 3.14159265f / 180.0f);
  return sqrtf(dLat*dLat + dLon*dLon);
}

// How long estimates may run for the current request interval.
inline uint32_t radarMotionLimitMs(uint32_t intervalMs) {
  uint32_t limit = intervalMs + RADAR_MOTION_GRACE_MS;
  if (limit < RADAR_MOTION_MIN_MS) limit = RADAR_MOTION_MIN_MS;
  if (limit > RADAR_MOTION_MAX_MS) limit = RADAR_MOTION_MAX_MS;
  return limit;
}

inline uint32_t radarMotionElapsedMs(const RadarMotion& m, uint32_t nowMs, uint32_t limitMs) {
  uint32_t dt = nowMs - m.sampleMs;           // wraps correctly with millis()
  if (dt > 0x80000000UL) dt = 0;              // sample "in the future": no estimate
  return dt > limitMs ? limitMs : dt;
}

inline void radarMotionPosition(const RadarMotion& m, uint32_t nowMs, uint32_t limitMs, float& lat, float& lon) {
  lat = m.lat; lon = m.lon;
  if (!m.hasMotion) return;
  float hours = radarMotionElapsedMs(m, nowMs, limitMs) / 3600000.0f;
  float km = m.speedKt * 1.852f * hours;
  float rad = m.trackDeg * 3.14159265f / 180.0f;
  lat = m.lat + km * cosf(rad) / 111.0f;
  float c = cosf(m.lat * 3.14159265f / 180.0f);
  if (c > 0.01f) lon = m.lon + km * sinf(rad) / (111.0f * c);
  if (lon > 180.0f) lon -= 360.0f;
  if (lon < -180.0f) lon += 360.0f;
}

inline float radarMotionDistanceKm(const RadarMotion& m, float homeLat, float homeLon, uint32_t nowMs, uint32_t limitMs) {
  float lat, lon;
  radarMotionPosition(m, nowMs, limitMs, lat, lon);
  return radarFlatDistanceKm(homeLat, homeLon, lat, lon);
}

inline int32_t radarMotionAltitudeFt(const RadarMotion& m, int32_t altitudeFt, uint32_t nowMs, uint32_t limitMs) {
  if (!m.hasRate) return altitudeFt;
  uint32_t altLimit = limitMs < RADAR_MOTION_ALT_MS ? limitMs : RADAR_MOTION_ALT_MS;
  float minutes = radarMotionElapsedMs(m, nowMs, altLimit) / 60000.0f;
  float alt = altitudeFt + m.rateFpm * minutes;
  return alt < 0 ? 0 : (int32_t)lroundf(alt);
}

// Footer dot: white briefly after new data, green on schedule, yellow when an
// update is overdue or failed, grey once the values are frozen.
enum RadarDot : uint8_t { RADAR_DOT_NEW, RADAR_DOT_FRESH, RADAR_DOT_ESTIMATED, RADAR_DOT_FROZEN };
static const uint32_t RADAR_DOT_NEW_MS = 1500UL;

inline RadarDot radarDotState(uint32_t ageMs, uint32_t intervalMs, bool failed) {
  if (ageMs >= radarMotionLimitMs(intervalMs)) return RADAR_DOT_FROZEN;
  if (ageMs < RADAR_DOT_NEW_MS && !failed) return RADAR_DOT_NEW;
  if (!failed && ageMs <= intervalMs + RADAR_MOTION_GRACE_MS) return RADAR_DOT_FRESH;
  return RADAR_DOT_ESTIMATED;
}
