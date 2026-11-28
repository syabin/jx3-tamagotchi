#pragma once
#include <Arduino.h>
#include "config.h"

struct MailEntry {
  uint8_t scene;
  uint8_t reward;      // RW_EXP / RW_COIN / RW_ITEM
  uint16_t amount;
  uint32_t ts;         // 游戏秒
};

struct SaveData {
  uint32_t magic;
  uint16_t ver;
  uint8_t  hunger, mood;
  uint16_t exp;
  uint16_t coins;
  uint8_t  homeLv, mess, flags;      // flags bit0 poisoned bit1 down
  uint32_t bornSec;                  // 建号时的总游戏秒
  uint32_t totalSec;                 // 累计游戏秒（1x 真实时间）
  int32_t  cool[CD_N];               // 冷却到期（总游戏秒，-1 表示无）
  uint16_t bag[GOODS_COUNT];
  uint8_t  equip[3];                 // 0xFF = 空
  uint8_t  decos;                    // bit0 盆栽 bit1 屏风 bit2 长明灯
  uint16_t nannyTenth;               // 保姆剩余 0.1 游戏小时
  uint8_t  loginStreak;
  uint32_t lastLoginDay;             // 总游戏秒 / 86400
  uint8_t  mailN;
  MailEntry mail[MAIL_MAX];
  uint8_t  lastTitle;
  int32_t  dungeonUntil;             // 副本结束时刻（总游戏秒）
  uint8_t  dungeonIdx;
};

extern SaveData G;
extern bool G_hasSave;
extern char g_toast[72];
extern uint32_t g_toastUntil;

void gameInit();
void gameNew();
void gameSave();

uint32_t frameCounter();
void     frameTick();

uint8_t gameHour();
bool    gameIsDay();
bool    gameIsSleeping();
uint32_t gameNowSec();

bool cooling(uint8_t id);
void setCool(uint8_t id, uint32_t ms);

int         titleIndex();
const char* titleName();
const char* repName();
uint16_t    repColor();
uint8_t     homeNameIdx();

void toast(const char* msg);
void toastf(const char* fmt, ...);

void applyTime(float dtSec);

/* 行动 */
void doSit();
void doTrain();
void doDig();
void doHomeUpgrade();
void doClean();
void doNanny();
void startDungeon(uint8_t idx);
void eatGoods(uint8_t gid);
void useMed();
void usePage(uint8_t gid);
void equipToggle(uint8_t gid);
void sellGoods(uint8_t gid);
void buyGoods(uint8_t gid);
void checkDailyLogin();
void rollEncounter();
void checkTitle();

uint8_t petPose();
uint16_t bagCount(uint8_t gid);
bool hasEquip(uint8_t gid);
uint8_t decoCount();
