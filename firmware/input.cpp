/* input.cpp — 按键状态机 + 全页面导航（对齐网页 v8.3） */
#include "input.h"
#include "game.h"
#include "ui.h"
#include "display.h"

struct KeyState {
  uint8_t  pin;
  bool     down;        // 已确认按下（去抖后）
  bool     rawLast;     // 上一次读到的原始电平
  uint32_t rawAt;       // 原始电平最后一次跳变的时间
  uint32_t downAt;
  uint32_t lastRepeat;
  bool     holdFired;   // 本次按住已执行过长按动作
  bool     armed;       // 长按动作后必须先「真正松手」才允许再次按下
  uint32_t lockUntil;   // 松手后的静默期，吞掉触点回弹
};
static KeyState K[4] = {
  { PIN_K1, false, false, 0, 0, 0, false, true, 0 },
  { PIN_K2, false, false, 0, 0, 0, false, true, 0 },
  { PIN_K3, false, false, 0, 0, 0, false, true, 0 },
  { PIN_K4, false, false, 0, 0, 0, false, true, 0 },
};

void inpInit() {
  uint32_t now = millis();
  for (int i = 0; i < 4; i++) {
    pinMode(K[i].pin, INPUT_PULLUP);
    bool raw = (digitalRead(K[i].pin) == LOW);
    K[i].rawLast = raw;
    K[i].rawAt   = now;
    K[i].down    = raw;        // 上电时若已按住，当成「已按下」
    K[i].downAt  = now;
    K[i].holdFired = raw;      // 开机就按住的那一次不算功能，松手也不触发单击
    K[i].armed   = !raw;       // 若开机时按着，必须先松手才重新武装
  }
}

/* ---------- 导航辅助 ---------- */
static uint8_t filteredIds[GOODS_COUNT];
static int filteredN() {
  return uiFiltered(U.page == PG_SHOP || U.itemFrom == 1, filteredIds, GOODS_COUNT);
}

static void moveSel(int d) {
  char buf[64];
  switch (U.page) {
    case PG_MAIL:
      if (!U.mailDetail && G_mailCount > 0) U.mailSel = (U.mailSel + d + G_mailCount) % G_mailCount;
      emitTone(600, 25); break;
    case PG_MAIN:
      if (U.menuIdx >= 9) U.menuIdx = 0;
      U.menuIdx = (U.menuIdx + d + 9) % 9;
      snprintf(buf, sizeof(buf), "%s · %s", MENU_LABELS[U.menuIdx], MENU_DESCS[U.menuIdx]);
      hint(buf);
      emitTone(600, 25); break;
    case PG_BAG: case PG_SHOP: case PG_ITEM: {
      bool shop = (U.page == PG_SHOP) || (U.page == PG_ITEM && U.itemFrom == 1);
      if (U.page == PG_BAG || U.page == PG_SHOP) {
        if (U.catMode) {
          U.catIdx = (U.catIdx + d + 6) % 6;
          U.listSel = 0;
          emitTone(700, 25);
          return;
        }
      }
      int n = uiFiltered(shop, filteredIds, GOODS_COUNT);
      if (n > 0) {
        U.listSel = (U.listSel + d + n) % n;
        emitTone(600, 25);
      }
      break;
    }
    case PG_HOME: {
      int n = (G.homeLv < 4 ? 1 : 0) + (G.mess > 0 ? 1 : 0) + 1;
      if (n > 1) U.homeSel = (U.homeSel + d + n) % n;
      emitTone(600, 25); break;
    }
    case PG_DAILY:
      U.dailySel = (U.dailySel + d + 5) % 5;
      emitTone(600, 25); break;
    case PG_GAME:
      if (GM.phase == PH_SIDE) {
        GM.side = 1 - GM.side; emitTone(600, 25);
      } else if (GM.phase == PH_BET) {
        int cap = gambleCap();
        int v = (int)GM.betIdx + d;
        if (v < 0) v = 0; if (v > cap) v = cap;
        GM.betIdx = v; emitTone(600, 25);
      }
      break;
    case PG_SETTINGS:
      if (U.setPage == 0) {
        U.setIdx = (U.setIdx + d + 6) % 6; emitTone(600, 25);
      } else if (U.setPage == 1) {
        int v = (int)G.speedIdx + d;
        if (v < 0) v = 0; if (v > 3) v = 3;
        G.speedIdx = v; emitTone(600, 25);
      } else if (U.setPage == 4) {
        U.confirmSel = d > 0 ? 1 : 0; emitTone(600, 25);
      }
      break;
    default: break;
  }
}

/* ---------- A 键 ---------- */
static void pressA() {
  char buf[48];
  switch (U.page) {
    case PG_BOOT:
      gameNew();
      uiGoto(PG_MAIN);
      toast("初入江湖，多多关照"); emitTone(880, 100); break;
    case PG_MAIN:
      if (U.menuIdx == 8) { U.mailSel = 0; U.mailDetail = false; uiGoto(PG_MAIL); emitTone(659, 50); break; }
      /* 就寝时段：除背包/商城外不干活（网页版 canAct 行为） */
      if (isSleeping() && U.menuIdx != 1 && U.menuIdx != 2) {
        toast(MSG_SLEEP); return;
      }
      uiDoMenu(); break;
    case PG_BAG: case PG_SHOP:
      if (U.catMode) { U.catMode = false; emitTone(659, 50); }
      else { U.itemFrom = (U.page == PG_SHOP) ? 1 : 0; U.listSel = 0; uiGoto(PG_ITEM); }
      break;
    case PG_ITEM:
      uiItemAction(); break;
    case PG_HOME: {
      uint8_t opts[3]; int n = 0;
      if (G.homeLv < 4) opts[n++] = 0;
      if (G.mess > 0) opts[n++] = 1;
      opts[n++] = 2;
      if (U.homeSel >= n) U.homeSel = 0;
      switch (opts[U.homeSel]) {
        case 0: actUpgradeHome(); break;
        case 1: actClean(); break;
        case 2: actNanny(); break;
      }
      break;
    }
    case PG_DAILY:
      if (G.hunger < 30) { toast(MSG_NOFOOD); return; }
      if (nowEpoch() < G.coolAct[3]) { toast(MSG_COOL); return; }
      /* 下本成本（网页版 startDungeon）：体力-12 心情-6 冷却 60 秒 */
      G.hunger = max(0.0f, G.hunger - 12.0f);
      G.mood   = max(0.0f, G.mood - 6.0f);
      G.coolAct[3] = nowEpoch() + 60;
      actDungeon(U.dailySel);
      uiGoto(PG_MAIN); break;
    case PG_GAME:
      if (GM.phase == PH_SIDE) { GM.phase = PH_BET; emitTone(659, 50); }
      else if (GM.phase == PH_BET) {
        if (BETS[GM.betIdx] > G.coins) { toast(MSG_NOMONEY); return; }
        GM.phase = PH_ROLL; GM.rollTick = 0;
        GM.dice[0] = GM.dice[1] = GM.dice[2] = 1;
      } else if (GM.phase == PH_RESULT) {
        if (G.coins < 50) { uiGoto(PG_MAIN); toast(MSG_NOMONEY); return; }
        GM.phase = PH_SIDE;
      }
      break;
    case PG_MAIL:
      if (G_mailCount == 0) { toast(TXT_NO_MAIL); break; }
      if (!U.mailDetail) { U.mailDetail = true; emitTone(659, 50); }
      else { U.mailDetail = false; emitTone(440, 40); }
      break;
    case PG_SETTINGS:
      if (U.setPage == 0) {
        U.setPage = U.setIdx + 1;
        U.confirmSel = 0;
        emitTone(659, 50);
      } else if (U.setPage == 2) {
        G.muted = !G.muted;
        toast(G.muted ? "音效已关" : "音效已开");
        gameSave(true);
      } else if (U.setPage == 3) {
        gameSave(true); toast(MSG_SAVED);
      } else if (U.setPage == 4) {
        if (U.confirmSel == 1) {
          gameNew();
          U.setPage = 0;
          uiGoto(PG_MAIN);
          toast(MSG_RESET);
        } else U.setPage = 0;
      }
      break;
    default: break;
  }
}

/* ---------- B 键 ---------- */
static uint32_t settingsEnterAt = 0;
static bool     settingsHoldGuard = false;   // 长按呼出设置后，吞掉紧随其后的一次 B 短按

static void pressB() {
  if (U.page == PG_SETTINGS) {
    /* 长按 B 进来后，松手回弹可能被当成一次短按，导致刚进设置就被弹回上一页。
     * 这里做最后一道保险：进设置 0.9 秒内的第一次 B 短按直接丢弃。 */
    if (settingsHoldGuard && (uint32_t)(millis() - settingsEnterAt) < 900) {
      settingsHoldGuard = false;
      return;
    }
    settingsHoldGuard = false;
    if (U.setPage != 0) { U.setPage = 0; emitTone(440, 40); }
    else { U.setPage = 0; uiGoto(U.settingsFrom); }
    return;
  }
  if (U.page == PG_MAIL) {
    if (U.mailDetail) { U.mailDetail = false; emitTone(440, 40); }
    else uiGoto(PG_MAIN);
    emitTone(440, 40); return;
  }
  if (U.page == PG_STATUS) { uiGoto(PG_MAIN); emitTone(440, 40); return; }
  if (U.page == PG_MAIN)   { uiGoto(PG_STATUS); emitTone(440, 40); return; }
  if (U.page == PG_BAG || U.page == PG_SHOP) {
    if (!U.catMode) U.catMode = true;
    else uiGoto(PG_MAIN);
    emitTone(440, 40); return;
  }
  if (U.page == PG_ITEM) {
    U.catMode = false;
    uiGoto(U.itemFrom == 1 ? PG_SHOP : PG_BAG);
    emitTone(440, 40); return;
  }
  uiGoto(PG_MAIN);
  emitTone(440, 40);
}

/* ---------- 长按 B：全局呼出设置 ---------- */
static void openSettings() {
  U.settingsFrom = U.page;
  U.setPage = 0;
  U.setIdx = 0;
  settingsEnterAt   = millis();
  settingsHoldGuard = true;
  uiGoto(PG_SETTINGS);
  toast("系统设置");
  emitTone(880, 80);
}

/* ---------- 轮询 ----------
 * 机械按键按下和松手都会有 5~20ms 抖动。之前没有去抖，B 键松手时那一下回弹
 * 会被当成「又按了一次」，于是长按进设置、松手瞬间又执行了短按返回。
 * 现在的规则：
 *   1) 电平必须连续稳定 KEY_DEBOUNCE_MS 才认；
 *   2) 长按动作一旦触发就把该键「解除武装」(armed=false)，
 *      必须先出现一次稳定松开才允许再次按下；
 *   3) 长按松手后再加 KEY_RELEASE_LOCK_MS 静默期，彻底吞掉回弹。
 */
void inpPoll() {
  uint32_t now = millis();
  for (int i = 0; i < 4; i++) {
    KeyState& k = K[i];
    bool raw = (digitalRead(k.pin) == LOW);

    /* --- 去抖 --- */
    if (raw != k.rawLast) { k.rawLast = raw; k.rawAt = now; continue; }
    if (now - k.rawAt < KEY_DEBOUNCE_MS) continue;

    /* --- 稳定按下 --- */
    if (raw && !k.down) {
      if (!k.armed) continue;                              // 长按后的残留按压
      if ((int32_t)(now - k.lockUntil) < 0) continue;       // 松手回弹静默期
      k.down = true; k.downAt = now; k.lastRepeat = now; k.holdFired = false;
      if (i == 0) moveSel(-1);
      else if (i == 1) moveSel(1);
      continue;
    }

    /* --- 稳定松开 --- */
    if (!raw && k.down) {
      k.down = false;
      if (i == 2) pressA();
      else if (i == 3 && !k.holdFired) pressB();
      if (k.holdFired) k.lockUntil = now + KEY_RELEASE_LOCK_MS;
      k.holdFired = false;
      k.armed = true;
      continue;
    }

    if (!k.down) continue;

    uint32_t held = now - k.downAt;
    if (i == 0) {
      /* 长按 ▲ 3 秒：进入屏幕校准（色序 / 反显） */
      if (!k.holdFired && held >= DISP_HOLD_MS) {
        k.holdFired = true;
        k.armed = false;
        emitTone(880, 60);
        dispCalibrate();             // A 确定 / ▲ 换档 / B 退出
      }
    } else if (i == 1) {
      if (held >= KEY_REPEAT_WAIT && now - k.lastRepeat >= KEY_REPEAT_RATE) {
        moveSel(1);
        k.lastRepeat = now;
      }
    } else if (i == 3) {
      /* 长按 B 1.5 秒：呼出系统设置 */
      if (!k.holdFired && held >= KEY_HOLD_MS) {
        k.holdFired = true;
        k.armed = false;
        openSettings();
      }
    }
  }
}

/* ---------- 赌坊摇骰（帧驱动） ---------- */
void inpGambleTick() {
  if (U.page != PG_GAME || GM.phase != PH_ROLL) return;
  GM.rollTick++;
  if (GM.rollTick % 4 == 0) {
    GM.dice[0] = 1 + random(6);
    GM.dice[1] = 1 + random(6);
    GM.dice[2] = 1 + random(6);
  }
  if (GM.rollTick > 60) gambleRoll();
}
