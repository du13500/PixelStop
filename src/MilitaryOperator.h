#pragma once
// Branch names for well-known military callsign prefixes, for the radar's
// operator display (variant B: branch, variant C: branch / unit). Unknown
// callsigns return false, so the display keeps the name of the callsign (A).
// The table lives in flash; entries are copied out one at a time.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
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

struct MilitaryOperator { char prefix[5]; char de[19]; char en[19]; char unit[6]; };

// Copies the entry for the callsign into out; false if none matches.
inline bool militaryOperatorFor(const char* callsign, MilitaryOperator& out) {
  static const MilitaryOperator table[] PROGMEM = {
    {"RCH","US Air Force","US Air Force","AMC"},          // Air Mobility Command ("Reach")
    {"SAM","US Air Force","US Air Force","SAM"},          // Special Air Mission
    {"CNV","US Navy","US Navy",""},
    {"PAT","US Army","US Army",""},
    {"GAF","Luftwaffe","German Air Force",""},
    {"GAM","Heer","German Army",""},
    {"GNY","Marine","German Navy",""},
    {"RRR","Royal Air Force","Royal Air Force",""},
    {"RFR","Royal Air Force","Royal Air Force",""},
    {"CTM","French Air Force","French Air Force","COTAM"},
    {"FAF","French Air Force","French Air Force",""},
    {"IAM","Italian Air Force","Italian Air Force",""},
    {"AME","Spanish Air Force","Spanish Air Force",""},
    {"BAF","Belgian Air Force","Belgian Air Force",""},
    {"NAF","Royal Neth. AF","Royal Neth. AF",""},
    {"PLF","Polish Air Force","Polish Air Force",""},
    {"CEF","Czech Air Force","Czech Air Force",""},
    {"HAF","Hellenic Air Force","Hellenic Air Force",""},
    {"SUI","Swiss Air Force","Swiss Air Force",""},
    {"CFC","Royal Canadian AF","Royal Canadian AF",""},
    {"ASY","Royal Austr. AF","Royal Austr. AF",""},
    {"NATO","NATO","NATO","AWACS"},
    {"RFF","Russian Air Force","Russian Air Force",""},
  };
  if (!callsign || !*callsign) return false;
  for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
    memcpy_P(&out, &table[i], sizeof(out));
    size_t n = strlen(out.prefix);
    if (!strncmp(callsign, out.prefix, n)) {
      // A prefix must be followed by the flight number (digits), not more letters.
      char next = callsign[n];
      if (n == 4 || (next >= '0' && next <= '9')) return true;
    }
  }
  return false;
}

// variant 1 = branch, 2 = branch / unit (if it fits in `limit` characters).
inline bool militaryOperatorText(const char* callsign, uint8_t variant, bool english, char* out, size_t cap, size_t limit = 18) {
  MilitaryOperator m;
  if ((variant != 1 && variant != 2) || !out || !cap || !militaryOperatorFor(callsign, m)) return false;
  const char* branch = english ? m.en : m.de;
  size_t combined = strlen(branch) + 3 + strlen(m.unit);
  out[0] = 0;
  if (variant == 2 && m.unit[0] && combined <= limit && combined < cap) {
    strcpy(out, branch); strcat(out, " / "); strcat(out, m.unit);
  } else {
    strncpy(out, branch, cap - 1); out[cap - 1] = 0;
  }
  return true;
}
