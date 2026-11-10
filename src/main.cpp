
#include <Arduino.h>
#include <sigma_delta.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266WebServer.h>
#include <Updater.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include "Vvo.h"
#include "Weather.h"
#include "AdsbLol.h"
#include "Flight.h"
#include "TrackFlight.h"
#include "Geo.h"
#include "RequestPolicy.h"
#include "Diagnostics.h"
#include "PixelFont.h"
#include "TimeZones.h"
#include "WeatherVisual.h"
#include "AircraftLabel.h"
#include "AirlineName.h"
#include "MilitaryOperator.h"
#include "RadarFilter.h"
#include "RadarPolicy.h"
#include "DestinationFormat.h"
#include "WebGuard.h"
#include "NetWatch.h"
#include "AdsbQuota.h"

// -----------------------------------------------------------------------------
// SmallTV-Ultra hardware
// ESP8266 HW SPI: SCLK GPIO14, MOSI GPIO13
// Pin map based on the open-source smalltv-mod hardware definition.
// -----------------------------------------------------------------------------
#define TFT_DC   0
#define TFT_RST  2
#define TFT_CS   15
#define TFT_BL   5
#define TFT_W    240
#define TFT_H    240

#include "Version.h"
// Backlight PWM (since v0.4.14.6, against flashes at 1 % brightness)
#define BL_PWM_FREQ 300
#define BL_PWM_RANGE 1023

#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_YELLOW  0xFFE0
#define C_RED     0xF800
#define C_GREEN   0x07E0
#define C_GRAY    0x8410
#define C_DGRAY   0x4208
#define C_BLUE    0x041F
#define C_CYAN    0x07FF
#define C_KHAKI   0xBDAD

enum AppMode : uint8_t {
  MODE_DEPARTURES = 0,
  MODE_WEATHER    = 1,
  MODE_PLANE      = 2,
  MODE_CLOCK      = 3,
  MODE_TRACKED_FLIGHT = 4,
  MODE_COUNTDOWN1 = 5,
  MODE_COUNTDOWN2 = 6,
  MODE_COUNTDOWN3 = 7,
  MODE_COUNTDOWN4 = 8,
  MODE_FORECAST   = 9   // three-day forecast; appended so stored masks keep their bits
};
static const uint8_t MODE_COUNT = 10;
// Rotation follows the order of the list in the web interface.
static const uint8_t ROTATION_ORDER[MODE_COUNT] = {
  MODE_DEPARTURES, MODE_WEATHER, MODE_FORECAST, MODE_PLANE, MODE_TRACKED_FLIGHT, MODE_CLOCK,
  MODE_COUNTDOWN1, MODE_COUNTDOWN2, MODE_COUNTDOWN3, MODE_COUNTDOWN4
};

enum Language : uint8_t { LANG_DE = 0, LANG_EN = 1 };

enum CountdownType : uint8_t {
  COUNTDOWN_DAYS = 0,
  COUNTDOWN_HMS  = 1,
  COUNTDOWN_MS   = 2
};

struct CountdownSlot {
  String title = "";
  String titleColor = "#FFFFFF";
  String countColor = "#FF5D56";
  uint8_t type = COUNTDOWN_DAYS;
  String targetDate = "";        // YYYY-MM-DD, intentionally date-only
  uint32_t durationSec = 600;     // used for HMS / MS start/restart
  uint32_t targetEpoch = 0;       // absolute target for running duration timers
  bool active = false;
  uint8_t dayDigits = 3;          // keep leading zeroes stable once activated
};

struct Settings {
  String ssid;
  String pass;
  String hostname = "pixelstop";
  uint8_t lang = LANG_DE;          // display and web interface language

  uint8_t mode = MODE_DEPARTURES; // current runtime view
  uint16_t rotationMask = (1U << MODE_DEPARTURES);
  uint16_t rotationSec[MODE_COUNT] = {30,20,20,15,20,20,20,20,20,15};
  uint8_t brightness = 70;
  uint8_t backlightMode = 0;         // 0 = PWM, 1 = sigma-delta (test against flashes)
  String timeZone = "Europe/Berlin";

  bool nightEnabled = true;
  uint16_t nightStart = 22 * 60;
  uint16_t nightEnd = 7 * 60;
  uint8_t nightBrightness = 12;

  String stopName = "Pohlandplatz";
  String stopPlace = "";             // town of the stop, from the VVO search
  String stopId = "";
  uint8_t departureCount = 4;
  uint16_t departurePoll = 30;
  uint8_t vvoMotMask = VVO_MOT_ALL;
  bool departureCompactDestinations = false;
  bool departureAbbreviate = true;   // Hbf, Str., Pl. ...
  bool departureDropTown = true;     // "Dresden Bühlau" -> "Bühlau"

  String weatherCity = "Dresden"; // selected real place / fallback title
  String weatherDisplayName = ""; // optional display-only title
  float weatherLat = 51.0504f;
  float weatherLon = 13.7373f;
  uint16_t weatherPoll = 600;
  bool weatherTempColor = false;
  bool weatherFahrenheit = false;
  bool geoGermanNames = false;

  String planePlace = "Dresden";
  String planeDisplayName = "";
  float planeLat = 51.0504f;
  float planeLon = 13.7373f;
  uint16_t planeRadiusKm = 25;
  uint16_t planePoll = 10;
  bool planeShowPlace = false;
  bool planeShowAge = true;
  uint16_t planePlaceToggleSec = 10;
  uint8_t planeOperator = 0;         // 0 = name of the callsign, 1 = branch, 2 = both
  uint8_t planeSpeed = 0;            // 0 = off, 1 = km/h, 2 = knots
  bool planeTrend = false;           // climb/descent arrow
  bool planeMarkMilitary = true;     // "MIL" next to the callsign
  uint8_t planeHide = 0;             // RadarFilter.h bits

  String trackedFlightNo = "";
  String aviationstackKey = "";
  String adsbLolKey = "";
  uint16_t trackerStatusPollMin = 15;

  CountdownSlot countdown[4];
};

static Settings cfg;
static ESP8266WebServer server(80);
static DNSServer dns;
static bool apMode = false;
// Rescue mode after repeated crashes: web interface and update only (no requests).
static bool rescueMode = false;
// millis() of the first radio association (Wi-Fi event), for the start-up log.
static volatile uint32_t wifiAssocMs = 0;
static uint32_t lastSecond = 0;
static uint8_t lastMinute = 255;
static uint32_t lastModeSwitchMs = 0;
static String lastCountdownRendered[4];
static uint8_t departurePage = 0;
static uint8_t weatherPage = 0;            // 0 = today, 1 = three-day forecast
static bool departurePagingReady = false;
static uint32_t lastDeparturePageSwitchMs = 0;
static bool planePlacePhase = false;
static uint32_t lastPlanePlaceSwitchMs = 0;

// ---- language ---------------------------------------------------------------
// Display texts live in flash (PSTR), both languages; only the selected one
// is copied into a String when drawn.
static String tr(PGM_P de, PGM_P en) { return String(FPSTR(cfg.lang==LANG_EN ? en : de)); }
#define TR(de,en) tr(PSTR(de),PSTR(en))

// ---- ST7789 quirk -----------------------------------------------------------
class SmallTV_ST7789 : public Arduino_ST7789 {
 public:
  using Arduino_ST7789::Arduino_ST7789;
  bool begin(int32_t speed = GFX_NOT_DEFINED) override {
    _override_datamode = SPI_MODE3;
    return Arduino_TFT::begin(speed);
  }

  void setRotation(uint8_t r) override {
    Arduino_TFT::setRotation(r);
    uint8_t madctl;
    switch (_rotation) {
      case 1:  madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MV; break;
      case 2:  madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MY; break;
      case 3:  madctl = ST7789_MADCTL_MY | ST7789_MADCTL_MV; break;
      default: madctl = 0; break;
    }
    _bus->beginWrite();
    _bus->writeC8D8(ST7789_MADCTL, madctl); // RGB order
    _bus->endWrite();
  }
};

static Arduino_DataBus* bus = nullptr;
static Arduino_GFX* gfx = nullptr;

// Web interface: src/webui.html, stored gzip-compressed by tools/webui_build.py.
#include "webui_html.h"


// ---- helpers ----------------------------------------------------------------
static bool nextCodepoint(const String& s, size_t& i, uint32_t& cp) {
  return nextDisplayCodepoint(s.c_str(), s.length(), i, cp);
}

static uint16_t glyphCount(const String& s) {
  uint16_t n=0; uint32_t cp=0; size_t i=0;
  while(nextCodepoint(s,i,cp)) n++;
  return n;
}

static void drawSpecialGlyph(int x,int y,uint8_t size,const uint8_t rows[7],uint16_t color) {
  for(uint8_t ry=0;ry<7;ry++) for(uint8_t rx=0;rx<5;rx++) if(pixelRow(rows+ry)&(1U<<(4-rx))) gfx->fillRect(x+rx*size,y+ry*size,size,size,color);
}

static void printDisplayTextAt(int x,int y,const String& s,uint8_t size,uint16_t color) {
  gfx->setTextSize(size); gfx->setTextColor(color);
  size_t i=0; uint32_t cp=0;
  while(nextCodepoint(s,i,cp)) {
    const uint8_t* g=specialGlyph(cp);
    if(g) drawSpecialGlyph(x,y,size,g,color);
    else if(cp>=32 && cp<127) { gfx->setCursor(x,y); gfx->write((uint8_t)cp); }
    else drawSpecialGlyph(x,y,size,missingGlyph,color);
    x += 6*size;
  }
}

static int textW(const String& s, uint8_t size) { return glyphCount(s) * 6 * size; }

static void centerText(const String& s, int y, uint8_t size, uint16_t color) {
  int x = (TFT_W - textW(s, size)) / 2;
  if (x < 0) x = 0;
  printDisplayTextAt(x,y,s,size,color);
}

static String crop(const String& s, uint8_t maxChars) {
  if (glyphCount(s) <= maxChars) return s;
  if (maxChars < 2) return "";
  size_t i=0,end=0; uint32_t cp=0; uint8_t n=0;
  while(i<s.length() && n<maxChars-1){size_t before=i;if(!nextCodepoint(s,i,cp))break;end=i;n++;(void)before;}
  end=destCropEnd(s.c_str(),end); // no "Leipzig ." with a blank before the dot
  return s.substring(0,end)+".";
}

// Cache only the active view. Compare formatted visible data, not API timestamps.
struct VisibleField { String text; int16_t x=0,y=0,w=0,h=0,tx=0,ty=0; uint16_t color=0; uint8_t size=0; bool valid=false; };
static VisibleField visibleFields[32];
static bool viewFresh=true;
static void beginView(bool force) {
  viewFresh=force;
  if(force){for(auto& f:visibleFields){f.text="";f.valid=false;}gfx->fillScreen(C_BLACK);}
}
static bool visibleText(uint8_t id,const String& text,int x,int y,uint8_t size,uint16_t color,int bx,int by,int bw,int bh,bool force=false) {
  VisibleField& old=visibleFields[id];
  if(!force&&old.valid&&old.text==text&&old.tx==x&&old.ty==y&&old.w==bw&&old.h==bh&&old.size==size&&old.color==color)return false;
  if(old.valid||!viewFresh){
    if(old.valid)gfx->fillRect(old.x,old.y,old.w,old.h,C_BLACK);
    gfx->fillRect(bx,by,bw,bh,C_BLACK);
  }
  printDisplayTextAt(x,y,text,size,color);
  old.text=text;old.tx=x;old.ty=y;old.x=bx;old.y=by;old.w=bw;old.h=bh;old.size=size;old.color=color;old.valid=true;
  // Store bounds separately from actual text alignment in the signature below.
  return true;
}
static bool visibleCenter(uint8_t id,const String& text,int y,uint8_t size,uint16_t color,int top,int height) {
  return visibleText(id,text,(TFT_W-textW(text,size))/2,y,size,color,8,top,224,height);
}

static String timeHHMM() {
  time_t now = time(nullptr);
  if (now < 1609459200) return "--:--";
  struct tm t;
  localtime_r(&now, &t);
  char b[6];
  snprintf(b, sizeof(b), "%02d:%02d", t.tm_hour, t.tm_min);
  return String(b);
}

static void applyTimeZone() {
  String rule=String(FPSTR(timeZoneRule(timeZoneIndex(cfg.timeZone.c_str()))));
  setenv("TZ",rule.c_str(),1);tzset();lastMinute=255;
}

static bool nightWindow(uint16_t now, uint16_t start, uint16_t end) {
  if (start == end) return false;
  if (start < end) return now >= start && now < end;
  return now >= start || now < end;
}

static void applyBrightness() {
  uint8_t b = cfg.brightness;
  time_t now = time(nullptr);
  if (cfg.nightEnabled && now > 1609459200) {
    struct tm t; localtime_r(&now, &t);
    uint16_t m = t.tm_hour * 60 + t.tm_min;
    if (nightWindow(m, cfg.nightStart, cfg.nightEnd)) b = cfg.nightBrightness;
  }
  if (b > 100) b = 100;
  // SmallTV backlight is active-low.
  int duty = BL_PWM_RANGE - ((int)b * BL_PWM_RANGE / 100);
  // Reapplying PWM can interrupt very dim backlighting. Update only on change.
  static int lastDuty=-1,lastMode=-1;
  if(duty==lastDuty&&cfg.backlightMode==lastMode)return;
  if(cfg.backlightMode==1){
    // Sigma-delta: the chip's own pulse generator, not interrupted by Wi-Fi.
    // Active-low: the share of LOW (= light) is b %, at least one step.
    uint16_t on=(uint16_t)(((uint32_t)b*256U+99U)/100U);if(on>256)on=256;
    if(lastMode!=1){digitalWrite(TFT_BL,HIGH);sigmaDeltaEnable();sigmaDeltaSetPrescaler(255);sigmaDeltaAttachPin(TFT_BL,0);}
    sigmaDeltaWrite(0,(uint8_t)(256-on));
  } else {
    if(lastMode==1)sigmaDeltaDetachPin(TFT_BL);
    analogWrite(TFT_BL, duty);
  }
  lastDuty=duty;lastMode=cfg.backlightMode;
}

static bool modeEnabled(uint8_t mode) {
  return mode < MODE_COUNT && (cfg.rotationMask & (1U << mode));
}

static uint8_t enabledModeCount() {
  uint8_t n=0; for(uint8_t i=0;i<MODE_COUNT;i++) if(modeEnabled(i)) n++; return n;
}

static uint8_t firstEnabledMode() {
  for(uint8_t i=0;i<MODE_COUNT;i++) if(modeEnabled(ROTATION_ORDER[i])) return ROTATION_ORDER[i];
  return MODE_DEPARTURES;
}

static uint8_t nextEnabledMode(uint8_t current) {
  uint8_t pos=0; for(uint8_t i=0;i<MODE_COUNT;i++) if(ROTATION_ORDER[i]==current) pos=i;
  for(uint8_t step=1;step<=MODE_COUNT;step++) { uint8_t m=ROTATION_ORDER[(pos+step)%MODE_COUNT]; if(modeEnabled(m)) return m; }
  return current;
}

// Radar requests run only while the radar is on screen, plus a short
// prefetch before the rotation switches to it, so a fresh aircraft is ready.
static const uint32_t RADAR_PREFETCH_MS = 10000UL;
static bool radarActive() {
  if(!modeEnabled(MODE_PLANE)) return false;
  if(cfg.mode==MODE_PLANE) return true;
  if(enabledModeCount()<=1||nextEnabledMode(cfg.mode)!=MODE_PLANE) return false;
  uint32_t stayMs=(uint32_t)max((uint16_t)5,cfg.rotationSec[cfg.mode])*1000UL;
  uint32_t shown=(uint32_t)(millis()-lastModeSwitchMs);
  return shown+RADAR_PREFETCH_MS>=stayMs;
}
// Share of the rotation the radar is visible; used to spread the daily budget.
static float radarVisibleShare() {
  if(enabledModeCount()<=1) return 1.0f;
  uint32_t total=0;
  for(uint8_t i=0;i<MODE_COUNT;i++) if(modeEnabled(i)) total+=max((uint16_t)5,cfg.rotationSec[i]);
  uint32_t radar=max((uint16_t)5,cfg.rotationSec[MODE_PLANE])+RADAR_PREFETCH_MS/1000UL;
  return total?min(1.0f,(float)radar/(float)total):1.0f;
}

// ---- local config ------------------------------------------------------------
static const char* CFG_PATH = "/config.json";

static bool validHexColor(const String& s) {
  if (s.length() != 7 || s[0] != '#') return false;
  for (uint8_t i=1;i<7;i++) if (!isxdigit((unsigned char)s[i])) return false;
  return true;
}

static bool saveConfig() {
  JsonDocument d;
  d["ssid"] = cfg.ssid; d["pass"] = cfg.pass; d["hostname"] = cfg.hostname;
  d["timeZone"] = cfg.timeZone;
  d["mode"] = cfg.mode; d["rotationMask"] = cfg.rotationMask; d["brightness"] = cfg.brightness; d["backlightMode"] = cfg.backlightMode;
  JsonArray rs = d["rotationSec"].to<JsonArray>(); for(uint8_t i=0;i<MODE_COUNT;i++) rs.add(cfg.rotationSec[i]);
  d["nightEnabled"] = cfg.nightEnabled; d["nightStart"] = cfg.nightStart; d["nightEnd"] = cfg.nightEnd; d["nightBrightness"] = cfg.nightBrightness;
  d["stopName"] = cfg.stopName; d["stopPlace"] = cfg.stopPlace; d["stopId"] = cfg.stopId; d["departureCount"] = cfg.departureCount; d["departurePoll"] = cfg.departurePoll; d["vvoMotMask"] = cfg.vvoMotMask; d["lang"] = cfg.lang; d["departureCompactDestinations"] = cfg.departureCompactDestinations; d["departureAbbreviate"] = cfg.departureAbbreviate; d["departureDropTown"] = cfg.departureDropTown;
  d["weatherCity"] = cfg.weatherCity; d["weatherDisplayName"] = cfg.weatherDisplayName; d["weatherLat"] = cfg.weatherLat; d["weatherLon"] = cfg.weatherLon; d["weatherPoll"] = cfg.weatherPoll; d["weatherTempColor"] = cfg.weatherTempColor; d["weatherFahrenheit"] = cfg.weatherFahrenheit; d["geoGermanNames"] = cfg.geoGermanNames;
  d["planePlace"] = cfg.planePlace; d["planeDisplayName"] = cfg.planeDisplayName; d["planeLat"] = cfg.planeLat; d["planeLon"] = cfg.planeLon; d["planeRadiusKm"] = cfg.planeRadiusKm; d["planePoll"] = cfg.planePoll; d["planeShowAge"] = cfg.planeShowAge; d["planeShowPlace"] = cfg.planeShowPlace; d["planePlaceToggleSec"] = cfg.planePlaceToggleSec;
  d["planeOperator"] = cfg.planeOperator; d["planeSpeed"] = cfg.planeSpeed; d["planeTrend"] = cfg.planeTrend; d["planeMarkMilitary"] = cfg.planeMarkMilitary; d["planeHide"] = cfg.planeHide;
  d["trackedFlightNo"] = cfg.trackedFlightNo; d["aviationstackKey"] = cfg.aviationstackKey; d["trackerStatusPollMin"] = cfg.trackerStatusPollMin; d["adsbLolKey"] = cfg.adsbLolKey;
  JsonArray cds = d["countdowns"].to<JsonArray>();
  for (uint8_t i=0;i<4;i++) {
    JsonObject c = cds.add<JsonObject>();
    c["title"] = cfg.countdown[i].title; c["titleColor"] = cfg.countdown[i].titleColor; c["countColor"] = cfg.countdown[i].countColor;
    c["type"] = cfg.countdown[i].type; c["targetDate"] = cfg.countdown[i].targetDate; c["durationSec"] = cfg.countdown[i].durationSec;
    c["targetEpoch"] = cfg.countdown[i].targetEpoch; c["active"] = cfg.countdown[i].active; c["dayDigits"] = cfg.countdown[i].dayDigits;
  }
  if(d.overflowed()) return false;
  static const char* TMP_PATH = "/config.json.tmp";
  File f = LittleFS.open(TMP_PATH, "w");
  if (!f) return false;
  const size_t expected = measureJson(d);
  const size_t written = serializeJson(d, f);
  f.flush();
  const bool complete = written == expected && !f.getWriteError() && f.size() == expected;
  f.close();
  if (!complete) { LittleFS.remove(TMP_PATH); return false; }
  if (!LittleFS.rename(TMP_PATH, CFG_PATH)) { LittleFS.remove(TMP_PATH); return false; }
  return true;
}

static void loadConfig() {
  if (!LittleFS.begin()) LittleFS.format(), LittleFS.begin();
  File f = LittleFS.open(CFG_PATH, "r"); if (!f) return;
  JsonDocument d; if (deserializeJson(d, f)) { f.close(); return; } f.close();
  cfg.ssid = d["ssid"] | cfg.ssid; cfg.pass = d["pass"] | cfg.pass; cfg.hostname = d["hostname"] | cfg.hostname; cfg.hostname.trim(); cfg.hostname.toLowerCase(); // mDNS ignores case; saving must not see a "new" name
  if(d["timeZone"].is<const char*>() && timeZoneIndex(d["timeZone"].as<const char*>())>=0)cfg.timeZone=d["timeZone"].as<String>();
  cfg.mode = constrain((int)(d["mode"] | cfg.mode), 0, MODE_COUNT-1);
  if(d["rotationMask"].is<uint16_t>() || d["rotationMask"].is<int>()) cfg.rotationMask = ((uint16_t)(d["rotationMask"] | cfg.rotationMask)) & 0x03FFU;
  else cfg.rotationMask = (1U << cfg.mode); // migrate v0.4 single-view configuration
  if(!cfg.rotationMask) cfg.rotationMask = (1U << cfg.mode);
  JsonArrayConst rs=d["rotationSec"].as<JsonArrayConst>(); if(!rs.isNull()) for(uint8_t i=0;i<MODE_COUNT&&i<rs.size();i++) cfg.rotationSec[i]=constrain((int)(rs[i]|cfg.rotationSec[i]),5,3600);
  // v0.5.1/v0.5.2: the forecast alternated inside the weather view; now it is a view of its own.
  if(!rs.isNull() && rs.size()<MODE_COUNT && (d["weatherForecast"] | true) && (cfg.rotationMask & (1U<<MODE_WEATHER))) cfg.rotationMask |= (1U<<MODE_FORECAST);
  cfg.brightness = constrain((int)(d["brightness"] | cfg.brightness), 1, 100);
  cfg.backlightMode = (uint8_t)(d["backlightMode"] | cfg.backlightMode) == 1 ? 1 : 0;
  cfg.nightEnabled = d["nightEnabled"] | cfg.nightEnabled; cfg.nightStart = constrain((int)(d["nightStart"] | cfg.nightStart), 0, 1439); cfg.nightEnd = constrain((int)(d["nightEnd"] | cfg.nightEnd), 0, 1439); cfg.nightBrightness = constrain((int)(d["nightBrightness"] | cfg.nightBrightness), 1, 100);
  cfg.stopName = d["stopName"] | cfg.stopName; cfg.stopPlace = d["stopPlace"] | cfg.stopPlace; cfg.stopId = d["stopId"] | cfg.stopId; {int dc=(int)(d["departureCount"] | cfg.departureCount); cfg.departureCount=(dc==10)?10:constrain(dc,3,5);} cfg.departurePoll = constrain((int)(d["departurePoll"] | cfg.departurePoll), 10, 300); cfg.vvoMotMask = ((uint8_t)(d["vvoMotMask"] | cfg.vvoMotMask)) & VVO_MOT_ALL; if(!cfg.vvoMotMask)cfg.vvoMotMask=VVO_MOT_ALL; cfg.lang = (uint8_t)(d["lang"] | cfg.lang) == LANG_EN ? LANG_EN : LANG_DE; cfg.departureCompactDestinations = d["departureCompactDestinations"] | cfg.departureCompactDestinations; cfg.departureAbbreviate = d["departureAbbreviate"] | cfg.departureAbbreviate; cfg.departureDropTown = d["departureDropTown"] | cfg.departureDropTown;
  cfg.weatherCity = d["weatherCity"] | cfg.weatherCity; cfg.weatherDisplayName = d["weatherDisplayName"] | cfg.weatherDisplayName; cfg.weatherLat = d["weatherLat"] | cfg.weatherLat; cfg.weatherLon = d["weatherLon"] | cfg.weatherLon; cfg.weatherPoll = constrain((int)(d["weatherPoll"] | cfg.weatherPoll), 60, 3600); cfg.weatherTempColor = d["weatherTempColor"] | cfg.weatherTempColor; cfg.weatherFahrenheit = d["weatherFahrenheit"] | cfg.weatherFahrenheit; cfg.geoGermanNames = d["geoGermanNames"] | cfg.geoGermanNames;
  cfg.planePlace = d["planePlace"] | cfg.planePlace; cfg.planeDisplayName = d["planeDisplayName"] | cfg.planeDisplayName; cfg.planeLat = d["planeLat"] | cfg.planeLat; cfg.planeLon = d["planeLon"] | cfg.planeLon; cfg.planeRadiusKm = constrain((int)(d["planeRadiusKm"] | cfg.planeRadiusKm), 1, 250); cfg.planePoll = constrain((int)(d["planePoll"] | cfg.planePoll), (int)RADAR_MIN_POLL_SEC, 300); cfg.planeShowAge = d["planeShowAge"] | cfg.planeShowAge; cfg.planeShowPlace = d["planeShowPlace"] | cfg.planeShowPlace; cfg.planePlaceToggleSec = (d["planePlaceToggleSec"] | cfg.planePlaceToggleSec)==0 ? 0 : constrain((int)(d["planePlaceToggleSec"] | cfg.planePlaceToggleSec), 5, 30);
  cfg.planeOperator = constrain((int)(d["planeOperator"] | cfg.planeOperator), 0, 2); cfg.planeSpeed = constrain((int)(d["planeSpeed"] | cfg.planeSpeed), 0, 2); cfg.planeTrend = d["planeTrend"] | cfg.planeTrend; cfg.planeMarkMilitary = d["planeMarkMilitary"] | cfg.planeMarkMilitary; cfg.planeHide = ((uint8_t)(d["planeHide"] | cfg.planeHide)) & RADAR_HIDE_ALL;
  cfg.trackedFlightNo = d["trackedFlightNo"] | cfg.trackedFlightNo; cfg.aviationstackKey = d["aviationstackKey"] | cfg.aviationstackKey; cfg.adsbLolKey = d["adsbLolKey"] | cfg.adsbLolKey; cfg.trackerStatusPollMin = constrain((int)(d["trackerStatusPollMin"] | cfg.trackerStatusPollMin), 5, 120);
  JsonArrayConst cds = d["countdowns"].as<JsonArrayConst>();
  if (!cds.isNull()) for (uint8_t i=0;i<4 && i<cds.size();i++) {
    JsonObjectConst c=cds[i].as<JsonObjectConst>(); CountdownSlot& x=cfg.countdown[i];
    x.title = c["title"] | x.title; String tc=c["titleColor"] | x.titleColor; if(validHexColor(tc))x.titleColor=tc; String cc=c["countColor"] | x.countColor; if(validHexColor(cc))x.countColor=cc;
    x.type=constrain((int)(c["type"]|x.type),0,2); x.targetDate=c["targetDate"]|x.targetDate; x.durationSec=c["durationSec"]|x.durationSec; x.targetEpoch=c["targetEpoch"]|x.targetEpoch; x.active=c["active"]|x.active; x.dayDigits=constrain((int)(c["dayDigits"]|x.dayDigits),3,6);
  }
  if(!modeEnabled(cfg.mode)) cfg.mode=firstEnabledMode();
}

// ---- display views -----------------------------------------------------------
static void drawFooterClockOnly() {
  String tm=timeHHMM();visibleText(31,tm,232-textW(tm,1),226,1,C_GRAY,196,224,36,12);
}

static void drawFooter(const String& label, bool withClock=true, int x=8) {
  if(viewFresh)gfx->drawFastHLine(8,218,224,C_DGRAY);
  visibleText(30,label,x,226,1,C_GRAY,8,223,186,14);
  if(withClock)drawFooterClockOnly();
}



static void drawFooterPageArrow(bool down) {
  const int cx=120, cy=230;
  uint16_t c=C_GRAY;
  if(down){gfx->drawLine(cx,cy-4,cx,cy+3,c);gfx->drawLine(cx,cy+3,cx-4,cy-1,c);gfx->drawLine(cx,cy+3,cx+4,cy-1,c);}
  else{gfx->drawLine(cx,cy+4,cx,cy-3,c);gfx->drawLine(cx,cy-3,cx-4,cy+1,c);gfx->drawLine(cx,cy-3,cx+4,cy+1,c);}
}

static String displayAscii(String s) {
  // Name kept for compatibility with the v0.4 rendering code. UTF-8 European
  // glyphs are rendered natively by printDisplayTextAt()/centerText().
  // Town names are no longer stripped here; see departureText().
  return s;
}

// Town of the configured stop. The VVO search delivers it separately
// (stored as stopPlace when a result is picked). Stop names themselves carry
// no town ("S-Bahnhof Reick", "Postplatz"), so their first word is never
// taken as one. Without a stored place, a "Town, Stop" name still works;
// otherwise Dresden, the VVO home town, is assumed.
static String stopTown() {
  if(cfg.stopPlace.length())return cfg.stopPlace;
  int comma=cfg.stopName.indexOf(',');
  return comma>0?cfg.stopName.substring(0,comma):String("Dresden");
}
static String departureText(const char* raw,bool abbreviate) {
  char out[96];String town=stopTown();
  formatDestination(raw,town.c_str(),cfg.departureDropTown,abbreviate,out,sizeof(out));
  return String(out);
}


static uint16_t color565(const String& hex) {
  if (!validHexColor(hex)) return C_WHITE;
  uint32_t rgb = strtoul(hex.substring(1).c_str(), nullptr, 16);
  uint8_t r=(rgb>>16)&0xFF, g=(rgb>>8)&0xFF, b=rgb&0xFF;
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static int64_t daysFromCivilDate(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

static bool parseDateOnly(const String& iso, int& y, int& m, int& d) {
  if (iso.length() != 10 || iso[4] != '-' || iso[7] != '-') return false;
  y=iso.substring(0,4).toInt(); m=iso.substring(5,7).toInt(); d=iso.substring(8,10).toInt();
  return y>=1970 && m>=1 && m<=12 && d>=1 && d<=31;
}

static bool countdownDayDifference(const String& target, int32_t& outDays) {
  time_t now=time(nullptr); if(now<1609459200) return false;
  struct tm t; localtime_r(&now,&t);
  int y,m,d; if(!parseDateOnly(target,y,m,d)) return false;
  int64_t today=daysFromCivilDate(t.tm_year+1900,(unsigned)(t.tm_mon+1),(unsigned)t.tm_mday);
  int64_t goal=daysFromCivilDate(y,(unsigned)m,(unsigned)d);
  int64_t diff=goal-today;
  if(diff>2147483647LL || diff<-2147483647LL) return false;
  outDays=(int32_t)diff; return true;
}

static uint8_t decimalDigits(uint32_t v) {
  uint8_t n=1; while(v>=10){v/=10;n++;} return n;
}

static String padUnsigned(uint32_t v, uint8_t digits) {
  String s=String(v); while(s.length()<digits)s="0"+s; return s;
}

static uint8_t segMask(uint8_t d) {
  static const uint8_t masks[10]={
    0b00111111, // 0: a b c d e f
    0b00000110, // 1: b c
    0b01011011, // 2: a b d e g
    0b01001111, // 3: a b c d g
    0b01100110, // 4: b c f g
    0b01101101, // 5: a c d f g
    0b01111101, // 6: a c d e f g
    0b00000111, // 7: a b c
    0b01111111, // 8
    0b01101111  // 9
  };
  return d<10?masks[d]:0;
}

static void drawSevenDigit(int x,int y,int w,int h,int t,uint8_t d,uint16_t color) {
  uint8_t m=segMask(d); int half=h/2;
  if(m&0x01)gfx->fillRect(x+t,y,w-2*t,t,color);
  if(m&0x02)gfx->fillRect(x+w-t,y+t,t,half-t,color);
  if(m&0x04)gfx->fillRect(x+w-t,y+half,t,half-t,color);
  if(m&0x08)gfx->fillRect(x+t,y+h-t,w-2*t,t,color);
  if(m&0x10)gfx->fillRect(x,y+half,t,half-t,color);
  if(m&0x20)gfx->fillRect(x,y+t,t,half-t,color);
  if(m&0x40)gfx->fillRect(x+t,y+half-t/2,w-2*t,t,color);
}

static int sevenUnit(char c) {
  if(c>='0'&&c<='9')return 100;
  if(c==':')return 34;
  if(c=='+'||c=='-'||c==' ')return 62;
  return 45;
}

static int sevenCharWidth(char c,int digitW) {
  return max(4,(digitW*sevenUnit(c))/100);
}

static void drawSevenString(const String& value,int centerY,uint16_t color,bool partial=false,const String& old="") {
  const int gap=3;
  int units=0; for(size_t i=0;i<value.length();i++)units+=sevenUnit(value[i]);
  int gaps=value.length()>0?(int)(value.length()-1)*gap:0;
  int digitW=(224-gaps)*100/max(100,units);
  digitW=constrain(digitW,18,42);
  int h=(digitW*19)/10;
  int total=gaps; for(size_t i=0;i<value.length();i++)total+=sevenCharWidth(value[i],digitW);
  int x=(TFT_W-total)/2, y=centerY-h/2, t=max(3,digitW/6);
  for(size_t i=0;i<value.length();i++){
    char c=value[i]; int w=sevenCharWidth(c,digitW);
    if(partial&&i<old.length()&&old[i]==c){x+=w+gap;continue;}
    if(partial)gfx->fillRect(x,y,w,h,C_BLACK);
    if(c>='0'&&c<='9') drawSevenDigit(x,y,digitW,h,t,(uint8_t)(c-'0'),color);
    else if(c==':') { int s=max(3,t-1); gfx->fillRect(x+(w-s)/2,y+h/3-s/2,s,s,color); gfx->fillRect(x+(w-s)/2,y+(2*h)/3-s/2,s,s,color); }
    else if(c=='-'||c=='+') { int cy=y+h/2; gfx->fillRect(x,cy-t/2,w,t,color); if(c=='+')gfx->fillRect(x+w/2-t/2,cy-w/2,t,w,color); }
    // Space deliberately draws nothing but keeps the sign column width stable.
    x+=w+gap;
  }
}

static int8_t countdownSlotFromMode(uint8_t mode) {
  if(mode<MODE_COUNTDOWN1||mode>MODE_COUNTDOWN4)return -1;
  return (int8_t)(mode-MODE_COUNTDOWN1);
}

static String countdownValue(const CountdownSlot& c) {
  if(!c.active)return "---";
  if(c.type==COUNTDOWN_DAYS){
    int32_t days=0;if(!countdownDayDifference(c.targetDate,days))return "---";
    uint32_t a=days<0?(uint32_t)(-(int64_t)days):(uint32_t)days;
    String sign=days>0?"-":(days<0?"+":" ");
    return sign+padUnsigned(a,c.dayDigits);
  }
  time_t now=time(nullptr);if(now<1609459200||!c.targetEpoch)return "---";
  int64_t diff=(int64_t)c.targetEpoch-(int64_t)now;
  uint64_t a=diff<0?(uint64_t)(-diff):(uint64_t)diff;
  String sign=diff>0?"-":(diff<0?"+":" ");
  if(c.type==COUNTDOWN_MS){
    uint32_t mins=(uint32_t)(a/60ULL), sec=(uint32_t)(a%60ULL);
    return sign+padUnsigned(mins,2)+":"+padUnsigned(sec,2);
  }
  uint32_t hours=(uint32_t)(a/3600ULL), mins=(uint32_t)((a%3600ULL)/60ULL), sec=(uint32_t)(a%60ULL);
  return sign+padUnsigned(hours,2)+":"+padUnsigned(mins,2)+":"+padUnsigned(sec,2);
}

static String flightStatusGerman(const char* s) {
  if(!s||!*s)return "N/A";
  if(!strcmp(s,"active"))return TR("IN DER LUFT","IN THE AIR");
  if(!strcmp(s,"landed"))return TR("GELANDET","LANDED");
  if(!strcmp(s,"scheduled"))return TR("GEPLANT","SCHEDULED");
  if(!strcmp(s,"cancelled"))return TR("ANNULLIERT","CANCELLED");
  if(!strcmp(s,"diverted"))return TR("UMGELEITET","DIVERTED");
  if(!strcmp(s,"incident"))return TR("STÖRUNG","INCIDENT");
  return String(s);
}

static String trackedFlightDuration(const TrackedFlightSnapshot& f) {
  int64_t start=f.depActualEpoch?f.depActualEpoch:f.depScheduledEpoch;
  if(!start)return "N/A";
  int64_t end=0;
  if(f.arrActualEpoch)end=f.arrActualEpoch;
  else if(f.depActualEpoch){time_t now=time(nullptr);if(now>1609459200)end=now;}
  else end=f.arrScheduledEpoch;
  if(!end||end<start)return "N/A";
  uint64_t sec=(uint64_t)(end-start);uint32_t h=sec/3600ULL,m=(sec%3600ULL)/60ULL;
  return padUnsigned(h,2)+":"+padUnsigned(m,2);
}

static void drawDepartures(bool force=true) {
  static uint8_t lastVisible=255,lastPage=255;static bool lastError=false,lastUnserved=false;
  uint8_t count=min(vvoCount(),cfg.departureCount);
  bool pagedNow=cfg.departureCount==10&&count>5;
  uint8_t startNow=pagedNow&&departurePage?5:0;
  uint8_t shown=count>startNow?min((uint8_t)5,(uint8_t)(count-startNow)):0;
  force=force||shown!=lastVisible||(pagedNow&&lastPage!=departurePage)||(!shown&&(lastError!=vvoError()||lastUnserved!=vvoNotServed()));
  beginView(force);lastVisible=shown;lastPage=departurePage;lastError=vvoError();lastUnserved=vvoNotServed();
  String title = crop(cfg.stopName.length() ? departureText(cfg.stopName.c_str(),false) : String(TR("HALTESTELLE","STOP")), 18);
  visibleText(0,title,8,7,2,C_YELLOW,0,0,240,29);
  if(force)gfx->drawFastHLine(8,29,224,C_DGRAY);

  if (!cfg.stopId.length()) {
    if(force)centerText(TR("HALTESTELLE","STOP"), 78, 2, C_WHITE);
    if(force)centerText(TR("IM WEBINTERFACE","IN THE WEB"), 108, 2, C_GRAY);
    if(force)centerText(TR("AUSWÄHLEN","INTERFACE"), 136, 2, C_GRAY);
    drawFooter(TR("VVO NICHT KONFIGURIERT","VVO NOT CONFIGURED"));
    return;
  }

  uint8_t n = vvoCount();
  if (!n) {
    if (vvoNotServed()) {
      if(force){centerText(TR("Haltestelle wird","Stop is"),106,2,C_GRAY);centerText(TR("nicht bedient","not served"),128,2,C_GRAY);}
      drawFooter("VVO");
    } else if (vvoError()) {
      if(force)centerText("VVO", 82, 4, C_RED);
      if(force)centerText(TR("nicht erreichbar","not reachable"), 128, 2, C_GRAY);
      drawFooter(TR("VVO FEHLER","VVO ERROR"));
    } else {
      if(force)centerText("VVO", 82, 4, C_YELLOW);
      if(force)centerText(TR("lade Abfahrten...","loading departures..."), 128, 2, C_GRAY);
      drawFooter("LIVE");
    }
    return;
  }

  if (n > cfg.departureCount) n = cfg.departureCount;
  bool paged = cfg.departureCount == 10 && n > 5;
  uint8_t start = paged && departurePage ? 5 : 0;
  uint8_t visible = n > start ? min((uint8_t)5, (uint8_t)(n - start)) : 0;
  int y0 = 41, step = paged ? 34 : (visible >= 5 ? 34 : (visible == 4 ? 42 : 52));
  for (uint8_t row = 0; row < visible; row++) {
    const VvoDeparture& d = vvoAt(start + row);
    int y = y0 + row * step;
    String line = displayAscii(String(d.line));
    String rawDest = departureText(d.direction,cfg.departureAbbreviate);
    String right;
    uint16_t rightColor = C_WHITE;
    if (d.cancelled) { right = "X"; rightColor = C_RED; }
    else if (d.minutes < 0) { right = "?"; rightColor = C_GRAY; }
    else if (d.minutes <= 0) { right = TR("JETZT","NOW"); rightColor = C_GREEN; }
    else { right = String(d.minutes); rightColor = d.delayed ? C_YELLOW : C_WHITE; }

    uint16_t lc = glyphCount(line);
    uint8_t lineSize = lc <= 2 ? 3 : (lc <= 4 ? 2 : 1);
    int lineY = y + (lineSize == 3 ? 0 : (lineSize == 2 ? 6 : 12));
    visibleText(1+row*3,line,8,lineY,lineSize,C_YELLOW,8,y,50,step);

    uint8_t rs = (d.minutes == 0 && !d.cancelled) ? 1 : 2;
    int rightX = 232 - textW(right, rs);
    uint8_t destSize = 2;
    String dest = crop(rawDest, 9);
    if (cfg.departureCompactDestinations && glyphCount(rawDest) > 9) {
      destSize = 1;
      int maxChars = max(2, (rightX - 70) / 6);
      dest = crop(rawDest, (uint8_t)min(maxChars, 31));
    }
    bool destChanged=visibleText(2+row*3,dest,62,y+(destSize==2?6:12),destSize,C_WHITE,62,y,rightX-64,step);
    visibleText(3+row*3,right,rightX,y+(rs==1?10:6),rs,rightColor,rightX,y,232-rightX,step,destChanged);
  }
  drawFooter(vvoError() ? TR("LIVE / LETZTER STAND","LIVE / LAST DATA") : "VVO LIVE");
  if(paged&&force)drawFooterPageArrow(!departurePage);
}

// Weather icons are drawn for a 1.0 scale around (120,55); drawWeatherIconAt()
// places and scales the same motif, e.g. for the small forecast icons.
static int wxLen(int v, float s) { int r=(int)lroundf(v*s); return r<1?1:r; }

static void drawCloud(int cx, int cy, uint16_t color, float s=1.0f) {
  gfx->fillCircle(cx-wxLen(18,s), cy+wxLen(4,s), wxLen(11,s), color);
  gfx->fillCircle(cx, cy-wxLen(5,s), wxLen(17,s), color);
  gfx->fillCircle(cx+wxLen(20,s), cy+wxLen(4,s), wxLen(12,s), color);
  gfx->fillRect(cx-wxLen(27,s), cy+wxLen(4,s), wxLen(56,s), wxLen(15,s), color);
}

static void drawSun(int cx, int cy, uint16_t color, int reach=30) {
  gfx->fillCircle(cx, cy, reach/2, color);
  for (int a=0; a<360; a+=45) {
    float r=a*(float)PI/180.0f;
    int x1=cx+(int)((reach*0.73f)*cosf(r)), y1=cy+(int)((reach*0.73f)*sinf(r));
    int x2=cx+(int)(reach*cosf(r)), y2=cy+(int)(reach*sinf(r));
    gfx->drawLine(x1,y1,x2,y2,color);
  }
}

static void drawMoon(int cx,int cy,int radius=18) {
  gfx->fillCircle(cx,cy,radius,C_WHITE);gfx->fillCircle(cx+radius/2,cy-radius*2/5,radius,C_BLACK);
}

static void drawSnowflake(int x,int y,int radius,uint16_t color) {
  gfx->drawLine(x-radius,y,x+radius,y,color);
  gfx->drawLine(x,y-radius,x,y+radius,color);
  gfx->drawLine(x-radius+1,y-radius+1,x+radius-1,y+radius-1,color);
  gfx->drawLine(x-radius+1,y+radius-1,x+radius-1,y-radius+1,color);
}

static void drawWeatherIconAt(int code, bool day, int ox, int oy, float s) {
  auto X=[&](int x){return ox+(int)lroundf((x-120)*s);};
  auto Y=[&](int y){return oy+(int)lroundf((y-55)*s);};
  auto L=[&](int v){return wxLen(v,s);};
  WeatherVisual w=weatherVisual(code);
  if(w.motif==WX_CLEAR){if(day)drawSun(X(120),Y(55),C_YELLOW,L(30));else drawMoon(X(120),Y(55),L(18));return;}
  if(w.motif==WX_PARTLY){if(day)drawSun(X(103),Y(47),C_YELLOW,L(20));else drawMoon(X(103),Y(47),L(18));drawCloud(X(125),Y(58),C_WHITE,s);return;}
  if(w.motif==WX_FOG){
    for(int row=0;row<4;row++)gfx->fillRect(X(row%2?95:87),Y(37+row*12),L(row%2?60:76),L(4),C_GRAY);
    if(code==48)drawSnowflake(X(164),Y(71),L(4),C_CYAN);
    return;
  }
  drawCloud(X(120),Y(w.motif==WX_CLOUD?55:44),C_WHITE,s);
  if(!w.intensity)return;
  int count=w.intensity==1?2:(w.intensity==2?4:6);
  bool drizzle=w.motif==WX_DRIZZLE||w.motif==WX_FREEZING_DRIZZLE;
  bool snow=w.motif==WX_SNOW||w.motif==WX_SNOW_SHOWER;
  bool grains=w.motif==WX_GRAINS||w.motif==WX_THUNDER_HAIL;
  for(int i=0;i<count;i++){
    int x=91+i*58/(count-1),y=72+(i%2)*4;
    if(snow)drawSnowflake(X(x),Y(y+4),L(3),C_CYAN);
    else if(grains)gfx->fillCircle(X(x),Y(y+4),L(w.motif==WX_GRAINS?1:2),C_CYAN);
    else gfx->drawLine(X(x),Y(y),X(x-(drizzle?2:5)),Y(y+(drizzle?4:10)),C_CYAN);
  }
  if(w.motif==WX_FREEZING_DRIZZLE||w.motif==WX_FREEZING_RAIN)drawSnowflake(X(166),Y(78),L(5),C_CYAN);
  if(w.motif==WX_RAIN_SHOWER||w.motif==WX_SNOW_SHOWER){if(day)drawSun(X(91),Y(40),C_YELLOW,L(16));else drawMoon(X(91),Y(40),L(12));drawCloud(X(120),Y(44),C_WHITE,s);}
  if(w.motif==WX_THUNDER||w.motif==WX_THUNDER_HAIL){gfx->drawLine(X(124),Y(59),X(113),Y(78),C_YELLOW);gfx->drawLine(X(113),Y(78),X(125),Y(75),C_YELLOW);gfx->drawLine(X(125),Y(75),X(116),Y(86),C_YELLOW);}
}

static void drawWeatherIcon(int code, bool day) { drawWeatherIconAt(code, day, 120, 55, 1.0f); }

static uint16_t weatherTempColor565(float c);
static void drawSunEventIcon(int x,int y,bool rise);
static float displayTempValue(float c);
static String weatherUnit();

static void centerTextIn(const String& s, int cx, int y, uint8_t size, uint16_t color) { printDisplayTextAt(cx-textW(s,size)/2,y,s,size,color); }

// Three columns: weekday, small icon, high, low, chance of rain.
static void drawForecast(bool force) {
  const WeatherSnapshot& w=weatherData();
  static String shown;
  String sig=String(cfg.lang)+String(cfg.weatherFahrenheit)+String(w.days);
  for(uint8_t i=0;i<w.days;i++)sig+=String(w.day[i].code)+"/"+String((int)lroundf(w.day[i].highC))+"/"+String((int)lroundf(w.day[i].lowC))+"/"+String(w.day[i].rainPct)+";";
  if(!force&&sig==shown){drawFooter("OPEN-METEO LIVE");return;}
  shown=sig;beginView(true);
  String title=cfg.weatherDisplayName.length()?cfg.weatherDisplayName:(cfg.weatherCity.length()?cfg.weatherCity:String(TR("WETTER","WEATHER")));
  visibleText(0,crop(displayAscii(title),12),8,7,2,C_YELLOW,0,0,160,22);
  String head=TR("3 TAGE","3 DAYS");printDisplayTextAt(232-textW(head,1),11,head,1,C_GRAY);
  static const char* wdDe[]={"SO","MO","DI","MI","DO","FR","SA"};
  static const char* wdEn[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
  time_t now=time(nullptr);struct tm t;localtime_r(&now,&t);
  for(uint8_t i=0;i<3;i++){
    int cx=40+i*80;
    if(i)gfx->drawFastVLine(i*80,34,170,C_DGRAY);
    if(i>=w.days)continue;
    const WeatherSnapshot::Day& d=w.day[i];
    String name=now>1609459200?String(cfg.lang==LANG_EN?wdEn[(t.tm_wday+1+i)%7]:wdDe[(t.tm_wday+1+i)%7]):String("+")+String(i+1);
    centerTextIn(name,cx,38,2,C_WHITE);
    drawWeatherIconAt(d.code,true,cx,92,0.52f);
    centerTextIn(String((int)lroundf(displayTempValue(d.highC)))+"°",cx,124,3,C_WHITE);
    centerTextIn(String((int)lroundf(displayTempValue(d.lowC)))+"°",cx,158,2,C_GRAY);
    if(d.rainPct>=0)centerTextIn(String(d.rainPct)+"%",cx,186,2,C_CYAN);
  }
  drawFooter("OPEN-METEO LIVE");
}

static void drawWeather(bool force=true) {
  const WeatherSnapshot& w=weatherData();static bool valid=false,error=false;
  static uint8_t lastPage=0;
  if(weatherPage==1&&w.valid&&w.days){bool f=force||lastPage!=1;lastPage=1;if(f)valid=false;drawForecast(f);return;}
  if(lastPage!=0){lastPage=0;force=true;}
  force=force||valid!=w.valid||(!w.valid&&error!=w.error);beginView(force);valid=w.valid;error=w.error;
  String title=cfg.weatherDisplayName.length()?cfg.weatherDisplayName:(cfg.weatherCity.length()?cfg.weatherCity:String(TR("WETTER","WEATHER")));
  visibleText(0,crop(displayAscii(title),18),8,7,2,C_YELLOW,0,0,240,22);
  if(!w.valid){if(force){centerText(w.error?TR("WETTER FEHLER","WEATHER ERROR"):TR("WETTER LÄDT","WEATHER LOADING"),88,2,w.error?C_RED:C_YELLOW);centerText(w.error?TR("Open-Meteo nicht erreichbar","Open-Meteo not reachable"):TR("Daten werden geladen...","Loading data..."),124,1,C_GRAY);}drawFooter("OPEN-METEO");return;}
  String icon=String(w.code)+(w.isDay?"D":"N");
  if(force||visibleFields[1].text!=icon){if(!force)gfx->fillRect(8,22,224,66,C_BLACK);drawWeatherIcon(w.code,w.isDay);visibleFields[1].text=icon;}
  String temp=String((int)lroundf(displayTempValue(w.currentC)));
  uint16_t color=cfg.weatherTempColor?weatherTempColor565(w.currentC):C_WHITE;
  // Number and unit as one centred group, so "5°C" has no gap.
  const String unit=weatherUnit();const int tempW=textW(temp,6),tempX=(TFT_W-(tempW+4+textW(unit,2)))/2;
  bool tempChanged=visibleText(2,temp,tempX,88,6,color,8,88,224,48);
  visibleText(3,unit,tempX+tempW+4,103,2,cfg.weatherTempColor?color:C_GRAY,tempX+tempW+4,103,textW(unit,2),16,tempChanged);
  String label=cfg.lang==LANG_EN?String(FPSTR(weatherLabelEn(w.code))):String(FPSTR(weatherLabel(w.code)));visibleCenter(4,label,142,textW(label,2)<=224?2:1,C_WHITE,140,19);
  String hi="H "+String((int)lroundf(displayTempValue(w.highC)))+weatherUnit();
  String lo=TR("T ","L ")+String((int)lroundf(displayTempValue(w.lowC)))+weatherUnit();
  visibleText(5,hi,18,169,2,C_GRAY,8,166,112,24);
  visibleText(6,lo,222-textW(lo,2),169,2,C_GRAY,120,166,112,24);
  String rain=w.rainPct>=0?String(TR("REGEN ","RAIN "))+String(w.rainPct)+"%":String(TR("REGEN --","RAIN --"));
  visibleText(7,rain,18,197,1,C_CYAN,8,194,106,16);
  if(force){drawSunEventIcon(118,196,true);drawSunEventIcon(174,196,false);}
  visibleText(8,String(w.sunrise),133,197,1,C_GRAY,133,194,32,16);
  visibleText(9,String(w.sunset),189,197,1,C_GRAY,189,194,36,16);
  drawFooter(w.error?TR("LIVE / LETZTER STAND","LIVE / LAST DATA"):"OPEN-METEO LIVE");
}

static uint16_t weatherTempColor565(float c) {
  if(c <= -10.0f) return 0x001F; // deep blue
  if(c <=   0.0f) return C_BLUE;
  if(c <=  10.0f) return C_CYAN;
  if(c <=  17.0f) return C_WHITE;
  if(c <=  23.0f) return C_GREEN;
  if(c <=  28.0f) return C_YELLOW;
  if(c <=  34.0f) return 0xFD20; // orange
  return C_RED;
}

static void drawSunEventIcon(int x,int y,bool rise) {
  // 13x9 tiny pixel-style sunrise/sunset mark. No emoji/font dependency.
  uint16_t c=C_GRAY;
  gfx->drawFastHLine(x,y+7,13,c);
  gfx->drawPixel(x+6,y+1,c);
  gfx->drawPixel(x+3,y+3,c); gfx->drawPixel(x+9,y+3,c);
  gfx->drawLine(x+4,y+6,x+4,y+5,c); gfx->drawLine(x+8,y+6,x+8,y+5,c);
  gfx->drawFastHLine(x+5,y+4,3,c);
  gfx->drawLine(x+6,y+8,x+6,y+8,c);
  if(rise){gfx->drawLine(x+6,y+8,x+4,y+6,c);gfx->drawLine(x+6,y+8,x+8,y+6,c);}
  else{gfx->drawLine(x+6,y+6,x+4,y+8,c);gfx->drawLine(x+6,y+6,x+8,y+8,c);}
}

static float displayTempValue(float c) { return cfg.weatherFahrenheit ? (c * 9.0f / 5.0f + 32.0f) : c; }
static String weatherUnit() { return cfg.weatherFahrenheit ? String("°F") : String("°C"); }



static String planePlaceTitle() {
  String s=cfg.planeDisplayName.length()?cfg.planeDisplayName:cfg.planePlace;
  return crop(displayAscii(s),18);
}

static void drawPlaneHeader() {
  gfx->fillRect(0,0,TFT_W,30,C_BLACK);
  String title=(cfg.planeShowPlace&&(cfg.planePlaceToggleSec==0||planePlacePhase)&&planePlaceTitle().length())?planePlaceTitle():String(TR("IN DER NÄHE","NEARBY"));
  printDisplayTextAt(8,7,title,2,C_YELLOW);
  gfx->drawFastHLine(8,29,224,C_DGRAY);
}

static String routePart(const FlightRoute& route, uint8_t begin, uint8_t end) {
  String out;
  for(uint8_t i=begin;i<end;i++){if(i>begin)out+=" > ";out+=route.airports[i];}
  if(end==route.count && route.truncated)out+=" > ...";
  return out;
}

static void drawFlightRoute(const FlightRoute& route, const char* origin, const char* destination, int y, uint16_t color=C_YELLOW) {
  if(route.count<=2){
    // The arrow stays in the middle: codes are padded to 4 characters, so
    // 3-letter routes look exactly as before and 4-character codes grow outward.
    String a=String(origin[0]?origin:"N/A"),b=String(destination[0]?destination:"N/A");
    while(a.length()<4)a=" "+a;
    while(b.length()<4)b+=" ";
    centerText(a+" > "+b,y,3,color);
    return;
  }
  String text=routePart(route,0,route.count);
  if(textW(text,2)<=224)centerText(text,y,2,color);
  else if(textW(text,1)<=224)centerText(text,y,1,color);
  else { // Long chains fit in the existing route area as two compact rows.
    uint8_t split=route.count/2;
    centerText(routePart(route,0,split)+" >",y,1,color);
    centerText(routePart(route,split,route.count),y+10,1,color);
  }
}

static FlightSnapshot lastPlaneRendered = {};
static bool planeRendered = false;

static bool samePlane(const FlightSnapshot& a,const FlightSnapshot& b) {
  if(a.hex[0]&&b.hex[0])return !strcmp(a.hex,b.hex);
  if(a.registration[0]&&b.registration[0])return !strcmp(a.registration,b.registration);
  return a.callsign[0]&&b.callsign[0]&&!strcmp(a.callsign,b.callsign);
}

// Distance and altitude are continued between requests (RadarMotion.h).
static String planeDistance(const FlightSnapshot& f) { return String(flightLiveDistanceKm(f),1)+" km"; }
static String planeAltitude(const FlightSnapshot& f) { return f.onGround?String("GND"):(f.altitudeKnown?String((int)lroundf(flightLiveAltitudeFt(f)*0.3048f))+" m":String("N/A")); }

static String compactType(const FlightSnapshot& f) {
  char stable[19];
  if(aircraftDisplayLabel(f.typeCode,stable,sizeof(stable))&&textW(stable,2)<=224)return String(stable);
  String full=f.typeName[0]?String(f.typeName):String(f.typeCode);
  if(!full.length())return "N/A";
  if(textW(full,2)<=224)return full;
  char shortName[24];
  if(aircraftMakerModel(full.c_str(),shortName,sizeof(shortName))&&textW(shortName,2)<=224)return String(shortName);
  return f.typeCode[0]?String(f.typeCode):crop(full,18);
}
// Reception age follows the current status. Only this number/unit field
// changes each second; the status label and clock keep their own areas.
static bool lastPlanePauseRendered=false,lastPlaneAgeRendered=true;
static String lastPlaneDist,lastPlaneAlt;static int32_t lastPlaneDot=-1;
static String lastPlaneCall,lastPlaneOperator,lastPlaneRoute,lastPlaneReg;static int8_t lastPlaneTrend=0;
// The footer starts with a status dot; label and age follow after it.
static const int PLANE_FOOTER_X=18;
// Name of the position source in the footer (prepared for further sources).
static const char* planeSourceLabel="ADSB.LOL";
static String planeFooterLabel() {
  return flightProviderPaused()?String(TR("ABRUFPAUSE","PAUSED")):(flightData().error?String(TR("LETZTER STAND","LAST DATA")):String(planeSourceLabel)+F(" LIVE"));
}
static void drawPlaneAge() {
  if(!cfg.planeShowAge||!flightData().valid)return;
  const int x=PLANE_FOOTER_X+textW(String(planeFooterLabel())+" ",1);
  String seconds=String((uint32_t)(millis()-flightLastOkMs())/1000UL)+"s";
  VisibleField& field=visibleFields[28];
  if(field.valid&&field.text==seconds&&field.tx==x)return;
  gfx->fillRect(x,223,48,14,C_BLACK);
  printDisplayTextAt(x,226,seconds,1,C_GRAY);
  field.text=seconds;field.tx=x;field.valid=true;
}
// Data state dot: white = just received, green = on schedule, yellow = update
// overdue or failed, grey = frozen (no estimate any more).
static void drawPlaneDot() {
  if(!flightData().valid)return;
  RadarDot state=radarDotState((uint32_t)(millis()-flightLastOkMs()),flightPollIntervalMs(),flightData().error||flightProviderPaused());
  const uint16_t colors[]={C_WHITE,C_GREEN,C_YELLOW,C_GRAY};
  int32_t c=colors[state];
  if(c==lastPlaneDot)return;
  gfx->fillCircle(11,229,3,(uint16_t)c);lastPlaneDot=c;
}
static void drawPlaneFooter() {
  drawFooter(planeFooterLabel(),true,PLANE_FOOTER_X);
  lastPlaneDot=-1;drawPlaneDot();
  drawPlaneAge();
}
// Second half of a 6 s cycle: squawk instead of route, speed instead of registration.
static bool planeSecondPhase() { return (millis()/3000UL)%2UL==1UL; }
// Operator line: name of the callsign (A), or for known military callsigns the branch (B) or branch / unit (C).
static String planeOperatorText(const FlightSnapshot& f) {
  char mil[24];
  if(cfg.planeOperator&&militaryOperatorText(f.callsign,cfg.planeOperator,cfg.lang==LANG_EN,mil,sizeof(mil)))return String(mil);
  return f.airline[0]?String(f.airline):String("N/A");
}
static String planeSpeedText(const FlightSnapshot& f) {
  return cfg.planeSpeed==2?String((int)lroundf(f.speedKt))+F(" kt"):String((int)lroundf(f.speedKt*1.852f))+F(" km/h");
}
static void drawPlane(bool force=true) {
  const FlightSnapshot& f=flightData();
  const bool paused=f.valid&&flightProviderPaused();
  bool full=force||!planeRendered||f.valid!=lastPlaneRendered.valid||
    (f.valid&&!samePlane(f,lastPlaneRendered))||(!f.valid&&f.error!=lastPlaneRendered.error);
  beginView(full);if(full)drawPlaneHeader();
  if(!f.valid){
    if(full){
      if(f.error&&adsbLolAccessDenied(flightLastHttp())){centerText("ADSB.LOL",80,3,C_RED);centerText(TR("Zugriff abgelehnt","Access denied"),118,2,C_WHITE);centerText(String("HTTP ")+String(flightLastHttp())+(flightLastHttp()==426?"":TR(" - Schlüssel prüfen"," - check key")),146,1,C_GRAY);centerText(TR("längere Pause vor neuem Versuch","pausing before next try"),162,1,C_GRAY);}
      else if(f.error){centerText(TR("FLUGRADAR","FLIGHT RADAR"),80,3,C_RED);centerText(TR("keine aktuellen Daten","no current data"),125,1,C_GRAY);}
      else{centerText(TR("KEIN FLUGZEUG","NO AIRCRAFT"),80,2,C_WHITE);centerText(TR("im gewählten Radius","in selected radius"),116,1,C_GRAY);}
      drawFooter(f.error?TR("ABRUF FEHLGESCHLAGEN","REQUEST FAILED"):"ADSB.LOL");
    }
    lastPlaneRendered=f;planeRendered=true;return;
  }
  // Emergency squawks turn the callsign red and alternate with the route
  // (3 s each); speed alternates with the registration in the same rhythm.
  const bool emergency=radarEmergencySquawk(f.squawk);
  const bool second=planeSecondPhase();
  String callKey=String(f.callsign)+(emergency?"!":"")+(cfg.planeMarkMilitary&&f.military?"M":"");
  if(full||callKey!=lastPlaneCall){
    if(!full)gfx->fillRect(8,35,224,40,C_BLACK);
    String cs=f.callsign[0]?String(f.callsign):String("N/A");centerText(cs,42,cs.length()<=6?4:3,emergency?C_RED:C_WHITE);
    if(cfg.planeMarkMilitary&&f.military)printDisplayTextAt(214,38,String(F("MIL")),1,C_KHAKI);
  }
  lastPlaneCall=callKey;
  // Long names: shortened, else two parts in the 3 s rhythm, else small type (AirlineName.h).
  String op=planeOperatorText(f);
  char opA[40],opB[40];uint8_t opParts=airlineParts(op.c_str(),18,opA,opB,sizeof(opA));
  String opShown=opParts==1?String(opA):(opParts==2?String(second?opB:opA):crop(op,36));
  if(opParts==0)opShown+="\x01"; // small type: different cache key
  if(full||opShown!=lastPlaneOperator){
    if(!full)gfx->fillRect(8,78,224,20,C_BLACK);
    if(opParts)centerText(opShown,80,2,C_GRAY);else centerText(crop(op,36),84,1,C_GRAY);
  }
  lastPlaneOperator=opShown;
  const bool routeKnown=f.route.count>0||(f.origin[0]&&strcmp(f.origin,"N/A"));
  // In an emergency the whole route line is red: route and squawk alternate.
  String routeKey=emergency?((second||!routeKnown)?String(F("SQUAWK "))+f.squawk:String(F("!"))):String();
  if(full||routeKey!=lastPlaneRoute||strcmp(f.origin,lastPlaneRendered.origin)||strcmp(f.destination,lastPlaneRendered.destination)||
    f.route.count!=lastPlaneRendered.route.count||f.route.truncated!=lastPlaneRendered.route.truncated||
    memcmp(f.route.airports,lastPlaneRendered.route.airports,sizeof(f.route.airports))){
    if(!full)gfx->fillRect(8,102,224,29,C_BLACK);
    if(routeKey.length()>1)centerText(routeKey,105,3,C_RED);
    else drawFlightRoute(f.route,f.origin,f.destination,105,emergency?C_RED:C_YELLOW);
  }
  lastPlaneRoute=routeKey;
  if(full||strcmp(f.typeName,lastPlaneRendered.typeName)||strcmp(f.typeCode,lastPlaneRendered.typeCode)){
    if(!full)gfx->fillRect(8,136,224,23,C_BLACK);
    centerText(compactType(f),139,2,C_WHITE);
  }
  String reg=f.registration[0]?String(f.registration):String("N/A");
  if(cfg.planeSpeed&&f.speedKnown&&(second||!f.registration[0]))reg=planeSpeedText(f);
  if(full||reg!=lastPlaneReg){
    if(!full)gfx->fillRect(8,161,224,24,C_BLACK);
    centerText(reg,164,2,C_GRAY);
  }
  lastPlaneReg=reg;
  String dist=planeDistance(f),alt=planeAltitude(f);
  if(full||dist!=lastPlaneDist){
    if(!full)gfx->fillRect(8,190,104,24,C_BLACK);
    gfx->setTextSize(2);gfx->setTextColor(C_CYAN);gfx->setCursor(10,194);gfx->print(dist);
  }
  if(full||alt!=lastPlaneAlt){
    if(!full)gfx->fillRect(128,190,104,24,C_BLACK);
    gfx->setTextSize(2);gfx->setTextColor(C_CYAN);gfx->setCursor(230-textW(alt,2),194);gfx->print(alt);
  }
  // Climb/descent arrow between distance and altitude (optional).
  int8_t trend=cfg.planeTrend&&!f.onGround?radarTrend(f.motion.hasRate,f.motion.rateFpm):0;
  if(full||trend!=lastPlaneTrend){
    if(!full)gfx->fillRect(112,190,16,24,C_BLACK);
    if(trend>0)gfx->fillTriangle(120,195,114,206,126,206,C_CYAN);
    else if(trend<0)gfx->fillTriangle(114,196,126,196,120,207,C_CYAN);
  }
  lastPlaneTrend=trend;
  lastPlaneDist=dist;lastPlaneAlt=alt;
  if(full||f.error!=lastPlaneRendered.error||paused!=lastPlanePauseRendered||cfg.planeShowAge!=lastPlaneAgeRendered){
    if(!full)gfx->fillRect(8,223,186,14,C_BLACK);
    visibleFields[28].valid=false;visibleFields[29].valid=false;visibleFields[30].valid=false;
    drawPlaneFooter();
  }
  drawPlaneDot();drawPlaneAge();
  lastPlanePauseRendered=paused;lastPlaneAgeRendered=cfg.planeShowAge;
  lastPlaneRendered=f;planeRendered=true;
}

static void drawTrackedFlight(bool force=true) {
  const TrackedFlightSnapshot& snapshot=trackedFlightData();
  static uint8_t lastState=255;static String oldRoute;
  uint8_t state=!cfg.trackedFlightNo.length()?0:(!cfg.aviationstackKey.length()?1:(!snapshot.valid?(snapshot.error?2:(snapshot.notFound?3:4)):5));
  String routeText=String(snapshot.origin)+">"+snapshot.destination+routePart(snapshot.route,0,snapshot.route.count);
  force=force||state!=lastState||routeText!=oldRoute;beginView(force);lastState=state;oldRoute=routeText;
  if(state!=5&&!force)return;
  if(!cfg.trackedFlightNo.length()){
    centerText("N/A",72,5,C_WHITE);centerText("N/A > N/A",126,3,C_YELLOW);centerText(TR("Flugnummer im","Enter flight number"),172,1,C_GRAY);centerText(TR("Webinterface eingeben","in the web interface"),184,1,C_GRAY);drawFooter("");return;
  }
  if(!cfg.aviationstackKey.length()){
    centerText(displayAscii(cfg.trackedFlightNo),62,4,C_WHITE);centerText("N/A > N/A",116,3,C_YELLOW);centerText(TR("API-KEY FEHLT","API KEY MISSING"),166,2,C_RED);drawFooter("");return;
  }
  const TrackedFlightSnapshot& f=trackedFlightData();
  if(!f.valid){
    centerText(displayAscii(cfg.trackedFlightNo),56,4,C_WHITE);
    centerText("N/A > N/A",108,3,C_YELLOW);
    if(f.error)centerText(TR("TRACKER FEHLER","TRACKER ERROR"),158,2,C_RED);
    else if(f.notFound)centerText(TR("FLUG N/A","FLIGHT N/A"),158,2,C_GRAY);
    else centerText(TR("lade Flugdaten...","loading flight data..."),158,2,C_GRAY);
    drawFooter(f.error?TR("TRACKER FEHLER","TRACKER ERROR"):(f.notFound?TR("FLUG NICHT GEFUNDEN","FLIGHT NOT FOUND"):TR("LADE FLUGDATEN","LOADING FLIGHT DATA")));
    return;
  }
  String no=f.flightIata[0]?String(f.flightIata):cfg.trackedFlightNo;
  visibleCenter(0,no,5,no.length()<=7?3:2,C_WHITE,0,29);
  if(force)drawFlightRoute(f.route,f.origin,f.destination,36);
  visibleCenter(1,flightStatusGerman(f.status),64,1,C_GRAY,62,12);

  auto row=[](uint8_t id,const String& label,const char* value,int y,uint16_t col){
    String v=(value&&*value)?String(value):String("N/A");
    visibleText(id+10,label,8,y+5,1,C_GRAY,8,y,88,20);
    visibleText(id,v,232-textW(v,2),y,2,col,100,y,132,20);
  };
  row(2,TR("AB PLAN","DEP SCHED"),f.depScheduled,82,C_WHITE);
  row(3,TR("AB IST","DEP ACTUAL"),f.depActual,105,C_WHITE);
  row(4,TR("AN PLAN","ARR SCHED"),f.arrScheduled,128,C_WHITE);
  if(strcmp(f.arrActual,"N/A"))row(5,TR("AN IST","ARR ACTUAL"),f.arrActual,151,C_GREEN);else row(5,TR("AN ETA","ARR ETA"),f.arrEstimated,151,C_CYAN);
  String dur=trackedFlightDuration(f);row(6,TR("FLUGZEIT","FLIGHT TIME"),dur.c_str(),174,C_WHITE);
  String alt=f.altitudeM>0?String(f.altitudeM)+" m":String("N/A");
  String spd=f.speedKmh>1?String((int)lroundf(f.speedKmh))+" km/h":String("N/A");
  visibleText(7,alt,8,204,1,C_CYAN,8,200,110,14);visibleText(8,spd,232-textW(spd,1),204,1,C_CYAN,120,200,112,14);
  drawFooter(f.error ? TR("LETZTER STAND","LAST DATA") : "AVIATIONSTACK + ADSB.LOL");
}

static void drawCountdown(uint8_t slot,bool force=true) {
  if(slot>3)return;
  const CountdownSlot& c=cfg.countdown[slot];
  String title=crop(displayAscii(c.title),24),value=countdownValue(c),old=lastCountdownRendered[slot];
  static int lastSlot=-1;static String oldTitle,oldTitleColor,oldColor;static uint8_t oldType=255;
  bool layout=force||lastSlot!=slot||oldTitle!=title||oldTitleColor!=c.titleColor||oldColor!=c.countColor||oldType!=c.type||old.length()!=value.length();
  if(!layout)for(size_t i=0;i<value.length();i++)if(sevenUnit(value[i])!=sevenUnit(old[i]))layout=true;
  beginView(layout);bool hasTitle=glyphCount(title)>0;uint16_t color=color565(c.countColor);
  if(layout&&hasTitle)centerText(title,28,glyphCount(title)<=12?3:2,color565(c.titleColor));
  int cy=c.type==COUNTDOWN_DAYS?(hasTitle?127:111):(hasTitle?135:120);
  if(layout||value!=old)drawSevenString(value,cy,color,!layout,old);
  if(layout&&c.type==COUNTDOWN_DAYS)centerText(TR("TAGE","DAYS"),hasTitle?183:168,2,color);
  lastCountdownRendered[slot]=value;lastSlot=slot;oldTitle=title;oldTitleColor=c.titleColor;oldColor=c.countColor;oldType=c.type;
}

static void drawClock(bool fullRedraw=true) {
  beginView(fullRedraw);if(fullRedraw)drawFooter(crop(cfg.timeZone,36).c_str(),false);
  String t=timeHHMM(),old=visibleFields[1].text;int x=(TFT_W-textW(t,6))/2;
  if(fullRedraw||old.length()!=t.length()){if(!fullRedraw)gfx->fillRect(0,72,TFT_W,66,C_BLACK);centerText(t,84,6,C_WHITE);}
  else for(size_t i=0;i<t.length();i++)if(t[i]!=old[i]){gfx->fillRect(x+i*36,84,36,48,C_BLACK);printDisplayTextAt(x+i*36,84,t.substring(i,i+1),6,C_WHITE);}
  visibleFields[1].text=t;
  time_t now=time(nullptr);String date=TR("warte auf NTP","waiting for NTP");
  if(now>1609459200){struct tm tmv;localtime_r(&now,&tmv);static const char* wdDe[]={"SO","MO","DI","MI","DO","FR","SA"};static const char* wdEn[]={"SU","MO","TU","WE","TH","FR","SA"};char b[32];if(cfg.lang==LANG_EN)snprintf(b,sizeof(b),"%s  %04d-%02d-%02d",wdEn[tmv.tm_wday],tmv.tm_year+1900,tmv.tm_mon+1,tmv.tm_mday);else snprintf(b,sizeof(b),"%s  %02d.%02d.%04d",wdDe[tmv.tm_wday],tmv.tm_mday,tmv.tm_mon+1,tmv.tm_year+1900);date=b;}
  visibleCenter(2,date,145,2,C_GRAY,140,28);
}


static void drawActive() {
  applyBrightness();
  int8_t slot=countdownSlotFromMode(cfg.mode);
  if(slot>=0){drawCountdown((uint8_t)slot);return;}
  switch (cfg.mode) {
    case MODE_WEATHER: weatherPage=0; drawWeather(); break;
    case MODE_FORECAST: weatherPage=1; drawWeather(); break;
    case MODE_PLANE: planePlacePhase=false; lastPlanePlaceSwitchMs=millis(); drawPlane(); break;
    case MODE_CLOCK: drawClock(); break;
    case MODE_TRACKED_FLIGHT: drawTrackedFlight(); break;
    default: drawDepartures(); break;
  }
}

// ---- network ----------------------------------------------------------------
static void startAP() {
  apMode = true;
  // Keep STA enabled while hosting setup AP so /api/wifi/scan can scan nearby networks.
  WiFi.mode(WIFI_AP_STA);
  IPAddress ip(192,168,4,1);
  WiFi.softAPConfig(ip,ip,IPAddress(255,255,255,0));
  WiFi.softAP("PixelStop-Setup");
  dns.start(53,"*",ip);

  gfx->fillScreen(C_BLACK);
  centerText("SETUP", 55, 4, C_YELLOW);
  centerText(TR("WLAN:","WI-FI:"), 105, 2, C_GRAY);
  centerText("PixelStop-Setup", 128, 2, C_WHITE);
  centerText("192.168.4.1", 174, 2, C_GREEN);
  // With a stored network, the hotspot keeps looking for it (e.g. after a power cut,
  // when the router starts more slowly than the cube).
  if(cfg.ssid.length()){WiFi.disconnect();centerText(TR("sucht gespeichertes WLAN","looking for saved Wi-Fi"),205,1,C_GRAY);}
}


static void connectWiFi() {
  if (!cfg.ssid.length()) { startAP(); return; }
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.hostname(cfg.hostname.c_str());
  WiFi.setAutoReconnect(true);
  // The SSID is used exactly as stored, including blanks at the end.
  const uint32_t begun=millis();
  WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
  gfx->fillScreen(C_BLACK);
  centerText(TR("WLAN","WI-FI"), 80, 4, C_YELLOW);
  centerText(TR("verbinden...","connecting..."), 126, 2, C_GRAY);

  // 30 s: the radio link may stand while the address (DHCP) still takes a moment.
  uint32_t start=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-start<30000){delay(200);yield();}
  // Log: HTTP field = radio link after ms (0 = none), value = address after ms / Wi-Fi status.
  const uint32_t assoc=netAfter(wifiAssocMs,begun)?wifiAssocMs-begun:0;
  if(WiFi.status()!=WL_CONNECTED){diagEvent(D_WIFI,DS_FAILED,(int)assoc,WiFi.status());startAP();return;}
  diagEvent(D_WIFI,DS_WIFI_UP,(int)assoc,millis()-begun);

  apMode=false;
  if(MDNS.begin(cfg.hostname.c_str())) MDNS.addService("http","tcp",80);
  String rule=String(FPSTR(timeZoneRule(timeZoneIndex(cfg.timeZone.c_str()))));
  configTime(rule.c_str(),"pool.ntp.org","time.cloudflare.com");
}


// ---- restarts, rescue mode, network watchdog ------------------------------------
// Boot state in RTC memory (survives restarts, not power loss). A crash is a
// watchdog/exception reset or a restart PixelStop did not plan itself.
struct BootState { uint32_t magic; uint8_t crashes, planned, hangRestarts, pad; uint32_t checksum; };
static const uint32_t BOOT_MAGIC = 0x50534231UL, BOOT_RTC_WORD = 80; // Diagnostics 32..42, quota 64..
static BootState bootState = {};
static uint32_t bootChecksum(const BootState& b) { return (b.magic ^ (b.crashes | (b.planned << 8) | (b.hangRestarts << 16))) * 2654435761UL; }
static void bootStateWrite() { bootState.magic = BOOT_MAGIC; bootState.checksum = bootChecksum(bootState); ESP.rtcUserMemoryWrite(BOOT_RTC_WORD, (uint32_t*)&bootState, sizeof(bootState)); }
static void bootStateBegin() {
  BootState b = {};
  if (!ESP.rtcUserMemoryRead(BOOT_RTC_WORD, (uint32_t*)&b, sizeof(b)) || b.magic != BOOT_MAGIC || b.checksum != bootChecksum(b)) b = {};
  const uint32_t reason = ESP.getResetInfoPtr()->reason;
  const bool crash = reason == REASON_WDT_RST || reason == REASON_EXCEPTION_RST || reason == REASON_SOFT_WDT_RST ||
                     (reason == REASON_SOFT_RESTART && !b.planned);
  bootState = b;
  bootState.crashes = crash ? (uint8_t)min(255, b.crashes + 1) : 0;
  if (reason != REASON_SOFT_RESTART) bootState.hangRestarts = 0;
  bootState.planned = 0;
  rescueMode = bootState.crashes >= 3;
  bootStateWrite();
}
// Every restart PixelStop wants (settings, button, update, network watchdog).
static void restartNow() {
  bootState.planned = 1; bootStateWrite();
  adsbLolQuotaSave(true);
  diagEvent(D_REBOOT, DS_BEGIN);
  delay(300);
  ESP.restart();
}
static void drawRescue() {
  gfx->fillScreen(C_BLACK);
  centerText(TR("RETTUNGSMODUS","RESCUE MODE"), 30, 2, C_RED);
  centerText(TR("mehrere Abstürze in Folge","repeated crashes"), 64, 1, C_GRAY);
  centerText(TR("keine Abrufe","no requests"), 80, 1, C_GRAY);
  centerText(apMode ? String("192.168.4.1") : WiFi.localIP().toString(), 112, 2, C_WHITE);
  centerText(TR("Firmware-Update:","Firmware update:"), 146, 1, C_GRAY);
  centerText("/update", 162, 2, C_YELLOW);
  centerText(TR("Neustart beendet den Modus","a restart ends this mode"), 200, 1, C_GRAY);
}

// Wi-Fi events arrive outside loop(); they are only noted there.
static WiFiEventHandler wifiLostHandler, wifiGotIpHandler, wifiAssocHandler;
static volatile uint8_t wifiEvents = 0, wifiLostReason = 0;
static uint32_t wifiLostAt = 0;
static NetWatch netWatch = {};
static uint16_t wifiReconnects = 0;
static void wifiEventsBegin() {
  wifiLostHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& e) { wifiLostReason = e.reason; wifiEvents |= 1; });
  wifiGotIpHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP&) { wifiEvents |= 2; });
  wifiAssocHandler = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected&) { if (!wifiAssocMs) wifiAssocMs = millis(); });
}
static void wifiEventsHandle(uint32_t now) {
  if (!netWatch.start) { wifiEvents = 0; return; }  // first connection during start-up
  if (wifiEvents & 1) { wifiEvents &= ~1; if (!wifiLostAt) { wifiLostAt = now ? now : 1; diagEvent(D_WIFI, DS_WIFI_LOST, wifiLostReason); } }
  if (wifiEvents & 2) {
    wifiEvents &= ~2;
    if (wifiLostAt) { wifiReconnects++; diagEvent(D_WIFI, DS_WIFI_UP, WiFi.RSSI(), (now - wifiLostAt) / 1000UL); wifiLostAt = 0; }
  }
}
// Network watchdog (NetWatch.h): reconnect after 10 silent minutes, restart 5 minutes later.
static void networkWatchdog(uint32_t now) {
  const uint32_t answer = diagNetworkAnswerMs();
  if (bootState.hangRestarts && netAfter(answer, netWatch.start)) { bootState.hangRestarts = 0; bootStateWrite(); }
  const uint32_t factor = 1UL << (bootState.hangRestarts > 3 ? 3 : bootState.hangRestarts);
  uint32_t silent = 0;
  const NetAction action = netWatchStep(netWatch, now, answer, diagNetworkAttemptMs(),
                                        WiFi.status() != WL_CONNECTED && wifiLostAt, wifiLostAt, factor, silent);
  if (action == NET_RECONNECT) {
    diagEvent(D_WIFI, DS_WIFI_RETRY, WiFi.status(), silent / 1000UL);
    WiFi.disconnect(); WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
  } else if (action == NET_RESTART) {
    if (bootState.hangRestarts < 255) bootState.hangRestarts++;
    diagEvent(D_WIFI, DS_REJECTED, WiFi.status(), silent / 1000UL);
    restartNow();
  }
}
// Setup hotspot with a stored network: try it every minute while nobody is
// connected to the hotspot; once it works, restart into normal operation.
static uint32_t apRetryAt = 0, apTryStart = 0;
static bool apTrying = false;
static void setupHotspotRetry(uint32_t now) {
  if (!cfg.ssid.length()) return;
  const bool guests = WiFi.softAPgetStationNum() > 0;
  if (apTrying) {
    if (WiFi.status() == WL_CONNECTED && !guests) { diagEvent(D_WIFI, DS_WIFI_UP, WiFi.RSSI()); restartNow(); }
    if (guests || (uint32_t)(now - apTryStart) >= 20000UL) { WiFi.disconnect(); apTrying = false; apRetryAt = now + 60000UL; }
  } else if (!guests && (int32_t)(now - apRetryAt) >= 0) {
    WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
    apTrying = true; apTryStart = now;
  } else if (guests) apRetryAt = now + 60000UL;
}

// ---- web API ----------------------------------------------------------------
static void sendJson(JsonDocument& d) {
  String out; serializeJson(d,out); server.send(200,"application/json",out);
}

static void countdownToJson(JsonObject out,const CountdownSlot& c) {
  out["title"]=c.title;out["titleColor"]=c.titleColor;out["countColor"]=c.countColor;out["type"]=c.type;out["targetDate"]=c.targetDate;
  out["durationSec"]=c.durationSec;out["targetEpoch"]=c.targetEpoch;out["active"]=c.active;out["dayDigits"]=c.dayDigits;
}

static void updateCountdownSettings(JsonObjectConst in,CountdownSlot& c) {
  if(in["title"].is<const char*>())c.title=in["title"].as<String>();
  c.title=crop(c.title,24);
  if(in["titleColor"].is<const char*>()){String x=in["titleColor"].as<String>();if(validHexColor(x))c.titleColor=x;}
  if(in["countColor"].is<const char*>()){String x=in["countColor"].as<String>();if(validHexColor(x))c.countColor=x;}
  uint8_t oldType=c.type;c.type=constrain((int)(in["type"]|c.type),0,2);
  if(oldType!=c.type){c.active=false;c.targetEpoch=0;c.dayDigits=3;}
  if(in["targetDate"].is<const char*>())c.targetDate=in["targetDate"].as<String>();
  uint32_t dur=in["durationSec"]|c.durationSec;if(dur>31536000UL)dur=31536000UL;c.durationSec=dur;
}

static void apiGetConfig() {
  JsonDocument d;
  d["ssid"]=cfg.ssid;d["hostname"]=cfg.hostname;d["mode"]=cfg.mode;d["rotationMask"]=cfg.rotationMask;d["brightness"]=cfg.brightness;d["backlightMode"]=cfg.backlightMode;d["timeZone"]=cfg.timeZone;
  JsonArray rs=d["rotationSec"].to<JsonArray>();for(uint8_t i=0;i<MODE_COUNT;i++)rs.add(cfg.rotationSec[i]);
  d["nightEnabled"]=cfg.nightEnabled;d["nightStart"]=cfg.nightStart;d["nightEnd"]=cfg.nightEnd;d["nightBrightness"]=cfg.nightBrightness;
  d["stopName"]=cfg.stopName;d["stopPlace"]=cfg.stopPlace;d["stopId"]=cfg.stopId;d["departureCount"]=cfg.departureCount;d["departurePoll"]=cfg.departurePoll;d["vvoMotMask"]=cfg.vvoMotMask;d["lang"]=cfg.lang;d["departureCompactDestinations"]=cfg.departureCompactDestinations;d["departureAbbreviate"]=cfg.departureAbbreviate;d["departureDropTown"]=cfg.departureDropTown;
  d["weatherCity"]=cfg.weatherCity;d["weatherDisplayName"]=cfg.weatherDisplayName;d["weatherLat"]=cfg.weatherLat;d["weatherLon"]=cfg.weatherLon;d["weatherPoll"]=cfg.weatherPoll;d["weatherTempColor"]=cfg.weatherTempColor;d["weatherFahrenheit"]=cfg.weatherFahrenheit;d["geoGermanNames"]=cfg.geoGermanNames;
  d["planePlace"]=cfg.planePlace;d["planeDisplayName"]=cfg.planeDisplayName;d["planeLat"]=cfg.planeLat;d["planeLon"]=cfg.planeLon;d["planeRadiusKm"]=cfg.planeRadiusKm;d["planePoll"]=cfg.planePoll;d["planeShowAge"]=cfg.planeShowAge;d["planeShowPlace"]=cfg.planeShowPlace;d["planePlaceToggleSec"]=cfg.planePlaceToggleSec;
  d["planeOperator"]=cfg.planeOperator;d["planeSpeed"]=cfg.planeSpeed;d["planeTrend"]=cfg.planeTrend;d["planeMarkMilitary"]=cfg.planeMarkMilitary;d["planeHide"]=cfg.planeHide;
  d["trackedFlightNo"]=cfg.trackedFlightNo;d["trackerStatusPollMin"]=cfg.trackerStatusPollMin;d["aviationstackKeySet"]=cfg.aviationstackKey.length()>0;d["adsbLolKeySet"]=cfg.adsbLolKey.length()>0;d["rescue"]=rescueMode;
  JsonArray a=d["countdowns"].to<JsonArray>();for(uint8_t i=0;i<4;i++)countdownToJson(a.add<JsonObject>(),cfg.countdown[i]);
  sendJson(d);
}

static void apiPostConfig() {
  if(!server.hasArg("plain")){server.send_P(400,"application/json",PSTR("{\"ok\":false,\"error\":\"Keine Daten empfangen.\"}"));return;}
  JsonDocument d;if(deserializeJson(d,server.arg("plain"))){server.send_P(400,"application/json",PSTR("{\"ok\":false,\"error\":\"Einstellungen konnten nicht gelesen werden.\"}"));return;}
  if(d["timeZone"].is<const char*>() && timeZoneIndex(d["timeZone"].as<const char*>())<0){server.send_P(400,"application/json",PSTR("{\"ok\":false,\"error\":\"Unbekannte Zeitzone.\"}"));return;}
  Settings previous=cfg; String oldSsid=cfg.ssid,oldHost=cfg.hostname,oldTracked=cfg.trackedFlightNo,oldKey=cfg.aviationstackKey,oldAdsbKey=cfg.adsbLolKey;
  if(d["ssid"].is<const char*>())cfg.ssid=d["ssid"].as<String>();
  if(d["pass"].is<const char*>()){String x=d["pass"].as<String>();if(x.length())cfg.pass=x;}
  if(d["hostname"].is<const char*>()){String h=d["hostname"].as<String>();h.trim();h.toLowerCase();
    if(h!=cfg.hostname&&!webGuardHostnameValid(h.c_str())){cfg=previous;server.send_P(400,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Ungültiger Hostname: nur a–z, 0–9 und Bindestrich, 1–32 Zeichen.\"}"));return;}
    cfg.hostname=h;}
  uint16_t mask=((uint16_t)(d["rotationMask"]|cfg.rotationMask))&0x03FFU;if(mask)cfg.rotationMask=mask;
  JsonArrayConst rs=d["rotationSec"].as<JsonArrayConst>();if(!rs.isNull())for(uint8_t i=0;i<MODE_COUNT&&i<rs.size();i++)cfg.rotationSec[i]=constrain((int)(rs[i]|cfg.rotationSec[i]),5,3600);
  if(!modeEnabled(cfg.mode))cfg.mode=firstEnabledMode();
  cfg.brightness=constrain((int)(d["brightness"]|cfg.brightness),1,100);
  cfg.backlightMode=(uint8_t)(d["backlightMode"]|cfg.backlightMode)==1?1:0;
  if(d["timeZone"].is<const char*>())cfg.timeZone=d["timeZone"].as<String>();
  cfg.nightEnabled=d["nightEnabled"]|cfg.nightEnabled;cfg.nightStart=constrain((int)(d["nightStart"]|cfg.nightStart),0,1439);cfg.nightEnd=constrain((int)(d["nightEnd"]|cfg.nightEnd),0,1439);cfg.nightBrightness=constrain((int)(d["nightBrightness"]|cfg.nightBrightness),1,100);
  if(d["stopName"].is<const char*>()) cfg.stopName=d["stopName"].as<String>();
  if(d["stopPlace"].is<const char*>()) cfg.stopPlace=d["stopPlace"].as<String>();
  if(d["stopId"].is<const char*>()) cfg.stopId=d["stopId"].as<String>();
  {int dc=(int)(d["departureCount"]|cfg.departureCount);cfg.departureCount=(dc==10)?10:constrain(dc,3,5);}
  cfg.departurePoll=constrain((int)(d["departurePoll"]|cfg.departurePoll),10,300);
  cfg.vvoMotMask=((uint8_t)(d["vvoMotMask"]|cfg.vvoMotMask))&VVO_MOT_ALL;
  if(!cfg.vvoMotMask) cfg.vvoMotMask=VVO_MOT_ALL;
  cfg.lang=(uint8_t)(d["lang"]|cfg.lang)==LANG_EN?LANG_EN:LANG_DE;
  cfg.departureCompactDestinations=d["departureCompactDestinations"]|cfg.departureCompactDestinations;
  cfg.departureAbbreviate=d["departureAbbreviate"]|cfg.departureAbbreviate;
  cfg.departureDropTown=d["departureDropTown"]|cfg.departureDropTown;
  if(d["weatherCity"].is<const char*>()) cfg.weatherCity=d["weatherCity"].as<String>();
  if(d["weatherDisplayName"].is<const char*>()) cfg.weatherDisplayName=d["weatherDisplayName"].as<String>();
  cfg.weatherLat=d["weatherLat"]|cfg.weatherLat; cfg.weatherLon=d["weatherLon"]|cfg.weatherLon;
  cfg.weatherPoll=constrain((int)(d["weatherPoll"]|cfg.weatherPoll),60,3600);
  cfg.weatherTempColor=d["weatherTempColor"]|cfg.weatherTempColor;
  cfg.weatherFahrenheit=d["weatherFahrenheit"]|cfg.weatherFahrenheit;
  cfg.geoGermanNames=d["geoGermanNames"]|cfg.geoGermanNames;
  if(d["planePlace"].is<const char*>()) cfg.planePlace=d["planePlace"].as<String>();
  if(d["planeDisplayName"].is<const char*>()) cfg.planeDisplayName=d["planeDisplayName"].as<String>();
  cfg.planeLat=d["planeLat"]|cfg.planeLat; cfg.planeLon=d["planeLon"]|cfg.planeLon;
  cfg.planeRadiusKm=constrain((int)(d["planeRadiusKm"]|cfg.planeRadiusKm),1,250);
  cfg.planePoll=constrain((int)(d["planePoll"]|cfg.planePoll),(int)RADAR_MIN_POLL_SEC,300);
  cfg.planeShowAge=d["planeShowAge"]|cfg.planeShowAge;cfg.planeShowPlace=d["planeShowPlace"]|cfg.planeShowPlace;
  cfg.planePlaceToggleSec=(d["planePlaceToggleSec"]|cfg.planePlaceToggleSec)==0?0:constrain((int)(d["planePlaceToggleSec"]|cfg.planePlaceToggleSec),5,30);
  cfg.planeOperator=constrain((int)(d["planeOperator"]|cfg.planeOperator),0,2);cfg.planeSpeed=constrain((int)(d["planeSpeed"]|cfg.planeSpeed),0,2);
  cfg.planeTrend=d["planeTrend"]|cfg.planeTrend;cfg.planeMarkMilitary=d["planeMarkMilitary"]|cfg.planeMarkMilitary;cfg.planeHide=((uint8_t)(d["planeHide"]|cfg.planeHide))&RADAR_HIDE_ALL;
  if(d["trackedFlightNo"].is<const char*>()){cfg.trackedFlightNo=d["trackedFlightNo"].as<String>();cfg.trackedFlightNo.trim();cfg.trackedFlightNo.toUpperCase();}
  if(d["aviationstackKey"].is<const char*>()){String x=d["aviationstackKey"].as<String>();x.trim();if(x.length())cfg.aviationstackKey=x;}
  // adsb.lol key: empty keeps the stored key, "-" removes it.
  if(d["adsbLolKey"].is<const char*>()){String x=d["adsbLolKey"].as<String>();x.trim();if(x=="-")cfg.adsbLolKey="";else if(x.length()&&x.length()<72)cfg.adsbLolKey=x;}
  cfg.trackerStatusPollMin=constrain((int)(d["trackerStatusPollMin"]|cfg.trackerStatusPollMin),5,120);
  JsonArrayConst a=d["countdowns"].as<JsonArrayConst>();if(!a.isNull())for(uint8_t i=0;i<4&&i<a.size();i++){updateCountdownSettings(a[i].as<JsonObjectConst>(),cfg.countdown[i]);if(cfg.countdown[i].active&&cfg.countdown[i].type==COUNTDOWN_DAYS){int32_t dd=0;if(countdownDayDifference(cfg.countdown[i].targetDate,dd)){uint32_t av=dd<0?(uint32_t)(-(int64_t)dd):(uint32_t)dd;cfg.countdown[i].dayDigits=max(cfg.countdown[i].dayDigits,max((uint8_t)3,decimalDigits(av)));}}}
  d.clear();
  diagEvent(D_CONFIG,DS_BEGIN);
  if(!saveConfig()){diagEvent(D_CONFIG,DS_FAILED);cfg=previous;server.send_P(500,"application/json",PSTR("{\"ok\":false,\"error\":\"Einstellungen konnten nicht gespeichert werden.\"}"));return;}diagEvent(D_CONFIG,DS_DONE);applyTimeZone();applyBrightness();if(oldAdsbKey!=cfg.adsbLolKey){adsbLolSetKey(cfg.adsbLolKey);serviceClear(SERVICE_ADSB_LOL);flightReset();}
  uint16_t activated=cfg.rotationMask & ~previous.rotationMask;
  if(previous.stopId!=cfg.stopId||previous.departureCount!=cfg.departureCount||previous.vvoMotMask!=cfg.vvoMotMask||(activated&(1U<<MODE_DEPARTURES)))vvoReset();
  if(previous.weatherLat!=cfg.weatherLat||previous.weatherLon!=cfg.weatherLon||previous.timeZone!=cfg.timeZone||(activated&((1U<<MODE_WEATHER)|(1U<<MODE_FORECAST))))weatherReset();
  if(previous.planeLat!=cfg.planeLat||previous.planeLon!=cfg.planeLon||previous.planeRadiusKm!=cfg.planeRadiusKm||previous.planeHide!=cfg.planeHide||(activated&(1U<<MODE_PLANE)))flightReset();
  deferBackgroundNetwork(1000);departurePage=0;departurePagingReady=false;lastDeparturePageSwitchMs=millis();planePlacePhase=false;lastPlanePlaceSwitchMs=millis();if(oldTracked!=cfg.trackedFlightNo||oldKey!=cfg.aviationstackKey)trackedFlightReset();lastModeSwitchMs=millis();drawActive();
  bool reboot=(oldSsid!=cfg.ssid)||!oldHost.equalsIgnoreCase(cfg.hostname);server.send(200,"application/json",reboot?"{\"ok\":true,\"reboot\":true}":"{\"ok\":true,\"reboot\":false}");if(reboot){delay(200);restartNow();}
}

// Shows a view right away (web interface "▶"); the rotation continues from it.
static void apiShow() {
  JsonDocument d;
  if(!server.hasArg("plain")||deserializeJson(d,server.arg("plain"))){server.send_P(400,"application/json",PSTR("{\"ok\":false,\"error\":\"Ungültige Daten.\"}"));return;}
  int mode=d["mode"]|-1;
  if(mode<0||mode>=MODE_COUNT){server.send_P(400,"application/json",PSTR("{\"ok\":false,\"error\":\"Unbekanntes Programm.\"}"));return;}
  // Tell the web interface how long the view stays before the rotation continues.
  const bool rotating=enabledModeCount()>1;
  char reply[56];snprintf_P(reply,sizeof(reply),PSTR("{\"ok\":true,\"sec\":%u,\"rotation\":%s}"),
    (unsigned)max((uint16_t)5,cfg.rotationSec[mode]),rotating?"true":"false");
  server.send(200,"application/json",reply);
  cfg.mode=(uint8_t)mode;lastModeSwitchMs=millis();drawActive();
}

static void apiCountdown() {
  if(!server.hasArg("plain")){server.send_P(400,"application/json",PSTR("{\"ok\":false,\"error\":\"Keine Daten empfangen.\"}"));return;}
  JsonDocument d;if(deserializeJson(d,server.arg("plain"))){server.send_P(400,"application/json",PSTR("{\"ok\":false,\"error\":\"Ungültige Daten.\"}"));return;}
  int slot=d["slot"]|-1;if(slot<0||slot>3){server.send_P(400,"application/json",PSTR("{\"ok\":false,\"error\":\"Ungültiger Countdown-Slot.\"}"));return;}
  Settings previous=cfg; CountdownSlot& c=cfg.countdown[slot];updateCountdownSettings(d.as<JsonObjectConst>(),c);String action=d["action"]|"";
  if(action=="stop"){c.active=false;}
  else if(action=="start"){
    if(c.type==COUNTDOWN_DAYS){
      int32_t diff=0;if(!countdownDayDifference(c.targetDate,diff)){server.send_P(409,"application/json",PSTR("{\"ok\":false,\"error\":\"Zieltag fehlt oder Uhrzeit ist noch nicht synchronisiert\"}"));return;}
      uint32_t a=diff<0?(uint32_t)(-(int64_t)diff):(uint32_t)diff;c.dayDigits=max((uint8_t)3,decimalDigits(a));c.active=true;c.targetEpoch=0;
    } else {
      time_t now=time(nullptr);if(now<1609459200){server.send_P(409,"application/json",PSTR("{\"ok\":false,\"error\":\"Uhrzeit ist noch nicht synchronisiert\"}"));return;}
      if(c.durationSec==0){server.send_P(400,"application/json",PSTR("{\"ok\":false,\"error\":\"Dauer muss größer als 0 sein.\"}"));return;}
      c.targetEpoch=(uint32_t)now+c.durationSec;c.active=true;
    }
  } else {server.send_P(400,"application/json",PSTR("{\"ok\":false,\"error\":\"Unbekannte Aktion.\"}"));return;}
  if(!saveConfig()){cfg=previous;server.send_P(500,"application/json",PSTR("{\"ok\":false,\"error\":\"Countdown konnte nicht gespeichert werden.\"}"));return;}if(countdownSlotFromMode(cfg.mode)==slot)drawCountdown((uint8_t)slot);server.send_P(200,"application/json",PSTR("{\"ok\":true}"));
}

static void apiStatus() {
  JsonDocument d;d["fw"]=FW_NAME;d["version"]=FW_VERSION;d["ssid"]=apMode?"PixelStop-Setup":WiFi.SSID();d["ip"]=apMode?WiFi.softAPIP().toString():WiFi.localIP().toString();d["rssi"]=apMode?0:WiFi.RSSI();d["heap"]=ESP.getFreeHeap();d["uptime"]=millis()/1000;
  d["maxBlock"]=ESP.getMaxFreeBlockSize();d["vvoCount"]=vvoCount();d["vvoError"]=vvoError();d["weatherValid"]=weatherData().valid;d["weatherError"]=weatherData().error;d["flightValid"]=flightData().valid;d["flightError"]=flightData().error;d["flightStage"]=flightStageName();d["flightHttp"]=flightLastHttp();d["flightTlsRx"]=flightTlsRx();d["flightTlsMinBlock"]=flightTlsMinBlock();d["flightWaitSec"]=flightWaitSec();d["flightMinIntervalSec"]=serviceMinIntervalMs(SERVICE_ADSB_LOL)/1000;d["adsbToday"]=adsbLolRequestsToday();{JsonObject q=d["adsbQuota"].to<JsonObject>();const char* keys[]={"h","d","w","m"};for(uint8_t i=0;i<4;i++){uint16_t u,l;adsbLolQuotaUsage(Q_HOUR+i,u,l);JsonArray a=q[keys[i]].to<JsonArray>();a.add(u);a.add(l);}}d["radarIntervalSec"]=flightPollIntervalMs()/1000;d["wifiReconnects"]=wifiReconnects;d["rescue"]=rescueMode;d["adsbDenied"]=adsbLolAccessDenied(flightLastHttp());d["flightScanned"]=flightLastScanned();d["flightCall"]=flightData().valid?flightData().callsign:"";d["adsbKey"]=adsbLolHasKey();d["routeStatus"]=flightRouteStatus();d["routeSource"]=flightRouteSource();d["routeHttp"]=flightRouteHttp();d["flightAgeSec"]=flightLastOkMs()?(millis()-flightLastOkMs())/1000:0;d["trackerValid"]=trackedFlightData().valid;d["trackerError"]=trackedFlightData().error;sendJson(d);
}

static void apiVvoSearch() {
  if(apMode||WiFi.status()!=WL_CONNECTED){server.send_P(503,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Kein Internetzugang.\"}"));return;}
  String q=server.arg("q");q.trim();if(q.length()<2){server.send_P(400,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Suchbegriff zu kurz.\"}"));return;}
  String out;if(!vvoSearchStops(q,out)){server.send_P(502,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"VVO-Suche fehlgeschlagen.\"}"));return;}server.send(200,"application/json; charset=utf-8",out);
}

static void apiGeoSearch() {
  if(apMode||WiFi.status()!=WL_CONNECTED){server.send_P(503,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Kein Internetzugang.\"}"));return;}
  String q=server.arg("q");q.trim();
  if(q.length()<2){server.send_P(400,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Suchbegriff zu kurz.\"}"));return;}
  if(serviceWaitMs(SERVICE_GEO)){server.send_P(429,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Ortsdienst pausiert nach zu vielen Anfragen. Bitte später erneut versuchen.\"}"));return;}
  String out;
  bool german=server.arg("german")=="1";
  if(!geoSearchPlaces(q,german,out)){server.send_P(502,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Ortssuche fehlgeschlagen.\"}"));return;}
  server.send(200,"application/json; charset=utf-8",out);
}

static void apiGeoReverse() {
  if(apMode||WiFi.status()!=WL_CONNECTED){server.send_P(503,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Kein Internetzugang.\"}"));return;}
  float lat=server.arg("lat").toFloat(),lon=server.arg("lon").toFloat();
  if(lat<-90||lat>90||lon<-180||lon>180){server.send_P(400,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Ungültige Koordinaten.\"}"));return;}
  if(serviceWaitMs(SERVICE_GEO)){server.send_P(429,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Ortsdienst pausiert nach zu vielen Anfragen. Bitte später erneut versuchen.\"}"));return;}
  String out;
  bool german=server.arg("german")=="1";
  if(!geoReversePlace(lat,lon,german,out)){server.send_P(502,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"Ortsname konnte nicht ermittelt werden.\"}"));return;}
  server.send(200,"application/json; charset=utf-8",out);
}

static void apiWifiScan() {
  int n=WiFi.scanNetworks(false,false);
  if(n<0){server.send_P(500,"application/json; charset=utf-8",PSTR("{\"ok\":false,\"error\":\"WLAN-Suche fehlgeschlagen.\"}"));return;}
  JsonDocument d; d["ok"]=true; JsonArray a=d["networks"].to<JsonArray>();
  for(int i=0;i<n;i++){
    String ssid=WiFi.SSID(i); if(!ssid.length())continue;
    JsonObject x=a.add<JsonObject>(); x["ssid"]=ssid; x["rssi"]=WiFi.RSSI(i); x["open"]=(WiFi.encryptionType(i)==ENC_TYPE_NONE);
  }
  WiFi.scanDelete(); sendJson(d);
}

class LogResponse : public Print {
 public: size_t write(uint8_t c) override {return write(&c,1);}
 size_t write(const uint8_t* data,size_t len) override {server.sendContent((const char*)data,len);yield();return len;}
};
static void apiLog(){
 server.sendHeader("Cache-Control","no-store");
 if(server.hasArg("download"))server.sendHeader("Content-Disposition","attachment; filename=pixelstop-log.txt");
 server.setContentLength(CONTENT_LENGTH_UNKNOWN);server.send_P(200,"text/plain; charset=utf-8",PSTR(""));
 LogResponse response;diagPrint(response);
 char w[160];snprintf_P(w,sizeof(w),PSTR("WLAN jetzt: %s | RSSI %d dBm | Kanal %d | IP %s | Neuverbindungen %u | Rettungsmodus %s\n"),WiFi.status()==WL_CONNECTED?"verbunden":"getrennt",WiFi.RSSI(),WiFi.channel(),WiFi.localIP().toString().c_str(),(unsigned)wifiReconnects,rescueMode?"ja":"nein");response.print(w);
 server.sendContent("");deferBackgroundNetwork();
}
// ---- request guard (WebGuard.h) ----------------------------------------------
static void rejectForeign() {
  diagEvent(D_WEB, DS_REJECTED);
  server.send_P(403, "application/json; charset=utf-8", PSTR("{\"ok\":false,\"error\":\"Anfrage von fremder Seite abgelehnt.\"}"));
}
static bool hostOk() {
  if (apMode) return true;  // setup hotspot: no foreign names to rebind
  return webGuardHostAllowed(server.hostHeader().c_str(), WiFi.localIP().toString().c_str(), cfg.hostname.c_str());
}
static bool originOk() { return webGuardOriginAllowed(server.header("Origin").c_str(), server.hostHeader().c_str()); }
// Reading: the Host must name the cube. Changing: additionally no foreign Origin.
static bool guardRead() { if (hostOk()) return true; rejectForeign(); return false; }
static bool guardChange() { if (hostOk() && originOk()) return true; rejectForeign(); return false; }

// Web interface and English texts, stored gzip-compressed (tools/webui_build.py).
static void sendGzip(PGM_P type, const uint8_t* data, size_t len, bool cache) {
  if (!guardRead()) return;
  deferBackgroundNetwork(8000);  // the page loads its data right after; keep the cube free
  if (cache) server.sendHeader(F("Cache-Control"), F("max-age=86400"));
  server.sendHeader(F("Content-Encoding"), F("gzip"));
  server.send_P(200, type, (PGM_P)data, len);
}

// ---- firmware update (own handler, so the guard applies to the upload) -------
static bool otaAccepted = false, otaFailed = false;
static void otaPage() {
  if (!guardRead()) return;
  const bool en = cfg.lang == LANG_EN;
  String page = F("<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>PixelStop Update</title>"
                  "<style>body{background:#17191c;color:#f4f4f4;font-family:-apple-system,BlinkMacSystemFont,\"Segoe UI\",sans-serif;max-width:520px;margin:auto;padding:24px}"
                  "input,button{font-size:16px;margin:12px 0;display:block}button{background:#f3ce00;border:0;border-radius:10px;padding:11px 15px;font-weight:700}a{color:#f3ce00}</style></head><body><h1>PixelStop ");
  page += F(FW_VERSION);
  page += en ? F("</h1><p>Choose the firmware file (.bin) and start the update. The cube restarts afterwards.</p>")
             : F("</h1><p>Firmware-Datei (.bin) auswählen und Update starten. Der Würfel startet danach neu.</p>");
  page += F("<form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\"><input type=\"file\" accept=\".bin,.bin.gz\" name=\"firmware\" required><button type=\"submit\">");
  page += en ? F("Start update") : F("Update starten");
  page += F("</button></form><p><a href=\"/\">");
  page += en ? F("Back") : F("Zurück");
  page += F("</a></p></body></html>");
  server.send(200, F("text/html; charset=utf-8"), page);
}
static void otaUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    otaFailed = false;
    otaAccepted = hostOk() && originOk();
    if (!otaAccepted) { diagEvent(D_WEB, DS_REJECTED); return; }
    deferBackgroundNetwork(600000UL);
    WiFiUDP::stopAll();
    diagEvent(D_WEB, DS_BEGIN);
    if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000, U_FLASH)) otaFailed = true;
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (otaAccepted && !otaFailed && Update.write(up.buf, up.currentSize) != up.currentSize) otaFailed = true;
  } else if (up.status == UPLOAD_FILE_END) {
    if (otaAccepted && !otaFailed && !Update.end(true)) otaFailed = true;
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (otaAccepted) Update.end();
    otaFailed = true;
  }
  yield();
}
static void otaDone() {
  if (!otaAccepted) { rejectForeign(); return; }
  const bool en = cfg.lang == LANG_EN;
  if (otaFailed || Update.hasError()) {
    diagEvent(D_WEB, DS_FAILED, Update.getError());
    deferBackgroundNetwork(1000);
    server.send(500, F("text/plain; charset=utf-8"), (en ? String(F("Update failed: ")) : String(F("Update fehlgeschlagen: "))) + Update.getErrorString());
    return;
  }
  diagEvent(D_WEB, DS_DONE);
  server.send(200, F("text/html; charset=utf-8"), en
    ? F("<!doctype html><meta charset=\"utf-8\"><meta http-equiv=\"refresh\" content=\"20;url=/\"><body style=\"background:#17191c;color:#f4f4f4;font-family:sans-serif;padding:24px\">Update OK – PixelStop restarts…</body>")
    : F("<!doctype html><meta charset=\"utf-8\"><meta http-equiv=\"refresh\" content=\"20;url=/\"><body style=\"background:#17191c;color:#f4f4f4;font-family:sans-serif;padding:24px\">Update OK – PixelStop startet neu…</body>"));
  delay(200);
  restartNow();
}

static void setupWeb() {
  server.collectHeaders(F("Origin"));
  server.on("/api/log",HTTP_GET,[]{if(guardRead())apiLog();});
  server.on("/",HTTP_GET,[]{sendGzip(PSTR("text/html; charset=utf-8"),WEBUI_GZ,WEBUI_GZ_LEN,false);});
  server.on("/i18n/en.json",HTTP_GET,[]{sendGzip(PSTR("application/json; charset=utf-8"),WEBUI_EN_GZ,WEBUI_EN_GZ_LEN,true);});
  server.on("/api/config",HTTP_GET,[]{if(guardRead())apiGetConfig();});
  server.on("/api/config",HTTP_POST,[]{if(guardChange())apiPostConfig();});
  server.on("/api/countdown",HTTP_POST,[]{if(guardChange())apiCountdown();});
  server.on("/api/show",HTTP_POST,[]{if(guardChange())apiShow();});
  server.on("/api/status",HTTP_GET,[]{if(guardRead())apiStatus();});
  server.on("/api/vvo/search",HTTP_GET,[]{if(guardRead())apiVvoSearch();});
  server.on("/api/geo/search",HTTP_GET,[]{if(guardRead())apiGeoSearch();});
  server.on("/api/geo/reverse",HTTP_GET,[]{if(guardRead())apiGeoReverse();});
  server.on("/api/wifi/scan",HTTP_GET,[]{if(guardRead())apiWifiScan();});
  server.on("/api/reboot",HTTP_POST,[]{if(!guardChange())return;server.send_P(200,"application/json",PSTR("{\"ok\":true}"));restartNow();});
  server.on("/update",HTTP_GET,otaPage);
  server.on("/update",HTTP_POST,otaDone,otaUpload);
  server.onNotFound([](){if(apMode){server.sendHeader("Location","http://192.168.4.1/",true);server.send_P(302,"text/plain",PSTR(""));}else server.send_P(404,"text/plain",PSTR("Nicht gefunden"));});server.begin();
}

// ---- setup / loop ------------------------------------------------------------
void setup() {
  diagInit();
  Serial.begin(115200);
  // Select the framework phase-locked generator for the single backlight PWM.
  enablePhaseLockedWaveform();
  // Lower PWM frequency: at 1 % the on-pulse becomes ~33 us instead of ~8 us,
  // so interrupt jitter from WiFi/TLS/flash access is far less visible.
  analogWriteFreq(BL_PWM_FREQ);
  analogWriteRange(BL_PWM_RANGE);
  pinMode(TFT_BL,OUTPUT);

  bus = new Arduino_HWSPI(TFT_DC,TFT_CS);
  gfx = new SmallTV_ST7789(bus,TFT_RST,0,true,TFT_W,TFT_H,0,0,0,80);
  gfx->begin();
  gfx->setRotation(0);
  gfx->setTextWrap(false);
  gfx->fillScreen(C_BLACK);
  centerText(FW_NAME,82,3,C_YELLOW);
  centerText(FW_VERSION,118,2,C_GRAY);

  loadConfig();
  bootStateBegin();
  if(rescueMode)diagEvent(D_BOOT,DS_RESCUE,0,bootState.crashes);
  adsbLolQuotaBegin();
  adsbLolSetKey(cfg.adsbLolKey);
  applyTimeZone();
  applyBrightness();
  wifiEventsBegin();
  connectWiFi();
  setupWeb();
  netWatch.start=millis();
  if(apMode)apRetryAt=millis()+60000UL;

  if(rescueMode){drawRescue();return;}
  if(!apMode){
    gfx->fillScreen(C_BLACK);
    centerText(TR("VERBUNDEN","CONNECTED"),50,3,C_GREEN);
    centerText(WiFi.localIP().toString(),105,2,C_WHITE);
    centerText(cfg.hostname+".local",137,2,C_GRAY);
    delay(2200);
    drawActive();
    lastModeSwitchMs=millis();
  }
}

void loop() {
  server.handleClient();
  if(apMode)dns.processNextRequest();else MDNS.update();

  uint32_t nowMs=millis();
  wifiEventsHandle(nowMs);
  if(apMode)setupHotspotRetry(nowMs);
  // Ten stable minutes: earlier crashes no longer count towards rescue mode.
  if(bootState.crashes&&!rescueMode&&nowMs>600000UL){bootState.crashes=0;bootStateWrite();}
  if(rescueMode){
    if(nowMs-lastSecond>=1000){lastSecond=nowMs;applyBrightness();}
    yield();return;
  }
  if(nowMs-lastSecond>=1000){
    lastSecond=nowMs; applyBrightness();
    adsbLolQuotaSave();
    if(!apMode)networkWatchdog(nowMs);
    flightExpire();
    if(cfg.mode==MODE_PLANE)drawPlane(false); // continued distance/altitude, status dot and age
    time_t ep=time(nullptr);
    if(ep>1609459200){
      struct tm t; localtime_r(&ep,&t);
      if(t.tm_min!=lastMinute){
        lastMinute=t.tm_min;
        if(cfg.mode==MODE_CLOCK) drawClock(false);
        else if(cfg.mode==MODE_DEPARTURES||cfg.mode==MODE_WEATHER||cfg.mode==MODE_FORECAST||cfg.mode==MODE_PLANE||cfg.mode==MODE_TRACKED_FLIGHT) drawFooterClockOnly();
      }
    }
    int8_t slot=countdownSlotFromMode(cfg.mode);
    if(slot>=0){
      String visible=countdownValue(cfg.countdown[(uint8_t)slot]);
      if(visible!=lastCountdownRendered[(uint8_t)slot]) drawCountdown((uint8_t)slot,false);
    }
  }

  // View rotation is separate from data refreshes. API data can stay fresh while hidden.
  if(!apMode&&enabledModeCount()>1){
    uint32_t stayMs=(uint32_t)max((uint16_t)5,cfg.rotationSec[cfg.mode])*1000UL;
    if(nowMs-lastModeSwitchMs>=stayMs){
      cfg.mode=nextEnabledMode(cfg.mode); lastModeSwitchMs=nowMs; drawActive();
    }
  }


  if(cfg.departureCount==10&&vvoCount()>5){
    if(!departurePagingReady){departurePagingReady=true;departurePage=0;lastDeparturePageSwitchMs=nowMs;}
    else if((uint32_t)(nowMs-lastDeparturePageSwitchMs)>=5000UL){departurePage^=1;lastDeparturePageSwitchMs=nowMs;if(cfg.mode==MODE_DEPARTURES)drawDepartures(false);}
  }else{departurePagingReady=false;departurePage=0;lastDeparturePageSwitchMs=nowMs;}

  if(cfg.mode==MODE_PLANE&&cfg.planeShowPlace&&cfg.planePlaceToggleSec>0&&planePlaceTitle().length()){
    if((uint32_t)(nowMs-lastPlanePlaceSwitchMs)>=(uint32_t)cfg.planePlaceToggleSec*1000UL){planePlacePhase=!planePlacePhase;lastPlanePlaceSwitchMs=nowMs;drawPlaneHeader();}
  }else if(!cfg.planeShowPlace){planePlacePhase=false;lastPlanePlaceSwitchMs=nowMs;}

  if(!apMode&&backgroundNetworkReady()){
    static uint8_t cursor=0;
    for(uint8_t n=0;n<6;n++){
      uint8_t task=cursor;cursor=(cursor+1)%6;bool ran=false;
      if(task==0&&modeEnabled(MODE_DEPARTURES)&&cfg.stopId.length()&&vvoPollDue(cfg.departurePoll)){
        vvoFetchDepartures(cfg.stopId,cfg.departureCount,cfg.vvoMotMask);if(cfg.mode==MODE_DEPARTURES)drawDepartures(false);ran=true;
      }else if(task==1&&(modeEnabled(MODE_WEATHER)||modeEnabled(MODE_FORECAST))&&weatherPollDue(cfg.weatherPoll)){
        weatherFetch(cfg.weatherLat,cfg.weatherLon,cfg.timeZone.c_str());if(cfg.mode==MODE_WEATHER||cfg.mode==MODE_FORECAST)drawWeather(false);ran=true;
      }else if(task==2&&radarActive()&&flightPollDue(cfg.planePoll,radarVisibleShare())){
        flightFetch(cfg.planeLat,cfg.planeLon,cfg.planeRadiusKm,cfg.planeHide);if(cfg.mode==MODE_PLANE)drawPlane(false);ran=true;
      }else if(task==3&&radarActive()&&flightEnrichmentDue()){
        flightEnrich();if(cfg.mode==MODE_PLANE)drawPlane(false);ran=true;
      }else if(cfg.mode==MODE_TRACKED_FLIGHT&&cfg.trackedFlightNo.length()&&cfg.aviationstackKey.length()){
        if(task==4&&trackedFlightStatusPollDue(cfg.trackerStatusPollMin)){trackedFlightFetchStatus(cfg.aviationstackKey,cfg.trackedFlightNo);trackedFlightFetchRoute();drawTrackedFlight(false);ran=true;}
        else if(task==5&&trackedFlightLivePollDue(15)){trackedFlightFetchLive();trackedFlightFetchRoute();drawTrackedFlight(false);ran=true;}
      }
      if(ran){deferBackgroundNetwork();break;}
    }
  }
  yield();
}
