/* ui.cpp — 全部页面渲染（布局对齐网页 v8.3） */
#include "ui.h"
#include "game.h"
#include "display.h"
#include "boot_logo.h"

UIState U = { PG_BOOT, 0, 0, true, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, 0 };

static const char* const SET_ITEMS[6] = {
  TXT_SET_SPEED, TXT_SET_SOUND, TXT_SET_SAVE, TXT_SET_RESET, TXT_SET_ABOUT, TXT_HOLD_SET
};
#define SET_COUNT 6
static const char* const DAY_ITEMS[5] = { D_WAR, D_BATTLE, D_CARD, D_RAID, D_OTHER };

int uiFiltered(bool shop, uint8_t* out, int maxN) {
  int n = 0;
  const uint8_t* order = shop ? SHOP_ORDER : BAG_ORDER;
  int total = shop ? SHOP_COUNT : GOODS_COUNT;
  for (int i = 0; i < total && n < maxN; i++) {
    uint8_t idx = order[i];
    if (U.catIdx == CAT_ALL || GOODS[idx].cat == U.catIdx) out[n++] = idx;
  }
  return n;
}

void uiGoto(uint8_t page) {
  U.page = page;
  G_needRefresh = true;
}

/* ---------------- 开机 ---------------- */
static void renderBoot() {
  dispClear(C_BG);
  /* SeasunGame 火焰剑标（48×48，悬浮微动） */
  int by = 14 + ((U.frame / 30) % 2);
  for (int y = 0; y < BOOT_LOGO_H; y++) {
    for (int x = 0; x < BOOT_LOGO_W; x++) {
      uint16_t c = pgm_read_word(&BOOT_LOGO[y * BOOT_LOGO_W + x]);
      if (c) dispPixel(x + 40, y + by, c);
    }
  }
  dispRect(0, 78, 128, 1, C_GOLD);
  dispTextC(TXT_BOOT_B, 64, 62, C_GOLD_HI);
  if ((U.frame / 20) % 2 == 0) dispTextC(TXT_PRESS_A, 64, 94, C_TEXT);
  dispTextC(TXT_HOLD_UP, 64, 112, C_DIM);
  for (int i = 0; i < 6; i++) {
    int sx = (i * 29 + U.frame / 3) % 120 + 4;
    int sy = 20 + ((i * 17) % 58);
    if ((U.frame / 10 + i) % 5 == 0) dispPixel(sx, sy, C_GOLD_HI);
  }
}

/* ---------------- 主界面 ---------------- */
static void renderMain() {
  dispClear(C_BG);
  bool night = isSleeping();
  dispScene(night, U.frame);

  /* 上排 4 图标 */
  const int xs[4] = { 6, 38, 70, 102 };
  for (int i = 0; i < 4; i++) dispIcon(i, xs[i], 0, i == U.menuIdx);
  for (int i = 0; i < 4; i++) dispIcon(i + 4, xs[i], 101, i + 4 == U.menuIdx);

  /* 左侧状态面板（不参与 ▲▼） */
  dispRect(2, 21, 38, 58, C_PANEL2);
  dispFrame(2, 21, 38, 58, C_DIM);
  dispText(TXT_HUNGER, 4, 23, C_GOLD);
  dispBar(4, 35, 34, 5, (int)G.hunger, C_GREEN);
  dispText(TXT_MOOD, 4, 42, C_GOLD);
  dispBar(4, 54, 34, 5, (int)G.mood, 0xF3DD);
  char buf[24];
  snprintf(buf, sizeof(buf), "%s%d", TXT_EXP, G.exp);
  dispText(buf, 4, 62, C_TEXT);
  snprintf(buf, sizeof(buf), "%s%d", TXT_COINS, G.coins);
  dispText(buf, 4, 70, C_GOLD_HI);
  dispText(condText(), 4, 78, G.down ? C_RED : (G.poisoned ? C_ORANGE : C_DIM));

  /* 小人 */
  uint8_t pose = isSleeping() ? 8 : 0;
  if (G.down) pose = 3;
  else if (G.poisoned) pose = 2;
  else if (G_fxKind == FX_EAT && millis() < G_fxUntil) pose = 1;
  else if (G.dungeonUntil > nowEpoch()) pose = 4;
  else if (nowEpoch() < G.coolAct[0]) pose = 5;
  else if (nowEpoch() < G.coolAct[1]) pose = 6;
  else if (nowEpoch() < G.coolAct[2]) pose = 7;
  dispPet(46, 36, pose, 0);   /* v0.2.7：固定配色 */
  /* 邮箱角标（第 9 格，右上角；有未读时金色高亮） */
  dispIcon(8, 102, 24, U.menuIdx == 8);
  if (G_mailCount > 0) {
    char mb[4]; snprintf(mb, sizeof(mb), "%d", G_mailCount);
    dispTextC(mb, 114, 20, C_RED);
  }

  /* 非常驻提示：切换选中后 2 秒消失 */
  if (millis() < G_hintUntil && G_hint) {
    dispRect(4, 86, 120, 13, C_PANEL);
    dispFrame(4, 86, 120, 13, C_GOLD);
    dispTextC(G_hint, 64, 88, C_GOLD_HI);
  }
}

/* ---------------- 背包 / 商城：分类栏 + 列表 ---------------- */
static void renderList(bool shop) {
  dispClear(C_BG);
  dispTextC(shop ? TXT_SHOP : TXT_BAG, 64, 3, C_GOLD);
  char buf[32];
  snprintf(buf, sizeof(buf), "%s%d", TXT_COINS, G.coins);
  dispTextR(buf, 124, 4, C_GOLD_HI);

  /* 分类栏两行三列 */
  for (int i = 0; i < 6; i++) {
    int col = i % 3, row = i / 3;
    int x = 4 + col * 42, y = 16 + row * 14;
    bool on = i == U.catIdx;
    if (on) {
      dispRect(x, y - 1, 38, 12, U.catMode ? C_PANEL2 : C_PANEL);
      dispFrame(x, y - 1, 38, 12, U.catMode ? C_GOLD_HI : C_DIM);
    }
    dispText(CAT_LABELS[i], x + 7, y, on ? C_GOLD_HI : C_DIM);
  }

  uint8_t ids[GOODS_COUNT];
  int n = uiFiltered(shop, ids, GOODS_COUNT);
  int start = (U.listSel / 4) * 4;
  if (start >= n) { start = 0; U.listSel = 0; }
  for (int r = 0; r < 4; r++) {
    int i = start + r;
    int y = 46 + r * 16;
    if (i >= n) break;
    bool on = (i == U.listSel) && !U.catMode;
    if (on) { dispRect(3, y - 1, 122, 15, C_PANEL2); dispSelFrame(3, y - 1, 122, 15); }
    uint8_t idx = ids[i];
    uint8_t cnt = shop ? 0 : G.bag[idx];
    dispItemIcon(idx, 6, y, 1);
    uint16_t col = (!shop && cnt == 0) ? C_DIM : (on ? C_GOLD_HI : C_TEXT);
    dispText(GOODS[idx].name, 21, y, col);
    if (GOODS[idx].type == T_EQUIP) {
      dispRect(76, y + 1, 2, 8, GOODS[idx].tier == 3 ? C_PURPLE : (GOODS[idx].tier == 2 ? C_BLUE : C_DIM));
    }
    if (shop) {
      snprintf(buf, sizeof(buf), "%d%s", GOODS[idx].price, TXT_PRICE);
      dispTextR(buf, 122, y, G.coins >= GOODS[idx].price ? C_GOLD_HI : C_DIM);
    } else {
      snprintf(buf, sizeof(buf), "%s%d", TXT_OWNED, cnt);
      dispTextR(buf, 122, y, cnt ? C_TEXT : C_DIM);
    }
  }
  /* 右侧滚动条 */
  if (n > 4) {
    int th = 40 * 4 / n;
    int ty = 46 + (start * 40 / n);
    dispRect(126, 46, 2, 40, C_PANEL2);
    dispRect(126, ty, 2, max(4, th), C_GOLD);
  }
  dispTextC(U.catMode ? "▲▼ 换分类 · A 进列表 · B 返回"
                      : "▲▼ 选物 · A 详情 · B 回分类", 64, 115, C_DIM);
}

/* ---------------- 物品详情 ---------------- */
static void renderItem() {
  dispClear(C_BG);
  bool shop = U.itemFrom == 1;
  uint8_t ids[GOODS_COUNT];
  int n = uiFiltered(shop, ids, GOODS_COUNT);
  if (n == 0) { dispTextC(TXT_NOITEM, 64, 60, C_RED); return; }
  if (U.listSel >= n) U.listSel = 0;
  uint8_t idx = ids[U.listSel];
  const GoodDef& g = GOODS[idx];

  dispTextC(g.name, 64, 3, C_GOLD_HI);
  char buf[48];
  snprintf(buf, sizeof(buf), "%d/%d", U.listSel + 1, n);
  dispTextR(buf, 124, 4, C_DIM);

  /* 56x56 大图（s=4 → 48px） */
  dispRect(36, 17, 56, 56, C_PANEL2);
  dispFrame(36, 17, 56, 56, C_GOLD);
  dispItemIcon(idx, 40, 21, 4);

  dispTextC(g.eff, 64, 77, C_TEXT);
  snprintf(buf, sizeof(buf), "%s%d  %s%d", TXT_OWNED, G.bag[idx], TXT_PRICE, g.price);
  dispTextC(buf, 64, 90, C_GOLD);
  if (g.type == T_EQUIP) {
    dispTextC(TIER_NAMES[g.tier], 64, 101, g.tier == 3 ? C_PURPLE : (g.tier == 2 ? C_BLUE : C_DIM));
  }

  const char* act = "A 返回";
  if (shop) act = "A 购买 · ▲▼ 翻页";
  else if (g.type == T_FOOD) act = "A 吃掉 · ▲▼ 翻页";
  else if (g.type == T_MED)  act = "A 服用 · ▲▼ 翻页";
  else if (g.type == T_USE)  act = "A 参悟 · ▲▼ 翻页";
  else if (g.type == T_EQUIP) act = hasEq(idx) ? "A 卸下 · ▲▼ 翻页" : "A 穿戴 · ▲▼ 翻页";
  else if (g.sell) act = "A 卖出 · ▲▼ 翻页";
  dispTextC(act, 64, 114, C_DIM);
}

/* ---------------- 使用演出 ---------------- */
static void renderFx() {
  uint8_t idx = G_fxIdx >= GOODS_COUNT ? 0 : G_fxIdx;
  dispClear(C_BG);
  dispTextC(GOODS[idx].name, 64, 6, C_GOLD_HI);
  uint32_t total = G_fxKind == FX_EAT ? FX_EAT_MS : (G_fxKind == FX_MED ? FX_MED_MS : FX_USE_MS);
  uint32_t left = millis() < G_fxUntil ? G_fxUntil - millis() : 0;
  int pct = 100 - (int)(left * 100 / total);
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  int breathe = (U.frame / 6) % 3;
  dispItemIcon(idx, 40, 30 - breathe, 4);
  if (G_fxKind == FX_MED) {
    for (int i = 0; i < 6; i++) {
      int yy = 80 - ((U.frame / 3 + i * 7) % 30);
      dispPixel(38 + i * 9, yy, C_GREEN);
    }
  } else if (G_fxKind == FX_USE) {
    for (int i = 0; i < 8; i++) {
      int yy = 88 - ((U.frame / 2 + i * 9) % 50);
      dispPixel(34 + i * 8, yy, C_GOLD_HI);
    }
  }
  dispBar(14, 100, 100, 6, pct, C_GOLD);
}

/* ---------------- 家园 ---------------- */
static void renderHome() {
  dispClear(C_BG);
  char buf[48];
  snprintf(buf, sizeof(buf), "%s %s", TXT_HOME, HOME_NAMES[G.homeLv]);
  dispTextC(buf, 64, 4, C_GOLD);
  dispRect(0, 18, 128, 64, C_PANEL2);
  dispRect(8, 70, 112, 4, 0x4228);
  dispHouse(48, 70);
  if (G.decos[0]) dispDecos(70);
  if (G.decos[1]) { dispRect(24, 56, 14, 14, 0x8A40); dispRect(26, 58, 10, 10, 0xD4A0); }
  if (G.decos[2]) { dispRect(103, 59, 1, 11, 0x8A40); dispRect(101, 54, 5, 6, C_GOLD_HI); }
  if (G.nannyGH > 0) {
    dispRect(18, 62, 4, 8, 0xD8B8); dispRect(17, 58, 6, 4, 0x5A46);
    snprintf(buf, sizeof(buf), "%s %s%d%s", TXT_NANNY, TXT_LEFT, (int)ceilf(G.nannyGH), TXT_HOUR);
    dispText(buf, 8, 21, C_GREEN);
  }
  snprintf(buf, sizeof(buf), "%s -%d%%", TXT_MOOD, moodDecayCutPct());
  dispText(buf, 8, 84, C_TEXT);
  snprintf(buf, sizeof(buf), "%s%d/3", TXT_DECO, decoCount());
  dispText(buf, 8, 95, C_TEXT);
  snprintf(buf, sizeof(buf), "%s%d", TXT_COINS, G.coins);
  dispText(buf, 8, 106, C_GOLD_HI);

  /* 选项：升级 / 打扫 / 保姆 */
  uint8_t opts[3]; int n = 0;
  if (G.homeLv < 4) opts[n++] = 0;
  if (G.mess > 0) opts[n++] = 1;
  opts[n++] = 2;
  if (U.homeSel >= n) U.homeSel = 0;
  const char* labels[3] = { TXT_UP, TXT_CLEAN, TXT_HIRE };
  for (int i = 0; i < n; i++) {
    int x = 6 + i * 42, y = 112;
    bool on = i == U.homeSel;
    dispRect(x, y, 38, 14, on ? C_PANEL2 : C_PANEL);
    if (on) dispSelFrame(x, y, 38, 14);
    dispTextC(labels[opts[i]], x + 19, y + 3, on ? C_GOLD_HI : C_DIM);
  }
}

/* ---------------- 日常 ---------------- */
static void renderDaily() {
  dispClear(C_BG);
  dispTextC(TXT_DAILY, 64, 4, C_GOLD);
  dispRect(10, 17, 108, 1, C_GOLD);
  for (int i = 0; i < 5; i++) {
    int y = 24 + i * 16;
    bool on = i == U.dailySel;
    if (on) { dispRect(6, y - 2, 116, 15, C_PANEL2); dispSelFrame(6, y - 2, 116, 15); }
    dispText("▶", 9, y, on ? C_GOLD_HI : C_DIM);
    dispText(DAY_ITEMS[i], 24, y, on ? C_GOLD_HI : C_TEXT);
    dispRect(100, y + 1, 16, 8, C_PANEL2);
    dispText("本", 103, y + 1, C_GOLD);
  }
  dispTextC(D_ENTER, 64, 114, C_DIM);
}

/* ---------------- 奇遇邮箱（v0.2.8） ---------------- */
static void renderMail() {
  dispClear(C_BG);
  if (!G_mailCount) {
    dispTextC(TXT_MAIL, 64, 4, C_GOLD);
    dispTextC(TXT_NO_MAIL, 64, 56, C_DIM);
    dispTextC(TXT_MAIL_HINT, 64, 72, C_DIM);
    return;
  }
  if (U.mailSel >= G_mailCount) U.mailSel = 0;
  const MailEntry* m = &G_mail[G_mailCount - 1 - U.mailSel];   /* 最新在前 */
  if (U.mailDetail) {
    dispTextC("奇遇来信", 64, 4, C_GOLD);
    dispIcon(8, 52, 16, false);
    /* 叙事文案：场景 + 遭遇 + 经过 + 奖励 */
    char buf[40];
    snprintf(buf, sizeof(buf), "你的侠士，于%s", MAIL_WHERE[m->scene]);
    dispTextC(buf, 64, 48, C_TEXT);
    snprintf(buf, sizeof(buf), "遇到了%s", MAIL_WHO[m->scene]);
    dispTextC(buf, 64, 60, C_TEXT);
    snprintf(buf, sizeof(buf), "%s，获得", MAIL_HOW[m->scene]);
    dispTextC(buf, 64, 72, C_TEXT);
    snprintf(buf, sizeof(buf), "%s+%d", m->reward == 0 ? "修为" : "金币", m->amount);
    dispTextC(buf, 64, 84, C_GOLD_HI);
  } else {
    dispTextC(TXT_MAIL, 64, 4, C_GOLD);
    char buf[24]; snprintf(buf, sizeof(buf), "%d 封", G_mailCount);
    dispTextR(buf, 124, 6, C_DIM);
    dispIcon(8, 52, 18, false);
    char sub[32];
    snprintf(sub, sizeof(sub), "%s · %s+%d", MAIL_WHERE[m->scene],
             m->reward == 0 ? "修为" : "金币", m->amount);
    dispTextC(sub, 64, 76, C_GOLD_HI);
    dispTextC("A 查看 · B 返回", 64, 114, C_DIM);
  }
}

/* ---------------- 手札 ---------------- */
static void renderStatus() {
  dispClear(C_BG);
  dispTextC(TXT_STATUS, 64, 4, C_GOLD);
  dispRect(10, 17, 108, 1, C_GOLD);
  char buf[64];
  snprintf(buf, sizeof(buf), "%s  %s", TXT_TITLE, titleName());
  dispText(buf, 8, 23, C_GOLD_HI);
  snprintf(buf, sizeof(buf), "%s%d  %s%s", TXT_EXP, G.exp, TXT_REPUTE, reputationName());
  dispText(buf, 8, 35, C_TEXT);
  dispBar(8, 46, 112, 5, G.exp * 100 / TITLES[4].exp, C_GOLD);
  snprintf(buf, sizeof(buf), "%s  %.0f / 100", TXT_HUNGER, G.hunger);
  dispText(buf, 8, 56, C_TEXT); dispBar(8, 67, 112, 5, (int)G.hunger, C_GREEN);
  snprintf(buf, sizeof(buf), "%s  %.0f / 100", TXT_MOOD, G.mood);
  dispText(buf, 8, 77, C_TEXT); dispBar(8, 88, 112, 5, (int)G.mood, 0xF3DD);
  snprintf(buf, sizeof(buf), "%s%d  %s%d  %s%d",
           TXT_COINS, G.coins, TXT_HOMELV, G.homeLv, TXT_DAY, G.loginStreak);
  dispText(buf, 8, 98, C_GOLD_HI);
  dispText(condText(), 8, 110, G.down ? C_RED : C_TEXT);
  dispTextR(TXT_BACK, 124, 110, C_DIM);
}

/* ---------------- 黑市赌坊 ---------------- */
static void renderGame() {
  dispClear(C_BG);
  dispTextC(TXT_GAME, 64, 4, C_GOLD);
  char buf[40];
  snprintf(buf, sizeof(buf), "%s%d", TXT_COINS, G.coins);
  dispText(buf, 8, 18, C_GOLD_HI);
  bool rolling = GM.phase == PH_ROLL;
  for (int i = 0; i < 3; i++) {
    int dx = 22 + i * 30, dy = 32;
    dispRect(dx, dy, 24, 24, C_PANEL2);
    dispFrame(dx, dy, 24, 24, C_GOLD);
    if (!rolling || ((U.frame / 4 + i) % 2 == 0)) {
      char d[2]; d[0] = '0' + GM.dice[i]; d[1] = 0;
      dispTextC(d, dx + 12, dy + 8, C_GOLD_HI);
    } else dispTextC("?", dx + 12, dy + 8, C_TEXT);
  }
  if (GM.phase == PH_SIDE || GM.phase == PH_BET) {
    bool on1 = GM.side == 1, on2 = GM.side == 0;
    if (GM.phase == PH_SIDE) {
      if (on1) { dispRect(8, 65, 112, 12, C_PANEL2); dispSelFrame(8, 65, 112, 12); }
      if (on2) { dispRect(8, 79, 112, 12, C_PANEL2); dispSelFrame(8, 79, 112, 12); }
    }
    dispText(TXT_SIDE, 12, 67, on1 ? C_GOLD_HI : C_DIM);
    dispText(TXT_SMALL, 12, 81, on2 ? C_GOLD_HI : C_DIM);
  } else {
    snprintf(buf, sizeof(buf), "%d", GM.sum);
    dispTextC(buf, 64, 67, C_TEXT);
  }
  if (GM.phase == PH_RESULT) {
    if (GM.baozi) dispTextC(TXT_BAOZI, 64, 98, C_RED);
    else if (GM.win) {
      snprintf(buf, sizeof(buf), "%s +%d", TXT_WIN, GM.pay);
      dispTextC(buf, 64, 98, C_GREEN);
    } else {
      snprintf(buf, sizeof(buf), "%s -%d", TXT_LOSE, GM.pay);
      dispTextC(buf, 64, 98, C_RED);
    }
    dispTextC("A 再来 · B 离开", 64, 114, C_DIM);
  } else if (GM.phase == PH_BET) {
    snprintf(buf, sizeof(buf), "%s%d", TXT_BET, BETS[GM.betIdx]);
    dispTextC(buf, 64, 98, C_GOLD_HI);
    dispTextC("▲▼ 调整 · A 开骰 · B 返回", 64, 114, C_DIM);
  } else if (GM.phase == PH_SIDE) {
    dispTextC(TXT_BAOZI, 64, 98, C_DIM);
    dispTextC("▲▼ 选边 · A 押注 · B 离开", 64, 114, C_DIM);
  } else {
    dispTextC(TXT_ROLLING, 64, 98, C_GOLD_HI);
    dispTextC(TXT_LEAVE, 64, 114, C_DIM);
  }
}

/* ---------------- 系统设置 ---------------- */
static void renderSettings() {
  dispClear(C_BG);
  dispTextC(TXT_SETTINGS, 64, 4, C_GOLD);
  dispRect(10, 17, 108, 1, C_GOLD);
  if (U.setPage == 0) {
    for (int i = 0; i < 6; i++) {
      int y = 24 + i * 15;
      bool on = i == U.setIdx;
      if (on) { dispRect(8, y - 2, 112, 14, C_PANEL2); dispSelFrame(8, y - 2, 112, 14); }
      dispText(SET_ITEMS[i], 14, y, on ? C_GOLD_HI : C_TEXT);
    }
    dispTextC("A 进入 · B 返回", 64, 116, C_DIM);
    return;
  }
  char buf[64];
  if (U.setPage == 1) {           // 时间流速
    dispTextC(TXT_SET_SPEED, 64, 24, C_GOLD_HI);
    const char* names[4] = { TXT_SPEED_1, TXT_SPEED_10, TXT_SPEED_60, TXT_SPEED_600 };
    const int vals[4] = { 1, 10, 60, 600 };
    for (int i = 0; i < 4; i++) {
      int y = 42 + i * 16;
      bool on = i == G.speedIdx;
      if (on) { dispRect(16, y - 2, 96, 14, C_PANEL2); dispSelFrame(16, y - 2, 96, 14); }
      snprintf(buf, sizeof(buf), "×%d  %s", vals[i], names[i]);
      dispTextC(buf, 64, y, on ? C_GOLD_HI : C_TEXT);
    }
    dispTextC("A 选择 · B 返回", 64, 114, C_DIM);
  } else if (U.setPage == 2) {    // 音效
    dispTextC(TXT_SET_SOUND, 64, 30, C_GOLD_HI);
    dispTextC(G.muted ? TXT_OFF : TXT_ON, 64, 54, G.muted ? C_DIM : C_GREEN);
    dispRect(38, 74, 52, 14, C_PANEL2);
    dispFrame(38, 74, 52, 14, C_GOLD);
    dispTextC(G.muted ? TXT_OFF : TXT_ON, 64, 77, C_GOLD_HI);
    dispTextC("A 切换 · B 返回", 64, 108, C_DIM);
  } else if (U.setPage == 3) {    // 存档（保存动作在按键处理里做，避免每帧写 flash）
    dispTextC(MSG_SAVED, 64, 54, C_GREEN);
    dispTextC("B 返回", 64, 108, C_DIM);
  } else if (U.setPage == 4) {    // 清档确认
    dispTextC(TXT_CONFIRM, 64, 44, C_RED);
    const char* yn[2] = { TXT_NO, TXT_YES };
    for (int i = 0; i < 2; i++) {
      int x = 30 + i * 40;
      bool on = i == U.confirmSel;
      dispRect(x, 66, 32, 16, on ? C_PANEL2 : C_PANEL);
      if (on) dispSelFrame(x, 66, 32, 16);
      dispTextC(yn[i], x + 16, 69, on ? C_GOLD_HI : C_TEXT);
    }
    dispTextC("▲▼ 选择 · A 确认 · B 取消", 64, 108, C_DIM);
  } else if (U.setPage == 5) {    // 关于
    dispTextC(TXT_SET_ABOUT, 64, 26, C_GOLD_HI);
    snprintf(buf, sizeof(buf), "%s %s", TXT_ABOUT_FW, APP_VERSION);
    dispTextC(buf, 64, 48, C_TEXT);
    dispTextC(TXT_ABOUT_DEV, 64, 64, C_TEXT);
    dispTextC(TXT_ABOUT_SCR, 64, 80, C_DIM);
    dispTextC("B 返回", 64, 108, C_DIM);
  } else {                        // 操作提示
    dispTextC(TXT_HOLD_SET, 64, 34, C_GOLD_HI);
    dispTextC("任意页面长按 B 1.5 秒", 64, 54, C_TEXT);
    dispTextC("▲▼ 选单 · A 确认 · B 返回", 64, 74, C_DIM);
    dispTextC("B 返回", 64, 108, C_DIM);
  }
}

/* ---------------- 总渲染 ---------------- */
void uiRender() {
  U.frame++;
  if (G_fxKind != FX_NONE && millis() < G_fxUntil) {
    renderFx();
    uiToastOverlay();
    return;
  }
  if (G_fxKind != FX_NONE) G_fxKind = FX_NONE;
  switch (U.page) {
    case PG_BOOT:     renderBoot();     break;
    case PG_MAIL:     renderMail();     break;
    case PG_MAIN:     renderMain();     break;
    case PG_BAG:      renderList(false);break;
    case PG_SHOP:     renderList(true); break;
    case PG_ITEM:     renderItem();     break;
    case PG_HOME:     renderHome();     break;
    case PG_DAILY:    renderDaily();    break;
    case PG_STATUS:   renderStatus();   break;
    case PG_GAME:     renderGame();     break;
    case PG_SETTINGS: renderSettings(); break;
  }
  uiToastOverlay();
}

void uiToastOverlay() {
  if (millis() >= G_toastUntil || !G_toast) return;
  dispRect(2, 106, 124, 20, C_PANEL);
  dispFrame(2, 106, 124, 20, C_GOLD);
  /* 单行放不下就截成 10 个汉字，够短句用 */
  char buf[32];
  strncpy(buf, G_toast, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;
  dispTextC(buf, 64, 111, C_GOLD_HI);
}

/* ---------------- 菜单行为 ---------------- */
void uiDoMenu() {
  switch (U.menuIdx) {
    case 0: uiGoto(PG_DAILY); break;
    case 1: U.catIdx = 0; U.catMode = true; U.listSel = 0; uiGoto(PG_BAG); break;
    case 2: U.catIdx = 0; U.catMode = true; U.listSel = 0; uiGoto(PG_SHOP); break;
    case 3: actSit(); break;
    case 4: actTrain(); break;
    case 5: actDig(); break;
    case 6:
      if (G.coins < 50) { toast(MSG_NOMONEY); return; }
      GM.phase = PH_SIDE; GM.side = 1; GM.betIdx = 1;
      GM.dice[0] = GM.dice[1] = GM.dice[2] = 1;
      GM.sum = 0; GM.baozi = GM.win = false; GM.pay = 0;
      uiGoto(PG_GAME); break;
    case 7: U.homeSel = 0; uiGoto(PG_HOME); break;
  }
}

void uiItemAction() {
  bool shop = U.itemFrom == 1;
  uint8_t ids[GOODS_COUNT];
  int n = uiFiltered(shop, ids, GOODS_COUNT);
  if (n == 0 || U.listSel >= n) return;
  uint8_t idx = ids[U.listSel];
  const GoodDef& g = GOODS[idx];
  if (shop) { actBuy(idx); return; }
  switch (g.type) {
    case T_FOOD: actEat(idx); break;
    case T_MED:  actUseMed(); break;
    case T_USE:  actUsePage(); break;
    case T_EQUIP: actEquipToggle(idx); break;
    case T_DECO: toast(MSG_DECO); break;
    default: if (g.sell) actSell(idx); else toast(TXT_NOITEM); break;
  }
}
