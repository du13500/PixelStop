#pragma once
#include <Arduino.h>
#include "FlightRoute.h"

// Tracker enrichment is independent of the restored radar implementation.
bool trackerRouteLookup(const char* callsign, float lat, float lon, FlightRoute& route,
                        char origin[ROUTE_CODE_SIZE], char destination[ROUTE_CODE_SIZE]);
