#pragma once
// Protects the web interface without a password:
// - DNS rebinding: a foreign web page cannot make the browser talk to the cube
//   under its own name; the Host header must name the cube (IP, hostname with
//   a local suffix, or the setup address).
// - CSRF: a foreign page can send a form or request to the cube, but browsers
//   then send its Origin. Changes (POST, firmware update) are refused unless
//   the Origin is the cube itself. Tools like curl send no Origin and work.
// Pure functions without Arduino dependencies so they can be tested on the host.
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stddef.h>
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

// Host header without port, lower case; false if it does not fit.
inline bool webGuardHostName(const char* host, char* out, size_t cap) {
  size_t n = 0;
  for (; host[n] && host[n] != ':'; n++) {
    if (n + 1 >= cap) return false;
    out[n] = (char)tolower((unsigned char)host[n]);
  }
  out[n] = 0;
  if (n && out[n - 1] == '.') out[n - 1] = 0;   // "pixelstop.local."
  return true;
}

inline bool webGuardHostAllowed(const char* host, const char* ip, const char* hostname) {
  if (!host || !*host) return true;              // no Host header: not a browser
  char h[72];
  if (!webGuardHostName(host, h, sizeof(h))) return false;
  if (ip && *ip && !strcmp(h, ip)) return true;
  if (!strcmp(h, "192.168.4.1")) return true;    // setup hotspot
  size_t hl = hostname ? strlen(hostname) : 0;
  if (!hl || strncasecmp(h, hostname, hl)) return false;
  const char* rest = h + hl;
  if (!*rest) return true;
  static const char suffixes[][13] PROGMEM = {".local", ".fritz.box", ".lan", ".home", ".home.arpa",
                                              ".localdomain", ".box", ".internal", ".intranet"};
  for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
    char sfx[13]; memcpy_P(sfx, suffixes[i], sizeof(sfx));
    if (!strcmp(rest, sfx)) return true;
  }
  return false;
}

// Origin must be absent (tools) or the cube itself, as named in the Host header.
inline bool webGuardOriginAllowed(const char* origin, const char* host) {
  if (!origin || !*origin) return true;
  if (strncasecmp(origin, "http://", 7)) return false;   // also "null" and https pages
  char o[72], h[72];
  if (!host || !*host || !webGuardHostName(origin + 7, o, sizeof(o)) || !webGuardHostName(host, h, sizeof(h))) return false;
  return o[0] && !strcmp(o, h);
}

// Hostname for mDNS: 1-32 characters a-z, 0-9 and '-', not at the start or end.
inline bool webGuardHostnameValid(const char* name) {
  size_t n = name ? strlen(name) : 0;
  if (!n || n > 32 || name[0] == '-' || name[n - 1] == '-') return false;
  for (size_t i = 0; i < n; i++) {
    char c = name[i];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
  }
  return true;
}
