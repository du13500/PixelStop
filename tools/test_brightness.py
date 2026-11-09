#!/usr/bin/env python3
"""Exercise the actual brightness function, including night transitions."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/main.cpp').read_text()
setup=s[s.index('void setup()'):s.index('void loop()')]
assert setup.index('enablePhaseLockedWaveform();')<setup.index('analogWriteFreq(BL_PWM_FREQ);')<setup.index('analogWriteRange(BL_PWM_RANGE);')<setup.index('applyBrightness();')
assert '#define BL_PWM_FREQ 300' in s and '#define BL_PWM_RANGE 1023' in s
a=s.index('static bool nightWindow(');b=s.index('static bool modeEnabled(',a)
code=s[a:b].replace('time(nullptr)','fakeNow')
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);f=p/'test.cpp'
 f.write_text(r'''#include <cstdint>
#include <ctime>
#include <cassert>
#define TFT_BL 5
#define BL_PWM_RANGE 1023
struct {uint8_t brightness=1,nightBrightness=1,backlightMode=0;bool nightEnabled=false;uint16_t nightStart=1320,nightEnd=420;}cfg;
static time_t fakeNow=0;static int writes=0,last=-1;
void analogWrite(int pin,int duty){assert(pin==5);writes++;last=duty;}
#define HIGH 1
static int sdWrites=0,sdLast=-1,attached=0,detached=0,prescaler=-1;
void digitalWrite(int pin,int v){assert(pin==5&&v==HIGH);}
void sigmaDeltaEnable(){} void sigmaDeltaSetPrescaler(uint8_t p){prescaler=p;}
void sigmaDeltaAttachPin(int pin,int ch){assert(pin==5&&ch==0);attached++;} void sigmaDeltaDetachPin(int pin){assert(pin==5);detached++;}
void sigmaDeltaWrite(int ch,uint8_t v){assert(ch==0);sdWrites++;sdLast=v;}
'''+code+r'''
int main(){
 applyBrightness();assert(writes==1&&last==1013);
 for(int i=0;i<100;i++)applyBrightness();assert(writes==1);
 cfg.brightness=20;applyBrightness();assert(writes==2&&last==819);
 cfg.nightEnabled=true;fakeNow=1735772400; // 2025-01-01 23:00 UTC
 applyBrightness();assert(writes==3&&last==1013);applyBrightness();assert(writes==3);
 fakeNow+=8*3600;applyBrightness();assert(writes==4&&last==819);
 cfg.brightness=100;applyBrightness();assert(writes==5&&last==0);
 cfg.brightness=120;applyBrightness();assert(writes==5);
 // Sigma-delta: active-low pulse density, attach once, back to PWM detaches.
 cfg.nightEnabled=false;cfg.brightness=1;cfg.backlightMode=1;applyBrightness();assert(attached==1&&prescaler==255&&sdLast==253&&writes==5);
 applyBrightness();assert(sdWrites==1);cfg.brightness=50;applyBrightness();assert(sdLast==128&&attached==1);
 cfg.brightness=100;applyBrightness();assert(sdLast==0);
 cfg.backlightMode=0;applyBrightness();assert(detached==1&&writes==6&&last==0);
}
''')
 subprocess.run(['g++','-std=c++11',str(f),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True,env={'TZ':'UTC'})
print('Actual brightness: first PWM, unchanged values, night transitions and clamping OK')
