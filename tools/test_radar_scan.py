#!/usr/bin/env python3
"""Radar filter: all aircraft read one by one, hidden categories skipped, nearest kept."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
lib=root/'.pio/libdeps/smalltv_ultra/ArduinoJson/src'
if not lib.is_dir():raise SystemExit('Install pinned ArduinoJson with pio pkg install before this test')
test=r'''
#include <ArduinoJson.h>
#include "RadarScan.h"
#include "RadarFilter.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
// Minimal stream: read/peek for the scanner, readBytes for ArduinoJson.
struct Body {
  std::string s; size_t i=0;
  explicit Body(const char* t):s(t){}
  int read(){return i<s.size()?(unsigned char)s[i++]:-1;}
  int peek(){return i<s.size()?(unsigned char)s[i]:-1;}
  size_t readBytes(char* b,size_t n){size_t k=0;while(k<n&&i<s.size())b[k++]=s[i++];return k;}
};
struct Pick{std::string hex;float dist=1e9f;int seen=0;};
static Pick scan(const char* json,uint8_t mask,DeserializationError& err,uint16_t* count=nullptr){
  Body b(json);JsonDocument filter,doc;
  JsonObject f=filter.to<JsonObject>();f["hex"]=true;f["category"]=true;f["alt_baro"]=true;f["d"]=true;
  Pick p;
  uint16_t n=radarScanArray(b,doc,filter,[&](JsonObjectConst a){
    p.seen++;bool ground=a["alt_baro"].is<const char*>()&&!strcmp(a["alt_baro"],"ground");
    if(radarHidden(a["category"]|"",ground,mask))return;
    float d=a["d"]|1e9f;if(d<p.dist){p.dist=d;p.hex=a["hex"]|"";}
    // the next object must still be readable: nothing beyond '}' was consumed
    assert(b.i<=b.s.size());
  },err);
  if(count)*count=n;
  return p;
}
int main(){
  const char* json=R"({"ac": [
    {"hex":"heli","category":"A7","alt_baro":800,"d":1.0,"extra":{"x":[1,2,3]}},
    {"hex":"gnd","category":"A3","alt_baro":"ground","d":1.5},
    {"hex":"cessna","category":"A1","alt_baro":2500,"d":2.0},
    {"hex":"glider","category":"B1","alt_baro":3000,"d":2.5},
    {"hex":"car","category":"C1","alt_baro":"ground","d":0.2},
    {"hex":"nocat","alt_baro":5000,"d":4.0},
    {"hex":"a320","category":"A3","alt_baro":11000,"d":6.0}
  ],"msg":"No error","now":1,"total":7})";
  DeserializationError err;uint16_t n=0;
  assert(scan(json,0,err,&n).hex=="car"&&!err&&n==7);
  assert(scan(json,RADAR_HIDE_GROUND,err).hex=="heli");
  assert(scan(json,RADAR_HIDE_GROUND|RADAR_HIDE_HELI,err).hex=="cessna");
  assert(scan(json,RADAR_HIDE_GROUND|RADAR_HIDE_HELI|RADAR_HIDE_LIGHT,err).hex=="glider");
  assert(scan(json,RADAR_HIDE_ALL,err).hex=="nocat");   // no category: never hidden
  // Empty list, compact JSON, "ac" after other keys, wrong format.
  assert(scan(R"({"ac":[]})",RADAR_HIDE_ALL,err,&n).hex==""&&!err&&n==0);
  assert(scan(R"({"now":1,"msg":"ac","ac":[{"hex":"x","d":3}]})",RADAR_HIDE_ALL,err,&n).hex=="x"&&n==1);
  scan(R"({"error":"bad"})",RADAR_HIDE_ALL,err,&n);assert(err==DeserializationError::InvalidInput&&n==0);
  scan(R"({"ac":[{"hex":"x","d":)",RADAR_HIDE_ALL,err,&n);assert(err&&n==0);
  // Filters themselves.
  assert(!radarHidden("A7",false,0)&&radarHidden("A7",false,RADAR_HIDE_HELI)&&!radarHidden("A3",false,RADAR_HIDE_ALL&~RADAR_HIDE_GROUND));
  assert(radarHidden("A3",true,RADAR_HIDE_GROUND)&&radarHidden("C2",false,RADAR_HIDE_GROUND)&&radarHidden("B6",false,RADAR_HIDE_SPORT)&&!radarHidden("B6",false,RADAR_HIDE_LIGHT));
  assert(radarEmergencySquawk("7700")&&radarEmergencySquawk("7600")&&radarEmergencySquawk("7500")&&!radarEmergencySquawk("7000")&&!radarEmergencySquawk(""));
  assert(radarTrend(true,300)==1&&radarTrend(true,299)==0&&radarTrend(true,-300)==-1&&radarTrend(false,2000)==0);
  puts("Radar scan: aircraft read one by one, filters, nearest pick, empty/odd/broken answers OK");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'t.cpp').write_text(test)
 subprocess.run(['g++','-std=c++11','-Wall','-Wextra','-Werror','-I',str(lib),'-I',str(root/'src'),str(p/'t.cpp'),'-o',str(p/'t')],check=True)
 subprocess.run([str(p/'t')],check=True)
