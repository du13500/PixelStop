#!/usr/bin/env python3
"""Render the real display views from src/main.cpp into PNG screenshots.

The drawing code (helpers and all views) is taken verbatim from main.cpp and
compiled for the host against a 240x240 RGB565 framebuffer. Text uses the
same two fonts as the device: Arduino_GFX's classic 5x7 font for ASCII and
PixelFont.h for everything else. Data comes from the sample fixtures below.

Usage:
  python3 tools/render_screens.py [OUTPUT_DIR] [--font PATH/TO/glcdfont.h] [--scale N]

The font is part of the GFX Library for Arduino; after `pio pkg install` it is
found in .pio/libdeps automatically.
"""
from pathlib import Path
import argparse, re, subprocess, sys, tempfile

root = Path(__file__).resolve().parents[1]
src = (root / 'src/main.cpp').read_text()

ap = argparse.ArgumentParser()
ap.add_argument('out', nargs='?', default=str(root / 'docs/screens'))
ap.add_argument('--font', default=None)
ap.add_argument('--scale', type=int, default=2)
ap.add_argument('--frame', action='store_true', help='add a dark bezel around each screen')
args = ap.parse_args()

font = Path(args.font) if args.font else next(
    (p for p in (root / '.pio/libdeps').glob('*/GFX Library for Arduino/src/font/glcdfont.h')), None)
if not font or not font.is_file():
    sys.exit('glcdfont.h not found; run "pio pkg install -e smalltv_ultra" or pass --font')
glyphs = re.search(r'font\[\]\s*PROGMEM\s*=\s*\{(.*?)\};', font.read_text(), re.S).group(1)

def between(start, end):
    a = src.index(start); b = src.index(end, a)
    return src[a:b]

# Declarations (pins, colours, modes, settings) and the complete drawing block.
decl = between('#define TFT_DC', '// ---- ST7789 quirk')
decl = re.sub(r'static ESP8266WebServer[^\n]*\n|static ESP8266HTTPUpdateServer[^\n]*\n|static DNSServer[^\n]*\n', '', decl)
views_raw = between('// ---- helpers', '// ---- network')

def top_level_items(code):
    """Split code into top-level chunks (definitions incl. their bodies)."""
    items, depth, start, i, n = [], 0, 0, 0, len(code)
    while i < n:
        c = code[i]
        if code.startswith('//', i):
            j = code.find('\n', i); i = n if j < 0 else j; continue
        if c == '"' or c == "'":
            q = c; i += 1
            while i < n and code[i] != q: i += 2 if code[i] == '\\' else 1
        elif c == '{': depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                k = i + 1
                if code[k:k+1] == ';': k += 1
                items.append(code[start:k]); start = k
        elif c == ';' and depth == 0:
            items.append(code[start:i+1]); start = i + 1
        i += 1
    items.append(code[start:])
    return items

# Keep everything that only draws; drop config storage, JSON and network code.
SKIP = re.compile(r'JsonDocument|JsonObject|JsonArray|LittleFS|\bFile\b|server\.|WiFi|ESP\.|deserializeJson|serializeJson')
views = ''.join(x for x in top_level_items(views_raw) if not SKIP.search(x))

ARDUINO_H = r'''
#pragma once
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <ctime>
#include <algorithm>
#define PROGMEM
#define F(x) x
#define FPSTR(x) (x)
#define PSTR(x) (x)
#define PGM_P const char*
#define pgm_read_byte(p) (*(const uint8_t*)(p))
#define pgm_read_dword(p) (*(const uint32_t*)(p))
#define PI 3.14159265358979323846
using std::min; using std::max;
template<class T,class A,class B> T constrain(T x,A a,B b){return x<(T)a?(T)a:(x>(T)b?(T)b:x);}
uint32_t millis();
inline void analogWrite(int,int){}
#define HIGH 1
inline void digitalWrite(int,int){}
inline long lround_(double v){return lround(v);}
inline size_t strlcpy(char*d,const char*s,size_t n){size_t l=strlen(s);if(n){size_t c=l<n-1?l:n-1;memcpy(d,s,c);d[c]=0;}return l;}
struct String : std::string {
  String() {}
  String(const char* s) : std::string(s ? s : "") {}
  String(const std::string& s) : std::string(s) {}
  String(char c) : std::string(1, c) {}
  String(int v) : std::string(std::to_string(v)) {}
  String(unsigned v) : std::string(std::to_string(v)) {}
  String(long v) : std::string(std::to_string(v)) {}
  String(unsigned long v) : std::string(std::to_string(v)) {}
  String(long long v) : std::string(std::to_string(v)) {}
  String(unsigned long long v) : std::string(std::to_string(v)) {}
  String(double v, int digits = 2) { char b[48]; snprintf(b, sizeof b, "%.*f", digits, v); assign(b); }
  unsigned length() const { return (unsigned)size(); }
  String substring(unsigned a) const { return a < size() ? String(substr(a)) : String(); }
  String substring(unsigned a, unsigned b) const { if (a > b) std::swap(a, b); return a < size() ? String(substr(a, b - a)) : String(); }
  int indexOf(char c, unsigned from = 0) const { auto p = find(c, from); return p == npos ? -1 : (int)p; }
  int indexOf(const String& q, unsigned from = 0) const { auto p = find(q, from); return p == npos ? -1 : (int)p; }
  int lastIndexOf(char c) const { auto p = rfind(c); return p == npos ? -1 : (int)p; }
  bool startsWith(const String& p) const { return compare(0, p.size(), p) == 0; }
  bool endsWith(const String& p) const { return size() >= p.size() && compare(size() - p.size(), p.size(), p) == 0; }
  void toUpperCase() { for (auto& c : *this) if (c >= 'a' && c <= 'z') c -= 32; }
  void toLowerCase() { for (auto& c : *this) if (c >= 'A' && c <= 'Z') c += 32; }
  void trim() { size_t a = find_first_not_of(" \t\r\n"); if (a == npos) { clear(); return; } size_t b = find_last_not_of(" \t\r\n"); *this = substr(a, b - a + 1); }
  void replace(const String& a, const String& b) { if (a.empty()) return; size_t p = 0; while ((p = find(a, p)) != npos) { std::string::replace(p, a.size(), b); p += b.size(); } }
  long toInt() const { return atol(c_str()); }
  float toFloat() const { return (float)atof(c_str()); }
  char charAt(unsigned i) const { return i < size() ? (*this)[i] : 0; }
  bool equalsIgnoreCase(const String& o) const { if (o.size() != size()) return false; for (size_t i = 0; i < size(); i++) if (tolower((*this)[i]) != tolower(o[i])) return false; return true; }
  bool concat(const String& s) { append(s); return true; }
};
inline String operator+(const String& a, const String& b) { return String(static_cast<const std::string&>(a) + b); }
inline String operator+(const String& a, const char* b) { return String(static_cast<const std::string&>(a) + b); }
inline String operator+(const char* a, const String& b) { return String(a + static_cast<const std::string&>(b)); }
inline String operator+(const String& a, char b) { return String(static_cast<const std::string&>(a) + b); }
inline String operator+(const String& a, int b) { return a + String(b); }
inline String operator+(const String& a, unsigned b) { return a + String(b); }
inline String operator+(const String& a, long b) { return a + String(b); }
inline String operator+(const String& a, unsigned long b) { return a + String(b); }
'''

GFX = r'''
static const unsigned char glcd[] = {''' + glyphs + r'''};
struct Framebuffer {
  uint16_t px[240 * 240] = {};
  int cx = 0, cy = 0, size = 1; uint16_t color = 0xFFFF; uint8_t rot = 0;
  void setRotation(uint8_t r) { rot = r; }
  void setTextWrap(bool) {}
  void fillScreen(uint16_t c) { for (auto& p : px) p = c; }
  void drawPixel(int x, int y, uint16_t c) { if (x >= 0 && y >= 0 && x < 240 && y < 240) px[y * 240 + x] = c; }
  void fillRect(int x, int y, int w, int h, uint16_t c) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) drawPixel(x + i, y + j, c); }
  void drawFastHLine(int x, int y, int w, uint16_t c) { fillRect(x, y, w, 1, c); }
  void drawFastVLine(int x, int y, int h, uint16_t c) { fillRect(x, y, 1, h, c); }
  void drawRect(int x, int y, int w, int h, uint16_t c) { drawFastHLine(x, y, w, c); drawFastHLine(x, y + h - 1, w, c); drawFastVLine(x, y, h, c); drawFastVLine(x + w - 1, y, h, c); }
  void drawLine(int x0, int y0, int x1, int y1, uint16_t c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, e = dx + dy;
    for (;;) { drawPixel(x0, y0, c); if (x0 == x1 && y0 == y1) break; int e2 = 2 * e; if (e2 >= dy) { e += dy; x0 += sx; } if (e2 <= dx) { e += dx; y0 += sy; } }
  }
  void fillCircle(int x0, int y0, int r, uint16_t c) { for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++) if (x * x + y * y <= r * r + r) drawPixel(x0 + x, y0 + y, c); }
  void drawCircle(int x0, int y0, int r, uint16_t c) {
    int f = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r;
    drawPixel(x0, y0 + r, c); drawPixel(x0, y0 - r, c); drawPixel(x0 + r, y0, c); drawPixel(x0 - r, y0, c);
    while (x < y) { if (f >= 0) { y--; ddy += 2; f += ddy; } x++; ddx += 2; f += ddx;
      int pts[8][2] = {{x,y},{-x,y},{x,-y},{-x,-y},{y,x},{-y,x},{y,-x},{-y,-x}}; for (auto& p : pts) drawPixel(x0 + p[0], y0 + p[1], c); }
  }
  void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) { fillRect(x + r, y, w - 2 * r, h, c); fillRect(x, y + r, w, h - 2 * r, c); fillCircle(x + r, y + r, r, c); fillCircle(x + w - r - 1, y + r, r, c); fillCircle(x + r, y + h - r - 1, r, c); fillCircle(x + w - r - 1, y + h - r - 1, r, c); }
  void drawRoundRect(int x, int y, int w, int h, int, uint16_t c) { drawRect(x, y, w, h, c); }
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
    int minx = std::min({x0, x1, x2}), maxx = std::max({x0, x1, x2}), miny = std::min({y0, y1, y2}), maxy = std::max({y0, y1, y2});
    auto edge = [](int ax, int ay, int bx, int by, int px, int py) { return (bx - ax) * (py - ay) - (by - ay) * (px - ax); };
    for (int y = miny; y <= maxy; y++) for (int x = minx; x <= maxx; x++) {
      int a = edge(x0, y0, x1, y1, x, y), b = edge(x1, y1, x2, y2, x, y), d = edge(x2, y2, x0, y0, x, y);
      if ((a >= 0 && b >= 0 && d >= 0) || (a <= 0 && b <= 0 && d <= 0)) drawPixel(x, y, c); }
  }
  void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) { drawLine(x0, y0, x1, y1, c); drawLine(x1, y1, x2, y2, c); drawLine(x2, y2, x0, y0, c); }
  void setTextSize(int s) { size = s; }
  void setTextColor(uint16_t c) { color = c; }
  void setTextColor(uint16_t c, uint16_t) { color = c; }
  void setCursor(int x, int y) { cx = x; cy = y; }
  size_t write(uint8_t ch) {
    // Arduino_GFX classic font: 5 columns, LSB at the top, 6x8 cell.
    for (int i = 0; i < 5; i++) { uint8_t line = glcd[ch * 5 + i];
      for (int j = 0; j < 8; j++, line >>= 1) if (line & 1) fillRect(cx + i * size, cy + j * size, size, size, color); }
    cx += 6 * size; return 1;
  }
  void print(const String& s) { for (unsigned char c : s) write(c); }
  void print(const char* s) { print(String(s)); }
  void print(int v) { print(String(v)); }
};
static Framebuffer framebuffer;
static Framebuffer* gfx = &framebuffer;
'''

FIXTURES = r'''
static uint32_t clockMs = 1000000;
uint32_t millis() { return clockMs; }
static time_t fakeNow = 1793956212; // Fri 2026-11-06 10:10 CET
static time_t fakeTime(time_t* t) { if (t) *t = fakeNow; return fakeNow; }
#define time(x) fakeTime(x)

// ---- data accessors with sample values ----
static VvoDeparture deps[10]; static uint8_t depN = 0;
uint8_t vvoCount() { return depN; }
const VvoDeparture& vvoAt(uint8_t i) { return deps[i]; }
bool vvoError() { return false; } uint32_t vvoLastOkMs() { return 0; }
bool vvoNotServed() { return false; }
static WeatherSnapshot wx = {};
const WeatherSnapshot& weatherData() { return wx; }
const char* weatherLabel(int c) { return weatherVisual(c).label; }
static FlightSnapshot fl = {};
const FlightSnapshot& flightData() { return fl; }
static uint32_t okAgeMs = 4000;
uint32_t flightLastOkMs() { return clockMs - okAgeMs; }
bool flightProviderPaused() { return false; }
int flightLastHttp() { return 200; }
uint32_t flightWaitSec() { return 0; }
float flightLiveDistanceKm(const FlightSnapshot& f) { return f.distanceKm; }
int32_t flightLiveAltitudeFt(const FlightSnapshot& f) { return f.altitudeFt; }
uint32_t flightPollIntervalMs() { return 30000; }
static TrackedFlightSnapshot tf = {};
const TrackedFlightSnapshot& trackedFlightData() { return tf; }
'''

def stub_scan():
    # Functions used by the views but implemented elsewhere: keep fixture list explicit.
    return ''

MAIN = r'''
#include "Arduino.h"
#include "Vvo.h"
#include "Weather.h"
#include "Flight.h"
#include "TrackFlight.h"
#include "RequestPolicy.h"
#include "PixelFont.h"
#include "TimeZones.h"
#include "WeatherVisual.h"
#include "AircraftLabel.h"
#include "MilitaryOperator.h"
#include "AirlineName.h"
#include "RadarFilter.h"
#include "DestinationFormat.h"
static inline bool adsbLolAccessDenied(int c){return c==401||c==403||c==426;}
''' + GFX + '\n' + decl + '\n' + FIXTURES + '\n' + views + r'''
static void dump(const char* path) {
  FILE* f = fopen(path, "wb"); fprintf(f, "P6\n240 240\n255\n");
  for (uint16_t p : framebuffer.px) { uint8_t rgb[3] = {(uint8_t)(((p >> 11) & 31) * 255 / 31), (uint8_t)(((p >> 5) & 63) * 255 / 63), (uint8_t)((p & 31) * 255 / 31)}; fwrite(rgb, 1, 3, f); }
  fclose(f);
}
static void dep(const char* line, const char* dir, int min, bool delayed = false, bool cancelled = false) {
  VvoDeparture& d = deps[depN++]; strlcpy(d.line, line, sizeof d.line); strlcpy(d.direction, dir, sizeof d.direction);
  d.minutes = min; d.delayed = delayed; d.cancelled = cancelled;
}
int main(int argc, char** argv) {
  std::string out = argc > 1 ? argv[1] : ".";
  cfg.lang = (argc > 2 && !strcmp(argv[2], "en")) ? LANG_EN : LANG_DE;
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); tzset();
  cfg.timeZone = "Europe/Berlin";
  cfg.rotationMask = 0x1FF;

  const bool en = cfg.lang == LANG_EN;
  char name[96];
  // One frame per second: gif-<name>-<n>.ppm becomes <name>.gif.
  auto frame = [&](const char* gif, int i) { snprintf(name, sizeof name, "/gif-%s-%d.ppm", gif, i); dump((out + name).c_str()); };

  // ---- Abfahrten (Beispieldaten) ----
  cfg.mode = MODE_DEPARTURES; cfg.stopName = "Hauptbahnhof"; cfg.stopId = "33000028"; cfg.departureCount = 5;
  dep("S1", "Meißen Triebischtal", 1); dep("3", "Wilder Mann", 3); dep("RE50", "Leipzig Hbf", 6, true);
  dep("8", "Hellerau", 9); dep("10", "Messe Dresden", 12, false, true);
  drawDepartures(); dump((out + "/abfahrten.ppm").c_str());
  // Fahrtziele ohne eigenen Ort und abgekürzt (S-Bahnhof Reick, Dresden)
  depN = 0; cfg.stopName = "S-Bahnhof Reick"; cfg.stopPlace = "Dresden"; cfg.departureDropTown = true; cfg.departureAbbreviate = true;
  dep("S2", "Dresden Flughafen", 2); dep("S1", "Schöna", 5); dep("S2", "Pirna", 9); dep("S1", "Meißen Triebischtal", 14, true); dep("S2", "Dresden Flughafen", 22);
  drawDepartures(); dump((out + "/abfahrten-ziele.ppm").c_str());
  cfg.departureDropTown = false; cfg.departureAbbreviate = false; cfg.stopPlace = "";
  // Ten departures on two pages; the page changes every 5 s like on the cube.
  depN = 0; cfg.stopName = "Postplatz"; cfg.departureCount = 10;
  dep("1", "Prohlis", 0); dep("2", "Kleinzschachwitz", 1); dep("4", "Laubegast", 2); dep("12", "Striesen", 3, true); dep("1", "Leutewitz", 4);
  dep("11", "Bühlau", 5); dep("2", "Gorbitz", 7); dep("4", "Radebeul West", 8); dep("12", "Leutewitz", 10); dep("62", "Johannstadt", 11, false, true);
  for (int i = 0; i < 10; i++) { departurePage = i < 5 ? 0 : 1; drawDepartures(i == 0); frame("abfahrten-seiten", i); }
  departurePage = 0; cfg.departureCount = 5; depN = 0;

  // ---- Wetter: deutsch Dresden in °C, englisch Berlin in °F ----
  cfg.mode = MODE_WEATHER; cfg.weatherCity = en ? "Berlin" : "Dresden"; cfg.weatherDisplayName = ""; cfg.weatherFahrenheit = en;
  wx.valid = true; wx.currentC = 8.9f; wx.highC = 11.7f; wx.lowC = 5.6f; wx.rainPct = 70; wx.code = 61; wx.isDay = true;
  strlcpy(wx.sunrise, "07:21", sizeof wx.sunrise); strlcpy(wx.sunset, "16:08", sizeof wx.sunset);
  drawWeather(); dump((out + "/wetter.ppm").c_str());
  wx.currentC = 3.6f; wx.highC = 7.2f; wx.lowC = 1.4f; wx.rainPct = 10; wx.code = 1; wx.isDay = false;
  drawWeather(); dump((out + "/wetter-nacht.ppm").c_str());
  wx.currentC = 8.9f; wx.highC = 11.7f; wx.lowC = 5.6f; wx.rainPct = 70; wx.code = 61; wx.isDay = true;
  wx.days = 3;
  wx.day[0] = {11.7f, 5.6f, 61, 70}; wx.day[1] = {9.8f, 3.2f, 3, 20}; wx.day[2] = {4.2f, -1.4f, 71, 60};
  weatherPage = 1; drawWeather(true); dump((out + "/vorhersage.ppm").c_str()); weatherPage = 0;

  // ---- Flugradar ----
  // Every scene is an animation: one frame per second, starting at a full 6 s
  // cycle, so the 3 s changes (route/squawk, registration/speed, long airline
  // names) and the age counter are visible. Distance and altitude change per frame.
  cfg.mode = MODE_PLANE; cfg.planeShowAge = true; cfg.planeShowPlace = true; cfg.planePlaceToggleSec = 0; planePlacePhase = true;
  auto plane = [&](const char* hex, const char* call, const char* airline, const char* reg, const char* code, const char* type, const char* route) {
    fl = FlightSnapshot{}; fl.valid = true; strlcpy(fl.hex, hex, 8); strlcpy(fl.callsign, call, 12); strlcpy(fl.airline, airline, 64);
    strlcpy(fl.registration, reg, 16); strlcpy(fl.typeCode, code, 8); strlcpy(fl.typeName, type, 80);
    if (route) { strlcpy(fl.origin, route, 4); strlcpy(fl.destination, route + 4, 4); parseFlightRoute(route, fl.route); }
  };
  // km0/km1, ft0/ft1: values in the first and last frame; age0: seconds since the last answer.
  auto radarGif = [&](const char* gif, int frames, float km0, float km1, int32_t ft0, int32_t ft1, int age0) {
    for (int i = 0; i < frames; i++) {
      const float k = frames > 1 ? (float)i / (frames - 1) : 0;
      clockMs = 1200000 + i * 1000; okAgeMs = (age0 + i) * 1000;
      fl.distanceKm = km0 + (km1 - km0) * k; fl.altitudeFt = (int32_t)lroundf(ft0 + (ft1 - ft0) * k); fl.altitudeKnown = ft0 >= 0;
      drawPlane(i == 0); frame(gif, i);
      if (i == 0 && !strcmp(gif, "flugradar")) dump((out + "/flugradar.ppm").c_str());   // still for the overview
    }
  };
  // Normalfall: Emirates A380 über Dresden-Klotzsche
  cfg.planePlace = "Dresden-Klotzsche";
  plane("896180", "UAE45", "Emirates", "A6-EVK", "A388", "Airbus A380-800", "DXB-FRA");
  radarGif("flugradar", 6, 11.4f, 9.8f, 37000, 37000, 4);
  // Steigflug: Alrosa Tu-154M RA-85757 nach dem Start in Mirny (letzter ziviler
  // Tu-154-Linienflug in Russland, 28.10.2020, Mirny - Nowosibirsk)
  cfg.planePlace = "Mirny"; cfg.planeTrend = true;
  plane("15a5f1", "DRU154", "ALROSA", "RA-85757", "T154", "Tupolev Tu-154", "MJZ-OVB");
  fl.motion.hasRate = true; fl.motion.rateFpm = 2600;
  radarGif("radar-steigflug", 6, 6.2f, 7.9f, 6400, 6660, 3);
  // Sinkflug: European Air Charter MD-82 LZ-LDP im Anflug auf Varna (letzter
  // Flug einer MD-80 in Europa, 21.10.2023, Hurghada - Varna)
  cfg.planePlace = "Varna";
  plane("450a8e", "BUC7302", "European Air Charter", "LZ-LDP", "MD82", "MD-82", "HRG-VAR");
  fl.motion.hasRate = true; fl.motion.rateFpm = -1100;
  radarGif("radar-sinkflug", 6, 14.8f, 13.5f, 5200, 5090, 2);
  cfg.planeTrend = false;
  // Langer Airline-Name im Wechsel, dazu Geschwindigkeit statt Kennzeichen
  cfg.planePlace = "Leipzig/Halle"; cfg.planeSpeed = 1;
  plane("06a2b5", "QTR8152", "Qatar Airways Cargo", "A7-BFG", "B77L", "Boeing 777-200LR/F", "DOH-LEJ");
  fl.speedKnown = true; fl.speedKt = 248;
  radarGif("radar-name", 6, 18.6f, 17.4f, 7800, 7300, 5);
  cfg.planeSpeed = 0;
  // Statuspunkt gelb: letzter Abruf fehlgeschlagen, Werte geschätzt
  cfg.planePlace = "Frankfurt";
  plane("3c4b26", "DLH9AT", "Lufthansa", "D-AIEF", "A21N", "Airbus A321neo", "FRA-BER");
  fl.error = true;
  radarGif("radar-gelb", 6, 22.3f, 23.9f, 14000, 14000, 41);
  // Statuspunkt grau: zu lange keine Daten, Werte stehen still
  cfg.planePlace = "Hamburg";
  plane("3c66a7", "EWG7KA", "Eurowings", "D-AEWK", "A320", "Airbus A320", "HAM-PMI");
  fl.error = true;
  radarGif("radar-grau", 6, 31.5f, 31.5f, 23000, 23000, 64);
  // Kein Flugzeug im Radius
  cfg.planePlace = "Helgoland"; fl = FlightSnapshot{}; okAgeMs = 4000;
  drawPlane(); dump((out + "/radar-leer.ppm").c_str());

  // Fiktive Daten (Battlestar Galactica) für Militär und Notfall, damit keine
  // echten Flüge so aussehen: Quelle "DRADIS" statt adsb.lol, 4-stellige Codes.
  planeSourceLabel = "DRADIS"; cfg.planeMarkMilitary = true; cfg.planeDisplayName = "CIC Galactica";
  auto bsg = [&](const char* call, const char* op, const char* to, const char* type, const char* reg, bool mil) {
    fl = FlightSnapshot{}; fl.valid = true; strlcpy(fl.hex, "bs7500", 8); strlcpy(fl.callsign, call, 12); strlcpy(fl.airline, op, 64);
    strlcpy(fl.registration, reg, 16); strlcpy(fl.typeName, type, 80); strlcpy(fl.typeCode, "BSG", 8);
    strlcpy(fl.origin, "BS75", 5); strlcpy(fl.destination, to, 5);
    if (strcmp(to, "N/A")) { char r[12]; snprintf(r, sizeof r, "BS75-%s", to); parseFlightRoute(r, fl.route); }
    fl.military = mil;
  };
  bsg("COLONE", "Colonial Fleet", "BS62", "Colonial Heavy 798", "COL-ONE", false);
  fl.speedKnown = true; fl.speedKt = 860 / 1.852f; cfg.planeSpeed = 1;
  radarGif("radar-colone", 6, 12.4f, 12.9f, -1, -1, 33);
  cfg.planeSpeed = 0;
  bsg("APOLLO", "Col. Fleet / Viper", "N/A", "Viper MK II", "N7242C", true);
  radarGif("radar-apollo", 6, 0.4f, 0.9f, -1, -1, 2);
  // Notfall: Der Raptor fällt mit 1,4 km/s auf den Planeten zu und entfernt sich
  // vom CIC, knapp innerhalb des 100-km-Radius.
  bsg("KARMA", "Colonial Fleet", "N/A", "Raptor", "BS75-R1", true);
  fl.motion.hasRate = true; fl.motion.rateFpm = -3000; cfg.planeTrend = true; strlcpy(fl.squawk, "7700", 5);
  radarGif("radar-karma", 6, 92.7f, 99.7f, (int32_t)lroundf(98700 / 0.3048f), (int32_t)lroundf(91700 / 0.3048f), 33);
  planeSourceLabel = "ADSB.LOL"; okAgeMs = 4000; clockMs = 1000000; cfg.planeTrend = false; cfg.planeDisplayName = ""; cfg.planeShowPlace = false;

  // ---- Uhr ----
  cfg.mode = MODE_CLOCK; drawClock(); dump((out + "/uhr.ppm").c_str());

  // ---- Flug-Tracker ----
  cfg.mode = MODE_TRACKED_FLIGHT; cfg.trackedFlightNo = "LH1178"; cfg.aviationstackKey = "sample";
  tf.valid = true; strlcpy(tf.flightIata, "LH1178", 12); strlcpy(tf.flightIcao, "DLH1178", 12); strlcpy(tf.airline, "Lufthansa", 28);
  strlcpy(tf.origin, "DRS", 4); strlcpy(tf.destination, "FRA", 4); parseFlightRoute("DRS-FRA", tf.route);
  strlcpy(tf.status, "active", 16); strlcpy(tf.depScheduled, "09:40", 6); strlcpy(tf.depActual, "09:52", 6);
  strlcpy(tf.arrScheduled, "10:50", 6); strlcpy(tf.arrEstimated, "10:58", 6); strlcpy(tf.registration, "D-AIBH", 16);
  strlcpy(tf.typeCode, "A319", 8); tf.altitudeM = 10970; tf.speedKmh = 812;
  tf.depScheduledEpoch = fakeNow - 1800; tf.depActualEpoch = fakeNow - 1080; tf.arrScheduledEpoch = fakeNow + 2400; tf.arrEstimatedEpoch = fakeNow + 2880;
  drawTrackedFlight(); dump((out + "/flugtracker.ppm").c_str());

  // ---- Countdowns ----
  const time_t base = fakeNow;
  CountdownSlot& c1 = cfg.countdown[0]; c1.title = en ? "Vacation" : "Urlaub"; c1.type = COUNTDOWN_DAYS; c1.targetDate = "2026-12-23"; c1.active = true; c1.dayDigits = 3;
  cfg.mode = MODE_COUNTDOWN1; drawCountdown(0); dump((out + "/countdown-tage.ppm").c_str());
  // HUD, ohne Vorzeichen, Startdatum 1.10. -> Balken
  c1.style = COUNTDOWN_HUD; c1.showSign = false; c1.startDay = (int32_t)daysFromCivilDate(2026, 10, 1); c1.countColor = "#FF4F6E"; c1.titleColor = "#FF4F6E";
  drawCountdown(0); dump((out + "/countdown-tage-hud.ppm").c_str());
  CountdownSlot& c2 = cfg.countdown[1]; c2.title = "Pizza"; c2.type = COUNTDOWN_MS; c2.titleColor = "#F3CE00"; c2.countColor = "#5DD78B";
  c2.durationSec = 720; c2.targetEpoch = (uint32_t)base + 534; c2.active = true;
  cfg.mode = MODE_COUNTDOWN2;
  for (int i = 0; i < 6; i++) { fakeNow = base + i; drawCountdown(1, i == 0); frame("countdown-timer", i); }
  CountdownSlot& c3 = cfg.countdown[2]; c3.title = en ? "Departure" : "Abflug"; c3.type = COUNTDOWN_HMS; c3.style = COUNTDOWN_HUD;
  c3.titleColor = "#FF4F6E"; c3.countColor = "#FF4F6E"; c3.durationSec = 3 * 3600; c3.targetEpoch = (uint32_t)base + 4983; c3.active = true;
  cfg.mode = MODE_COUNTDOWN3;
  for (int i = 0; i < 6; i++) { fakeNow = base + i; drawCountdown(2, i == 0); frame("countdown-hud", i); }
  fakeNow = base;
  return 0;
}
'''

out = Path(args.out); out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory() as tmp:
    t = Path(tmp)
    (t / 'Arduino.h').write_text(ARDUINO_H)
    for name in ['ESP8266HTTPClient.h']:
        (t / name).write_text('#pragma once\nstruct HTTPClient{};\n')
    (t / 'render.cpp').write_text(MAIN)
    exe = t / 'render'
    r = subprocess.run(['g++', '-std=c++17', '-w', '-fpermissive', '-I', str(t), '-I', str(root / 'src'),
                        str(t / 'render.cpp'),
                        '-o', str(exe)], capture_output=True, text=True)
    if r.returncode:
        sys.stderr.write(r.stderr[-6000:]); sys.exit(1)
    (t / 'de').mkdir(); (t / 'en').mkdir()
    subprocess.run([str(exe), str(t / 'de'), 'de'], check=True)
    subprocess.run([str(exe), str(t / 'en'), 'en'], check=True)
    from PIL import Image, ImageDraw
    KEY = (255, 0, 255)   # transparent corners of the GIF frames

    def finish(img):
        """Scale up and, with --frame, add a thin dark-grey border with rounded,
        transparent corners: readable on light and dark GitHub pages alike."""
        if args.scale > 1:
            img = img.resize((240 * args.scale, 240 * args.scale), Image.NEAREST)
        if not args.frame:
            return img.convert('RGBA')
        b, r = args.scale, 7 * args.scale
        w, h = img.width + 2 * b, img.height + 2 * b
        out = Image.new('RGBA', (w, h), (0, 0, 0, 0))
        ImageDraw.Draw(out).rounded_rectangle((0, 0, w - 1, h - 1), r, fill=(72, 77, 84, 255))
        mask = Image.new('L', (img.width, img.height), 0)
        ImageDraw.Draw(mask).rounded_rectangle((0, 0, img.width - 1, img.height - 1), r - b, fill=255)
        out.paste(img, (b, b), mask)
        return out

    # Animated examples: frames gif-<name>-<n>.ppm become <name>.gif (1 s per frame).
    for lang_dir in [t / 'de', t / 'en']:
        dest = out if lang_dir.name == 'de' else out / 'en'
        dest.mkdir(parents=True, exist_ok=True)
        groups = {}
        for ppm in sorted(lang_dir.glob('gif-*.ppm')):
            groups.setdefault(ppm.stem.rsplit('-', 1)[0][4:], []).append(ppm)
        for name, frames in groups.items():
            imgs = []
            for ppm in sorted(frames, key=lambda f: int(f.stem.rsplit('-', 1)[1])):
                rgba = finish(Image.open(ppm).convert('RGB'))
                flat = Image.new('RGB', rgba.size, KEY); flat.paste(rgba, (0, 0), rgba)
                imgs.append(flat); ppm.unlink()
            # One palette for all frames (taken from all of them), corners transparent.
            strip = Image.new('RGB', (imgs[0].width, imgs[0].height * len(imgs)))
            for i, im in enumerate(imgs): strip.paste(im, (0, i * im.height))
            palette = strip.quantize(colors=128, method=Image.Quantize.MEDIANCUT)
            frames_q = [im.quantize(palette=palette, dither=Image.Dither.NONE) for im in imgs]
            pal = palette.getpalette()[:3 * 128]
            key = min(range(len(pal) // 3), key=lambda i: sum((pal[3 * i + k] - KEY[k]) ** 2 for k in range(3)))
            frames_q[0].save(dest / (name + '.gif'), save_all=True, append_images=frames_q[1:], duration=1000, loop=0,
                             optimize=False, transparency=key, disposal=1)
            print(dest / (name + '.gif'))
    for ppm in sorted(t.glob('*/*.ppm')):
        dest = out if ppm.parent.name == 'de' else out / 'en'
        dest.mkdir(parents=True, exist_ok=True)
        finish(Image.open(ppm).convert('RGB')).save(dest / (ppm.stem + '.png'))
        print(dest / (ppm.stem + '.png'))
