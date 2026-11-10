#!/usr/bin/env python3
"""Provider cooldowns and route regressions with an independent fake clock."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
test=r'''
#include "RequestPolicy.h"
#include "RoutePolicy.h"
#include "AdsbQuota.h"
#include <cassert>
#include <cmath>
#include <cstdio>
static uint32_t clockMs=100;
uint32_t millis(){return clockMs;}
int main(){
 assert(retryAfterMs("120",0)==120000);
 assert(retryAfterMs("garbage",0)==0);
 assert(retryAfterMs("Wed, 07 Oct 2026 12:02:00 GMT",1791374400)==120000);
 assert(retryAfterMs("Wed, 07 Oct 2026 12:00:00 GMT",1791374400)==0);
 serviceResponse(SERVICE_ADSB_LOL,429,"");assert(serviceWaitMs(SERVICE_ADSB_LOL)==30000);
 clockMs+=29999;assert(serviceWaitMs(SERVICE_ADSB_LOL)==1);
 clockMs++;assert(!serviceWaitMs(SERVICE_ADSB_LOL));
 serviceResponse(SERVICE_ADSB_LOL,429,"");assert(serviceWaitMs(SERVICE_ADSB_LOL)==60000);
 serviceResponse(SERVICE_ADSB_LOL,429,"180");assert(serviceWaitMs(SERVICE_ADSB_LOL)==180000);
 assert(!serviceWaitMs(SERVICE_ADSB_DB));
 serviceResponse(SERVICE_ADSB_LOL,201,"");assert(!serviceWaitMs(SERVICE_ADSB_LOL));
 assert(serviceMinIntervalMs(SERVICE_ADSB_LOL)==20000);
 serviceResponse(SERVICE_ADSB_LOL,200,"");assert(serviceMinIntervalMs(SERVICE_ADSB_LOL)==20000);
 clockMs+=599999;assert(serviceMinIntervalMs(SERVICE_ADSB_LOL)==20000);
 clockMs++;assert(!serviceMinIntervalMs(SERVICE_ADSB_LOL));
 serviceResponse(SERVICE_ADSB_LOL,429,"");assert(serviceWaitMs(SERVICE_ADSB_LOL)==30000);assert(serviceMinIntervalMs(SERVICE_ADSB_LOL)==15000);
 assert(!serviceMinIntervalMs(SERVICE_GEO));
 clockMs=0xfffffff0U;serviceResponse(SERVICE_GEO,429,"1");clockMs=983;assert(serviceWaitMs(SERVICE_GEO)==1);clockMs=984;assert(!serviceWaitMs(SERVICE_GEO));
 clockMs=5000000;serviceResponse(SERVICE_WEATHER,426,"");assert(serviceWaitMs(SERVICE_WEATHER)==600000);
 clockMs+=600000;assert(!serviceWaitMs(SERVICE_WEATHER));
 serviceResponse(SERVICE_WEATHER,401,"");assert(serviceWaitMs(SERVICE_WEATHER)==1200000);
 serviceResponse(SERVICE_WEATHER,403,"");assert(serviceWaitMs(SERVICE_WEATHER)==2400000);
 serviceResponse(SERVICE_WEATHER,426,"");assert(serviceWaitMs(SERVICE_WEATHER)==2400000);
 serviceResponse(SERVICE_WEATHER,200,"");assert(!serviceWaitMs(SERVICE_WEATHER));
 {const uint32_t t=(uint32_t)quotaDaysFromCivil(2026,11,10)*86400UL+12*3600UL+25*60UL; // Tue 10.11.2026 12:25 UTC
  assert(quotaDaysFromCivil(1970,1,1)==0&&quotaDaysFromCivil(2000,3,1)==11017);
  assert(quotaWindowEnd(Q_HOUR,t)==t+35*60);
  assert(quotaWindowEnd(Q_DAY,t)==(uint32_t)quotaDaysFromCivil(2026,11,11)*86400UL);
  assert(quotaWindowEnd(Q_WEEK,t)==(uint32_t)quotaDaysFromCivil(2026,11,16)*86400UL);   // next Monday
  assert(quotaWindowEnd(Q_MONTH,t)==(uint32_t)quotaDaysFromCivil(2026,12,1)*86400UL);
  assert(quotaWindowEnd(Q_MONTH,(uint32_t)quotaDaysFromCivil(2026,12,31)*86400UL)==(uint32_t)quotaDaysFromCivil(2027,1,1)*86400UL);
  assert(quotaWindowId(Q_WEEK,(uint32_t)quotaDaysFromCivil(2026,11,15)*86400UL+86399)!=quotaWindowId(Q_WEEK,(uint32_t)quotaDaysFromCivil(2026,11,16)*86400UL));
  assert(quotaWindowId(Q_WEEK,(uint32_t)quotaDaysFromCivil(2026,11,9)*86400UL)==quotaWindowId(Q_WEEK,t));
  QuotaState q={};
  for(int i=0;i<4;i++){assert(!quotaWaitMs(q,t,ADSB_QUOTA_KEY));quotaCount(q,t);}
  assert(quotaWaitMs(q,t,ADSB_QUOTA_KEY)==30000);                    // 5 per 30 s (4 planned): next window
  assert(!quotaWaitMs(q,t+30,ADSB_QUOTA_KEY));
  // 10.11. test: 151 requests in the hour -> blocked until the full hour
  QuotaState h={};for(int i=0;i<135;i++)quotaCount(h,t-1200+i*8);
  assert(quotaWaitMs(h,t,ADSB_QUOTA_KEY)==35*60*1000UL);
  // Spreading: fresh month, radar always visible -> month window decides
  QuotaState m={};quotaRoll(m,t);
  uint32_t monthLeft=quotaWindowEnd(Q_MONTH,t)-t;
  assert(quotaIntervalMs(m,t,ADSB_QUOTA_KEY,1.0f)==(uint32_t)((float)monthLeft*1000.0f/4750.0f));
  assert(quotaIntervalMs(m,t,ADSB_QUOTA_KEY,0.2f)<quotaIntervalMs(m,t,ADSB_QUOTA_KEY,1.0f));
  assert(!quotaIntervalMs(m,0,ADSB_QUOTA_KEY,1.0f));
  // Without a clock the stored windows stay (restart before NTP)
  QuotaState k=q;quotaRoll(k,0);assert(k.used[Q_DAY]==4);quotaCount(k,0);assert(k.used[Q_DAY]==5);
  // RTC and file: newer window wins, same window keeps the higher count
  QuotaState a={},b={};quotaCount(a,t);quotaCount(a,t);quotaCount(b,t);quotaCount(b,t+86400);
  quotaMerge(a,b);assert(a.used[Q_HOUR]==1&&a.used[Q_MONTH]==2&&a.id[Q_DAY]==quotaWindowId(Q_DAY,t+86400));
 }
 RoutePoint drs={51.134,13.768},fra={50.038,8.562},fmo={52.134,7.685},wro={51.103,16.886},ayt={36.899,30.801},dus={51.29,6.767},muc={48.354,11.786};
 RoutePoint wrong1[]={wro,fra},wrong2[]={fra,fmo},wrong3[]={ayt,dus};
 assert(!routePositionPlausible(drs,wrong1,2,true,false,0));
 assert(!routePositionPlausible(drs,wrong2,2,false,true,2450));
 assert(!routePositionPlausible(drs,wrong3,2,true,false,0));
 RoutePoint valid[]={drs,muc};assert(routePositionPlausible(drs,valid,2,true,false,0));
 RoutePoint peg[]={ayt,drs};assert(routePositionPlausible({51.04,13.65},peg,2,false,true,2900));
 assert(!routePositionPlausible({0,0},valid,2,false,true,33000));
 RoutePoint dateline[]={{0,170},{0,-170}};assert(routePositionPlausible({0,180},dateline,2,false,true,33000));
 RoutePoint invalid[]={{NAN,0},drs};assert(!routePositionPlausible(drs,invalid,2,false,false,0));
 // SKW4955 (10.11.2026): shown ORD>MKE while descending into O'Hare, really LEX>ORD.
 RoutePoint ord={41.978,-87.905},mke={42.947,-87.897},lex={38.036,-84.606},near={41.99,-87.70};
 assert(routeOriginContradicted(near,ord,mke,false,true,4000,true,-1200));
 assert(!routeOriginContradicted(near,lex,ord,false,true,4000,true,-1200));
 assert(!routeOriginContradicted(near,ord,mke,false,true,4000,true,1500));   // climbing (also go-around)
 assert(!routeOriginContradicted(near,ord,mke,false,true,4000,true,-300));   // nearly level
 assert(!routeOriginContradicted(near,ord,mke,false,true,15000,true,-1500)); // high
 assert(!routeOriginContradicted(near,ord,mke,false,true,4000,false,0));     // no vertical rate
 assert(!routeOriginContradicted(near,ord,ord,false,true,4000,true,-1200));  // round trip
 assert(!routeOriginContradicted(mke,ord,mke,false,true,4000,true,-1200));   // far from the origin
 puts("Retry-After, progressive/shared cooldowns, clock wrap, denied-request pauses, quota windows, descent rule and DRS route regressions: OK");
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'Arduino.h').write_text('#pragma once\n#include <stdint.h>\nuint32_t millis();\n')
 (p/'policy.cpp').write_text(test)
 subprocess.run(['g++','-std=c++11','-Wall','-Wextra','-Werror','-I'+str(p),'-I'+str(root/'src'),str(p/'policy.cpp'),str(root/'src/RequestPolicy.cpp'),'-o',str(p/'policy')],check=True)
 subprocess.run([str(p/'policy')],check=True)
