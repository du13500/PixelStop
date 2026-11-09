#pragma once
// Branch names for well-known military callsign prefixes, for the radar's
// operator display (variant B: branch, variant C: branch / unit). Unknown
// callsigns return false, so the display keeps the name of the callsign (A).
#include <stdint.h>
#include <stddef.h>
#include <string.h>

struct MilitaryOperator { const char* prefix; const char* de; const char* en; const char* unit; };

inline const MilitaryOperator* militaryOperatorFor(const char* callsign) {
  static const MilitaryOperator table[] = {
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
  if (!callsign || !*callsign) return nullptr;
  for (const auto& m : table) {
    size_t n = strlen(m.prefix);
    if (!strncmp(callsign, m.prefix, n)) {
      // A prefix must be followed by the flight number (digits), not more letters.
      char next = callsign[n];
      if (n == 4 || (next >= '0' && next <= '9')) return &m;
    }
  }
  return nullptr;
}

// variant 1 = branch, 2 = branch / unit (if it fits in `limit` characters).
inline bool militaryOperatorText(const char* callsign, uint8_t variant, bool english, char* out, size_t cap, size_t limit = 18) {
  const MilitaryOperator* m = (variant == 1 || variant == 2) ? militaryOperatorFor(callsign) : nullptr;
  if (!m || !out || !cap) return false;
  const char* branch = english ? m->en : m->de;
  out[0] = 0;
  if (variant == 2 && m->unit[0] && strlen(branch) + 3 + strlen(m->unit) <= limit && strlen(branch) + 3 + strlen(m->unit) < cap) {
    strcpy(out, branch); strcat(out, " / "); strcat(out, m->unit);
  } else {
    strncpy(out, branch, cap - 1); out[cap - 1] = 0;
  }
  return true;
}
