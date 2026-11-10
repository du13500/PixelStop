#include "../src/PixelFont.h"
#include "../src/FlightRoute.h"
#include "../src/AircraftPolicy.h"
#include "../src/RadarPolicy.h"
#include "../src/WeatherVisual.h"
#include "../src/TimeZones.h"
#include "../src/WebGuard.h"
#include "../src/NetWatch.h"
#include "../src/AirlineName.h"
#include "../src/AircraftLabel.h"
#include <assert.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static std::vector<uint32_t> decode(const std::string& s){
  size_t i=0;uint32_t cp=0;std::vector<uint32_t> v;
  while(nextDisplayCodepoint(s.data(),s.size(),i,cp))v.push_back(cp);
  return v;
}
static std::string parts(const char* name,int expect){char a[40],b[40];int n=airlineParts(name,18,a,b,sizeof(a));assert(n==expect);return std::string(a)+(b[0]?"|"+std::string(b):"");}
int main(){
 // Network watchdog: one reconnect, then a restart -- not a reconnect every second (v0.5.4 bug)
 {NetWatch w={1000,0,0};uint32_t sil=0;const uint32_t M=60000;
  assert(netWatchStep(w,5*M,2*M,4*M,false,0,1,sil)==NET_OK);                    // answer 3 min ago
  assert(netWatchStep(w,12*M+1,2*M,11*M,false,0,1,sil)==NET_RECONNECT&&sil==10*M+1);
  for(uint32_t t=12*M+2;t<17*M;t+=1000)assert(netWatchStep(w,t,2*M,11*M,false,0,1,sil)==NET_OK); // no reconnect storm
  assert(netWatchStep(w,17*M+1,2*M,16*M,false,0,1,sil)==NET_RESTART);          // tried after reconnect, still silent
  NetWatch r={1000,0,0};assert(netWatchStep(r,12*M+1,2*M,11*M,false,0,1,sil)==NET_RECONNECT);
  assert(netWatchStep(r,13*M,12*M+500,12*M+400,false,0,1,sil)==NET_OK&&r.stage==0); // new answer ends the alarm
  NetWatch q={1000,0,0};assert(netWatchStep(q,30*M,0,0,false,0,1,sil)==NET_OK);   // nothing tried: nothing to do
  assert(netWatchStep(q,30*M,0,0,true,19*M,1,sil)==NET_RECONNECT);                  // Wi-Fi down for 11 min
  NetWatch f={1000,0,0};assert(netWatchStep(f,12*M+1,2*M,11*M,false,0,2,sil)==NET_OK);      // after a hang restart: 20 min
  assert(netWatchStep(f,22*M+1,2*M,21*M,false,0,2,sil)==NET_RECONNECT);
 }
 // WebGuard: DNS rebinding (Host) and foreign pages (Origin)
 assert(webGuardHostAllowed("192.168.178.137","192.168.178.137","pixelstop"));
 assert(webGuardHostAllowed("192.168.178.137:80","192.168.178.137","pixelstop"));
 assert(webGuardHostAllowed("PixelStop.local","192.168.178.137","pixelstop"));
 assert(webGuardHostAllowed("pixelstop.fritz.box","192.168.178.137","pixelstop"));
 assert(webGuardHostAllowed("pixelstop","192.168.178.137","pixelstop"));
 assert(webGuardHostAllowed("192.168.4.1","0.0.0.0","pixelstop"));
 assert(!webGuardHostAllowed("pixelstop.evil.example","192.168.178.137","pixelstop"));
 assert(!webGuardHostAllowed("rebind.example","192.168.178.137","pixelstop"));
 assert(!webGuardHostAllowed("192.168.178.13","192.168.178.137","pixelstop"));
 assert(webGuardOriginAllowed("","192.168.178.137"));
 assert(webGuardOriginAllowed("http://192.168.178.137","192.168.178.137"));
 assert(webGuardOriginAllowed("http://pixelstop.local","pixelstop.local:80"));
 assert(!webGuardOriginAllowed("https://evil.example","192.168.178.137"));
 assert(!webGuardOriginAllowed("http://evil.example","192.168.178.137"));
 assert(!webGuardOriginAllowed("null","192.168.178.137"));
 assert(webGuardHostnameValid("pixelstop")&&webGuardHostnameValid("pixel-stop-2"));
 assert(!webGuardHostnameValid("")&&!webGuardHostnameValid("Pixel Stop")&&!webGuardHostnameValid("-pixel")&&!webGuardHostnameValid("pixelstöp")&&!webGuardHostnameValid("abcdefghijabcdefghijabcdefghijabc"));
 // Airline names: brand, shortened, two parts, small type
 {char b[19];assert(airlineBrand("KLM1234",b,sizeof(b))&&!strcmp(b,"KLM"));assert(airlineBrand("VOI700",b,sizeof(b))&&!strcmp(b,"Volaris"));assert(!airlineBrand("ZZZ123",b,sizeof(b)));assert(!airlineBrand("KLM",b,sizeof(b)));}
 assert(parts("Turkish Airlines",1)=="Turkish Airlines");
 assert(parts("Cargolux Airlines International",1)=="Cargolux Airlines");
 assert(parts("KLM Royal Dutch Airlines",1)=="KLM Royal Dutch");
 assert(parts("Air Mobility Command",2)=="Air Mobility|Command");
 assert(parts("Aerovias de Mexico SA de CV",2)=="Aerovias de Mexico|SA de CV");
 assert(parts("Internationalairlinesgroupholding",0).size());
 {char a[40];airlineShorten("The Very Long Name Airlines Inc.",a,sizeof(a),18);assert(!strcmp(a,"Very Long Name"));}
 // Aircraft: manufacturer + model
 {char o[24];assert(aircraftMakerModel("Northrop Grumman E-2D Advanced Hawkeye",o,sizeof(o))&&!strcmp(o,"Northrop E-2D"));assert(!aircraftMakerModel("Unknown Jet",o,sizeof(o)));assert(aircraftDisplayLabel("F5",o,sizeof(o))&&!strcmp(o,"Northrop F-5"));}
 assert(!metadataTypeMatches("","A3","ST75"));
 assert(metadataTypeMatches("","A3","A21N"));
 assert(metadataTypeMatches("B39M","A3","B39M"));
 assert(!metadataTypeMatches("A21N","A3","ST75"));
 assert(!metadataTypeMatches("","","A21N"));
 assert(!metadataTypeMatches("","A1","A21N"));
  assert(decode("é")==decode("e\xCC\x81"));
  assert(decode("č")==decode("c\xCC\x8C"));
  assert(decode("ą")==decode("a\xCC\xA8"));
  for(auto s:{"♥","❤","❤️","💛","💚","💙","💜","🖤","🤍","🤎","🩷","🩵","🩶","💔","💞","❤️‍🔥","❤️‍🩹"})assert(decode(s)==std::vector<uint32_t>{0x2665});
  assert(decode("🤏")==std::vector<uint32_t>{0x1F90F});
  assert(decode("A❤️B").size()==3);
  assert(decode("?")[0]=='?');assert(specialGlyph('?')==nullptr);
  assert(decode("🚀")[0]==0x1F680);assert(specialGlyph(0x1F680)==nullptr);
  auto broken=decode(std::string("\xE2" "A"));assert(broken.size()==2&&broken[0]==0xFFFD&&broken[1]=='A');
  assert(decode("\xC0\xAF")[0]==0xFFFD);assert(decode("\xED\xA0\x80")[0]==0xFFFD);
  assert(decode("\xF4\x90\x80\x80")[0]==0xFFFD);
  for(uint32_t cp:decode("éèêëçČčŘřŠšŽžĚěŮůŇňŤťĎďĄąĆćĘęŁłŃńŚśŹźŻżÖöÜüÄäßØøÆæŒœ"))assert(specialGlyph(cp));
  for(uint32_t cp:decode("ȘșȚțİıĞğŐőŰűĲĳĿŀƏəΑθήναΘεσσαλονίκηΐΰМоскваКиївЇїҐґЄєЎўЂђЉљЊњЋћЏџҚқӘәӨөҮүҰұҢңҒғҺһ“”‘’…№€"))assert(specialGlyph(cp));
  for(uint32_t cp=0x400;cp<=0x45F;cp++)assert(specialGlyph(cp));
  for(uint32_t cp=0xC0;cp<=0x17F;cp++)if(cp!=0xD7&&cp!=0xF7)assert(specialGlyph(cp));
  assert(decode("ЁЙЇάΐ")==decode("Е\xCC\x88" "И\xCC\x86" "І\xCC\x88" "α\xCC\x81" "ι\xCC\x88\xCC\x81"));
  auto small=radarTlsBudget(512,4096),medium=radarTlsBudget(1024,4096),large=radarTlsBudget(4096,4096);
  assert(small.block==medium.block&&medium.block==large.block);
  assert(small.total<medium.total&&medium.total<large.total);
  assert(radarTlsFits(25248,11368,512,4096)); // Felix's blocked v0.4.6 screenshot
  assert(!radarTlsFits(small.total-1,20000,512,4096));
  assert(!radarTlsFits(30000,small.block-1,512,4096));
  assert(!radarExpired(30999,1000,true)&&radarExpired(31000,1000,true));
  assert(!radarExpired(50000,1000,false));
  assert(radarExpired(29990,0xFFFFFFF6U,true)); // millis wraparound
  for(int code:{0,1,2,3,45,48,51,53,55,56,57,61,63,65,66,67,71,73,75,77,80,81,82,85,86,95,96,99}){
    auto w=weatherVisual(code);assert(w.label&&w.intensity<=3);
    for(uint32_t cp:decode(w.label))assert(cp<128||specialGlyph(cp));
    assert(decode(w.label).size()*6<=224);
  }
  assert(weatherVisual(51).intensity==1&&weatherVisual(55).intensity==3);
  assert(weatherVisual(45).motif==WX_FOG&&weatherVisual(48).motif==WX_FOG);
  assert(weatherVisual(96).motif==WX_THUNDER_HAIL&&weatherVisual(99).intensity==3);
  assert(weatherVisual(50).motif==WX_CLOUD); // unsupported codes are never invented rain
  assert(timeZoneIndex("bad/zone")==-1&&timeZoneIndex(nullptr)==-1);
  auto localHour=[](const char* zone,time_t epoch){setenv("TZ",timeZoneRule(timeZoneIndex(zone)),1);tzset();struct tm t={};localtime_r(&epoch,&t);return t.tm_hour;};
  assert(localHour("Europe/Berlin",1768478400)==13); // 2026-01-15 12:00 UTC
  assert(localHour("Europe/Berlin",1784116800)==14); // 2026-07-15 12:00 UTC
  assert(localHour("America/New_York",1768478400)==7);
  assert(localHour("America/New_York",1784116800)==8);
  assert(localHour("Asia/Tokyo",1768478400)==21);
  assert(localHour("Australia/Sydney",1768478400)==23);
  assert(localHour("Australia/Sydney",1784116800)==22);
  FlightRoute r={};assert(parseFlightRoute("LEJ-BLR-HKG",r));assert(r.count==3&&!strcmp(r.airports[1],"BLR"));
  assert(parseFlightRoute(" lej - blr - hkg ",r));assert(r.count==3&&!strcmp(r.airports[0],"LEJ"));
  assert(parseFlightRoute("OSL-IST",r));assert(r.count==2);
  assert(parseFlightRoute("AAA-BBB-CCC-DDD-EEE-FFF-GGG-HHH-III",r));assert(r.count==8&&r.truncated);
  for(auto s:{"", "unknown", "LEJ", "LEJ-", "LEJ- ", "LEJ--HKG", "LEJ-N/A-HKG", "LEJ-BL1-HKG", "LEJJJ-HKG", "1EJJ-HKG", "-HKG"}){assert(!parseFlightRoute(s,r));assert(r.count==0);}
  assert(parseFlightRoute("FRA-EDAK",r)&&r.count==2&&!strcmp(r.airports[1],"EDAK"));
  assert(parseFlightRoute("K1G4-KJFK",r)&&!strcmp(r.airports[0],"K1G4"));
  puts("Latin/Greek/Cyrillic, UTF-8/hearts, routes, radar memory/expiry, weather and time zones: OK");
}
