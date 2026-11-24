#!/usr/bin/env python3
"""Execute real radar guard, retry and stale-state functions with a fake clock/heap.
Network transfer and TLS handshake require the device; this tests the decisions
before a request and the state transitions independently of the data provider.
"""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/Flight.cpp').read_text()
def function(name):
 m=re.search(r'^(?:static )?[^\n]+\b'+name+r'\([^\n]*\) \{',s,re.M)
 assert m,name
 start=m.start();end=s.index('{',start)+1;depth=1
 while depth:
  if s[end]=='{':depth+=1
  elif s[end]=='}':depth-=1
  end+=1
 return s[start:end]
pre='''#include <cassert>
#include <cstdint>
#include <cstring>
#include "RadarPolicy.h"
enum {D_RADAR,DS_TLS,DS_HEAP,DS_NO_WIFI};struct DiagRequest {DiagRequest(int){} void stage(int){}};
static const uint16_t ADSB_LOL_TLS_RX=4096;static bool nameFound=true;static int resolves=0;
static bool adsbLolResolve(DiagRequest&){++resolves;return nameFound;}
static uint32_t clockMs=10000;
uint32_t millis(){return clockMs;}
struct br_ssl_client_context { char bytes[3408]; };
struct FakeESP { uint32_t heap=25000,block=11368; uint32_t getFreeHeap(){return heap;}uint32_t getMaxFreeBlockSize(){return block;} } ESP;
const int WL_CONNECTED=3;
struct FakeWifi { int state=WL_CONNECTED;int status(){return state;} } WiFi;
uint32_t serviceWaitMs(int){return 0;}
static uint32_t minimumInterval=0;
uint32_t serviceMinIntervalMs(int){return minimumInterval;}
static uint32_t budgetInterval=0;static float lastShare=0;
uint32_t adsbLolBudgetIntervalMs(float share){lastShare=share;return budgetInterval;}
const int SERVICE_ADSB_LOL=0;
static uint32_t quotaWait=0;uint32_t adsbLolWaitMs(){return quotaWait;}
static bool enrichDue=false;bool flightEnrichmentDue(){return enrichDue;}
struct Enrichment{uint8_t phase;};static Enrichment pending={0};static Enrichment* currentEnrichment(){return &pending;}static bool g_routeSincePoll=false;
struct Snapshot {bool valid,error;} g_f={};
static uint32_t g_lastPollMs=0,g_lastOkMs=0,g_firstFailureMs=0,g_intervalMs=0;
static bool g_hasPolled=false,g_providerPaused=false;
static uint8_t g_consecutiveFailures=0;static bool g_connectionFailed=false;
static int g_lastHttp=0;
static bool g_searching=false;static uint8_t g_searchFails=0,g_skips=0;static uint32_t g_searchStartMs=0;
static bool adsbLolAccessDenied(int c){return c==401||c==403||c==426;}
static const char* g_stage="bereit";
static uint16_t g_scanned=0;
static int requests=0;
'''
funcs='\n'.join(function(n) for n in ['tlsHeapReady','flightPollDue','flightExpire','flightFailure','flightSkipped','flightSuccess','flightActivated','flightSearching','flightSearchRetry'])
# Run the original pre-request section verbatim; replace only the network body.
fetch=function('flightFetch').split('  uint16_t radiusNm',1)[0]
fetch+='  (void)homeLat;(void)homeLon;(void)radiusKm;(void)scan;(void)rx;requests++;return true;\n}\n'
tests='''int main(){
 // adsb.lol: fixed 4 KB buffer, no probe (no Max Fragment Length since ~9.11.2026)
 const RadarTlsBudget need=radarTlsBudget(ADSB_LOL_TLS_RX,sizeof(br_ssl_client_context));
 ESP.heap=need.total;ESP.block=need.block;assert(flightFetch(51,13,25,0));assert(requests==1&&resolves==1);
 ESP.heap=need.total-1;assert(!flightFetch(51,13,25,0));assert(requests==1&&resolves==1&&!g_connectionFailed);
 ESP.heap=need.total;g_consecutiveFailures=0;
 g_f.valid=true;g_f.error=false;g_lastOkMs=10000;clockMs=15000;
 ESP.block=6000;assert(!flightFetch(51,13,25,0));assert(requests==1);
 assert(g_f.valid&&!g_f.error&&!g_consecutiveFailures&&g_skips==2);assert(!strcmp(g_stage,"übersprungen: Heap"));   // our own reserve: skipped, not failed
 // 5 s is no longer selectable: the radar asks at most every 10 s.
 assert(!flightPollDue(5,1.0f));clockMs=24999;assert(!flightPollDue(5,1.0f));clockMs=25000;assert(flightPollDue(5,1.0f)&&g_intervalMs==10000);
 // The quota (AdsbQuota.h) and a pending route lookup take precedence.
 quotaWait=1000;assert(!flightPollDue(10,1.0f));quotaWait=0;enrichDue=true;assert(!flightPollDue(10,1.0f));
 // Only the first lookup of a new aircraft goes first; later checks never block positions.
 pending.phase=3;assert(flightPollDue(10,1.0f));pending.phase=0;enrichDue=false;
 minimumInterval=15000;clockMs=29999;assert(!flightPollDue(5,1.0f));clockMs=30000;assert(flightPollDue(5,1.0f));minimumInterval=0;
 budgetInterval=12000;clockMs=26999;assert(!flightPollDue(5,0.25f)&&lastShare==0.25f);clockMs=27000;assert(flightPollDue(5,0.25f)&&g_intervalMs==12000);budgetInterval=0;
 g_f.error=true;   // a real failed request (the skip above leaves the data untouched)
 clockMs=39999;assert(!flightExpire());clockMs=40000;assert(flightExpire());assert(!g_f.valid&&g_f.error);assert(!flightExpire());
 ESP.heap=need.total;ESP.block=need.block;assert(flightFetch(51,13,25,0));
 g_f={true,false};flightSuccess();assert(!g_consecutiveFailures&&g_lastOkMs==40000);
 WiFi.state=0;clockMs=45000;assert(!flightFetch(51,13,25,0));assert(g_f.valid&&g_f.error);assert(!strcmp(g_stage,"kein WLAN"));
 clockMs=70000;assert(!flightFailure("HTTP-Fehler"));assert(!g_f.valid);
 // Confirmed provider pauses preserve data beyond normal expiry, even after
 // the cooldown timer ends, until the next actual result. Real errors expire.
 g_f={true,false};clockMs=80000;flightSuccess();clockMs=110001;
 assert(!flightFailure("Abrufpause: Anbieter",true));assert(g_f.valid&&g_f.error&&g_providerPaused);
 clockMs=590000;assert(!flightExpire()&&g_f.valid);
 assert(!flightFailure("Verbindung fehlgeschlagen"));assert(!g_f.valid&&!g_providerPaused);
 g_f={true,false};flightSuccess();assert(!g_providerPaused&&!g_consecutiveFailures);
 g_f={false,false};flightFailure("Abrufpause: Anbieter",true);assert(!g_f.valid);flightSuccess();
 // Name not found: marked as a connection failure, the next attempt comes after 45 s
 // instead of the chosen 180 s (twice), then the normal pace again.
 WiFi.state=WL_CONNECTED;g_f={true,false};flightSuccess();g_lastPollMs=clockMs;nameFound=false;
 assert(!flightFetch(51,13,25,0)&&g_connectionFailed&&!strcmp(g_stage,"Name nicht gefunden")&&g_consecutiveFailures==1);
 {uint32_t t0=g_lastPollMs;clockMs=t0+44999;assert(!flightPollDue(180,1.0f));clockMs=t0+45000;assert(flightPollDue(180,1.0f)&&g_intervalMs==45000);}
 assert(!flightFetch(51,13,25,0)&&g_consecutiveFailures==2);clockMs=g_lastPollMs+45000;assert(flightPollDue(180,1.0f));
 assert(!flightFetch(51,13,25,0)&&g_consecutiveFailures==3);clockMs=g_lastPollMs+45000;assert(!flightPollDue(180,1.0f));
 clockMs=g_lastPollMs+180000;assert(flightPollDue(180,1.0f));
 nameFound=true;assert(flightFetch(51,13,25,0));flightSuccess();assert(!g_connectionFailed);
 // Unsigned elapsed times remain valid across the millis wrap.
 g_f={true,true};g_lastOkMs=0xfffffff0U;clockMs=29983;assert(!flightExpire());clockMs=29984;assert(flightExpire());
 // ---- v0.5.4.7: back in the rotation after a pause -> search instead of an old aircraft
 g_searching=false;g_searchFails=0;g_skips=0;g_consecutiveFailures=0;g_providerPaused=false;g_connectionFailed=false;g_lastHttp=0;
 g_f={true,false};clockMs=100000;flightSuccess();
 clockMs=129999;flightActivated();assert(!flightSearching());          // younger than 30 s: goes on as before
 clockMs=130000;flightActivated();assert(flightSearching()&&!flightSearchRetry());
 g_f.error=true;assert(!flightExpire()&&g_f.valid);g_f.error=false;     // no expiry while searching
 assert(!flightFailure("Verbindung fehlgeschlagen"));assert(flightSearching()&&flightSearchRetry()&&g_f.valid&&!g_f.error);
 assert(!flightFailure("HTTP-Fehler"));assert(flightSearching());
 assert(!flightFailure("Abrufpause: Anbieter",true));assert(flightSearching()&&g_searchFails==2);   // a provider pause is no failure
 assert(!flightFailure("JSON-Fehler"));assert(flightSearching()&&g_searchFails==3&&g_f.valid);      // third failure, but searched less than 2 min: keep searching
 clockMs=249999;assert(!flightFailure("JSON-Fehler"));assert(flightSearching()&&g_f.valid);
 clockMs=250000;assert(!flightFailure("JSON-Fehler"));assert(!flightSearching()&&!g_f.valid&&g_f.error);   // 3 failures and 2 min: red screen
 g_f={true,false};flightSuccess();clockMs=280000;flightActivated();assert(flightSearching());
 g_f={true,false};flightSuccess();assert(!flightSearching()&&!g_searchFails);                       // first fresh answer ends the search
 clockMs=310000;flightActivated();assert(flightSearching());g_lastHttp=403;
 assert(!flightFailure("Zugriff abgelehnt"));assert(!flightSearching()&&!g_f.valid&&g_f.error);g_lastHttp=0;   // refused key: no retries
 g_f={};g_lastOkMs=0;flightActivated();assert(!flightSearching());                                 // never any data: nothing to search for
 // Skipped (own memory reserve): no failure, search goes on, next attempt after 10 s instead of 45/180 s
 g_f={true,false};clockMs=400000;flightSuccess();clockMs=440000;flightActivated();assert(flightSearching());
 ESP.heap=need.total;ESP.block=6000;
 for(int i=0;i<RADAR_MAX_SKIPS;i++){assert(!flightFetch(51,13,25,0));assert(flightSearching()&&!flightSearchRetry()&&!g_consecutiveFailures&&g_skips==i+1);}
 {uint32_t t0=g_lastPollMs;clockMs=t0+9999;assert(!flightPollDue(180,1.0f));clockMs=t0+10000;assert(flightPollDue(180,1.0f)&&g_intervalMs==10000);
  minimumInterval=15000;clockMs=t0+14999;assert(!flightPollDue(180,1.0f));clockMs=t0+15000;assert(flightPollDue(180,1.0f));minimumInterval=0;}   // quota limits still apply
 assert(!flightFetch(51,13,25,0));assert(!g_skips&&g_searchFails==1&&flightSearchRetry());          // shortage that does not go away is a failure after all
 ESP.block=need.block;
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp)/'radar.cpp';p.write_text('#include <initializer_list>\n'+pre+funcs+fetch+tests)
 out=Path(tmp)/'radar';subprocess.run(['g++','-std=c++11','-Wall','-Wextra','-Werror','-I'+str(root/'src'),str(p),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
print('Real radar guard/retry: fixed 4 KB adsb.lol buffer without probe, retry after 45 s, 10 s minimum, quota and route priority, failures marked, expiry/recovery/wrap OK')
