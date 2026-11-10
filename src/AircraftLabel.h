#pragma once
// Display names for ICAO aircraft type designators, at most 18 characters so
// they fit one line in large type. One code = exactly one text; where ICAO
// itself groups variants under one code, the text names what the code covers.
// Unknown codes are shown as the code itself. The table lives in flash.
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

struct AircraftTypeName { char code[5]; char name[19]; };

// Copies the display name for the code into out; false if the code is unknown.
inline bool aircraftDisplayLabel(const char* code, char* out, size_t cap) {
  static const AircraftTypeName table[] PROGMEM = {
    // Airbus
    {"A318","Airbus A318"},
    {"A319","Airbus A319"},
    {"A19N","Airbus A319neo"},
    {"A320","Airbus A320"},
    {"A20N","Airbus A320neo"},
    {"A321","Airbus A321"},
    {"A21N","Airbus A321neo"},
    {"BCS1","Airbus A220-100"},
    {"BCS3","Airbus A220-300"},
    {"A306","Airbus A300-600"},
    {"A310","Airbus A310"},
    {"A332","Airbus A330-200"},
    {"A333","Airbus A330-300"},
    {"A338","Airbus A330-800"},
    {"A339","Airbus A330-900"},
    {"A342","Airbus A340-200"},
    {"A343","Airbus A340-300"},
    {"A345","Airbus A340-500"},
    {"A346","Airbus A340-600"},
    {"A359","Airbus A350-900"},
    {"A35K","Airbus A350-1000"},
    {"A388","Airbus A380-800"},
    {"A3ST","Airbus Beluga"},
    {"A337","Airbus BelugaXL"},
    // Boeing
    {"B712","Boeing 717-200"},
    {"B732","Boeing 737-200"},
    {"B733","Boeing 737-300"},
    {"B734","Boeing 737-400"},
    {"B735","Boeing 737-500"},
    {"B736","Boeing 737-600"},
    {"B737","Boeing 737-700"},
    {"B738","Boeing 737-800"},
    {"B739","Boeing 737-900"},
    {"B37M","Boeing 737 MAX 7"},
    {"B38M","Boeing 737 MAX 8"},
    {"B39M","Boeing 737 MAX 9"},
    {"B3XM","Boeing 737 MAX 10"},
    {"B742","Boeing 747-200"},
    {"B744","Boeing 747-400"},
    {"B748","Boeing 747-8"},
    {"B752","Boeing 757-200"},
    {"B753","Boeing 757-300"},
    {"B762","Boeing 767-200"},
    {"B763","Boeing 767-300"},
    {"B764","Boeing 767-400"},
    {"B772","Boeing 777-200"},
    {"B77L","Boeing 777-200LR/F"},
    {"B773","Boeing 777-300"},
    {"B77W","Boeing 777-300ER"},
    {"B778","Boeing 777-8"},
    {"B779","Boeing 777-9"},
    {"B788","Boeing 787-8"},
    {"B789","Boeing 787-9"},
    {"B78X","Boeing 787-10"},
    // McDonnell Douglas
    {"MD11","MD-11"},
    {"DC10","DC-10"},
    {"MD82","MD-82"},
    {"MD83","MD-83"},
    {"MD88","MD-88"},
    {"MD90","MD-90"},
    // Embraer
    {"E135","Embraer ERJ-135"},
    {"E145","Embraer ERJ-145"},
    {"E170","Embraer E170"},
    {"E75S","Embraer E175 SW"},
    {"E75L","Embraer E175 LW"},
    {"E190","Embraer E190"},
    {"E195","Embraer E195"},
    {"E290","Embraer E190-E2"},
    {"E295","Embraer E195-E2"},
    {"E120","Embraer EMB-120"},
    // Regional
    {"CRJ1","Bombardier CRJ100"},
    {"CRJ2","Bombardier CRJ200"},
    {"CRJ7","Bombardier CRJ700"},
    {"CRJ9","Bombardier CRJ900"},
    {"CRJX","Bombardier CRJ1000"},
    {"DH8A","Dash 8-100"},
    {"DH8B","Dash 8-200"},
    {"DH8C","Dash 8-300"},
    {"DH8D","Dash 8-400"},
    {"AT43","ATR 42-300/320"},
    {"AT45","ATR 42-500"},
    {"AT46","ATR 42-600"},
    {"AT72","ATR 72-200"},
    {"AT75","ATR 72-500"},
    {"AT76","ATR 72-600"},
    {"SF34","Saab 340"},
    {"SB20","Saab 2000"},
    {"F70","Fokker 70"},
    {"F100","Fokker 100"},
    {"RJ85","Avro RJ85"},
    {"RJ1H","Avro RJ100"},
    {"B461","BAe 146-100"},
    {"B462","BAe 146-200"},
    {"B463","BAe 146-300"},
    {"D328","Dornier 328"},
    {"J328","Dornier 328JET"},
    {"D228","Dornier 228"},
    {"DHC6","DHC-6 Twin Otter"},
    {"B190","Beech 1900"},
    {"JS41","Jetstream 41"},
    {"SU95","Sukhoi Superjet"},
    {"C919","COMAC C919"},
    {"AJ27","COMAC ARJ21"},
    // Fracht & Schwer
    {"A124","Antonov An-124"},
    {"AN12","Antonov An-12"},
    {"IL76","Ilyushin Il-76"},
    // Russland & GUS
    {"T154","Tupolev Tu-154"},
    {"T204","Tupolev Tu-204"},
    {"T214","Tupolev Tu-214"},
    {"IL62","Ilyushin Il-62"},
    {"IL96","Ilyushin Il-96"},
    {"A148","Antonov An-148"},
    {"AN24","Antonov An-24"},
    {"AN26","Antonov An-26"},
    {"AN72","Antonov An-72"},
    {"YK40","Yakovlev Jak-40"},
    {"YK42","Yakovlev Jak-42"},
    {"MI8","Mil Mi-8"},
    // Militär
    {"C17","Boeing C-17A"},
    {"A400","Airbus A400M"},
    {"C130","Lockheed C-130"},
    {"C30J","Lockheed C-130J"},
    {"C5M","Lockheed C-5M"},
    {"K35R","Boeing KC-135R"},
    {"E3TF","Boeing E-3 AWACS"},
    {"P8","Boeing P-8A"},
    {"P3","Lockheed P-3 Orion"},
    {"C295","Airbus C295"},
    {"CN35","CASA CN-235"},
    {"C160","Transall C-160"},
    {"EUFI","Eurofighter"},
    {"TOR","Panavia Tornado"},
    {"F35","Lockheed F-35"},
    {"F16","Lockheed F-16"},
    {"B52","Boeing B-52"},
    {"B1","Boeing B-1B"},
    {"B2","Northrop B-2"},
    {"R135","Boeing RC-135"},
    {"E6","Boeing E-6 Mercury"},
    {"C5","Lockheed C-5"},
    {"U2","Lockheed U-2"},
    {"F15","Boeing F-15"},
    {"F18","Boeing F/A-18"},
    {"F22","Lockheed F-22"},
    {"A10","Fairchild A-10"},
    {"E2","Northrop E-2"},
    {"T38","Northrop T-38"},
    {"H64","Boeing AH-64"},
    {"H53","Sikorsky CH-53"},
    {"V22","Bell Boeing V-22"},
    {"F5","Northrop F-5"},
    {"F4","McDonnell F-4"},
    {"F14","Grumman F-14"},
    {"F104","Lockheed F-104"},
    {"RFAL","Dassault Rafale"},
    {"MIR2","Mirage 2000"},
    {"SU27","Sukhoi Su-27"},
    {"MG29","Mikoyan MiG-29"},
    // Jet trainers, private and airshow jets, warbirds
    {"L39","Aero L-39"},
    {"L159","Aero L-159"},
    {"L29","Aero L-29"},
    {"HAWK","BAe Hawk"},
    {"AJET","Alpha Jet"},
    {"M346","Leonardo M-346"},
    {"MG15","Mikoyan MiG-15"},
    {"MG17","Mikoyan MiG-17"},
    {"MG21","Mikoyan MiG-21"},
    {"HUNT","Hawker Hunter"},
    {"JPRO","Jet Provost"},
    {"VAMP","DH Vampire"},
    {"F86","F-86 Sabre"},
    {"A4","Douglas A-4"},
    {"PC7","Pilatus PC-7"},
    {"PC9","Pilatus PC-9"},
    {"TEX2","Beechcraft T-6"},
    {"T6","North American T-6"},
    {"P51","P-51 Mustang"},
    {"SPIT","Spitfire"},
    {"HURI","Hawker Hurricane"},
    {"YK52","Yakovlev Jak-52"},
    {"E300","Extra 300"},
    {"DC3","Douglas DC-3"},
    {"JU52","Junkers Ju 52"},
    // Business
    {"C510","Citation Mustang"},
    {"C525","Citation CJ1"},
    {"C25A","Citation CJ2"},
    {"C25B","Citation CJ3"},
    {"C25C","Citation CJ4"},
    {"C56X","Citation Excel/XLS"},
    {"C68A","Citation Latitude"},
    {"C700","Citation Longitude"},
    {"C750","Citation X"},
    {"E50P","Embraer Phenom 100"},
    {"E55P","Embraer Phenom 300"},
    {"E35L","Legacy 600/650"},
    {"CL30","Challenger 300"},
    {"CL35","Challenger 350"},
    {"CL60","Challenger 6XX"},
    {"GL5T","Global 5000"},
    {"GLEX","Global Express"},
    {"GL7T","Global 7500"},
    {"LJ35","Learjet 35"},
    {"LJ45","Learjet 45"},
    {"LJ60","Learjet 60"},
    {"LJ75","Learjet 75"},
    {"F2TH","Falcon 2000"},
    {"FA50","Falcon 50"},
    {"F900","Falcon 900"},
    {"FA7X","Falcon 7X"},
    {"FA8X","Falcon 8X"},
    {"GLF4","Gulfstream IV/G450"},
    {"GLF5","Gulfstream V/G550"},
    {"GLF6","Gulfstream G650"},
    {"G280","Gulfstream G280"},
    {"GA5C","Gulfstream G500"},
    {"GA6C","Gulfstream G600"},
    {"H25B","Hawker 800/900"},
    {"HDJT","HondaJet"},
    {"PC24","Pilatus PC-24"},
    {"SF50","Cirrus Vision Jet"},
    // Allgemeine Luftfahrt
    {"C152","Cessna 152"},
    {"C172","Cessna 172"},
    {"C182","Cessna 182"},
    {"C206","Cessna 206"},
    {"C208","Cessna 208 Caravan"},
    {"P28A","Piper PA-28"},
    {"PA34","Piper PA-34 Seneca"},
    {"SR20","Cirrus SR20"},
    {"SR22","Cirrus SR22"},
    {"DA20","Diamond DA20"},
    {"DA40","Diamond DA40"},
    {"DA42","Diamond DA42"},
    {"DA62","Diamond DA62"},
    {"PC12","Pilatus PC-12"},
    {"PC21","Pilatus PC-21"},
    {"BE9L","King Air 90"},
    {"BE20","King Air 200/250"},
    {"B350","King Air 350"},
    {"TBM7","Daher TBM 700"},
    {"TBM8","Daher TBM 850"},
    {"TBM9","Daher TBM 900-960"},
    {"GLID","Segelflugzeug"},
    {"BALL","Ballon"},
    {"ULAC","Ultraleicht"},
    // Hubschrauber
    {"EC20","Airbus H120"},
    {"AS50","Airbus H125/AS350"},
    {"EC30","Airbus H130"},
    {"EC35","Airbus H135"},
    {"EC45","Airbus H145"},
    {"EC55","Airbus H155"},
    {"EC75","Airbus H175"},
    {"BK17","MBB BK 117"},
    {"A109","Leonardo AW109"},
    {"A139","Leonardo AW139"},
    {"A169","Leonardo AW169"},
    {"A189","Leonardo AW189"},
    {"NH90","NH90"},
    {"H60","Sikorsky H-60"},
    {"S92","Sikorsky S-92"},
    {"CH47","Boeing CH-47"},
    {"B06","Bell 206"},
    {"B407","Bell 407"},
    {"B412","Bell 412"},
    {"R22","Robinson R22"},
    {"R44","Robinson R44"},
  };
  if (!code || !*code || !out || !cap) return false;
  for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
    AircraftTypeName e; memcpy_P(&e, &table[i], sizeof(e));
    if (!strcmp(code, e.code)) { strncpy(out, e.name, cap - 1); out[cap - 1] = 0; return true; }
  }
  return false;
}

// Short form "Manufacturer Model" for long provider names (when the code is
// not in the table): "Northrop Grumman E-2D Hawkeye" -> "Northrop E-2D".
// The model is the first word after the manufacturer that contains a digit.
inline bool aircraftMakerModel(const char* full, char* out, size_t cap) {
  static const char makers[][12] PROGMEM = {
    "Airbus","Boeing","Embraer","Cessna","Bombardier","Dassault","Gulfstream","Lockheed","Northrop",
    "Grumman","McDonnell","Douglas","Pilatus","Beechcraft","Beech","Piper","Diamond","Cirrus","Daher",
    "Socata","Learjet","Hawker","Textron","Honda","Mitsubishi","Fairchild","Antonov","Ilyushin",
    "Tupolev","Sukhoi","Mikoyan","Yakovlev","Saab","Fokker","Dornier","Sikorsky","Bell","Robinson",
    "Eurocopter","Agusta","Leonardo","Aermacchi","Tecnam","Extra","Zlin","Aero"};
  if (!full || !out || cap < 2) return false;
  for (size_t i = 0; i < sizeof(makers) / sizeof(makers[0]); i++) {
    char maker[12]; memcpy_P(maker, makers[i], sizeof(maker));
    const char* at = strstr(full, maker);
    size_t ml = strlen(maker);
    if (!at || (at != full && at[-1] != ' ' && at[-1] != '-') || (at[ml] != ' ' && at[ml] != 0)) continue;
    const char* p = at + ml;
    for (uint8_t word = 0; word < 3 && *p; word++) {
      while (*p == ' ') p++;
      const char* end = p; bool digit = false;
      while (*end && *end != ' ') { if (*end >= '0' && *end <= '9') digit = true; end++; }
      if (digit && end > p) {
        size_t n = (size_t)(end - p);
        if (ml + 1 + n + 1 > cap) return false;
        memcpy(out, maker, ml); out[ml] = ' '; memcpy(out + ml + 1, p, n); out[ml + 1 + n] = 0;
        return true;
      }
      p = end;
    }
  }
  return false;
}
