#pragma once
#include <Arduino.h>
enum DiagOp : uint8_t { D_BOOT,D_RADAR,D_RADAR_ROUTE,D_METADATA,D_AIRLINE,D_WEATHER,D_VVO,D_STOP_SEARCH,D_GEO_SEARCH,D_GEO_REVERSE,D_TRACK_STATUS,D_TRACK_LIVE,D_TRACK_ROUTE,D_CONFIG,D_REBOOT,D_COUNTDOWN,D_WIFI,D_WEB,D_COUNT };
enum DiagStage : uint8_t { DS_BEGIN,DS_TLS,DS_HTTP,DS_JSON,DS_DONE,DS_FAILED,DS_HEAP,DS_WAIT,DS_EMPTY,DS_SCHEMA,DS_JSON_MEMORY,DS_JSON_INCOMPLETE,DS_JSON_EMPTY,DS_JSON_DEPTH,DS_JSON_INVALID,DS_TRANSFER_ERROR,DS_FILE_ERROR,DS_RESPONSE_LIMIT,DS_GEO_PREPARE,DS_GEO_CONNECT,DS_GEO_RECEIVE,DS_GEO_RELEASE,DS_SKIPPED,
  DS_NO_WIFI,DS_ROUTE_PROVIDER,DS_ROUTE_POSITION,DS_ROUTE_DESCENT,DS_ROUTE_CALLSIGN,DS_WIFI_LOST,DS_WIFI_UP,DS_WIFI_RETRY,DS_RESCUE,DS_REJECTED,DS_LAST=DS_REJECTED };
void diagInit();
void diagEvent(DiagOp op,DiagStage stage,int code=0,uint32_t value=0);
void diagLookup(const char* aircraft,const char* callsign);
void diagPrint(Print& out);
const char* diagResetReason();
bool backgroundNetworkReady();
void deferBackgroundNetwork(uint32_t ms=750);
// Last time any request got an HTTP answer, and the last request attempt (millis).
uint32_t diagNetworkAnswerMs();
uint32_t diagNetworkAttemptMs();
// Radar positions run every few seconds: a successful request is logged as a
// single line (with the lowest heap seen during it); the individual steps are
// only written when the request fails. Other requests log every step.
class DiagRequest {
  DiagOp op_; uint32_t started_; int code_=0; DiagStage result_=DS_FAILED;
  bool compact_; uint8_t pending_=0; uint32_t minHeap_=0xFFFFFFFFUL,minBlock_=0xFFFFFFFFUL;
  struct Step { uint32_t at,heap,block; int16_t code; uint8_t stage; } steps_[6];
  void note(DiagStage stage,int code);
public:
  explicit DiagRequest(DiagOp op);
  ~DiagRequest();
  void stage(DiagStage stage);
  void http(int code);
  void finish(bool ok,DiagStage result=DS_DONE){result_=ok?result:DS_FAILED;}
};
