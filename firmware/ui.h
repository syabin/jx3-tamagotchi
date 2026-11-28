#pragma once
#include <Arduino.h>
#include "config.h"
#include "game.h"

#define LIST_BAG  0
#define LIST_SHOP 1
#define GAME_HUB  0xFF

struct GameSt {
  uint8_t id;          // 0..4，GAME_HUB = 大厅
  uint8_t phase;
  uint8_t sel, betIdx, side;
  uint8_t dice[3]; uint8_t sum; bool win, baozi;
  uint16_t pay;
  int16_t mole; uint16_t next; uint16_t t; uint8_t cur, hits, swings;
  uint8_t pos, dir, cast, zoneL, zoneW; uint8_t gain, good;
  uint8_t faces[6]; bool open[6]; bool done[6]; int8_t first; uint8_t matched; uint16_t wait;
  struct HorseRun { float pos; uint8_t burst; } hs[4];
  uint8_t winner;
  char pop[28]; uint16_t popT;
};

struct UIState {
  uint8_t page;
  uint8_t menuIdx;
  uint8_t settingsFrom;
  uint8_t bagSel, shopSel;
  uint8_t itemFrom;
  uint8_t bagCat, shopCat;
  bool    catMode;
  uint8_t mailSel;
  bool    mailDetail;
  uint8_t actSel;
  uint8_t homeSel;
  uint8_t notePage;
  bool    noteDetail;
  uint8_t noteDetailPage;
  uint8_t setIdx;
  int8_t  setPage;     // -1 = 菜单列表
  bool    apOn, muted;
  uint8_t otaPhase, otaPct;
  GameSt  gm;
};

extern UIState UI;

void uiInit();
void uiRender();
void uiMoveSel(int8_t d);
void uiConfirmA();
bool uiBack();          // 返回 true 表示已处理
void uiLongB();         // 长按 B → 设置
void uiTick();          // 每秒逻辑
void uiFrameTick();     // 每帧（游戏动画）
