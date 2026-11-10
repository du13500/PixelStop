#pragma once
// Request quota for adsb.lol. A free key allows 5 requests per 30 s, 20 per
// minute, 150 per hour, 500 per day, 2,000 per week and 5,000 per month (key
// portal, 10.11.2026); repeated 429 answers can cost the key. Windows are fixed
// in UTC: 30 s, minute, full hour, day, week from Monday, calendar month.
// PixelStop plans below these limits so that it never runs into them.
// Pure functions without Arduino dependencies so they can be tested on the host.
#include <stdint.h>

enum QuotaWindow : uint8_t { Q_30S, Q_MIN, Q_HOUR, Q_DAY, Q_WEEK, Q_MONTH, Q_COUNT };

// Planned use with a key and without one (without a key adsb.lol counts per
// internet connection; the daily limit is the documented one).
static const uint16_t ADSB_QUOTA_KEY[Q_COUNT]   = {4, 18, 135, 470, 1900, 4750};
static const uint16_t ADSB_QUOTA_NOKEY[Q_COUNT] = {4, 18, 135, 450, 1900, 4750};

struct QuotaState { uint32_t id[Q_COUNT]; uint16_t used[Q_COUNT]; };

// Days since 1970-01-01 for a civil date (proleptic Gregorian calendar).
inline int32_t quotaDaysFromCivil(int32_t y, uint32_t m, uint32_t d) {
  y -= m <= 2;
  const int32_t era = (y >= 0 ? y : y - 399) / 400;
  const uint32_t yoe = (uint32_t)(y - era * 400);
  const uint32_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int32_t)doe - 719468;
}
// Year and month (1..12) for a number of days since 1970-01-01.
inline void quotaCivilMonth(int32_t z, int32_t& y, uint32_t& m) {
  z += 719468;
  const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
  const uint32_t doe = (uint32_t)(z - era * 146097);
  const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const uint32_t mp = (5 * doy + 2) / 153;
  m = mp < 10 ? mp + 3 : mp - 9;
  y = (int32_t)yoe + era * 400 + (m <= 2);
}

// Number of the window that contains `now` (UTC seconds).
inline uint32_t quotaWindowId(uint8_t w, uint32_t now) {
  switch (w) {
    case Q_30S:  return now / 30;
    case Q_MIN:  return now / 60;
    case Q_HOUR: return now / 3600;
    case Q_DAY:  return now / 86400;
    case Q_WEEK: return (now / 86400 + 3) / 7;   // 1970-01-01 was a Thursday
    default: { int32_t y; uint32_t m; quotaCivilMonth((int32_t)(now / 86400), y, m); return (uint32_t)(y * 12 + (int32_t)m - 1); }
  }
}
// First second of the next window.
inline uint32_t quotaWindowEnd(uint8_t w, uint32_t now) {
  switch (w) {
    case Q_30S:  return (now / 30 + 1) * 30;
    case Q_MIN:  return (now / 60 + 1) * 60;
    case Q_HOUR: return (now / 3600 + 1) * 3600;
    case Q_DAY:  return (now / 86400 + 1) * 86400;
    case Q_WEEK: return (((now / 86400 + 3) / 7 + 1) * 7 - 3) * 86400;
    default: {
      int32_t y; uint32_t m; quotaCivilMonth((int32_t)(now / 86400), y, m);
      if (++m > 12) { m = 1; y++; }
      return (uint32_t)quotaDaysFromCivil(y, m, 1) * 86400UL;
    }
  }
}

// Starts new windows. Without a valid clock (now == 0) the stored windows stay.
inline void quotaRoll(QuotaState& s, uint32_t now) {
  if (!now) return;
  for (uint8_t w = 0; w < Q_COUNT; w++) {
    uint32_t id = quotaWindowId(w, now);
    if (s.id[w] != id) { s.id[w] = id; s.used[w] = 0; }
  }
}
inline void quotaCount(QuotaState& s, uint32_t now) {
  quotaRoll(s, now);
  for (uint8_t w = 0; w < Q_COUNT; w++) if (s.used[w] < 0xFFFF) s.used[w]++;
}
// Milliseconds until a request fits into every window (0 = now).
inline uint32_t quotaWaitMs(QuotaState& s, uint32_t now, const uint16_t* use) {
  quotaRoll(s, now);
  uint32_t wait = 0;
  for (uint8_t w = 0; w < Q_COUNT; w++) {
    if (s.used[w] < use[w]) continue;
    uint32_t ms = now ? (quotaWindowEnd(w, now) - now) * 1000UL : 60000UL;
    if (ms > wait) wait = ms;
  }
  return wait;
}
// Smallest poll interval that spreads the rest of every window over its
// remaining time. `visibleShare` (0..1] is the share of time the radar is on
// screen, because polls only run then. 0 = no limit beyond the windows.
inline uint32_t quotaIntervalMs(QuotaState& s, uint32_t now, const uint16_t* use, float visibleShare) {
  if (!now) return 0;
  if (visibleShare <= 0.0f || visibleShare > 1.0f) visibleShare = 1.0f;
  quotaRoll(s, now);
  uint32_t interval = 0;
  for (uint8_t w = Q_HOUR; w < Q_COUNT; w++) {
    uint32_t left = quotaWindowEnd(w, now) - now;
    if (s.used[w] >= use[w]) { if (left * 1000UL > interval) interval = left * 1000UL; continue; }
    uint32_t remaining = use[w] - s.used[w];
    uint32_t ms = (uint32_t)((float)left * visibleShare * 1000.0f / (float)remaining);
    if (ms > interval) interval = ms;
  }
  return interval;
}
// Combines two saved states (RAM across restarts, file across power loss):
// the newer window wins, equal windows keep the higher count.
inline void quotaMerge(QuotaState& into, const QuotaState& other) {
  for (uint8_t w = 0; w < Q_COUNT; w++) {
    if (other.id[w] > into.id[w]) { into.id[w] = other.id[w]; into.used[w] = other.used[w]; }
    else if (other.id[w] == into.id[w] && other.used[w] > into.used[w]) into.used[w] = other.used[w];
  }
}
