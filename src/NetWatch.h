#pragma once
// Network watchdog: 10 minutes without any HTTP answer although requests were
// tried (or without Wi-Fi) -> reconnect Wi-Fi; if 5 more minutes bring no new
// answer although requests were tried (or Wi-Fi is still down) -> restart.
// The waiting time doubles after each such restart in a row (`factor`), so an
// outage of one provider does not restart the cube every quarter of an hour.
// Only a NEW answer (after the reconnect) ends the alarm.
// Pure functions without Arduino dependencies so they can be tested on the host.
#include <stdint.h>

static const uint32_t NET_SILENT_MS = 600000UL, NET_RECONNECT_MS = 300000UL;

struct NetWatch { uint32_t start; uint32_t stageAt; uint8_t stage; };
enum NetAction : uint8_t { NET_OK, NET_RECONNECT, NET_RESTART };

// true if millis() value a is set and later than b (wrap-safe)
inline bool netAfter(uint32_t a, uint32_t b) { return a && (int32_t)(a - b) > 0; }

// answer/attempt: millis() of the last HTTP answer / request attempt (0 = none).
// silent receives the time since the last answer (or the start of watching).
inline NetAction netWatchStep(NetWatch& w, uint32_t now, uint32_t answer, uint32_t attempt,
                              bool wifiDown, uint32_t wifiLostAt, uint32_t factor, uint32_t& silent) {
  const uint32_t ref = netAfter(answer, w.start) ? answer : w.start;
  silent = now - ref;
  if (w.stage && netAfter(answer, w.stageAt)) w.stage = 0;          // new answer after reconnect
  const uint32_t limit = NET_SILENT_MS * (factor ? factor : 1);
  const uint32_t down = wifiDown && wifiLostAt ? now - wifiLostAt : 0;
  if (w.stage == 0) {
    if ((netAfter(attempt, ref) && silent >= limit) || down >= limit) {
      w.stage = 1; w.stageAt = now ? now : 1;
      return NET_RECONNECT;
    }
    return NET_OK;
  }
  if (now - w.stageAt >= NET_RECONNECT_MS && (netAfter(attempt, w.stageAt) || wifiDown)) return NET_RESTART;
  return NET_OK;
}
