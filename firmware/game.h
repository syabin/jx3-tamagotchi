/* game.h — 完整状态机（v8.3 全量玩法） */
#ifndef JX3_GAME_H
#define JX3_GAME_H

#include <Arduino.h>
#include "config.h"

/* 存档结构：定长、可直接二进制写 NVS。冷却用 epoch 秒，重启不失效。 */
struct GameState {
  uint8_t  version;
  uint8_t  sect;            // 旧档占位（v0.2.7 取消门派，不再使用）
  float    hunger;
  float    mood;
  int32_t  exp;
  int32_t  coins;
  uint8_t  homeLv;
  bool     poisoned;
  bool     down;
  uint8_t  mess;
  float    poisonedH;       // 中毒累计游戏小时
  float    hungerH;         // 体力见底累计
  float    moodH;           // 心情见底累计
  float    recoverH;        // 重伤静养累计
  uint32_t bornTs;          // 出生 epoch 秒（游戏日从辰时起算）
  uint32_t lastTs;          // 上次结算 epoch 秒
  uint8_t  bag[GOODS_COUNT];
  uint32_t cool[GOODS_COUNT];   // 各物品冷却截止 epoch 秒
  uint32_t coolAct[4];          // sit / train / dig / dungeon
  uint8_t  equip[3];            // 装备槽，255=空，值=GOODS 索引
  uint8_t  decos[3];            // 装饰是否已布置
  float    nannyGH;             // 保姆代管剩余游戏时辰
  uint32_t loginDay;            // yyyymmdd
  uint16_t loginStreak;
  uint8_t  speedIdx;            // 0=×1 1=×10 2=×60 3=×600
  bool     muted;
  uint8_t  lastTitleIdx;
  uint32_t dungeonUntil;
  uint8_t  dungeonKey;          // 0..3，255=无
  uint8_t  reserved[8];
};

/* 奇遇邮箱（v0.2.8）：内存队列，重启清空（存档结构不变，避免旧档失效） */
#define MAIL_MAX 8
struct MailEntry { uint8_t scene; uint8_t reward; uint16_t amount; uint32_t ts; };
extern MailEntry G_mail[MAIL_MAX];
extern uint8_t   G_mailCount;
extern void mailAdd(uint8_t scene, uint8_t reward, uint16_t amount);

extern GameState G;
extern bool      G_hasSave;
extern uint32_t  G_tickMs;
extern uint32_t  G_lastSaveMs;
extern const char* G_toast;
extern uint32_t  G_toastUntil;
extern uint32_t  G_hintUntil;
extern const char* G_hint;
extern FxKind    G_fxKind;
extern uint8_t   G_fxIdx;
extern uint32_t  G_fxUntil;
extern bool      G_wasSleep;
extern bool      G_needRefresh;

/* 生命周期 */
void    gameInit();
void    gameNew();          // v0.2.7：取消门派参数
bool    gameLoad();
void    gameSave(bool force = false);
void    gameTick();
void    gameAdvance(uint32_t secReal);   // 离线结算

/* 时间 / 状态 */
uint32_t nowEpoch();
void     setClock(uint32_t epoch);
float    gameHour();
bool     isSleeping();
float    actPenalty();
const char* condText();
int      titleIndex();
const char* titleName();
const char* reputationName();
int      bagCount(uint8_t idx);
int      bagTotal();
bool     hasEq(uint8_t idx);
int      decoCount();
int      moodDecayCutPct();
int      hungerDecayCutPct();
void     toast(const char* msg);
void     hint(const char* msg);
void     checkTitle();
void     emitTone(uint16_t freq, uint16_t ms);

/* 行为 */
bool  actEat(uint8_t idx);
bool  actUseMed();
bool  actUsePage();
bool  actEquipToggle(uint8_t idx);
bool  actSell(uint8_t idx);
bool  actBuy(uint8_t idx);
bool  actSit();
bool  actTrain();
bool  actDig();
bool  actClean();
bool  actUpgradeHome();
bool  actNanny();
void  actDungeon(uint8_t key);
void  gambleRoll();
void  checkDailyLogin();

/* 小游戏状态 */
struct GambleState {
  uint8_t phase;      // ActPhase
  uint8_t side;       // 0=小 1=大
  uint8_t betIdx;     // 0..4
  uint8_t dice[3];
  uint8_t sum;
  bool    baozi;
  bool    win;
  int     pay;
  uint16_t rollTick;
};
extern GambleState GM;
extern int gambleCap();          // 当前金币允许的最大注档下标

#endif /* JX3_GAME_H */
