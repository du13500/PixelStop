#!/usr/bin/env python3
"""Radar estimates between requests: position, distance, altitude, limits and status dot."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
test=r'''
#include "RadarMotion.h"
#include <cassert>
#include <cmath>
#include <cstdio>
static bool near(float a,float b,float tol){return fabsf(a-b)<=tol;}
int main(){
 // Dresden airport area, 450 kt due east: about 0.231 km per second.
 RadarMotion m={};m.lat=51.13f;m.lon=13.77f;m.speedKt=450;m.trackDeg=90;m.sampleMs=1000;m.hasMotion=true;
 const uint32_t lim=radarMotionLimitMs(30000);
 assert(lim==60000);
 float lat,lon;radarMotionPosition(m,1000+60000,lim,lat,lon);
 assert(near(lat,51.13f,0.0005f)&&lon>13.77f);
 assert(near(radarFlatDistanceKm(51.13f,13.77f,lat,lon),450*1.852f/60.0f,0.05f));
 // Home 10 km west of the aircraft: distance grows by ~13.9 km per minute.
 const float homeLon=13.77f-10.0f/(111.0f*cosf(51.13f*3.14159265f/180.0f));
 assert(near(radarMotionDistanceKm(m,51.13f,homeLon,1000,lim),10.0f,0.02f));
 assert(near(radarMotionDistanceKm(m,51.13f,homeLon,1000+30000,lim),10.0f+450*1.852f/120.0f,0.05f));
 // Beyond the limit the value stays frozen.
 assert(near(radarMotionDistanceKm(m,51.13f,homeLon,1000+60000,lim),radarMotionDistanceKm(m,51.13f,homeLon,1000+600000,lim),0.001f));
 // Heading north reduces the distance to a home point ahead.
 m.trackDeg=0;assert(radarMotionDistanceKm(m,51.23f,13.77f,1000+30000,lim)<radarMotionDistanceKm(m,51.23f,13.77f,1000,lim));
 // No motion data: unchanged.
 m.hasMotion=false;radarMotionPosition(m,1000+30000,lim,lat,lon);assert(lat==51.13f&&lon==13.77f);
 // Altitude: climbs with the rate, but at most 45 s, never below zero.
 m.hasRate=true;m.rateFpm=1200;
 assert(radarMotionAltitudeFt(m,10000,1000+30000,lim)==10600);
 assert(radarMotionAltitudeFt(m,10000,1000+59000,lim)==10900);
 m.rateFpm=-3000;assert(radarMotionAltitudeFt(m,500,1000+30000,lim)==0);
 m.hasRate=false;assert(radarMotionAltitudeFt(m,10000,1000+30000,lim)==10000);
 // millis() overflow and samples "from the future".
 m.hasRate=true;m.rateFpm=600;m.sampleMs=0xFFFFF000U;assert(radarMotionAltitudeFt(m,1000,0xFFFFF000U+6000U,lim)==1060);
 m.sampleMs=5000;assert(radarMotionAltitudeFt(m,1000,4000,lim)==1000);
 // Limits follow the interval: at least 60 s, at most 4 min.
 assert(radarMotionLimitMs(5000)==60000&&radarMotionLimitMs(120000)==135000&&radarMotionLimitMs(900000)==240000);
 // Status dot.
 assert(radarDotState(500,30000,false)==RADAR_DOT_NEW);
 assert(radarDotState(500,30000,true)==RADAR_DOT_ESTIMATED);
 assert(radarDotState(20000,30000,false)==RADAR_DOT_FRESH);
 assert(radarDotState(45000,30000,false)==RADAR_DOT_FRESH);
 assert(radarDotState(46000,30000,false)==RADAR_DOT_ESTIMATED);
 assert(radarDotState(20000,30000,true)==RADAR_DOT_ESTIMATED);
 assert(radarDotState(60000,30000,false)==RADAR_DOT_FROZEN);
 assert(radarDotState(190000,180000,false)==RADAR_DOT_FRESH&&radarDotState(195000,180000,false)==RADAR_DOT_FROZEN);
 puts("Radar estimates: position, distance, altitude, limits, overflow and status dot OK");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'t.cpp').write_text(test)
 subprocess.run(['g++','-std=c++11','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(p/'t.cpp'),'-o',str(p/'t')],check=True)
 subprocess.run([str(p/'t')],check=True)
