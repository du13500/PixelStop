#!/usr/bin/env python3
"""Simulates the radar scheduler for hours with the real scheduling functions of
src/Flight.cpp (flightPollDue, flightEnrichmentDue, flightEnrich, routeSlotFree and
the skip/slot lines of lookupRouteAdsbLol) and the real quota (AdsbQuota.h).
Transfers are stubs that only take time and count requests; the main loop's task
order and pause after each request are modelled like main.cpp.

Regression for v0.5.4: route re-checks pushed every position request back, so the
radar never asked for positions again (one position in two hours)."""
from pathlib import Path
import re, subprocess, tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/Flight.cpp').read_text()

def fn(name):
    m = re.search(r'^(?:static )?[^\n]+\b' + name + r'\([^\n]*\)\s*\{', source, re.M); assert m, name
    a = m.start(); i = source.index('{', a) + 1; depth = 1
    while depth:
        depth += (source[i] == '{') - (source[i] == '}'); i += 1
    return source[a:i]

route = fn('lookupRouteAdsbLol')
head = route[route.index('  uint8_t skip='):route.index('\n', route.index('g_lastPollMs=millis();'))]
head = head.replace('!tlsHeapReady(adsbLolRx())?4:0', '0')
HARNESS = r'''
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include "AdsbQuota.h"
#define strlcpy simStrlcpy
inline size_t simStrlcpy(char*d,const char*s,size_t n){size_t l=strlen(s);if(n){size_t k=l<n-1?l:n-1;memcpy(d,s,k);d[k]=0;}return l;}
static uint32_t now=1000;uint32_t millis(){return now;}
static const uint32_t T0=1794330000UL;  // 2026-11-10 17:00 UTC
static uint32_t epoch(){return T0+now/1000;}
struct RoutePoint{float lat,lon;};
static bool routePointValid(RoutePoint p){return !std::isnan(p.lat)&&!std::isnan(p.lon);}
struct FlightRoute{uint8_t count;};
struct FlightSnapshot{bool valid=false,error=false;char hex[8]={},callsign[12]={};};
struct Enrichment {
  char hex[8],callsign[12],airline[64],type[80],typeCode[8];
  FlightRoute route; RoutePoint airports[8]; uint8_t airportCount;
  uint32_t checked; bool used; const char* source; uint8_t phase;
};
static FlightSnapshot g_f;static RoutePoint g_position={51,13};
static Enrichment g_cache[3]={};static uint8_t g_cacheNext=0;static uint32_t g_lastEnrichmentMs=0;
static bool g_routeSincePoll=false;static uint32_t g_lastPollMs=0,g_lastOkMs=0,g_intervalMs=0;static bool g_hasPolled=false,g_providerPaused=false;
static const char* g_stage="";
static const uint16_t RADAR_MIN_POLL_SEC=10;
enum {SERVICE_ADSB_LOL,SERVICE_ADSB_DB};
static uint32_t providerUntil=0,minIntervalUntil=0,minInterval=0;
uint32_t serviceWaitMs(int s){return s==SERVICE_ADSB_LOL&&(int32_t)(providerUntil-now)>0?providerUntil-now:0;}
uint32_t serviceMinIntervalMs(int s){return s==SERVICE_ADSB_LOL&&(int32_t)(minIntervalUntil-now)>0?minInterval:0;}
// quota (real AdsbQuota.h, simulated clock)
static QuotaState Q={};static bool hasKey=true;
static const uint16_t* use(){return hasKey?ADSB_QUOTA_KEY:ADSB_QUOTA_NOKEY;}
uint32_t adsbLolQuotaWaitMs(){return quotaWaitMs(Q,epoch(),use());}
uint32_t adsbLolWaitMs(){uint32_t a=serviceWaitMs(SERVICE_ADSB_LOL),b=adsbLolQuotaWaitMs();return a>b?a:b;}
uint32_t adsbLolBudgetIntervalMs(float share){return quotaIntervalMs(Q,epoch(),use(),share);}
// v0.5.3.1 helpers
static uint16_t today=0;
bool adsbLolBudgetLow(){return false;}
// counters
static std::vector<uint32_t> positionAt,lolAt,routeAt;static int routeCalls=0,metaCalls=0,airlineCalls=0,skips=0;
static void countLol(){quotaCount(Q,epoch());lolAt.push_back(now);}
// scene: callsign shown at time t (set by scenario)
static int scenario=0;
static void sceneAircraft(FlightSnapshot& f){
  uint32_t min=now/60000;char cs[12];
  if(scenario==1){strcpy(cs,"SAS88C");}                         // same aircraft for hours
  else if(scenario==2){snprintf(cs,sizeof cs,"DLH%u",min/3);}   // new aircraft every 3 min
  else {snprintf(cs,sizeof cs,"EWG%u",min/15);}                  // new aircraft every 15 min
  f.valid=true;f.error=false;strcpy(f.callsign,cs);strcpy(f.hex,cs);
}
static void lookupMetadata(const FlightSnapshot&,RoutePoint,Enrichment&){metaCalls++;now+=400;}
static void lookupAirline(const FlightSnapshot&,Enrichment&){airlineCalls++;now+=300;}
static void enrichFlight(FlightSnapshot&,RoutePoint){}
static bool tlsHeapReady(uint16_t){return true;}
static uint16_t adsbLolRx(){return 4096;}
static void diagLookup(const char*,const char*){}
static void diagEvent(int,int,int,int){}
enum {D_RADAR_ROUTE,DS_SKIPPED};
// route lookup: real skip/slot lines are patched in by the script
static bool lookupRouteAdsbLol(const FlightSnapshot& f,RoutePoint pos,Enrichment& out){
  (void)f;(void)pos;(void)out;
ROUTE_HEAD
  routeCalls++;routeAt.push_back(now);countLol();now+=1300;return false;
}
static void flightFetch(){
  g_lastPollMs=now;g_hasPolled=true;g_routeSincePoll=false;countLol();now+=1300;
  sceneAircraft(g_f);g_lastOkMs=now;g_lastPollMs=now;positionAt.push_back(now);
}
FUNCTIONS
int main(int argc,char**argv){
  scenario=atoi(argv[1]);int rotation=atoi(argv[2]);int poll=atoi(argv[3]);uint32_t hours=atoi(argv[4]);
  int monthUsed=atoi(argv[5]);hasKey=atoi(argv[6]);
  quotaRoll(Q,epoch());Q.used[Q_MONTH]=monthUsed;Q.used[Q_WEEK]=monthUsed>350?350:monthUsed;Q.used[Q_DAY]=0;
  // rotation: radar visible `rotation` s out of 60 s (0 = radar only)
  float share=rotation?std::min(1.0f,(rotation+10)/60.0f):1.0f;
  uint32_t deferUntil=0;uint8_t cursor=0;uint32_t end=hours*3600000UL;
  while(now<end){
    bool active=true;
    if(rotation){uint32_t ph=(now/1000)%60;active=ph<(uint32_t)rotation||ph>=50;} // visible + 10 s prefetch
    if((int32_t)(now-deferUntil)>=0){
      for(uint8_t n=0;n<6;n++){uint8_t task=cursor;cursor=(cursor+1)%6;bool ran=false;
        if(task==2&&active&&flightPollDue(poll,share)){flightFetch();ran=true;}
        else if(task==3&&active&&flightEnrichmentDue()){flightEnrich();ran=true;}
        if(ran){deferUntil=now+750;break;}}
    }
    now+=100;
  }
  // metrics
  uint32_t maxGap=0;for(size_t i=1;i<positionAt.size();i++)maxGap=std::max(maxGap,positionAt[i]-positionAt[i-1]);
  if(!positionAt.empty())maxGap=std::max(maxGap,end-positionAt.back());
  int max30=0,maxHour=0;for(size_t i=0;i<lolAt.size();i++){int a=0,b=0;for(size_t j=i;j<lolAt.size()&&lolAt[j]-lolAt[i]<30000;j++)a++;for(size_t j=i;j<lolAt.size()&&lolAt[j]-lolAt[i]<3600000;j++)b++;max30=std::max(max30,a);maxHour=std::max(maxHour,b);}
  // at most one route lookup between two position requests
  int routesBetween=0,maxRoutesBetween=0;size_t p=0;std::vector<uint32_t> routes=routeAt;
  for(uint32_t r:routes){while(p<positionAt.size()&&positionAt[p]<r){p++;routesBetween=0;}routesBetween++;maxRoutesBetween=std::max(maxRoutesBetween,routesBetween);}
  printf("%zu %u %zu %d %d %d %d %d\n",positionAt.size(),maxGap/1000,lolAt.size(),routeCalls,metaCalls+airlineCalls,max30,maxHour,maxRoutesBetween);
}
'''
code = HARNESS.replace('ROUTE_HEAD', head).replace('FUNCTIONS', '\n'.join(fn(n) for n in ['currentEnrichment', 'routeSlotFree', 'flightEnrichmentDue', 'flightEnrich', 'flightPollDue']))
with tempfile.TemporaryDirectory() as tmp:
    p = Path(tmp); (p / 'sim.cpp').write_text(code)
    subprocess.run(['g++', '-std=c++14', '-O1', '-w', '-I' + str(root / 'src'), str(p / 'sim.cpp'), '-o', str(p / 'sim')], check=True)
    for rotation, minPositions, maxGap in [(0, 15, 450), (15, 35, 200)]:
        for scenario in (1, 2, 3):   # same aircraft for hours, new one every 3 min, every 15 min
            out = subprocess.run([str(p / 'sim'), str(scenario), str(rotation), '10', '2', '560', '1'], capture_output=True, text=True, check=True).stdout.split()
            positions, gap, lol, routes, adsbdb, max30, maxHour, between = map(int, out)
            ctx = f'rotation {rotation}, scenario {scenario}: {out}'
            assert positions >= minPositions and gap <= maxGap, ctx          # positions keep coming
            assert between <= 1, ctx                                         # never two routes in a row
            assert max30 <= 4 and maxHour <= 135, ctx                        # adsb.lol windows
            if scenario == 1: assert routes == 1, ctx                        # re-checks without adsb.lol
print('Radar schedule (2 h simulated): positions keep coming, one route per new aircraft, re-checks via adsbdb, quota windows kept OK')
