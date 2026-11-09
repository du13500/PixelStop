#pragma once
// Reads the "ac" array of an adsb.lol answer one aircraft at a time, so the
// memory need does not grow with the number of aircraft in the radius.
// Body needs read() and peek() (-1 at the end) and works as an ArduinoJson
// reader; visit(JsonObjectConst) is called for every aircraft object.
#include <ArduinoJson.h>
#include <string.h>

template <class Body>
inline bool radarScanSkipTo(Body& body, const char* key) {
  // Find "key" (with quotes) and the following '['.
  size_t matched = 0, len = strlen(key);
  for (;;) {
    int c = body.read();
    if (c < 0) return false;
    if (matched < len) {
      matched = (c == key[matched]) ? matched + 1 : (c == key[0] ? 1 : 0);
    } else if (c == '[') {
      return true;
    } else if (c != ':' && c != ' ' && c != '\n' && c != '\r' && c != '\t') {
      matched = (c == key[0]) ? 1 : 0;   // "ac" was a value or part of another key
    }
  }
}

// Returns the number of objects read; err is set on a JSON error.
template <class Body, class Visit>
inline uint16_t radarScanArray(Body& body, JsonDocument& doc, const JsonDocument& filter,
                               Visit visit, DeserializationError& err, uint16_t limit = 800) {
  err = DeserializationError::Ok;
  if (!radarScanSkipTo(body, "\"ac\"")) { err = DeserializationError::InvalidInput; return 0; }
  uint16_t n = 0;
  while (n < limit) {
    int c = body.peek();
    while (c == ',' || c == ' ' || c == '\n' || c == '\r' || c == '\t') { body.read(); c = body.peek(); }
    if (c != '{') break;                     // ']' or end of data
    doc.clear();
    err = deserializeJson(doc, body, DeserializationOption::Filter(filter));
    if (err) return n;
    ++n;
    visit(doc.as<JsonObjectConst>());
  }
  return n;
}
