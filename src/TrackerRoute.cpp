#include "Version.h"
#include "RequestPolicy.h"
#include "TrackerRoute.h"
#include "RadarPolicy.h"
#include "Diagnostics.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "NetworkJson.h"
#include "AdsbLol.h"
#include <math.h>

static uint16_t g_adsbLolRx = 0;

static uint16_t adsbLolRx() {
  if (g_adsbLolRx) return g_adsbLolRx;
  if (BearSSL::WiFiClientSecure::probeMaxFragmentLength("api.adsb.lol", 443, 512)) g_adsbLolRx = 512;
  else if (BearSSL::WiFiClientSecure::probeMaxFragmentLength("api.adsb.lol", 443, 1024)) g_adsbLolRx = 1024;
  else g_adsbLolRx = 4096;
  return g_adsbLolRx;
}

static bool tlsHeapReady(uint16_t rx) {
  return radarTlsFits(ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(), rx, sizeof(br_ssl_client_context));
}

static void makeSecure(BearSSL::WiFiClientSecure& client, BearSSL::Session* session, uint16_t rxBuf = 4096) {
  client.setInsecure();
  client.setBufferSizes(rxBuf, 512);
  client.setSession(session);
  client.setTimeout(9000);
}

static void routeFallback(char outOrigin[ROUTE_CODE_SIZE], char outDestination[ROUTE_CODE_SIZE]) {
  strlcpy(outOrigin, "N/A", ROUTE_CODE_SIZE);
  strlcpy(outDestination, "N/A", ROUTE_CODE_SIZE);
}

static bool routeComplete(const char origin[ROUTE_CODE_SIZE], const char destination[ROUTE_CODE_SIZE]) {
  return routeCode(origin) && routeCode(destination);
}

static bool lookupRouteAdsbLol(const char* callsign, float lat, float lon, char origin[ROUTE_CODE_SIZE], char destination[ROUTE_CODE_SIZE], FlightRoute& chain) {
  DiagRequest trace(D_TRACK_ROUTE);trace.stage(DS_TLS);
  if (serviceWaitMs(SERVICE_ADSB_LOL)||!tlsHeapReady(g_adsbLolRx ? g_adsbLolRx : 512)) return false;
  const uint16_t rx=adsbLolRx();
  if (!tlsHeapReady(rx)) return false;
  BearSSL::Session routeSession;
  BearSSL::WiFiClientSecure client;
  makeSecure(client, &routeSession, rx);
  HTTPClient http;
  http.setTimeout(9000);
  http.setReuse(false);
  if (!http.begin(client, "https://api.adsb.lol/api/0/routeset")) return false;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  adsbLolPrepare(http, FW_USER_AGENT);

  JsonDocument req(networkJsonAllocator());
  JsonArray planes = req["planes"].to<JsonArray>();
  JsonObject p = planes.add<JsonObject>();
  p["callsign"] = callsign;
  p["lat"] = lat;
  p["lng"] = lon;
  String body;
  serializeJson(req, body);

  if(serviceWaitMs(SERVICE_ADSB_LOL)){http.end();return false;}
  int code = http.POST(body);
  req.clear(); body = String();trace.http(code);
  serviceResponse(SERVICE_ADSB_LOL,code,http.header("Retry-After").c_str());
  if (code != HTTP_CODE_OK) { http.end(); return false; }

  JsonDocument filter(networkJsonAllocator());
  JsonArray filterArray = filter.to<JsonArray>();
  JsonObject filterItem = filterArray.add<JsonObject>();
  filterItem["_airport_codes_iata"] = true;
  JsonDocument doc(networkJsonAllocator());
  trace.stage(DS_JSON);
  if(!jsonFilterReady(filter,trace)){http.end();client.stop();return false;}
  AdsbLolBody routeBody(http);
  auto err = deserializeJson(doc, routeBody, DeserializationOption::Filter(filter));
  logJsonError(trace,err); filter.clear(); http.end(); client.stop();
  if (err || !doc.is<JsonArrayConst>() || doc.as<JsonArrayConst>().size() == 0) return false;

  String route = doc[0]["_airport_codes_iata"] | "";
  route.trim();
  if (!route.length() || route == "unknown") return false;
  if (!parseFlightRoute(route.c_str(), chain)) return false;
  strlcpy(origin, chain.airports[0], ROUTE_CODE_SIZE);
  // Preserve the true final airport even if the bounded display chain overflows.
  String last=route.substring(route.lastIndexOf('-')+1);last.trim();last.toUpperCase();
  strlcpy(destination, last.c_str(), ROUTE_CODE_SIZE);
  bool ok=routeComplete(origin,destination);trace.finish(ok);return ok;
}

// adsbdb: IATA code, or the ICAO code if the airport has none.
static String airportCode(JsonObjectConst ap) {
  String code = ap["iata_code"] | "";
  code.trim();
  if (!code.length()) { code = ap["icao_code"] | ""; code.trim(); }
  code.toUpperCase();
  return code;
}

static bool lookupRouteAdsbDb(const char* callsign, char origin[ROUTE_CODE_SIZE], char destination[ROUTE_CODE_SIZE], FlightRoute& chain) {
  DiagRequest trace(D_TRACK_ROUTE);trace.stage(DS_TLS);
  if (serviceWaitMs(SERVICE_ADSB_DB)||!tlsHeapReady(4096)) return false;
  BearSSL::Session routeSession;
  BearSSL::WiFiClientSecure client;
  makeSecure(client, &routeSession);
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(9000);
  http.setReuse(false);
  String url = F("https://api.adsbdb.com/v0/callsign/");
  url += callsign;
  if (!http.begin(client, url)) return false;
  http.addHeader("Accept", "application/json");
  http.setUserAgent(FW_USER_AGENT);

  const char* headers[]={"Retry-After"};http.collectHeaders(headers,1);
  int code = http.GET();trace.http(code);
  serviceResponse(SERVICE_ADSB_DB,code,http.header("Retry-After").c_str());
  if (code != HTTP_CODE_OK) { http.end(); return false; }

  JsonDocument filter(networkJsonAllocator());
  for (const char* key : {"origin", "destination", "midpoint"}) {
    filter["response"]["flightroute"][key]["iata_code"] = true;
    filter["response"]["flightroute"][key]["icao_code"] = true;   // for airfields without IATA code
  }
  JsonDocument doc(networkJsonAllocator());
  trace.stage(DS_JSON);
  if(!jsonFilterReady(filter,trace)){http.end();client.stop();return false;}
  auto err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  logJsonError(trace,err); filter.clear(); http.end(); client.stop();
  if (err) return false;

  String a = airportCode(doc["response"]["flightroute"]["origin"]);
  String b = airportCode(doc["response"]["flightroute"]["destination"]);
  if (routeCode(a.c_str())) strlcpy(origin, a.c_str(), ROUTE_CODE_SIZE);
  if (routeCode(b.c_str())) strlcpy(destination, b.c_str(), ROUTE_CODE_SIZE);
  if (!routeComplete(origin,destination)) return false;
  chain={};appendRouteAirport(chain,origin);
  String midpoint=airportCode(doc["response"]["flightroute"]["midpoint"]);
  if(midpoint.length())appendRouteAirport(chain,midpoint.c_str());
  appendRouteAirport(chain,destination);
  trace.finish(true);return true;
}

static bool performRouteLookup(const char* callsign, float lat, float lon, char origin[ROUTE_CODE_SIZE], char destination[ROUTE_CODE_SIZE], FlightRoute& chain) {
  routeFallback(origin, destination);
  chain={};
  bool ok = !isnan(lat) && !isnan(lon) && lookupRouteAdsbLol(callsign, lat, lon, origin, destination, chain);
  if (!ok) {
    routeFallback(origin, destination);
    ok = lookupRouteAdsbDb(callsign, origin, destination, chain);
  }
  return ok;
}

bool trackerRouteLookup(const char* callsign,float lat,float lon,FlightRoute& route,char origin[ROUTE_CODE_SIZE],char destination[ROUTE_CODE_SIZE]) {
  if(!callsign||strlen(callsign)<3||WiFi.status()!=WL_CONNECTED )return false;
  return performRouteLookup(callsign,lat,lon,origin,destination,route);
}
