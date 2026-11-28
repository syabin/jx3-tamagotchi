/* game.cpp — v8.3 全量数值逻辑（与网页版 index.html 对齐） */
#include "game.h"
#include <Preferences.h>
#include <time.h>

Preferences prefs;

GameState G;
MailEntry G_mail[MAIL_MAX];
uint8_t   G_mailCount = 0;
void mailAdd(uint8_t scene, uint8_t reward, uint16_t amount) {
  if (G_mailCount >= MAIL_MAX) {
    for (int i = 1; i < MAIL_MAX; i++) G_mail[i-1] = G_mail[i];
    G_mailCount = MAIL_MAX - 1;
  }
  G_mail[G_mailCount].scene = scene;
  G_mail[G_mailCount].reward = reward;
  G_mail[G_mailCount].amount = amount;
  G_mail[G_mailCount].ts = nowEpoch();
  G_mailCount++;
}
bool G_hasSave = false;
uint32_t G_tickMs = 0, G_lastSaveMs = 0;
const char* G_toast = NULL;      uint32_t G_toastUntil = 0;
const char* G_hint = NULL;       uint32_t G_hintUntil = 0;
FxKind  G_fxKind = FX_NONE;      uint8_t G_fxIdx = 255;      uint32_t G_fxUntil = 0;
bool    G_wasSleep = false;
bool    G_needRefresh = true;
GambleState GM = { PH_SIDE, 1, 1, {1,1,1}, 0, false, false, 0, 0 };

/* 时钟基准：epoch = clockBase + millis()/1000 */
static uint32_t clockBase = 0;
static const int SPEEDS[4] = { 1, 10, 60, 600 };

/* ---------------- 时间 ---------------- */
static uint32_t compileEpoch() {
  /* 用 __DATE__ __TIME__ 作为首次启动的近似时间，之后由 NTP / NVS 校正 */
  struct tm t = {};
  char mon[8] = {0};
  sscanf(__DATE__, "%7s %d %d", mon, &t.tm_mday, &t.tm_year);
  static const char* MN[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  for (int i = 0; i < 12; i++) if (!strncmp(mon, MN[i], 3)) { t.tm_mon = i; break; }
  t.tm_year -= 1900;
  sscanf(__TIME__, "%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);
  return (uint32_t)mktime(&t);
}

uint32_t nowEpoch() { return clockBase + (uint32_t)(millis() / 1000); }
void setClock(uint32_t epoch) { clockBase = epoch - (uint32_t)(millis() / 1000); }

/* UTC+8 的 yyyymmdd（不依赖系统时区） */
static uint32_t epochYmd(uint32_t ep) {
  int64_t z = (int64_t)ep + 8 * 3600;
  int64_t days = z / 86400;
  int64_t sod  = z % 86400; if (sod < 0) { sod += 86400; days--; }
  /* Howard Hinnant civil_from_days */
  days += 719468;
  int64_t era = (days >= 0 ? days : days - 146096) / 146097;
  int64_t doe = days - era * 146097;
  int64_t yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
  int64_t y   = yoe + era * 400;
  int64_t doy = doe - (365*yoe + yoe/4 - yoe/100);
  int64_t mp  = (5*doy + 2)/153;
  int64_t d   = doy - (153*mp+2)/5 + 1;
  int64_t m   = mp + (mp < 10 ? 3 : -9);
  y += (m <= 2);
  (void)sod;
  return (uint32_t)(y * 10000 + m * 100 + d);
}

/* 注意：调用方大量使用局部 char buf[]，必须复制进静态缓冲区，不能只存指针。 */
static char toastBuf[72], hintBuf[72];
void toast(const char* msg) {
  if (!msg) return;
  strncpy(toastBuf, msg, sizeof(toastBuf) - 1);
  toastBuf[sizeof(toastBuf) - 1] = 0;
  G_toast = toastBuf; G_toastUntil = millis() + TOAST_MS;
}
void hint(const char* msg) {
  if (!msg) return;
  strncpy(hintBuf, msg, sizeof(hintBuf) - 1);
  hintBuf[sizeof(hintBuf) - 1] = 0;
  G_hint = hintBuf; G_hintUntil = millis() + HINT_MS;
}

void emitTone(uint16_t freq, uint16_t ms) {
#if ENABLE_BUZZER
  if (PIN_BUZZER < 0 || G.muted) return;
  ledcAttachPin(PIN_BUZZER, 0);
  ledcWriteTone(0, freq);
  delay(ms);
  ledcWrite(0, 0);
  ledcDetachPin(PIN_BUZZER);
#else
  (void)freq; (void)ms;
#endif
}

/* ---------------- 初始化 / 存档 ---------------- */
void gameInit() {
  memset(&G, 0, sizeof(G));
  prefs.begin(NVS_NAMESPACE, false);
  uint32_t ep = prefs.getUInt(NVS_TIME_KEY, 0);
  prefs.end();
  setClock(ep ? ep : compileEpoch());
  G.sect = 0;   /* 旧字段占位，不再表示门派 */
  G.lastTitleIdx = 0;
  G_hasSave = gameLoad();
  G_tickMs = millis();
  G_lastSaveMs = millis();
  if (G_hasSave) {
    G_needRefresh = true;   /* 首次 tick 会做离线结算 */
  }
}

void gameNew() {
  memset(&G, 0, sizeof(G));
  G.version    = SAVE_VERSION;
  G.sect       = 0;   /* 旧字段占位 */
  G.hunger     = 80;
  G.mood       = 80;
  G.exp        = 0;
  G.coins      = 200;
  G.homeLv     = 0;
  G.bornTs     = nowEpoch();
  G.lastTs     = G.bornTs;
  for (int i = 0; i < GOODS_COUNT; i++) { G.bag[i] = 0; G.cool[i] = 0; }
  for (int i = 0; i < 4; i++) G.coolAct[i] = 0;
  for (int i = 0; i < 3; i++) { G.equip[i] = 255; G.decos[i] = 0; }
  G.bag[IDX_BAOZI] = 2;
  G.bag[IDX_PILL]  = 1;
  G.speedIdx = 0;         /* v8.2：默认真实 ×1 */
  G.muted = false;
  G.nannyGH = 0;
  G.dungeonKey = 255;
  G.lastTitleIdx = 0;
  G_hasSave = true;
  checkDailyLogin();
  gameSave(true);
}

bool gameLoad() {
  prefs.begin(NVS_NAMESPACE, true);
  size_t n = prefs.getBytesLength(NVS_KEY);
  if (n < sizeof(GameState)) { prefs.end(); return false; }
  uint8_t* buf = (uint8_t*)malloc(n);
  if (!buf) { prefs.end(); return false; }
  prefs.getBytes(NVS_KEY, buf, n);
  prefs.end();
  GameState loaded;
  memcpy(&loaded, buf, sizeof(GameState));
  free(buf);
  if (loaded.version != SAVE_VERSION) return false;
  memcpy(&G, &loaded, sizeof(GameState));
  /* 清洗 */
  if (G.hunger < 0 || G.hunger > 100) G.hunger = 80;
  if (G.mood   < 0 || G.mood   > 100) G.mood   = 80;
  if (G.speedIdx > 3) G.speedIdx = 0;
  if (G.dungeonKey > 3 && G.dungeonKey != 255) G.dungeonKey = 255;
  for (int i = 0; i < 3; i++) {
    if (G.equip[i] != 255 && (G.equip[i] >= GOODS_COUNT || GOODS[G.equip[i]].type != T_EQUIP)) G.equip[i] = 255;
  }
  for (int i = 0; i < GOODS_COUNT; i++) if (G.bag[i] > 99) G.bag[i] = 99;
  return true;
}

void gameSave(bool force) {
  if (!force && millis() - G_lastSaveMs < SAVE_INTERVAL_MS) return;
  G.lastTs = nowEpoch();
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putBytes(NVS_KEY, &G, sizeof(GameState));
  prefs.putUInt(NVS_TIME_KEY, nowEpoch());
  prefs.end();
  G_lastSaveMs = millis();
}

/* ---------------- 工具 ---------------- */
float actPenalty() {
  if (G.down)     return 0.6f;
  if (G.poisoned) return 0.8f;
  return 1.0f;
}
const char* penaltyText() {
  if (G.down)     return TXT_COND_DOW;
  if (G.poisoned) return TXT_COND_POI;
  return "";
}
int titleIndex() {
  int idx = 0;
  for (int i = 0; i < 5; i++) if (G.exp >= TITLES[i].exp) idx = i;
  return idx;
}
const char* titleName() { return TITLES[titleIndex()].name; }
const char* reputationName() {
  int idx = 0;
  for (int i = 0; i < 5; i++) if (G.exp >= REPUTATIONS[i].exp) idx = i;
  return REPUTATIONS[idx].name;
}
int bagCount(uint8_t idx) { return idx < GOODS_COUNT ? G.bag[idx] : 0; }
int bagTotal() { int t = 0; for (int i = 0; i < GOODS_COUNT; i++) t += G.bag[i]; return t; }
bool hasEq(uint8_t idx) {
  if (idx >= GOODS_COUNT) return false;
  for (int i = 0; i < 3; i++) if (G.equip[i] == idx) return true;
  return false;
}
int decoCount() { int n = 0; for (int i = 0; i < 3; i++) if (G.decos[i]) n++; return n; }
int moodDecayCutPct() {
  int cut = hasEq(IDX_AROBE) ? 20 : 0;
  for (int i = 0; i < 3; i++) if (G.decos[i]) cut += DECO_MOOD[i];
  return cut > 60 ? 60 : cut;
}
int hungerDecayCutPct() { return G.decos[2] ? 6 : 0; }   // 长明灯
const char* condText() {
  if (G.down)     return TXT_COND_DOW;
  if (G.poisoned) return TXT_COND_POI;
  if (isSleeping()) return TXT_SLEEP;
  if (G.nannyGH > 0) return TXT_NANNY;
  return TXT_COND_OK;
}
float gameHour() {
  if (!G.bornTs) return 8.0f;
  uint32_t ep = nowEpoch();
  float gh = (float)(ep > G.bornTs ? ep - G.bornTs : 0) * (float)SPEEDS[G.speedIdx] / 3600.0f;
  return fmodf(gh + 8.0f, 24.0f);
}
bool isSleeping() { float h = gameHour(); return h >= 22.0f || h < 6.0f; }

void checkTitle() {
  int idx = titleIndex();
  if (idx != G.lastTitleIdx) {
    G.lastTitleIdx = idx;
    toast(MSG_LEVELUP);
    emitTone(880, 90);
  }
}

/* ---------------- 时间推进 ---------------- */
static void applyHours(float gh, bool silent) {
  if (gh <= 0) return;
  bool sleeping = isSleeping();
  float hMul = 1.0f, mMul = 1.0f;
  if (G.nannyGH > 0)   { hMul *= 0.2f;  mMul *= 0.2f; }
  else if (sleeping)   { hMul *= 0.35f; mMul *= 0.2f; }
  hMul *= 1.0f - hungerDecayCutPct() / 100.0f;
  mMul *= 1.0f - moodDecayCutPct()   / 100.0f;
  if (G.poisoned) hMul *= 2.0f;
  if (G.mess > 0) mMul *= 2.0f;
  mMul *= 1.0f - 0.15f * G.homeLv;

  G.hunger = max(0.0f, G.hunger - 9.0f * gh * hMul);
  G.mood   = max(0.0f, G.mood   - 7.0f * gh * mMul);

  if (G.nannyGH > 0) {
    G.nannyGH = max(0.0f, G.nannyGH - gh);
    if (G.mess > 0) G.mess = 0;
    if (G.nannyGH == 0 && !silent) toast(MSG_NANNYEND);
  }
  if (!silent && G_wasSleep != sleeping) {
    toast(sleeping ? MSG_SLEEP : MSG_WAKE);
  }
  G_wasSleep = sleeping;

  /* 副本到期结算 */
  if (G.dungeonUntil > 0 && nowEpoch() >= G.dungeonUntil) {
    uint8_t key = G.dungeonKey;
    G.dungeonUntil = 0; G.dungeonKey = 255;
    if (!silent) actDungeon(key);
  }

  /* 随机事件：就寝 / 保姆 / 异常状态期间免疫 */
  if (!G.down && !G.poisoned && !silent && G.nannyGH <= 0 && !sleeping) {
    uint32_t r1 = random(10000), r2 = random(10000), r3 = random(10000);
    if (r1 < (uint32_t)(5000 * gh) && G.mess < 3) { G.mess++; toast("狼牙袭击 一片狼藉"); }
    if (r2 < (uint32_t)(1200 * gh)) {
      G.poisoned = true; G.poisonedH = 0;
      toast(MSG_POISON); emitTone(220, 120);
    }
    if (r3 < (uint32_t)(1000 * gh)) {
      static const int gains[3] = {8, 12, 20};
      int g = gains[random(3)];
      G.exp += g;
      mailAdd(random(8), 0, g);          /* 0=修为，场景 0..7 */
      toast("奇遇 来信"); emitTone(1046, 70);
    }
  }

  if (G.poisoned && !G.down) {
    G.poisonedH += gh;
    if (G.poisonedH >= 3.0f) {
      G.down = true; G.poisoned = false;
      toast(MSG_DOWN); emitTone(180, 180);
    }
  }

  /* 饿 / 郁见底各计 1 游戏小时转重伤 */
  if (G.hunger <= 0 && !G.down && !G.poisoned) {
    G.hungerH += gh;
    if (G.hungerH >= 1.0f) { G.down = true; G.hungerH = 0; toast(MSG_DOWN); }
  } else if (G.hunger > 10) G.hungerH = 0;

  if (G.mood <= 0 && !G.down && !G.poisoned) {
    G.moodH += gh;
    if (G.moodH >= 1.0f) { G.down = true; G.moodH = 0; toast(MSG_DOWN); }
  } else if (G.mood > 10) G.moodH = 0;

  /* 静养自愈：体力心情 ≥60 维持 2 游戏小时 */
  if (G.down) {
    if (G.hunger >= 60 && G.mood >= 60) {
      G.recoverH += gh;
      if (G.recoverH >= 2.0f) {
        G.down = false; G.recoverH = 0; G.poisonedH = 0;
        toast(MSG_RECOVER); emitTone(784, 100);
      }
    } else G.recoverH = 0;
  } else G.recoverH = 0;

  checkTitle();
}

void gameTick() {
  static bool first = true;
  if (first) {
    first = false;
    /* 离线结算：最多 4 游戏小时 */
    uint32_t now = nowEpoch();
    uint32_t away = now > G.lastTs ? now - G.lastTs : 0;
    float gh = (float)away * SPEEDS[G.speedIdx] / 3600.0f;
    if (gh > OFFLINE_MAX_GH) gh = OFFLINE_MAX_GH;
    if (gh > 0.01f) applyHours(gh, true);
    G.lastTs = now;
    checkDailyLogin();
    return;
  }
  /* 非首次 tick：只处理副本到期 + 常规衰减 */
  uint32_t now = nowEpoch();
  uint32_t dt = now > G.lastTs ? now - G.lastTs : 0;
  if (dt > 0) {
    float gh = (float)dt * SPEEDS[G.speedIdx] / 3600.0f;
    if (gh > 0) applyHours(gh, false);
    G.lastTs = now;
  }
}

void gameAdvance(uint32_t secReal) {
  float gh = (float)secReal / 3600.0f;
  if (gh > OFFLINE_MAX_GH) gh = OFFLINE_MAX_GH;
  applyHours(gh, true);
}

/* ---------------- 每日登录奖励 ---------------- */
void checkDailyLogin() {
  /* v0.2.7：取消门派，签到不再依赖 sect */
  uint32_t today = epochYmd(nowEpoch());
  if (G.loginDay == today) return;
  uint32_t y = today / 10000, m = (today / 100) % 100, d = today % 100;
  /* 判断昨天：用 epoch 减一天再算 */
  uint32_t yest = epochYmd(nowEpoch() > 86400 ? nowEpoch() - 86400 : 0);
  G.loginStreak = (G.loginDay == yest) ? (uint16_t)(G.loginStreak + 1) : 1;
  G.loginDay = today;
  (void)y; (void)m; (void)d;
  int reward = 50 + (G.loginStreak - 1) * 25;
  if (reward > 200) reward = 200;
  G.coins += reward;
  G.bag[IDX_BAOZI] = min(99, G.bag[IDX_BAOZI] + 1);
  char buf[40];
  if (random(100) < 30) {
    /* 凡品×5 良品×2 珍品×1，从商城装备池随机 */
    static const uint8_t pool[19] = {
      IDX_WSWORD,IDX_WSWORD,IDX_WSWORD,IDX_WSWORD,IDX_WSWORD,
      IDX_WPICK,IDX_WPICK,IDX_WPICK,IDX_WPICK,IDX_WPICK,
      IDX_TGOURD,IDX_TGOURD,IDX_TGOURD,IDX_TGOURD,IDX_TGOURD,
      IDX_AROBE,IDX_AROBE,IDX_TJADE,IDX_ACHARM
    };
    uint8_t gid = pool[random(19)];
    G.bag[gid] = min(99, G.bag[gid] + 1);
    snprintf(buf, sizeof(buf), "%s %s +%d", MSG_LUCKY, GOODS[gid].name, reward);
  } else {
    snprintf(buf, sizeof(buf), "%s%d %s+%d", MSG_DAILY, G.loginStreak, TXT_COINS, reward);
  }
  toast(buf);
  emitTone(659, 80);
  gameSave(true);
}

/* ---------------- 行为 ---------------- */
bool actEat(uint8_t idx) {
  if (idx >= GOODS_COUNT || GOODS[idx].type != T_FOOD) return false;
  if (G.bag[idx] <= 0) { toast(TXT_NOITEM); return false; }
  if (nowEpoch() < G.cool[idx]) { toast(MSG_COOL); return false; }
  G.bag[idx]--;
  G.hunger = min(100.0f, G.hunger + GOODS[idx].gain);
  G.mood   = min(100.0f, G.mood   + GOODS[idx].mood);
  G.cool[idx] = nowEpoch() + GOODS[idx].cd;
  G_fxKind = FX_EAT; G_fxIdx = idx; G_fxUntil = millis() + FX_EAT_MS;
  if (random(100) < 25 && G.mess < 3) { G.mess++; toast("吃剩的骨头 记得打扫"); }
  else toast(MSG_EAT);
  emitTone(523, 70);
  gameSave();
  return true;
}

bool actUseMed() {
  if (!G.down && !G.poisoned) { toast("侠士康健 不用吃药"); return false; }
  if (G.bag[IDX_PILL] <= 0) { toast(TXT_NOITEM); return false; }
  G.bag[IDX_PILL]--;
  G.poisoned = G.down = false;
  G.poisonedH = G.hungerH = G.moodH = G.recoverH = 0;
  G.hunger = max(G.hunger, 30.0f);
  G.mood   = min(100.0f, G.mood + 10.0f);
  G_fxKind = FX_MED; G_fxIdx = IDX_PILL; G_fxUntil = millis() + FX_MED_MS;
  toast(MSG_CURED); emitTone(880, 90); gameSave();
  return true;
}

bool actUsePage() {
  if (G.bag[IDX_PAGE] <= 0) { toast(TXT_NOITEM); return false; }
  G.bag[IDX_PAGE]--;
  G.exp += GOODS[IDX_PAGE].exp;
  G_fxKind = FX_USE; G_fxIdx = IDX_PAGE; G_fxUntil = millis() + FX_USE_MS;
  toast("参悟残页 修为+3"); emitTone(784, 80);
  checkTitle(); gameSave();
  return true;
}

bool actEquipToggle(uint8_t idx) {
  if (idx >= GOODS_COUNT || GOODS[idx].type != T_EQUIP) return false;
  uint8_t slot = GOODS[idx].slot;
  if (slot < 1 || slot > 3) return false;
  char buf[40];
  if (G.equip[slot - 1] == idx) {
    G.equip[slot - 1] = 255;
    snprintf(buf, sizeof(buf), "%s「%s」", MSG_EQOFF, GOODS[idx].name);
  } else {
    G.equip[slot - 1] = idx;
    snprintf(buf, sizeof(buf), "%s「%s」 %s", MSG_EQON, GOODS[idx].name, GOODS[idx].eff);
  }
  toast(buf); emitTone(698, 80); gameSave();
  return true;
}

bool actSell(uint8_t idx) {
  if (idx >= GOODS_COUNT || GOODS[idx].sell <= 0 || G.bag[idx] <= 0) { toast(TXT_NOITEM); return false; }
  G.bag[idx]--;
  G.coins += GOODS[idx].sell;
  toast(MSG_SELLOK); emitTone(587, 70); gameSave();
  return true;
}

bool actBuy(uint8_t idx) {
  if (idx >= GOODS_COUNT || GOODS[idx].price <= 0) return false;
  if (G.coins < GOODS[idx].price) { toast(TXT_NOCOIN); return false; }
  G.coins -= GOODS[idx].price;
  if (GOODS[idx].type == T_DECO) {
    G.decos[idx == IDX_DPLANT ? 0 : (idx == IDX_DSCREEN ? 1 : 2)] = 1;
    toast(MSG_DECO);
  } else {
    G.bag[idx] = min(99, G.bag[idx] + 1);
    toast(MSG_BUYOK);
  }
  emitTone(659, 70); gameSave();
  return true;
}

bool actSit() {
  if (nowEpoch() < G.coolAct[0]) { toast(MSG_COOL); return false; }
  float p = actPenalty();
  int ge = max(1, (int)lroundf(3 * p));
  if (hasEq(IDX_TGOURD)) ge += 2;              // 酒葫芦
  G.mood = min(100.0f, G.mood + 20.0f);
  G.hunger = max(0.0f, G.hunger - 3.0f);
  G.exp += ge;
  G.coolAct[0] = nowEpoch() + 15;
  char buf[32]; snprintf(buf, sizeof(buf), "%s+%d", MSG_SIT, ge);
  toast(buf); emitTone(440, 90); gameSave();
  return true;
}

bool actTrain() {
  if (G.hunger < 20) { toast(MSG_NOFOOD); return false; }
  if (nowEpoch() < G.coolAct[1]) { toast(MSG_COOL); return false; }
  float p = actPenalty();
  int ge = max(1, (int)lroundf(12 * p * (hasEq(IDX_WSWORD) ? 1.5f : 1.0f)));
  G.exp += ge;
  G.hunger = max(0.0f, G.hunger - 8.0f);
  G.mood   = max(0.0f, G.mood - 5.0f);
  G.coolAct[1] = nowEpoch() + 25;
  char buf[32]; snprintf(buf, sizeof(buf), "%s%d", MSG_TRAIN, ge);
  toast(buf); emitTone(330, 90); gameSave();
  return true;
}

bool actDig() {
  if (nowEpoch() < G.coolAct[2]) { toast(MSG_COOL); return false; }
  if (G.hunger < 10) { toast(MSG_NOFOOD); return false; }
  float p = actPenalty();
  G.hunger = max(0.0f, G.hunger - 6.0f);
  G.coolAct[2] = nowEpoch() + 30;
  int r = random(100);
  char buf[40];
  float pick = hasEq(IDX_WPICK) ? 1.5f : 1.0f;
  if (r < 55) {
    int g = (int)lroundf((50 + random(110)) * p * pick);
    G.coins += g;
    snprintf(buf, sizeof(buf), "%s%d", MSG_DIG, g);
  } else if (r < 80) {
    int ge = max(1, (int)lroundf(3 * p));
    G.coins += (int)lroundf(100 * p * pick);
    G.bag[IDX_PAGE] = min(99, G.bag[IDX_PAGE] + 1);
    G.exp += ge;
    snprintf(buf, sizeof(buf), "残页 修为+%d", ge);
  } else if (r < 93) {
    snprintf(buf, sizeof(buf), "%s", MSG_STONE);
  } else {
    if (G.mess < 3) G.mess++;
    snprintf(buf, sizeof(buf), "%s", MSG_STONE);
  }
  toast(buf); gameSave();
  return true;
}

bool actClean() {
  if (G.mess <= 0) return false;
  G.mess = 0;
  G.mood = min(100.0f, G.mood + 5.0f);
  G.exp += 1;
  toast(MSG_CLEANOK); gameSave();
  return true;
}

bool actUpgradeHome() {
  if (G.homeLv >= 4) { toast(TXT_MAXLV); return false; }
  int cost = (G.homeLv + 1) * 500;
  if (G.coins < cost) { toast(TXT_NOCOIN); return false; }
  G.coins -= cost; G.homeLv++;
  char buf[40]; snprintf(buf, sizeof(buf), "%s「%s」", MSG_UPHOME, HOME_NAMES[G.homeLv]);
  toast(buf); emitTone(880, 100); gameSave();
  return true;
}

bool actNanny() {
  if (G.nannyGH > 0) { toast("保姆照料中"); return false; }
  if (G.coins < 100) { toast(TXT_NOCOIN); return false; }
  G.coins -= 100; G.nannyGH = 8;
  toast(MSG_HIREOK); emitTone(659, 90); gameSave();
  return true;
}

void actDungeon(uint8_t key) {
  if (key > 3) key = 0;
  const DungeonDef& d = DUNGEONS[key];
  float p = actPenalty();
  float winRate = 0.4f + (float)G.exp / 500.0f + (float)G.hunger / 300.0f;
  if (hasEq(IDX_ACHARM)) winRate += 0.15f;      // 平安符
  if (winRate > 0.9f) winRate = 0.9f;
  if ((float)random(10000) / 10000.0f >= winRate) {
    G.mood = max(0.0f, G.mood - 10.0f);
    G.exp += max(1, (int)lroundf(5 * p));
    toast(MSG_WIPE); emitTone(196, 150); gameSave();
    return;
  }
  int coin = d.coinMin + random(d.coinMax - d.coinMin + 1);
  int exp  = d.expMin  + random(d.expMax  - d.expMin  + 1);
  coin = (int)lroundf(coin * p);
  exp  = max(1, (int)lroundf(exp * p));
  G.coins += coin; G.exp += exp;
  for (uint8_t i = 0; i < d.dropN; i++) {
    uint8_t idx = d.drops[i].idx;
    int pr = (int)(d.drops[i].pct * p);
    if ((uint32_t)random(100) >= (uint32_t)pr) continue;
    if (idx == 255) { G.coins += (int)lroundf(100 * p); }
    else if (idx < GOODS_COUNT) G.bag[idx] = min(99, G.bag[idx] + 1);
  }
  /* 团本 0.1% 玄晶（独立掷骰，不吃惩罚系数） */
  if (key == 3 && random(1000) == 0) {
    G.bag[IDX_XJ] = min(99, G.bag[IDX_XJ] + 1);
    toast(MSG_XJ); emitTone(1175, 200);
  } else {
    char buf[48];
    snprintf(buf, sizeof(buf), "%s %s+%d 修为+%d", MSG_DUNGEON, TXT_COINS, coin, exp);
    toast(buf); emitTone(784, 100);
  }
  checkTitle(); gameSave();
}

int gambleCap() {
  int cap = -1;
  for (int i = 0; i < BET_COUNT; i++) if (BETS[i] > G.coins) { cap = i - 1; break; }
  if (cap < 0) cap = BET_COUNT - 1;
  if (cap > BET_COUNT - 1) cap = BET_COUNT - 1;
  return cap;
}

void gambleRoll() {
  GM.dice[0] = 1 + random(6);
  GM.dice[1] = 1 + random(6);
  GM.dice[2] = 1 + random(6);
  GM.sum = GM.dice[0] + GM.dice[1] + GM.dice[2];
  GM.baozi = (GM.dice[0] == GM.dice[1] && GM.dice[1] == GM.dice[2]);
  GM.win = !GM.baozi && ((GM.sum >= 11) == (GM.side == 1));
  int bet = BETS[GM.betIdx];
  float p = actPenalty();
  if (GM.win) {
    GM.pay = max(1, (int)lroundf(bet * p * (hasEq(IDX_TJADE) ? 1.2f : 1.0f)));
    G.coins += GM.pay;
    G.exp += max(1, (int)lroundf(2 * p));
    checkTitle();
    emitTone(880, 120);
  } else {
    GM.pay = bet;
    G.coins = max<int32_t>(0, G.coins - bet);
    emitTone(165, 150);
  }
  GM.phase = PH_RESULT;
  gameSave();
}
