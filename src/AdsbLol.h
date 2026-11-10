#pragma once
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include "ChunkDecoder.h"

// adsb.lol rejects HTTP/1.0 with 426 "Upgrade Required" since late 2026.
// All adsb.lol requests therefore use HTTP/1.1 and read the body through
// AdsbLolBody, which removes chunked transfer framing on the fly.

void adsbLolSetKey(const String& key);
bool adsbLolHasKey();

// Request quota (AdsbQuota.h): every prepared request counts, including
// failed ones. Counters survive restarts (RTC memory) and power loss (file).
void adsbLolQuotaBegin();                 // after LittleFS.begin()
void adsbLolQuotaSave(bool force = false); // call regularly; writes at most every 10 min
uint16_t adsbLolRequestsToday();
// Smallest radar interval that keeps every window below its limit.
uint32_t adsbLolBudgetIntervalMs(float visibleShare);
// Time until the next request fits the quota, and the larger of that and a
// provider pause (Retry-After). Every adsb.lol request waits for it.
uint32_t adsbLolQuotaWaitMs();
uint32_t adsbLolWaitMs();
// Used and planned requests of a window (Q_HOUR ... Q_MONTH) for the status.
void adsbLolQuotaUsage(uint8_t window, uint16_t& used, uint16_t& limit);

// Call after http.begin(): HTTP/1.1, optional x-api-key, collected headers.
void adsbLolPrepare(HTTPClient& http, const char* userAgent);

// HTTP status codes that will not go away by simply asking again.
static inline bool adsbLolAccessDenied(int code) {
  return code == 401 || code == 403 || code == 426;
}

class AdsbLolBody : public Stream {
public:
  explicit AdsbLolBody(HTTPClient& http)
    : _src{&http.getStream()},
      _decoder(_src, http.header("Transfer-Encoding").equalsIgnoreCase("chunked")) {}

  int read() override {
    if (_peeked >= 0) { int c = _peeked; _peeked = -1; return c; }
    return _decoder.read();
  }
  int peek() override {
    if (_peeked < 0) _peeked = _decoder.read();
    return _peeked;
  }
  int available() override { return _peeked >= 0 ? 1 : _src.stream->available(); }
  size_t write(uint8_t) override { return 0; }
  bool malformed() const { return _decoder.failed(); }

private:
  struct Source {
    Stream* stream;
    int nextByte() {
      // readBytes honours the client's timeout, like HTTPClient itself.
      char c;
      return stream->readBytes(&c, 1) == 1 ? (uint8_t)c : -1;
    }
  };
  Source _src;
  ChunkDecoder<Source> _decoder;
  int _peeked = -1;
};
