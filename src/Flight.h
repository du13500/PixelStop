#pragma once
#include <Arduino.h>
#include "FlightRoute.h"
#include "RadarMotion.h"

struct FlightSnapshot {
  bool valid;
  bool error;
  char hex[8]; // ICAO address identifies the aircraft even when callsign changes.
  char callsign[12];
  char airline[64];
  char registration[16];
  char typeCode[8];
  char category[4];
  char typeName[80];
  char origin[ROUTE_CODE_SIZE];
  char destination[ROUTE_CODE_SIZE];
  FlightRoute route;
  float distanceKm;
  int32_t altitudeFt;
  bool altitudeKnown;
  bool onGround;
  RadarMotion motion; // for estimates between requests
  char squawk[5];     // transponder code, "7700" etc.
  bool military;      // adsb.lol database flag
  bool speedKnown;
  float speedKt;      // ground speed
};

void flightReset();
// visibleShare: fraction of the time the radar is on screen (for the daily budget).
bool flightPollDue(uint16_t pollSec, float visibleShare = 1.0f);
// hideMask (RadarFilter.h) > 0 asks for all aircraft in the radius and picks the
// nearest one that is not hidden; 0 asks adsb.lol for the nearest aircraft only.
bool flightFetch(float homeLat, float homeLon, uint16_t radiusKm, uint8_t hideMask = 0);
// Aircraft examined in the last filtered request (0 without filter).
uint16_t flightLastScanned();
const FlightSnapshot& flightData();
// Distance and altitude continued from speed, track and vertical rate.
float flightLiveDistanceKm(const FlightSnapshot& f);
int32_t flightLiveAltitudeFt(const FlightSnapshot& f);
// Current request interval (selection, minimum interval and daily budget).
uint32_t flightPollIntervalMs();
uint32_t flightLastOkMs();
bool flightProviderPaused();
const char* flightStageName();
int flightLastHttp();
uint16_t flightTlsRx();

bool flightExpire();
uint32_t flightTlsMinBlock();

uint32_t flightWaitSec();
const char* flightRouteStatus();
const char* flightRouteSource();
int flightRouteHttp();

bool flightEnrichmentDue();
void flightEnrich();
