#pragma once
#include <stdint.h>
#include <string.h>

// Fixed storage keeps API-controlled data bounded on the ESP8266.
// Airport codes: 3 letters (IATA) or 4 characters (ICAO, for airfields without
// an IATA code; letter first, then letters or digits).
static constexpr uint8_t ROUTE_CODE_SIZE=5;
struct FlightRoute {
  static constexpr uint8_t capacity=8;
  char airports[capacity][ROUTE_CODE_SIZE];
  uint8_t count;
  bool truncated;
};
static inline bool routeCode(const char* code) {
  if(!code)return false;
  size_t len=strlen(code);
  if(len!=3&&len!=4)return false;
  if(code[0]<'A'||code[0]>'Z')return false;
  for(uint8_t i=1;i<len;i++){
    bool letter=code[i]>='A'&&code[i]<='Z',digit=code[i]>='0'&&code[i]<='9';
    if(!letter&&!(digit&&len==4))return false;
  }
  return true;
}
static inline bool appendRouteAirport(FlightRoute& route,const char* code) {
  if(!routeCode(code))return false;
  if(route.count<FlightRoute::capacity)memcpy(route.airports[route.count++],code,ROUTE_CODE_SIZE);
  else route.truncated=true;
  return true;
}
static inline bool parseFlightRoute(const char* text,FlightRoute& out) {
  out={};if(!text)return false;
  FlightRoute next={};const char* p=text;
  while(*p){
    while(*p==' '||*p=='\t')p++;
    char code[ROUTE_CODE_SIZE]={};uint8_t len=0;
    while(*p&&*p!='-'&&*p!=' '&&*p!='\t'){
      char c=*p++;if(c>='a'&&c<='z')c-=32;
      if(len>=ROUTE_CODE_SIZE-1)return false;
      code[len++]=c;
    }
    if(!appendRouteAirport(next,code))return false;
    while(*p==' '||*p=='\t')p++;
    if(!*p)break;
    if(*p++!='-'||!*p)return false;
  }
  if(next.count<2)return false;
  out=next;return true;
}
