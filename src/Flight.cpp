#include "Version.h"
#include "Flight.h"
#include "AircraftPolicy.h"
#include "AircraftLabel.h"
#include "AirlineName.h"
#include "RadarFilter.h"
#include "RadarScan.h"
#include "RadarPolicy.h"
#include "RequestPolicy.h"
#include "RoutePolicy.h"
#include "Diagnostics.h"
#include "NetworkGuard.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "NetworkJson.h"
#include "AdsbLol.h"
#include <math.h>

static FlightSnapshot g_f = {};
static uint32_t g_lastPollMs = 0;
// At most one adsb.lol route lookup between two position requests (never starve positions).
static bool g_routeSincePoll = false;
static bool g_hasPolled = false;
static uint32_t g_lastOkMs = 0;
// A provider cooldown retains the last aircraft until a new request completes.
static bool g_providerPaused = false;
static uint32_t g_firstFailureMs = 0;
static uint8_t g_consecutiveFailures = 0;
static BearSSL::Session g_adsbSession;
static BearSSL::Session g_routeSession;
static BearSSL::Session g_adsbDbSession;
static uint16_t g_adsbDbRx=0;
static uint16_t g_adsbLolRx = 0;
static int g_lastHttp = 0;
static const char* g_stage = "bereit";
struct Enrichment {
  char hex[8],callsign[12],airline[64],type[80],typeCode[8];
  FlightRoute route; RoutePoint airports[8]; uint8_t airportCount;
  uint32_t checked; bool used; const char* source; uint8_t phase;
};
static Enrichment g_cache[3]={};
static RoutePoint g_position={NAN,NAN};
static uint8_t g_cacheNext=0;
static uint32_t g_lastEnrichmentMs=0;
static const char* g_routeStatus="nicht geprüft";
static const char* g_routeSource="keine";
static int g_routeHttp=0;
static uint16_t g_scanned=0;
static float g_homeLat=NAN,g_homeLon=NAN;
static uint32_t g_intervalMs=0;

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
  client.setTimeout(5000);
}

static void trimTail(char* s) {
  int n = strlen(s);
  while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t')) s[--n] = 0;
}

static float distanceKm(float lat1, float lon1, float lat2, float lon2) {
  return radarFlatDistanceKm(lat1, lon1, lat2, lon2);
}

static RoutePoint jsonPoint(JsonObjectConst o,const char* latKey,const char* lonKey) {
  return {o[latKey].is<float>()?o[latKey].as<float>():NAN,o[lonKey].is<float>()?o[lonKey].as<float>():NAN};
}

// Descent rule (RoutePolicy.h), checked once when a route arrives.
static bool routeContradicted(const FlightSnapshot& f,RoutePoint pos,const RoutePoint* airports,uint8_t count){
  return count>=2&&routeOriginContradicted(pos,airports[0],airports[count-1],f.onGround,f.altitudeKnown,f.altitudeFt,f.motion.hasRate,f.motion.rateFpm);
}

static bool lookupRouteAdsbLol(const FlightSnapshot& f,RoutePoint pos,Enrichment& out) {
  // Skipped requests are logged with a reason (value): 1 quota, 2 pause, 4 memory.
  // A route lookup takes the place of the next position request (savings mode).
  uint8_t skip=adsbLolQuotaWaitMs()?1:(serviceWaitMs(SERVICE_ADSB_LOL)?2:(!tlsHeapReady(adsbLolRx())?4:0));
  if(skip){diagLookup(f.hex,f.callsign);diagEvent(D_RADAR_ROUTE,DS_SKIPPED,0,skip);return false;}
  g_lastPollMs=millis();g_routeSincePoll=true;
  DiagRequest trace(D_RADAR_ROUTE);diagLookup(f.hex,f.callsign);trace.stage(DS_TLS);
  BearSSL::WiFiClientSecure client;makeSecure(client,&g_routeSession,adsbLolRx());
  HTTPClient http;http.setTimeout(5000);http.setReuse(false);
  if(!http.begin(client,"https://api.adsb.lol/api/0/routeset"))return false;
  adsbLolPrepare(http,FW_USER_AGENT);
  http.addHeader("Content-Type","application/json");
  JsonDocument request(networkJsonAllocator());JsonObject plane=request["planes"].to<JsonArray>().add<JsonObject>();
  plane["callsign"]=f.callsign;plane["lat"]=pos.lat;plane["lng"]=pos.lon;
  if(request.overflowed()){trace.stage(DS_JSON_MEMORY);http.end();return false;}
  String body;serializeJson(request,body);
  int code=http.POST(body);request.clear();body=String();trace.http(code);g_routeHttp=code;
  serviceResponse(SERVICE_ADSB_LOL,code,http.header("Retry-After").c_str());
  if(serviceWaitMs(SERVICE_ADSB_LOL))diagEvent(D_RADAR,DS_WAIT,code,serviceWaitMs(SERVICE_ADSB_LOL));
  if(code!=200){if(code==201)trace.finish(true,DS_EMPTY);http.end();client.stop();return false;}
  JsonDocument filter(networkJsonAllocator());JsonObject item=filter.to<JsonArray>().add<JsonObject>();
  item["callsign"]=true;item["plausible"]=true;item["_airport_codes_iata"]=true;
  JsonObject ap=item["_airports"].to<JsonArray>().add<JsonObject>();ap["lat"]=true;ap["lon"]=true;
  trace.stage(DS_JSON);JsonDocument doc(networkJsonAllocator());if(!jsonFilterReady(filter,trace)){http.end();client.stop();return false;}AdsbLolBody routeBody(http);auto err=deserializeJson(doc,routeBody,DeserializationOption::Filter(filter));logJsonError(trace,err); filter.clear(); http.end(); client.stop();
  if(err)return false;
  // From here the answer was read; "leer" = no usable route for this flight.
  if(!doc.is<JsonArray>()||!doc.size()){trace.finish(true,DS_EMPTY);return false;}
  JsonObjectConst r=doc[0];
  if(strcmp(r["callsign"]|"",f.callsign)){trace.finish(true,DS_ROUTE_CALLSIGN);return false;}
  if(!r["plausible"].is<bool>()||!r["plausible"].as<bool>()){trace.finish(true,DS_ROUTE_PROVIDER);return false;}
  FlightRoute route={};if(!parseFlightRoute(r["_airport_codes_iata"]|"",route)||route.truncated){trace.finish(true,DS_EMPTY);return false;}
  JsonArrayConst airports=r["_airports"];if(airports.size()!=route.count){trace.finish(true,DS_EMPTY);return false;}
  for(uint8_t i=0;i<route.count;i++)out.airports[i]=jsonPoint(airports[i],"lat","lon");
  if(!routePositionPlausible(pos,out.airports,route.count,f.onGround,f.altitudeKnown,f.altitudeFt)){trace.finish(true,DS_ROUTE_POSITION);return false;}
  if(routeContradicted(f,pos,out.airports,route.count)){trace.finish(true,DS_ROUTE_DESCENT);return false;}
  out.route=route;out.airportCount=route.count;out.source="adsb.lol";trace.finish(true);return true;
}

static bool lookupMetadata(const FlightSnapshot& f,RoutePoint pos,Enrichment& out) {
  if(serviceWaitMs(SERVICE_ADSB_DB)){diagLookup(f.hex,f.callsign);diagEvent(D_METADATA,DS_SKIPPED,0,2);return false;}
  if(!tlsHeapReady(networkTlsRx("api.adsbdb.com",g_adsbDbRx,D_METADATA))){diagLookup(f.hex,f.callsign);diagEvent(D_METADATA,DS_SKIPPED,0,4);return false;}
  String id=f.hex[0]?String(f.hex):String(f.registration);
  String url=id.length()?String("https://api.adsbdb.com/v0/aircraft/")+id+"?callsign="+f.callsign:
    String("https://api.adsbdb.com/v0/callsign/")+f.callsign;
  DiagRequest trace(D_METADATA);diagLookup(id.c_str(),f.callsign);trace.stage(DS_TLS);
  BearSSL::WiFiClientSecure client;makeSecure(client,&g_adsbDbSession,g_adsbDbRx?g_adsbDbRx:4096);
  HTTPClient http;http.useHTTP10(true);http.setTimeout(5000);http.setReuse(false);
  if(!http.begin(client,url))return false;
  const char* headers[]={"Retry-After"};http.collectHeaders(headers,1);http.setUserAgent(FW_USER_AGENT);
  int code=http.GET();trace.http(code);g_routeHttp=code;serviceResponse(SERVICE_ADSB_DB,code,http.header("Retry-After").c_str());
  // 404: adsbdb does not know the aircraft - an answer, not an error.
  if(code!=200){if(code==404)trace.finish(true,DS_EMPTY);http.end();return false;}
  JsonDocument filter(networkJsonAllocator());filter["response"]["aircraft"]["type"]=true;filter["response"]["aircraft"]["icao_type"]=true;
  filter["response"]["aircraft"]["mode_s"]=true;
  JsonObject routeFilter=filter["response"]["flightroute"].to<JsonObject>();
  routeFilter["callsign_icao"]=true;routeFilter["callsign"]=true;
  routeFilter["airline"]["name"]=true;routeFilter["airline"]["icao"]=true;
  for(const char* key:{"origin","midpoint","destination"}){
    routeFilter[key]["iata_code"]=true;routeFilter[key]["icao_code"]=true;routeFilter[key]["latitude"]=true;routeFilter[key]["longitude"]=true;
  }
  trace.stage(DS_JSON);JsonDocument doc(networkJsonAllocator());if(!jsonFilterReady(filter,trace)){http.end();client.stop();return false;}auto err=deserializeJson(doc,http.getStream(),DeserializationOption::Filter(filter));logJsonError(trace,err); filter.clear(); http.end(); client.stop();if(err)return false;
  trace.finish(true);
  JsonObjectConst aircraft=doc["response"]["aircraft"];
  const char* hex=aircraft["mode_s"]|"";const char* typeCode=aircraft["icao_type"]|"";
  if((!f.hex[0]||!strcasecmp(hex,f.hex))&&metadataTypeMatches(f.typeCode,f.category,typeCode)){
    strlcpy(out.type,aircraft["type"]|"",sizeof(out.type));
    strlcpy(out.typeCode,typeCode,sizeof(out.typeCode));
  }
  JsonObjectConst route=doc["response"]["flightroute"];
  const char* cs=route["callsign_icao"]|(route["callsign"]|"");
  if(!*cs)return true;
  if(strcmp(cs,f.callsign)){diagEvent(D_METADATA,DS_ROUTE_CALLSIGN);return true;}
  const char* airlineCode=route["airline"]["icao"]|"";
  if(strlen(f.callsign)>=3&&strlen(airlineCode)==3&&!strncmp(f.callsign,airlineCode,3))
    strlcpy(out.airline,route["airline"]["name"]|"",sizeof(out.airline));
  FlightRoute candidate={};RoutePoint points[3];uint8_t count=0;
  for(const char* key:{"origin","midpoint","destination"}){
    // IATA code, or the ICAO code for airfields without one.
    JsonObjectConst ap=route[key];const char* code=ap["iata_code"]|"";if(!*code)code=ap["icao_code"]|"";
    if(!*code&& !strcmp(key,"midpoint"))continue;
    if(!appendRouteAirport(candidate,code))return true;
    points[count++]=jsonPoint(ap,"latitude","longitude");
  }
  if(!out.route.count){
    if(!routePositionPlausible(pos,points,count,f.onGround,f.altitudeKnown,f.altitudeFt))diagEvent(D_METADATA,DS_ROUTE_POSITION);
    else if(routeContradicted(f,pos,points,count))diagEvent(D_METADATA,DS_ROUTE_DESCENT);
    else{out.route=candidate;out.airportCount=count;for(uint8_t i=0;i<count;i++)out.airports[i]=points[i];out.source="adsbdb";}
  }
  return true;
}

static void lookupAirline(const FlightSnapshot& f,Enrichment& out) {
  char brand[19];
  if(out.airline[0]||strlen(f.callsign)<4||airlineBrand(f.callsign,brand,sizeof(brand)))return;
  for(uint8_t i=0;i<3;i++)if(f.callsign[i]<'A'||f.callsign[i]>'Z')return;
  String registration=f.registration;registration.replace("-","");
  if(registration==f.callsign||!strcmp(f.hex,f.callsign))return;
  struct AirlineCache { char prefix[4],name[64];uint32_t checked;bool used; };
  static AirlineCache cache[4]={};static uint8_t slot=0;AirlineCache* entry=nullptr;
  char prefix[4]={f.callsign[0],f.callsign[1],f.callsign[2],0};
  for(auto& c:cache)if(c.used&&!strcmp(c.prefix,prefix)){entry=&c;break;}
  if(entry&&(uint32_t)(millis()-entry->checked)<1800000UL){strlcpy(out.airline,entry->name,sizeof(out.airline));return;}
  if(serviceWaitMs(SERVICE_ADSB_DB)||!tlsHeapReady(networkTlsRx("api.adsbdb.com",g_adsbDbRx,D_METADATA)))return;
  if(!entry){entry=&cache[slot];slot=(slot+1)%4;}
  DiagRequest trace(D_AIRLINE);trace.stage(DS_TLS);
  BearSSL::WiFiClientSecure client;makeSecure(client,&g_adsbDbSession,g_adsbDbRx?g_adsbDbRx:4096);
  HTTPClient http;http.useHTTP10(true);http.setTimeout(5000);http.setReuse(false);
  if(!http.begin(client,String("https://api.adsbdb.com/v0/airline/")+prefix))return;
  const char* headers[]={"Retry-After"};http.collectHeaders(headers,1);http.setUserAgent(FW_USER_AGENT);
  int code=http.GET();trace.http(code);serviceResponse(SERVICE_ADSB_DB,code,http.header("Retry-After").c_str());
  if(code!=200){http.end();return;}
  JsonDocument filter(networkJsonAllocator());filter["response"][0]["name"]=true;filter["response"][0]["icao"]=true;
  trace.stage(DS_JSON);JsonDocument doc(networkJsonAllocator());if(!jsonFilterReady(filter,trace)){http.end();client.stop();return;}auto err=deserializeJson(doc,http.getStream(),DeserializationOption::Filter(filter));logJsonError(trace,err); filter.clear(); http.end(); client.stop();if(err)return;
  for(JsonObjectConst airline:doc["response"].as<JsonArrayConst>())if(!strcmp(airline["icao"]|"",prefix)){
    const char* name=airline["name"]|"";if(!*name)break;
    *entry={};entry->used=true;entry->checked=millis();strlcpy(entry->prefix,prefix,4);
    strlcpy(entry->name,name,sizeof(entry->name));strlcpy(out.airline,entry->name,sizeof(out.airline));trace.finish(true);break;
  }
}

static void enrichFlight(FlightSnapshot& f,RoutePoint pos) {
  g_routeStatus="nicht verfügbar";g_routeSource="keine";
  f.route={};strlcpy(f.origin,"N/A",sizeof(f.origin));strlcpy(f.destination,"N/A",sizeof(f.destination));
  if(!routePointValid(pos)||!f.callsign[0])return;
  Enrichment* entry=nullptr;
  for(auto& e:g_cache)if(e.used&&!strcmp(e.hex,f.hex)&&!strcmp(e.callsign,f.callsign)){entry=&e;break;}
  uint32_t now=millis();
  // Network work is scheduled separately, never inside position parsing.
  if(!entry)return;
  char brand[19];
  if(entry->airline[0]&&!airlineBrand(f.callsign,brand,sizeof(brand)))strlcpy(f.airline,entry->airline,sizeof(f.airline));
  if(entry->type[0]&&metadataTypeMatches(f.typeCode,f.category,entry->typeCode))strlcpy(f.typeName,entry->type,sizeof(f.typeName));
  // Routes stay valid between the 10-minute checks; the position check still applies.
  if(entry->route.count && (uint32_t)(now-entry->checked)<900000UL &&
      routePositionPlausible(pos,entry->airports,entry->airportCount,f.onGround,f.altitudeKnown,f.altitudeFt)){
    f.route=entry->route;strlcpy(f.origin,f.route.airports[0],sizeof(f.origin));
    strlcpy(f.destination,f.route.airports[f.route.count-1],sizeof(f.destination));
    g_routeStatus="positionsgeprüft";g_routeSource=entry->source;
  } else if(entry->route.count)g_routeStatus="verworfen: Position/Alter";
}

static Enrichment* currentEnrichment(){
 for(auto& e:g_cache)if(e.used&&!strcmp(e.hex,g_f.hex)&&!strcmp(e.callsign,g_f.callsign))return &e;
 return nullptr;
}
// The adsb.lol route lookup replaces a position request: it needs a free slot
// in the quota and keeps the minimum distance after a provider limit.
static bool routeSlotFree(){
  if(g_routeSincePoll||adsbLolWaitMs())return false;
  uint32_t minimum=serviceMinIntervalMs(SERVICE_ADSB_LOL);
  return !minimum||(uint32_t)(millis()-g_lastPollMs)>=minimum;
}
bool flightEnrichmentDue(){
 if(!g_f.valid||g_f.error||!routePointValid(g_position)||!g_f.callsign[0])return false;
 Enrichment* e=currentEnrichment();
 if(e&&e->phase<3)return e->phase==0?routeSlotFree():!serviceWaitMs(SERVICE_ADSB_DB);
 if(g_lastEnrichmentMs&&(uint32_t)(millis()-g_lastEnrichmentMs)<60000UL)return false;
 // New aircraft: full lookup including adsb.lol. The same aircraft later: adsbdb
 // only (no adsb.lol quota), every 10 minutes, keeping what was found.
 if(!e)return routeSlotFree();
 return (uint32_t)(millis()-e->checked)>=600000UL&&!serviceWaitMs(SERVICE_ADSB_DB);
}
void flightEnrich(){
 if(!flightEnrichmentDue())return;
 Enrichment* e=currentEnrichment();
 if(!e){
   e=&g_cache[g_cacheNext];g_cacheNext=(g_cacheNext+1)%3;
   *e={};e->used=true;e->checked=millis();g_lastEnrichmentMs=e->checked;
   strlcpy(e->hex,g_f.hex,sizeof(e->hex));strlcpy(e->callsign,g_f.callsign,sizeof(e->callsign));
 }else if(e->phase>=3){e->phase=1;e->checked=millis();g_lastEnrichmentMs=e->checked;}
 if(e->phase==0)lookupRouteAdsbLol(g_f,g_position,*e);
 else if(e->phase==1)lookupMetadata(g_f,g_position,*e);
 else lookupAirline(g_f,*e);
 ++e->phase;
 enrichFlight(g_f,g_position);
 if(serviceWaitMs(SERVICE_ADSB_LOL)){g_providerPaused=true;g_f.error=true;g_stage="Abrufpause: Anbieter";}
}

void flightReset() {
  memset(&g_f, 0, sizeof(g_f));
  g_lastPollMs = 0;
  g_hasPolled = false;
  g_lastOkMs = 0;
  g_providerPaused = false;
  g_firstFailureMs = 0;
  g_consecutiveFailures = 0;
  // Keep bounded enrichment and provider cooldowns when saving settings.
  g_lastHttp = 0;
  g_stage = "bereit";
}

bool flightPollDue(uint16_t pollSec, float visibleShare) {
  if(adsbLolWaitMs())return false;
  // The first route lookup of a new aircraft goes first and takes this slot.
  {Enrichment* e=currentEnrichment();if((!e||e->phase==0)&&flightEnrichmentDue())return false;}
  if (!g_hasPolled) return true;
  if(pollSec<RADAR_MIN_POLL_SEC)pollSec=RADAR_MIN_POLL_SEC;
  uint32_t interval=(uint32_t)pollSec*1000UL;
  uint32_t minimum=serviceMinIntervalMs(SERVICE_ADSB_LOL);
  if(interval<minimum)interval=minimum;
  uint32_t budget=adsbLolBudgetIntervalMs(visibleShare);
  if(interval<budget)interval=budget;
  g_intervalMs=interval;
  return (uint32_t)(millis()-g_lastPollMs)>=interval;
}

const FlightSnapshot& flightData() { return g_f; }
uint16_t flightLastScanned() { return g_scanned; }
uint32_t flightPollIntervalMs() { return g_intervalMs ? g_intervalMs : RADAR_MOTION_MIN_MS; }
float flightLiveDistanceKm(const FlightSnapshot& f) {
  if(!f.valid||!f.motion.hasMotion||isnan(g_homeLat)||isnan(g_homeLon))return f.distanceKm;
  return radarMotionDistanceKm(f.motion,g_homeLat,g_homeLon,millis(),radarMotionLimitMs(flightPollIntervalMs()));
}
int32_t flightLiveAltitudeFt(const FlightSnapshot& f) {
  if(!f.valid||!f.altitudeKnown||f.onGround)return f.altitudeFt;
  return radarMotionAltitudeFt(f.motion,f.altitudeFt,millis(),radarMotionLimitMs(flightPollIntervalMs()));
}
uint32_t flightLastOkMs() { return g_lastOkMs; }
bool flightProviderPaused() { return g_providerPaused; }
const char* flightStageName() { return adsbLolQuotaWaitMs()&&!serviceWaitMs(SERVICE_ADSB_LOL)?"Abrufpause: Kontingent":g_stage; }
int flightLastHttp() { return g_lastHttp; }
uint32_t flightWaitSec() { return (serviceWaitMs(SERVICE_ADSB_LOL)+999)/1000; }
const char* flightRouteStatus() { return g_routeStatus; }
const char* flightRouteSource() { return g_routeSource; }
int flightRouteHttp() { return g_routeHttp; }
uint16_t flightTlsRx() { return g_adsbLolRx; }
uint32_t flightTlsMinBlock() { return radarTlsBudget(g_adsbLolRx ? g_adsbLolRx : 512, sizeof(br_ssl_client_context)).block; }
bool flightExpire() {
  if(g_providerPaused||!g_f.error||!radarExpired(millis(),g_lastOkMs,g_f.valid))return false;
  g_f.valid=false;g_f.error=true;g_stage="Daten veraltet";
  return true;
}

static bool flightFailure(const char* stage = "Fehler", bool providerPause = false) {
  g_providerPaused = providerPause;
  g_stage = stage;
  uint32_t now = millis();
  if (g_consecutiveFailures == 0) g_firstFailureMs = now;
  if (g_consecutiveFailures < 255) g_consecutiveFailures++;
  uint32_t since = g_lastOkMs ? (uint32_t)(now - g_lastOkMs) : (uint32_t)(now - g_firstFailureMs);
  g_f.error = true;
  if (!providerPause && since >= RADAR_MAX_AGE_MS) {
    g_f.valid = false;
    g_f.error = true;
  }
  return false;
}

static void flightSuccess(const char* stage = "ok") {
  g_providerPaused = false;
  g_stage = stage;
  g_consecutiveFailures = 0;
  g_firstFailureMs = 0;
  g_lastOkMs = millis();
}

// Fields the radar reads per aircraft.
static void addAircraftFields(JsonObject f) {
  f["flight"] = true; f["r"] = true; f["t"] = true; f["category"] = true;
  f["lat"] = true; f["lon"] = true; f["alt_baro"] = true; f["hex"] = true;
  f["gs"] = true; f["track"] = true; f["baro_rate"] = true; f["geom_rate"] = true; f["seen_pos"] = true;
  f["squawk"] = true; f["dbFlags"] = true;
}

// One aircraft object into a snapshot; false without a usable position.
static bool readAircraft(JsonObjectConst a, float homeLat, float homeLon, FlightSnapshot& next) {
  strlcpy(next.origin,"N/A",sizeof(next.origin));
  strlcpy(next.destination,"N/A",sizeof(next.destination));
  const char* rawCall = a["flight"] | (a["hex"] | "");
  strlcpy(next.callsign, rawCall, sizeof(next.callsign));
  trimTail(next.callsign);
  strlcpy(next.hex, a["hex"] | "", sizeof(next.hex));
  strlcpy(next.registration, a["r"] | "", sizeof(next.registration));
  strlcpy(next.category,a["category"]|"",sizeof(next.category));
  strlcpy(next.typeCode, a["t"] | "", sizeof(next.typeCode));
  strlcpy(next.squawk, a["squawk"] | "", sizeof(next.squawk));
  next.military = ((int)(a["dbFlags"] | 0)) & 1;
  next.onGround=a["alt_baro"].is<const char*>()&&!strcmp(a["alt_baro"].as<const char*>(),"ground");
  next.altitudeKnown=a["alt_baro"].is<float>();
  next.altitudeFt=next.altitudeKnown?(int32_t)lroundf(a["alt_baro"].as<float>()):0;
  float acLat = NAN, acLon = NAN;
  if (a["lat"].is<float>()) acLat = a["lat"].as<float>();
  if (a["lon"].is<float>()) acLon = a["lon"].as<float>();
  if(!isfinite(acLat)||!isfinite(acLon)||acLat<-90||acLat>90||acLon<-180||acLon>180)return false;
  next.distanceKm = distanceKm(homeLat, homeLon, acLat, acLon);
  // Motion for estimates until the next request (not on the ground).
  RadarMotion& m = next.motion;
  m.lat = acLat; m.lon = acLon;
  float seen = a["seen_pos"] | 0.0f;
  if (!(seen >= 0.0f) || seen > 60.0f) seen = 0.0f;
  m.sampleMs = millis() - (uint32_t)(seen * 1000.0f);
  float gs = a["gs"] | -1.0f, track = a["track"] | -1.0f;
  next.speedKnown = gs >= 0.0f && gs <= 800.0f;
  next.speedKt = next.speedKnown ? gs : 0.0f;
  m.hasMotion = !next.onGround && next.speedKnown && track >= 0.0f && track <= 360.0f;
  m.speedKt = m.hasMotion ? gs : 0.0f;
  m.trackDeg = m.hasMotion ? track : 0.0f;
  JsonVariantConst rate = a["baro_rate"].is<float>() ? a["baro_rate"] : a["geom_rate"];
  m.hasRate = !next.onGround && next.altitudeKnown && rate.is<float>() && fabsf(rate.as<float>()) <= 8000.0f;
  m.rateFpm = m.hasRate ? rate.as<float>() : 0.0f;
  return true;
}

// Names from the built-in tables, only for the aircraft that is shown.
static void labelAircraft(FlightSnapshot& f) {
  if(!aircraftDisplayLabel(f.typeCode, f.typeName, sizeof(f.typeName))) strlcpy(f.typeName, f.typeCode, sizeof(f.typeName));
  // Brand names (AirlineName.h) take precedence over provider names.
  if(!airlineBrand(f.callsign, f.airline, sizeof(f.airline))) f.airline[0] = 0;
}

bool flightFetch(float homeLat, float homeLon, uint16_t radiusKm, uint8_t hideMask) {
  const bool scan = hideMask != 0;
  g_scanned = 0;
  if(adsbLolWaitMs())return false;
  DiagRequest trace(D_RADAR);
  struct Completion {~Completion(){g_lastPollMs=millis();}} completion;
  g_lastPollMs = millis();
  g_hasPolled = true;
  g_routeSincePoll = false;
  g_lastHttp = 0;
  if (WiFi.status() != WL_CONNECTED) { trace.stage(DS_NO_WIFI); return flightFailure("kein WLAN"); }
  if (!tlsHeapReady(g_adsbLolRx ? g_adsbLolRx : 512)) { trace.stage(DS_HEAP); return flightFailure("übersprungen: Heap"); }
  trace.stage(DS_TLS);
  const uint16_t rx=adsbLolRx();
  if (!tlsHeapReady(rx)) { trace.stage(DS_HEAP); return flightFailure("übersprungen: Heap"); }

  uint16_t radiusNm = (uint16_t)ceilf(radiusKm / 1.852f);
  if (radiusNm < 1) radiusNm = 1;
  if (radiusNm > 250) radiusNm = 250;

  FlightSnapshot next = {};
  strlcpy(next.origin,"N/A",sizeof(next.origin));
  strlcpy(next.destination,"N/A",sizeof(next.destination));
  float acLat = NAN, acLon = NAN;

  {
    BearSSL::WiFiClientSecure client;
    makeSecure(client, &g_adsbSession, rx);
    HTTPClient http;
    http.setTimeout(5000);
    http.setReuse(false);

    String url = scan ? F("https://api.adsb.lol/v2/point/") : F("https://api.adsb.lol/v2/closest/");
    url += String(homeLat, 4); url += '/';
    url += String(homeLon, 4); url += '/';
    url += String(radiusNm);
    if (!http.begin(client, url)) return flightFailure("Verbindung fehlgeschlagen");
    http.addHeader("Accept", "application/json");
    adsbLolPrepare(http, FW_USER_AGENT);

    int code = http.GET();trace.http(code);
    serviceResponse(SERVICE_ADSB_LOL,code,http.header("Retry-After").c_str());
  if(serviceWaitMs(SERVICE_ADSB_LOL))diagEvent(D_RADAR,DS_WAIT,code,serviceWaitMs(SERVICE_ADSB_LOL));
    g_lastHttp = code;
    if (code != HTTP_CODE_OK) { http.end(); client.stop(); const bool paused=(code==429||code==503)&&serviceWaitMs(SERVICE_ADSB_LOL);return flightFailure(adsbLolAccessDenied(code)?"Zugriff abgelehnt":(paused?"Abrufpause: Anbieter":(code < 0 ? "Verbindung fehlgeschlagen" : "HTTP-Fehler")),paused); }

    JsonDocument filter(networkJsonAllocator());
    addAircraftFields(scan ? filter.to<JsonObject>() : filter["ac"][0].to<JsonObject>());
    JsonDocument doc(networkJsonAllocator());
    trace.stage(DS_JSON);
    if(!jsonFilterReady(filter,trace)){http.end();client.stop();return flightFailure("JSON-Speicher knapp");}
    AdsbLolBody body(http);
    bool found = false;
    if (!scan) {
      auto err = deserializeJson(doc, body, DeserializationOption::Filter(filter));
      logJsonError(trace,err); filter.clear(); http.end(); client.stop();
      if (err) return flightFailure("JSON-Fehler");
      JsonArrayConst ac = doc["ac"].as<JsonArrayConst>();
      if(ac.isNull()){trace.stage(DS_SCHEMA);return flightFailure("Antwortformat-Fehler");}
      if (ac.size()) {
        if(!readAircraft(ac[0].as<JsonObjectConst>(),homeLat,homeLon,next)){trace.stage(DS_SCHEMA);return flightFailure("Positionsdaten fehlen");}
        found = true;
      }
    } else {
      // All aircraft in the radius, read one at a time (RadarScan.h).
      FlightSnapshot cand;
      DeserializationError err;
      g_scanned = radarScanArray(body, doc, filter, [&](JsonObjectConst a) {
        memset(&cand, 0, sizeof(cand));
        if (!readAircraft(a, homeLat, homeLon, cand) || radarHidden(cand.category, cand.onGround, hideMask)) return;
        if (!found || cand.distanceKm < next.distanceKm) { next = cand; found = true; }
        yield();
      }, err);
      if (err) {
        logJsonError(trace,err); filter.clear(); http.end(); client.stop();
        if (err == DeserializationError::InvalidInput && !g_scanned) { trace.stage(DS_SCHEMA); return flightFailure("Antwortformat-Fehler"); }
        return flightFailure("JSON-Fehler");
      }
      filter.clear(); http.end(); client.stop();
    }
    if (!found) {
      memset(&g_f, 0, sizeof(g_f));
      g_f.error = false;
      flightSuccess("keine Flugzeuge");trace.finish(true,DS_EMPTY);
      return true;
    }
    labelAircraft(next);
    acLat = next.motion.lat; acLon = next.motion.lon;
    next.valid = true;
    next.error = false;
  }

  flightSuccess(); // age belongs to position data, not slow enrichment requests
  g_position={acLat,acLon};
  g_homeLat=homeLat;g_homeLon=homeLon;
  enrichFlight(next,g_position);

  next.error=serviceWaitMs(SERVICE_ADSB_LOL)>0;
  g_f = next;
  g_stage=next.error?"Abrufpause: Rate-Limit":"ok";
  trace.finish(true);
  g_lastPollMs=millis(); // poll interval starts after the connection has completed
  return true;
}

