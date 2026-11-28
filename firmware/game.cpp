/* game.cpp — 存档与玩法逻辑（对齐网页版数值） */
#include "game.h"
#include <Preferences.h>
#include <stdarg.h>
#include <string.h>

SaveData G;
bool G_hasSave = false;
char g_toast[72] = { 0 };
uint32_t g_toastUntil = 0;

static uint32_t s_frame = 0;
static uint32_t s_bootMs = 0;
static uint32_t s_baseSec = 0;

uint32_t frameCounter() { return s_frame; }
void frameTick() { s_frame++; }

uint32_t gameNowSec() { return s_baseSec + (millis() - s_bootMs) / 1000UL; }

/* ================= 存档 ================= */
static void defaults() {
  memset(&G, 0, sizeof(G));
  G.magic = SAVE_MAGIC;
  G.ver = SAVE_VER;
  G.hunger = 80; G.mood = 80; G.exp = 0; G.coins = 200;
  G.homeLv = 0; G.mess = 0; G.flags = 0;
  G.bornSec = 0; G.totalSec = 0;
  for (int i = 0; i < CD_N; i++) G.cool[i] = -1;
  G.bag[G_BAOZI] = 2; G.bag[G_PILL] = 1;
  for (int i = 0; i < 3; i++) G.equip[i] = 0xFF;
  G.decos = 0; G.nannyTenth = 0;
  G.loginStreak = 0; G.lastLoginDay = 0xFFFFFFFFUL;
  G.mailN = 0;
  G.lastTitle = 0;
  G.dungeonUntil = -1; G.dungeonIdx = 0;
}

void gameNew() {
  defaults();
  G.bornSec = 0;
  G.totalSec = 0;
  s_baseSec = 0;
  s_bootMs = millis();
  G_hasSave = true;
  gameSave();
}

void gameSave() {
  G.totalSec = gameNowSec();
  Preferences p;
  if (!p.begin(NVS_NAMESPACE, false)) return;
  p.putBytes(NVS_KEY, &G, sizeof(G));
  p.end();
}

static bool loadFromNVS() {
  Preferences p;
  if (!p.begin(NVS_NAMESPACE, true)) return false;
  size_t n = p.getBytesLength(NVS_KEY);
  bool ok = false;
  if (n == sizeof(G)) {
    SaveData tmp;
    if (p.getBytes(NVS_KEY, &tmp, sizeof(tmp)) == sizeof(tmp) && tmp.magic == SAVE_MAGIC && tmp.ver == SAVE_VER) {
      G = tmp; ok = true;
    }
  }
  p.end();
  return ok;
}

void gameInit() {
  s_bootMs = millis();
  if (loadFromNVS()) {
    G_hasSave = true;
    s_baseSec = G.totalSec;
    /* 离线结算：最多 4 游戏小时 */
    uint32_t nowReal = G.totalSec;
    (void)nowReal;
    if (G.hunger > 100) G.hunger = 100;
    if (G.mood > 100) G.mood = 100;
    if (G.coins > 60000) G.coins = 60000;
    for (int i = 0; i < GOODS_COUNT; i++) if (G.bag[i] > 999) G.bag[i] = 999;
    if (G.mailN > MAIL_MAX) G.mailN = MAIL_MAX;
  } else {
    defaults();
    s_baseSec = 0;
    G.bornSec = 0;
    G_hasSave = false;
  }
  s_bootMs = millis();
}

/* ================= 基础查询 ================= */
uint8_t gameHour() { return (uint8_t)(((gameNowSec() - G.bornSec) / 3600UL + 8UL) % 24UL); }
bool gameIsDay() { uint8_t h = gameHour(); return h > 6 && h < 18; }
bool gameIsSleeping() { uint8_t h = gameHour(); return h >= 22 || h < 6; }

bool cooling(uint8_t id) {
  if (id >= CD_N) return false;
  return G.cool[id] > (int32_t)gameNowSec();
}
void setCool(uint8_t id, uint32_t ms) {
  if (id >= CD_N) return;
  G.cool[id] = (int32_t)(gameNowSec() + ms / 1000UL);
}

int titleIndex() {
  int k = 0;
  for (int i = 4; i >= 0; i--) if (G.exp >= TITLES[i].exp) { k = i; break; }
  return k;
}
const char* titleName() { return TITLES[titleIndex()].name; }
const char* repName() {
  int k = 0;
  for (int i = 4; i >= 0; i--) if (G.exp >= REPUTATIONS[i].exp) { k = i; break; }
  return REPUTATIONS[k].name;
}
uint16_t repColor() {
  int k = 0;
  for (int i = 4; i >= 0; i--) if (G.exp >= REPUTATIONS[i].exp) { k = i; break; }
  return REPUTATIONS[k].color;
}
uint8_t homeNameIdx() { return G.homeLv > 4 ? 4 : G.homeLv; }

uint16_t bagCount(uint8_t gid) { return gid < GOODS_COUNT ? G.bag[gid] : 0; }

bool hasEquip(uint8_t gid) {
  for (int i = 0; i < 3; i++) if (G.equip[i] == gid) return true;
  return false;
}
uint8_t decoCount() {
  uint8_t n = 0;
  for (int i = 0; i < 3; i++) if (G.decos & (1 << i)) n++;
  return n;
}

void toast(const char* msg) {
  if (!msg) return;
  strncpy(g_toast, msg, sizeof(g_toast) - 1);
  g_toast[sizeof(g_toast) - 1] = 0;
  g_toastUntil = millis() + TOAST_MS;
}

void toastf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(g_toast, sizeof(g_toast), fmt, ap);
  va_end(ap);
  g_toastUntil = millis() + TOAST_MS;
}

/* ================= 时间推进 ================= */
static float hungerDecayCut() { return (G.decos & 0x04) ? 0.06f : 0; }      // 长明灯
static float moodDecayCut() {
  float cut = hasEquip(G_A_ROBE) ? 0.20f : 0;
  if (G.decos & 0x01) cut += 0.05f;      // 盆栽
  if (G.decos & 0x02) cut += 0.08f;      // 屏风
  if (G.decos & 0x04) cut += 0.06f;      // 长明灯
  return cut > 0.6f ? 0.6f : cut;
}
static bool isPoisoned() { return (G.flags & 0x01) != 0; }
static bool isDown() { return (G.flags & 0x02) != 0; }

void applyTime(float dtSec) {
  float gh = dtSec / 3600.0f;
  if (gh <= 0) return;
  bool sleeping = gameIsSleeping();
  float hMul = 1, mMul = 1;
  if (G.nannyTenth > 0) { hMul *= 0.2f; mMul *= 0.2f; }
  else if (sleeping) { hMul *= 0.35f; mMul *= 0.2f; }
  hMul *= 1 - hungerDecayCut();
  mMul *= 1 - moodDecayCut();
  if (isPoisoned()) hMul *= 2;
  if (G.mess > 0) mMul *= 2;
  mMul *= 1 - 0.15f * G.homeLv;

  float h = G.hunger - 9 * gh * hMul;
  float m = G.mood - 7 * gh * mMul;
  G.hunger = (uint8_t)(h < 0 ? 0 : (h > 100 ? 100 : h));
  G.mood = (uint8_t)(m < 0 ? 0 : (m > 100 ? 100 : m));

  if (G.nannyTenth > 0) {
    G.nannyTenth = (G.nannyTenth > gh * 10) ? (uint16_t)(G.nannyTenth - gh * 10) : 0;
    if (G.mess > 0) G.mess = 0;
  }
  /* 饿到 0 → 重伤 */
  if (G.hunger == 0 && !isDown()) { G.flags |= 0x02; toast("侠士饿晕了，落下重伤"); }

  /* 副本结算 */
  if (G.dungeonUntil > 0 && (int32_t)gameNowSec() >= G.dungeonUntil) {
    G.dungeonUntil = -1;
    if (G.dungeonIdx < DNG_N) {
      const DngDef& d = DUNGEONS[G.dungeonIdx];
      float pen = isDown() ? 0.6f : (isPoisoned() ? 0.8f : 1.0f);
      float winP = 0.4f + G.exp / 500.0f + G.hunger / 300.0f + (hasEquip(G_A_CHARM) ? 0.15f : 0);
      if (winP > 0.9f) winP = 0.9f;
      bool win = (random(0, 10000) / 10000.0f) < winP;
      if (!win) {
        int ge = (int)(5 * pen); if (ge < 1) ge = 1;
        G.exp += ge;
        G.mood = (G.mood > 10) ? G.mood - 10 : 0;
        toastf("【%s】团灭了…修为+%d", d.name, ge);
      } else {
        int coin = (int)((d.coinMin + random(0, d.coinMax - d.coinMin + 1)) * pen);
        int ex = (int)((d.expMin + random(0, d.expMax - d.expMin + 1)) * pen);
        if (ex < 1) ex = 1;
        G.coins = (G.coins + coin > 60000) ? 60000 : G.coins + coin;
        G.exp += ex;
        char got[48] = { 0 };
        for (int i = 0; i < 4; i++) {
          const DropDef& dr = DROPS[G.dungeonIdx][i];
          if (dr.permille == 0) continue;
          if (random(0, 1000) >= (int)(dr.permille * pen)) continue;
          if (dr.gid == DROP_COIN) {
            int g2 = (int)(100 * pen);
            G.coins = (G.coins + g2 > 60000) ? 60000 : G.coins + g2;
            strncat(got, "金币", sizeof(got) - strlen(got) - 1);
          } else if (dr.gid < GOODS_COUNT) {
            G.bag[dr.gid]++;
            strncat(got, GOODS[dr.gid].name, sizeof(got) - strlen(got) - 1);
          }
          strncat(got, " ", sizeof(got) - strlen(got) - 1);
        }
        /* 团本 0.1% 玄晶 */
        if (G.dungeonIdx == 3 && random(0, 1000) == 0) {
          G.bag[G_XJ_ZT]++;
          toastf("橙光乍现！获得%s", GOODS[G_XJ_ZT].name);
        } else if (got[0]) {
          toastf("【%s】通关 金+%d 修为+%d 得%s", d.name, coin, ex, got);
        } else {
          toastf("【%s】通关 金+%d 修为+%d", d.name, coin, ex);
        }
      }
      checkTitle();
    }
  }

  /* 随机事件 */
  if (!isDown() && !isPoisoned() && G.nannyTenth == 0 && !sleeping) {
    if ((random(0, 10000) / 10000.0f) < 0.5f * gh && G.mess < 3) {
      G.mess++;
      toast("狼牙袭击！现场一片狼藉…");
    }
    if ((random(0, 10000) / 10000.0f) < 0.12f * gh) {
      G.flags |= 0x01;
      toast("中毒了！收益 -20%，玉露丸可解");
    }
    if ((random(0, 10000) / 10000.0f) < 0.10f * gh) rollEncounter();
  }
}

void checkTitle() {
  int k = titleIndex();
  if (k != G.lastTitle) {
    G.lastTitle = k;
    toastf("称号晋升为「%s」！", TITLES[k].name);
  }
}

/* ================= 每日登录（按游戏天） ================= */
void checkDailyLogin() {
  uint32_t day = gameNowSec() / 86400UL;
  if (G.lastLoginDay == day) return;
  bool cont = (G.lastLoginDay != 0xFFFFFFFFUL) && (G.lastLoginDay + 1 == day);
  G.loginStreak = cont ? (uint8_t)(G.loginStreak + 1) : 1;
  G.lastLoginDay = day;
  int reward = 50 + (G.loginStreak - 1) * 25;
  if (reward > 200) reward = 200;
  G.coins = (G.coins + reward > 60000) ? 60000 : G.coins + reward;
  G.bag[G_BAOZI]++;
  if (random(0, 100) < 30) {
    /* 鸿运当头：凡品×5 良品×2 珍品×1 加权 */
    static const uint8_t pool[] = {
      G_W_SWORD, G_W_SWORD, G_W_SWORD, G_W_SWORD, G_W_SWORD,
      G_W_PICK,  G_W_PICK,  G_W_PICK,  G_W_PICK,  G_W_PICK,
      G_T_GOURD, G_T_GOURD, G_T_GOURD, G_T_GOURD, G_T_GOURD,
      G_A_ROBE,  G_A_ROBE,  G_A_CHARM, G_A_CHARM, G_T_JADE, G_T_JADE
    };
    uint8_t gid = pool[random(0, sizeof(pool))];
    G.bag[gid]++;
    toastf("鸿运当头！赠「%s」 金+%d", GOODS[gid].name, reward);
  } else {
    toastf("每日登录第%d天 金+%d 包子×1", G.loginStreak, reward);
  }
  gameSave();
}

/* ================= 奇遇 ================= */
void rollEncounter() {
  uint8_t i = random(0, MAIL_N);
  const MailSceneDef& sc = MAIL_SCENES[i];
  uint16_t gain = 0;
  if (sc.reward == RW_EXP) {
    gain = sc.minv + random(0, sc.maxv - sc.minv + 1);
    G.exp += gain;
  } else if (sc.reward == RW_COIN) {
    gain = sc.minv + random(0, sc.maxv - sc.minv + 1);
    G.coins = (G.coins + gain > 60000) ? 60000 : G.coins + gain;
  } else {
    G.bag[sc.item]++;
    gain = 1;
  }
  /* 入信箱（最近的在最前） */
  if (G.mailN < MAIL_MAX) G.mailN++;
  for (int k = G.mailN - 1; k > 0; k--) G.mail[k] = G.mail[k - 1];
  G.mail[0].scene = i;
  G.mail[0].reward = sc.reward;
  G.mail[0].amount = gain;
  G.mail[0].ts = gameNowSec();
  toast("奇遇！获得邮件一封");
  gameSave();
}

/* ================= 行动 ================= */
static float actPenalty() { return isDown() ? 0.6f : (isPoisoned() ? 0.8f : 1.0f); }

static bool canAct(const char** why) {
  if (G.dungeonUntil > 0 && (int32_t)gameNowSec() < G.dungeonUntil) { *why = "副本进行中，侠士在拼命"; return false; }
  if (gameIsSleeping()) { *why = "侠士睡熟了，醒了再忙"; return false; }
  return true;
}

void doSit() {
  const char* why;
  if (!canAct(&why)) { toast(why); return; }
  if (cooling(CD_SIT)) { toast("气息未定，稍后再打坐"); return; }
  float p = actPenalty();
  int ge = (int)(3 * p + 0.5f);
  if (ge < 1) ge = 1;
  if (hasEquip(G_T_GOURD)) ge += 2;
  G.mood = (G.mood + 20 > 100) ? 100 : G.mood + 20;
  G.hunger = (G.hunger > 3) ? G.hunger - 3 : 0;
  G.exp += ge;
  setCool(CD_SIT, 15000);
  toastf("打坐吐纳，心境澄明 修为+%d", ge);
  checkTitle();
  gameSave();
}

void doTrain() {
  const char* why;
  if (!canAct(&why)) { toast(why); return; }
  if (G.hunger < 20) { toast("饿得握不动剑，先吃点东西"); return; }
  if (cooling(CD_TRAIN)) { toast("木桩刚打完，歇口气"); return; }
  float p = actPenalty();
  int ge = (int)(12 * p * (hasEquip(G_W_SWORD) ? 1.5f : 1.0f) + 0.5f);
  if (ge < 1) ge = 1;
  G.exp += ge;
  G.hunger = (G.hunger > 8) ? G.hunger - 8 : 0;
  G.mood = (G.mood > 5) ? G.mood - 5 : 0;
  setCool(CD_TRAIN, 25000);
  toastf("打木桩一百式 修为+%d", ge);
  checkTitle();
  gameSave();
}

void doDig() {
  const char* why;
  if (!canAct(&why)) { toast(why); return; }
  if (cooling(CD_DIG)) { toast("铁锹都挖冒烟了，歇会儿"); return; }
  if (G.hunger < 10) { toast("饿得举不起锹，先吃点东西"); return; }
  float p = actPenalty();
  G.hunger = (G.hunger > 6) ? G.hunger - 6 : 0;
  setCool(CD_DIG, 30000);
  int r = random(0, 100);
  float pickMul = hasEquip(G_W_PICK) ? 1.5f : 1.0f;
  if (r < 55) {
    int g = (int)((50 + random(0, 110)) * p * pickMul);
    G.coins = (G.coins + g > 60000) ? 60000 : G.coins + g;
    toastf("挖到 %d 金币！", g);
  } else if (r < 80) {
    int ge = (int)(3 * p + 0.5f); if (ge < 1) ge = 1;
    int g = (int)(100 * p * pickMul);
    G.coins = (G.coins + g > 60000) ? 60000 : G.coins + g;
    G.exp += ge;
    toastf("挖出《剑意残页》 修为+%d", ge);
    checkTitle();
  } else if (r < 93) {
    toast("挖了半天，只有一块石头…");
  } else if (G.mess < 3) {
    G.mess++;
    toast("挖塌了土堆，弄得到处是泥…");
  } else {
    toast("挖到一块碎瓦，正好补屋顶");
  }
  gameSave();
}

void doClean() {
  if (G.mess == 0) { toast("屋里挺干净的，无需打扫"); return; }
  float p = actPenalty();
  G.mess = 0;
  G.mood = (G.mood + 5 > 100) ? 100 : G.mood + 5;
  G.exp += (int)(1 * p + 0.5f);
  toast("打扫完毕，窗明几净 修为+1");
  gameSave();
}

void doNanny() {
  if (G.nannyTenth > 0) { toast("保姆正在照料中"); return; }
  if (G.coins < 100) { toast("雇佣保姆需要 100 金币"); return; }
  G.coins -= 100;
  G.nannyTenth = 80;      // 8 游戏小时
  toast("已雇保姆 8 时辰：代管起居、自动洒扫");
  gameSave();
}

void doHomeUpgrade() {
  if (G.homeLv >= 4) { toast("家园已至「山庄」，此乃极致"); return; }
  int cost = (G.homeLv + 1) * 500;
  if (G.coins < cost) { toastf("金币不足，还差 %d 金", cost - G.coins); return; }
  G.coins -= cost;
  G.homeLv++;
  toastf("家园升为「%s」！", HOME_NAMES[G.homeLv]);
  gameSave();
}

void startDungeon(uint8_t idx) {
  const char* why;
  if (idx >= DNG_N) idx = 0;
  if (!canAct(&why)) { toast(why); return; }
  if (G.hunger < 30) { toast("体力不足 30，下副本会送人头"); return; }
  if (cooling(CD_DUNGEON)) { toast("侠士刚出本，缓缓再战"); return; }
  G.hunger = (G.hunger > 12) ? G.hunger - 12 : 0;
  G.mood = (G.mood > 6) ? G.mood - 6 : 0;
  setCool(CD_DUNGEON, 60000);
  G.dungeonUntil = (int32_t)(gameNowSec() + 10);
  G.dungeonIdx = idx;
  toastf("进入【%s】！战斗中…", DUNGEONS[idx].name);
  gameSave();
}

void eatGoods(uint8_t gid) {
  if (gid >= GOODS_COUNT) return;
  const GoodsDef& f = GOODS[gid];
  if (f.type != GT_FOOD) return;
  if (G.bag[gid] == 0) { toastf("行囊里没有%s了", f.name); return; }
  if (G.cool[CD_EAT] > (int32_t)gameNowSec()) { toastf("%s还没凉透，稍后再吃", f.name); return; }
  G.bag[gid]--;
  G.hunger = (G.hunger + f.gain > 100) ? 100 : G.hunger + f.gain;
  G.mood = (G.mood + f.mood > 100) ? 100 : G.mood + f.mood;
  setCool(CD_EAT, f.cd * 1000UL);
  if (random(0, 100) < 25 && G.mess < 3) { G.mess++; toast("吃剩的骨头丢了一地…记得打扫"); }
  else toastf("吃下%s，气力恢复！", f.name);
  gameSave();
}

void useMed() {
  if (!isDown() && !isPoisoned()) { toast("侠士好得很，不用吃药"); return; }
  if (G.bag[G_PILL] == 0) { toast("没有玉露丸了，去商城买一瓶"); return; }
  G.bag[G_PILL]--;
  G.flags = 0;
  if (G.hunger < 30) G.hunger = 30;
  G.mood = (G.mood + 10 > 100) ? 100 : G.mood + 10;
  toast("玉露丸下肚，满血复活！");
  gameSave();
}

void usePage(uint8_t gid) {
  if (gid != G_PAGE) gid = G_PAGE;
  if (G.bag[G_PAGE] == 0) { toast("行囊里没有剑意残页"); return; }
  G.bag[G_PAGE]--;
  G.exp += GOODS[G_PAGE].exp;
  toastf("参悟剑意残页 修为+%d", GOODS[G_PAGE].exp);
  checkTitle();
  gameSave();
}

void equipToggle(uint8_t gid) {
  if (gid >= GOODS_COUNT) return;
  const GoodsDef& g = GOODS[gid];
  if (g.type != GT_EQUIP) return;
  uint8_t slot = (g.slot >= 1 && g.slot <= 3) ? (g.slot - 1) : 0;
  if (G.equip[slot] == gid) {
    G.equip[slot] = 0xFF;
    toastf("卸下「%s」", g.name);
  } else {
    G.equip[slot] = gid;
    toastf("装备「%s」！", g.name);
  }
  gameSave();
}

void sellGoods(uint8_t gid) {
  if (gid >= GOODS_COUNT) return;
  const GoodsDef& g = GOODS[gid];
  if (!g.sell) return;
  if (G.bag[gid] == 0) { toastf("行囊里没有%s", g.name); return; }
  G.bag[gid]--;
  G.coins = (G.coins + g.sell > 60000) ? 60000 : G.coins + g.sell;
  toastf("卖出「%s」 金+%d", g.name, g.sell);
  gameSave();
}

void buyGoods(uint8_t gid) {
  if (gid >= GOODS_COUNT) return;
  const GoodsDef& g = GOODS[gid];
  if (!g.price) return;
  if (G.coins < g.price) { toastf("金币不足，还差 %d 金", g.price - G.coins); return; }
  G.coins -= g.price;
  if (g.type == GT_DECO) {
    uint8_t bit = (gid == G_D_PLANT) ? 0x01 : (gid == G_D_SCREEN) ? 0x02 : 0x04;
    G.decos |= bit;
    toastf("购得「%s」，已布置庭院", g.name);
  } else {
    G.bag[gid]++;
    toastf("购得「%s」！已入行囊", g.name);
  }
  gameSave();
}

/* ================= 姿态 ================= */
uint8_t petPose() {
  if (gameIsSleeping()) return 8;
  if (isDown()) return 3;
  if (isPoisoned()) return 2;
  if (G.dungeonUntil > 0 && (int32_t)gameNowSec() < G.dungeonUntil) return 4;
  uint32_t now = gameNowSec();
  if (G.cool[CD_DIG] > (int32_t)now && (uint32_t)(G.cool[CD_DIG] - 30) > now && (G.cool[CD_DIG] - (int32_t)now) > 24) return 7;
  if (G.cool[CD_SIT] > (int32_t)now && (G.cool[CD_SIT] - (int32_t)now) > 7) return 5;
  if (G.cool[CD_TRAIN] > (int32_t)now && (G.cool[CD_TRAIN] - (int32_t)now) > 16) return 6;
  if (G.cool[CD_EAT] > (int32_t)now && (G.cool[CD_EAT] - (int32_t)now) > 10) return 1;
  return 0;
}
