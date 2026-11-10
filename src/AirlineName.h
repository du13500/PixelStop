#pragma once
// Airline names on the radar screen (one line, 18 characters in large type).
// 1. Brand names by ICAO callsign prefix take precedence over provider names
//    ("KLM Royal Dutch Airlines" -> "KLM").
// 2. Other long names lose legal and generic endings ("... Airlines Inc").
// 3. Still too long: two parts at a word boundary, shown alternately (3 s).
// 4. Last resort: small type.
// Pure functions without Arduino dependencies so they can be tested on the host.
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#ifdef ARDUINO
#include <pgmspace.h>
#else
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef memcpy_P
#define memcpy_P memcpy
#endif
#endif

struct AirlineBrand { char key[4]; char name[19]; };

// Copies the brand name for the callsign prefix; false if there is none.
inline bool airlineBrand(const char* callsign, char* out, size_t cap) {
  static const AirlineBrand table[] PROGMEM = {
    // Europe
    {"DLH","Lufthansa"},{"CLH","Lufthansa CityLine"},{"GEC","Lufthansa Cargo"},{"EWG","Eurowings"},
    {"CFG","Condor"},{"TUI","TUI"},{"BOX","AeroLogic"},{"DHK","DHL"},{"BCS","DHL"},
    {"AUA","Austrian"},{"SWR","SWISS"},{"EDW","Edelweiss"},{"KLM","KLM"},{"TRA","Transavia"},
    {"AFR","Air France"},{"BAW","British Airways"},{"VIR","Virgin Atlantic"},{"EXS","Jet2"},
    {"RYR","Ryanair"},{"EJU","easyJet"},{"EZY","easyJet"},{"EZS","easyJet"},{"WZZ","Wizz Air"},
    {"VLG","Vueling"},{"IBE","Iberia"},{"TAP","TAP Air Portugal"},{"ITY","ITA Airways"},
    {"SAS","SAS"},{"NOZ","Norwegian"},{"NSZ","Norwegian"},{"FIN","Finnair"},{"ICE","Icelandair"},
    {"LOT","LOT"},{"TVS","Smartwings"},{"CSA","Czech Airlines"},{"BEL","Brussels Airlines"},
    {"AEE","Aegean"},{"THY","Turkish Airlines"},{"PGT","Pegasus"},{"AFL","Aeroflot"},
    {"CLX","Cargolux"},{"FDX","FedEx"},{"UPS","UPS Airlines"},{"GTI","Atlas Air"},
    // Middle East and Asia
    {"UAE","Emirates"},{"QTR","Qatar Airways"},{"ETD","Etihad"},{"SIA","Singapore Airlines"},
    {"CPA","Cathay Pacific"},{"JAL","Japan Airlines"},{"ANA","ANA"},{"KAL","Korean Air"},
    {"CCA","Air China"},
    // Americas
    {"AAL","American"},{"UAL","United"},{"DAL","Delta"},{"SWA","Southwest"},{"ASA","Alaska"},
    {"JBU","JetBlue"},{"NKS","Spirit"},{"FFT","Frontier"},{"AAY","Allegiant"},
    {"ENY","Envoy"},{"SKW","SkyWest"},{"RPA","Republic"},{"EDV","Endeavor"},{"PDT","Piedmont"},
    {"JIA","PSA Airlines"},{"GJS","GoJet"},{"ASH","Mesa"},{"QXE","Horizon"},
    {"ACA","Air Canada"},{"WJA","WestJet"},{"AMX","Aeromexico"},{"VOI","Volaris"},
    {"AVA","Avianca"},{"LAN","LATAM"},{"TAM","LATAM"},
  };
  if (!callsign || strlen(callsign) < 4) return false;
  for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
    AirlineBrand e; memcpy_P(&e, &table[i], sizeof(e));
    if (!strncmp(callsign, e.key, 3)) { strncpy(out, e.name, cap - 1); out[cap - 1] = 0; return true; }
  }
  return false;
}

// Characters as shown (UTF-8 continuation bytes do not count).
inline size_t airlineGlyphs(const char* s) {
  size_t n = 0;
  for (; *s; s++) if (((uint8_t)*s & 0xC0) != 0x80) n++;
  return n;
}

// Removes generic and legal endings while the name is longer than `limit`.
inline void airlineShorten(const char* in, char* out, size_t cap, size_t limit) {
  static const char* const endings[] = {
    " Inc.", " Inc", " Ltd.", " Ltd", " Limited", " LLC", " PLC", " plc", " S.A.", " SA", " S.p.A.", " AG",
    " GmbH", " Co.", " Corp.", " Corporation", " Company", " International", " Intl", " Airlines",
    " Air Lines", " Airways", " Airline", " Aviation"};
  strncpy(out, in, cap - 1); out[cap - 1] = 0;
  if (!strncmp(out, "The ", 4) && airlineGlyphs(out) > limit) memmove(out, out + 4, strlen(out + 4) + 1);
  bool changed = true;
  while (changed && airlineGlyphs(out) > limit) {
    changed = false;
    size_t len = strlen(out);
    while (len && (out[len - 1] == ' ' || out[len - 1] == ',')) out[--len] = 0;
    for (const char* e : endings) {
      size_t el = strlen(e);
      if (len > el && !strcmp(out + len - el, e)) { out[len - el] = 0; changed = true; break; }
    }
  }
}

// Splits the (shortened) name for the line. Returns 1 if it fits as is (in a),
// 2 for two parts shown alternately (a, b), 0 if only small type helps.
inline uint8_t airlineParts(const char* name, size_t limit, char* a, char* b, size_t cap) {
  airlineShorten(name, a, cap, limit);
  b[0] = 0;
  if (airlineGlyphs(a) <= limit) return 1;
  // Last blank so that the first part still fits.
  size_t len = strlen(a), cut = 0, glyphs = 0;
  for (size_t i = 0; i < len; i++) {
    if (((uint8_t)a[i] & 0xC0) != 0x80) glyphs++;
    if (a[i] == ' ' && glyphs - 1 <= limit) cut = i;
  }
  if (!cut || airlineGlyphs(a + cut + 1) > limit) return 0;
  strncpy(b, a + cut + 1, cap - 1); b[cap - 1] = 0;
  a[cut] = 0;
  return 2;
}
