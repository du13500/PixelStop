#include "AdsbLol.h"
#include "AdsbQuota.h"
#include "RequestPolicy.h"
#include <LittleFS.h>
#include <time.h>

// Optional key from adsb.lol.
static char g_key[72] = "";
// Requests per window (AdsbQuota.h). Kept in RTC memory across restarts and
// in a small file across power loss, so a reboot never resets the counters.
static QuotaState g_quota = {};
static bool g_dirty = false;
static uint32_t g_savedMs = 0;
static const char QUOTA_PATH[] = "/adsblol-quota.bin";
static const uint32_t QUOTA_MAGIC = 0x51544131UL;  // "QTA1"
static const uint32_t QUOTA_RTC_WORD = 64;          // Diagnostics uses words 32..42
static const uint32_t QUOTA_SAVE_MS = 600000UL;     // file at most every 10 min
struct QuotaRecord { uint32_t magic; QuotaState state; uint32_t checksum; };

static uint32_t quotaNow() {
  time_t now = time(nullptr);
  return now > 1609459200 ? (uint32_t)now : 0;
}
static uint32_t recordChecksum(const QuotaRecord& r) {
  uint32_t h = 2166136261UL; const uint8_t* p = (const uint8_t*)&r;
  for (size_t i = 0; i < sizeof(r) - sizeof(r.checksum); i++) h = (h ^ p[i]) * 16777619UL;
  return h;
}
static void writeRtc() {
  QuotaRecord r = {QUOTA_MAGIC, g_quota, 0}; r.checksum = recordChecksum(r);
  ESP.rtcUserMemoryWrite(QUOTA_RTC_WORD, (uint32_t*)&r, sizeof(r));
}

void adsbLolQuotaBegin() {
  QuotaRecord r = {};
  File f = LittleFS.open(QUOTA_PATH, "r");
  if (f) {
    if (f.read((uint8_t*)&r, sizeof(r)) == sizeof(r) && r.magic == QUOTA_MAGIC && r.checksum == recordChecksum(r)) quotaMerge(g_quota, r.state);
    f.close();
  }
  if (ESP.rtcUserMemoryRead(QUOTA_RTC_WORD, (uint32_t*)&r, sizeof(r)) && r.magic == QUOTA_MAGIC && r.checksum == recordChecksum(r))
    quotaMerge(g_quota, r.state);
  writeRtc();
  g_savedMs = millis();
}

void adsbLolQuotaSave(bool force) {
  if (!g_dirty || (!force && (uint32_t)(millis() - g_savedMs) < QUOTA_SAVE_MS)) return;
  QuotaRecord r = {QUOTA_MAGIC, g_quota, 0}; r.checksum = recordChecksum(r);
  File f = LittleFS.open(QUOTA_PATH, "w");
  if (f) { f.write((const uint8_t*)&r, sizeof(r)); f.close(); g_dirty = false; }
  g_savedMs = millis();
}

void adsbLolSetKey(const String& key) {
  strlcpy(g_key, key.c_str(), sizeof(g_key));
}

bool adsbLolHasKey() { return g_key[0] != 0; }

static const uint16_t* quotaUse() { return g_key[0] ? ADSB_QUOTA_KEY : ADSB_QUOTA_NOKEY; }

void adsbLolPrepare(HTTPClient& http, const char* userAgent) {
  quotaCount(g_quota, quotaNow());
  g_dirty = true;
  writeRtc();
  http.useHTTP10(false);
  http.setUserAgent(userAgent);
  if (g_key[0]) http.addHeader("x-api-key", g_key);
  static const char* headers[] = {"Retry-After", "Transfer-Encoding"};
  http.collectHeaders(headers, 2);
}

uint16_t adsbLolRequestsToday() {
  quotaRoll(g_quota, quotaNow());
  return g_quota.used[Q_DAY];
}

uint32_t adsbLolBudgetIntervalMs(float visibleShare) {
  return quotaIntervalMs(g_quota, quotaNow(), quotaUse(), visibleShare);
}

uint32_t adsbLolQuotaWaitMs() {
  return quotaWaitMs(g_quota, quotaNow(), quotaUse());
}

uint32_t adsbLolWaitMs() {
  uint32_t provider = serviceWaitMs(SERVICE_ADSB_LOL), quota = adsbLolQuotaWaitMs();
  return provider > quota ? provider : quota;
}

void adsbLolQuotaUsage(uint8_t window, uint16_t& used, uint16_t& limit) {
  quotaRoll(g_quota, quotaNow());
  used = window < Q_COUNT ? g_quota.used[window] : 0;
  limit = window < Q_COUNT ? quotaUse()[window] : 0;
}
