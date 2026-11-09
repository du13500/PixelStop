#include "Version.h"
#include "Diagnostics.h"
#include <string.h>
struct Event {uint32_t at,heap,block,value;int16_t code;uint8_t op,stage;};
static Event lastError={};static bool hasLastError=false;
static char lookupContext[64]={},lastErrorLookup[64]={};
static Event events[24]={};static uint8_t next=0,count=0;
// Start at byte 128, leaving the bootloader's first RTC words untouched.
struct Marker {uint32_t magic,version,boot,at,heap,block,value,code,op,stage,checksum;};
static Marker marker={},previous={};static bool previousValid=false;
static char resetReason[64]={},resetInfo[192]={};
static uint32_t deferredAt=0,deferredFor=0;
static const char ops[][20] PROGMEM={"Start","Radar.Position","Radar.Route","Radar.Metadata","Radar.Airline","Wetter","VVO.Abfahrten","VVO.Suche","Ort.Suche","Ort.Karte","Tracker.Status","Tracker.Position","Tracker.Route","Speichern","Neustart","Countdown","WLAN"};
static const char stages[][24] PROGMEM={"Start","TLS","HTTP","JSON","OK","Fehler","Heap-knapp","Pause","leer","Antwortformat","JSON:NoMemory","JSON:IncompleteInput","JSON:EmptyInput","JSON:TooDeep","JSON:InvalidInput","Empfang:Fehler","Datei:Fehler","Antwort:ZuGross","Geo:Vorbereitung","Geo:Verbinden","Geo:Empfang","Geo:TLS-frei","übersprungen"};
static uint32_t checksum(const Marker& m){uint32_t h=2166136261UL;const uint8_t* p=(const uint8_t*)&m;for(size_t i=0;i<sizeof(m)-sizeof(m.checksum);i++)h=(h^p[i])*16777619UL;return h;}
static void writeMarker(){marker.checksum=checksum(marker);ESP.rtcUserMemoryWrite(32,(uint32_t*)&marker,sizeof(marker));}
void diagInit(){
 hasLastError=false;
 previousValid=ESP.rtcUserMemoryRead(32,(uint32_t*)&previous,sizeof(previous))&&previous.magic==0x53445439UL&&previous.version==1&&previous.checksum==checksum(previous)&&previous.op<D_COUNT&&previous.stage<=DS_SKIPPED;
 strlcpy(resetReason,ESP.getResetReason().c_str(),sizeof(resetReason));strlcpy(resetInfo,ESP.getResetInfo().c_str(),sizeof(resetInfo));
 marker.magic=0x53445439UL;marker.version=1;marker.boot=previousValid?previous.boot+1:1;diagEvent(D_BOOT,DS_DONE);
}
const char* diagResetReason(){return resetReason;}
void diagLookup(const char* aircraft,const char* callsign){snprintf(lookupContext,sizeof(lookupContext),"Flugzeug %.15s | Callsign %.15s",aircraft?aircraft:"",callsign?callsign:"");}
static void writeEvent(const Event& e){events[next]=e;next=(next+1)%24;if(count<24)++count;}
static void markerFrom(const Event& e){marker.at=e.at;marker.heap=e.heap;marker.block=e.block;marker.value=e.value;marker.code=(uint32_t)(int32_t)e.code;marker.op=e.op;marker.stage=e.stage;writeMarker();}
void diagEvent(DiagOp op,DiagStage stage,int code,uint32_t value){
 Event e={millis(),ESP.getFreeHeap(),ESP.getMaxFreeBlockSize(),value,(int16_t)code,(uint8_t)op,(uint8_t)stage};writeEvent(e);
 if(stage==DS_BEGIN)lookupContext[0]=0;
 if(stage==DS_FAILED||stage==DS_HEAP||(stage>=DS_JSON_MEMORY&&stage<=DS_RESPONSE_LIMIT)){lastError=e;hasLastError=true;strlcpy(lastErrorLookup,lookupContext,sizeof(lastErrorLookup));}
 marker.at=e.at;marker.heap=e.heap;marker.block=e.block;marker.value=value;marker.code=(uint32_t)(int32_t)code;marker.op=op;marker.stage=stage;writeMarker();
}
void deferBackgroundNetwork(uint32_t ms){deferredAt=millis();deferredFor=ms;}
bool backgroundNetworkReady(){return (uint32_t)(millis()-deferredAt)>=deferredFor;}
void DiagRequest::note(DiagStage stage,int code){
 if(!compact_){diagEvent(op_,stage,code);return;}
 Event e={millis(),ESP.getFreeHeap(),ESP.getMaxFreeBlockSize(),0,(int16_t)code,(uint8_t)op_,(uint8_t)stage};
 if(e.heap<minHeap_)minHeap_=e.heap;
 if(e.block<minBlock_)minBlock_=e.block;
 if(pending_<6)steps_[pending_++]={e.at,e.heap,e.block,e.code,e.stage};
 if(stage==DS_BEGIN)lookupContext[0]=0;
 markerFrom(e); // the RTC marker still follows every step for crash analysis
}
DiagRequest::DiagRequest(DiagOp op):op_(op),started_(millis()),compact_(op==D_RADAR){note(DS_BEGIN,0);}
DiagRequest::~DiagRequest(){
 if(compact_&&(result_==DS_DONE||result_==DS_EMPTY)){
  uint32_t heap=ESP.getFreeHeap(),block=ESP.getMaxFreeBlockSize();
  Event e={millis(),minHeap_<heap?minHeap_:heap,minBlock_<block?minBlock_:block,millis()-started_,(int16_t)code_,(uint8_t)op_,(uint8_t)result_};
  writeEvent(e);markerFrom(e);
 }else{
  for(uint8_t i=0;i<pending_;i++){Event e={steps_[i].at,steps_[i].heap,steps_[i].block,0,steps_[i].code,(uint8_t)op_,steps_[i].stage};writeEvent(e);}
  diagEvent(op_,result_,code_,millis()-started_);
 }
 deferBackgroundNetwork();
}
void DiagRequest::stage(DiagStage stage){note(stage,code_);}
void DiagRequest::http(int code){code_=code;note(DS_HTTP,code);}
static void printEvent(Print& out,const Event& e){
 char op[20],stage[24];strncpy_P(op,ops[e.op],sizeof(op));strncpy_P(stage,stages[e.stage],sizeof(stage));
 char b[160];snprintf(b,sizeof(b),"%lu ms | %s | %s | HTTP %d | Heap %lu B | Block %lu B | Wert %lu\n",(unsigned long)e.at,op,stage,e.code,(unsigned long)e.heap,(unsigned long)e.block,(unsigned long)e.value);out.print(b);
}
void diagPrint(Print& out){
 out.print(F(FW_NAME " v" FW_VERSION " Diagnose\nWert: bei OK/Fehler Dauer in ms; bei Pause Wartezeit in ms. Zeit: seit jeweiligem Start.\nHTTP -1: Verbindung fehlgeschlagen; -11: Antwort-Timeout; 429: Anbieterlimit.\nRadar.Position: erfolgreiche Abrufe als eine Zeile mit dem kleinsten Heap waehrend des Abrufs.\nleer: Anbieter kennt keine Daten zum Flugzeug. uebersprungen (Wert): 1 Budget, 2 Pause, 3 Mindestintervall, 4 Speicher.\nReset: "));out.println(resetReason);out.print(F("Reset-Info: "));out.println(resetInfo);
 if(previousValid){out.print(F("Letzter RTC-Marker des vorherigen Starts (Hinweis, kein Ursachenbeweis):\n"));Event e={previous.at,previous.heap,previous.block,previous.value,(int16_t)previous.code,(uint8_t)previous.op,(uint8_t)previous.stage};printEvent(out,e);}else out.println(F("Kein gueltiger vorheriger RTC-Marker. Stromverlust/OTA kann ihn loeschen."));
 if(hasLastError){out.println(F("Letzter Fehler dieses Starts (separat behalten):"));printEvent(out,lastError);if(lastErrorLookup[0])out.println(lastErrorLookup);}
 out.println(F("Letzte 24 Ereignisse dieses Starts:"));for(uint8_t i=0;i<count;i++)printEvent(out,events[(next+24-count+i)%24]);
}
