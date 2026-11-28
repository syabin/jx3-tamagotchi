/* ui.cpp — 全部页面渲染与交互（对齐网页版 0.2.11） */
#include "ui.h"
#include "display.h"
#include "boot_logo.h"
#include <string.h>
#include <stdio.h>

UIState UI;

/* ================= 工具 ================= */
static uint8_t orderLen(uint8_t kind) { return kind == LIST_SHOP ? SHOP_N : GOODS_COUNT; }

static uint8_t catType(uint8_t catIdx) { return (catIdx < CAT_N) ? CATS[catIdx].type : 0xFF; }

static uint8_t orderAt(uint8_t kind, uint8_t i) {
  const uint8_t* base = (kind == LIST_SHOP) ? SHOP_ORDER : BAG_ORDER;
  uint8_t n = orderLen(kind);
  uint8_t cat = (kind == LIST_SHOP) ? UI.shopCat : UI.bagCat;
  uint8_t want = catType(cat);
  if (want == 0xFF) return (i < n) ? base[i] : 0;
  uint8_t k = 0;
  for (uint8_t j = 0; j < n; j++) {
    const GoodsDef& g = GOODS[base[j]];
    if (catOfType(g.type) != want) continue;
    if (k == i) return base[j];
    k++;
  }
  return 0;
}

static uint8_t orderCount(uint8_t kind) {
  const uint8_t* base = (kind == LIST_SHOP) ? SHOP_ORDER : BAG_ORDER;
  uint8_t n = orderLen(kind);
  uint8_t cat = (kind == LIST_SHOP) ? UI.shopCat : UI.bagCat;
  uint8_t want = catType(cat);
  if (want == 0xFF) return n;
  uint8_t k = 0;
  for (uint8_t j = 0; j < n; j++) if (catOfType(GOODS[base[j]].type) == want) k++;
  return k;
}

static uint8_t& selRef(uint8_t kind) { return kind == LIST_SHOP ? UI.shopSel : UI.bagSel; }

static const char* itemEffect(const GoodsDef& g) {
  static char buf[36];
  switch (g.type) {
    case GT_FOOD: snprintf(buf, sizeof(buf), "体力+%d 心情+%d", g.gain, g.mood); break;
    case GT_MED:  snprintf(buf, sizeof(buf), "解毒疗伤 回满状态"); break;
    case GT_USE:  snprintf(buf, sizeof(buf), "参悟 修为+%d", g.exp); break;
    case GT_EQUIP: strncpy(buf, g.eff, sizeof(buf) - 1); buf[sizeof(buf) - 1] = 0; break;
    case GT_DECO: strncpy(buf, g.eff, sizeof(buf) - 1); buf[sizeof(buf) - 1] = 0; break;
    case GT_XJ:   snprintf(buf, sizeof(buf), "本赛季玄晶 卖价%d", g.sell); break;
    default:      snprintf(buf, sizeof(buf), "卖价 %d 金", g.sell); break;
  }
  return buf;
}

static uint16_t itemColor(const GoodsDef& g) {
  if (g.type == GT_EQUIP && g.tier) return TIER_COLOR[g.tier];
  if (g.type == GT_XJ) return C_ORANGE;
  return 0;
}

static void drawToastBar() {
  if (!g_toast[0] || millis() > g_toastUntil) return;
  dispRect(4, 99, 120, 11, C_PANEL);
  dispRect(4, 99, 120, 1, C_GOLD);
  dispRect(4, 109, 120, 1, C_GOLD);
  dispTextClip(g_toast, 6, 100, 116, C_GOLD_HI, (int)(millis() / 12 % 200));
}

/* 大图台座 56×56（活动/游戏/手札/物品共用） */
static void pedestal56(uint8_t spr) {
  int bx = 36, by = 22, bs = 56;
  dispRect(bx - 3, by - 3, bs + 6, bs + 6, C_BTN);
  dispRect(bx - 3, by - 3, bs + 6, 1, C_LINE);
  dispRect(bx + 14, by + bs - 1, 28, 3, HEXC(0x161b28));
  dispSprS(spr, bx + 3, by + 3, 56);
}

static void drawZzz() {
  int ox = 104, py = 40 - (int)((frameCounter() / 6) % 10);
  dispText("Z", ox, py, HEXC(0xb8c8e8));
}

/* ================= 开机 ================= */
static void renderBoot() {
  dispClear(HEXC(0x0d0f18));
  static const uint8_t ST[12][2] = {
    {14,16},{30,10},{52,20},{80,12},{104,18},{22,40},
    {100,44},{12,72},{112,66},{40,98},{90,100},{64,8}
  };
  for (int i = 0; i < 12; i++)
    if ((frameCounter() + i * 23) % 120 < 82) dispPixel(ST[i][0], ST[i][1], HEXC(0x3a4160));
  dispRect(14, 22, 100, 1, HEXC(0x8a7440)); dispRect(14, 76, 100, 1, HEXC(0x8a7440));
  dispRect(14, 22, 6, 1, HEXC(0xd8c68a)); dispRect(14, 22, 1, 5, HEXC(0xd8c68a));
  dispRect(108, 22, 6, 1, HEXC(0xd8c68a)); dispRect(113, 22, 1, 5, HEXC(0xd8c68a));
  dispRect(14, 71, 6, 1, HEXC(0xd8c68a)); dispRect(14, 67, 1, 5, HEXC(0xd8c68a));
  dispRect(108, 71, 6, 1, HEXC(0xd8c68a)); dispRect(113, 67, 1, 5, HEXC(0xd8c68a));

  int bob = (frameCounter() / 30) % 2;
  for (int y = 0; y < BOOT_LOGO_H; y++) {
    for (int x = 0; x < BOOT_LOGO_W; x++) {
      uint16_t c = pgm_read_word(&BOOT_LOGO[y * BOOT_LOGO_W + x]);
      if (c) dispPixel(40 + x, 24 + bob + y, c);
    }
  }
  dispTextC("剑侠情缘·单机版三", 64, 80 + bob, HEXC(0xd8c68a));
  if (frameCounter() % 64 < 40) dispTextC("PRESS A START", 64, 96, HEXC(0xf5ecd0));
  dispTextC("v" APP_VERSION, 64, 116, HEXC(0x5a6078));
}

/* ================= 主界面 ================= */
static void renderMain() {
  bool night = !gameIsDay();
  dispClear(C_BG);

  /* 状态栏：称号 + 金币；数值条 */
  dispRect(0, 0, SCR_W, 16, C_BAR);
  dispRect(0, 16, SCR_W, 1, C_LINE);
  dispText(titleName(), 4, 1, C_GOLD_HI);
  char buf[24];
  snprintf(buf, sizeof(buf), "金%d", G.coins);
  dispTextR(buf, 124, 1, C_GOLD);
  dispRect(0, 17, SCR_W, 8, C_BG);
  snprintf(buf, sizeof(buf), "体%d 心%d 修%d", G.hunger, G.mood, G.exp);
  dispText(buf, 4, 17, (G.hunger < 25 || G.mood < 25) ? C_WARN : C_TEXT);

  dispScene(night, frameCounter());

  /* 家园小屋 + 狼藉 + 保姆 */
  dispHouse(14, 93, night);
  dispMess(G.mess);
  if (G.nannyTenth > 0) {
    dispRect(44, 77, 1, 10, HEXC(0x8a6a40));
    dispRect(41, 75, 6, 3, HEXC(0xc9b070));
    dispText("保姆", 38, 63, HEXC(0xb8e88a));
  }

  /* 人物 */
  uint8_t pose = petPose();
  dispPet(pose, 56, 28);
  if (pose == 8) drawZzz();

  /* 副本进度条 */
  if (G.dungeonUntil > 0 && (int32_t)gameNowSec() < G.dungeonUntil) {
    int left = G.dungeonUntil - (int32_t)gameNowSec();
    int p = 10 - (left > 10 ? 10 : left);
    dispRect(36, 27, 44, 6, C_BTN);
    dispRect(36, 27, p * 44 / 10, 6, HEXC(0xe8a04a));
    dispText("副本中", 82, 27, HEXC(0xf5ecd0));
  }

  drawToastBar();

  /* 底部 5 个功能按钮 */
  static const uint8_t BOT[5] = { 16, 40, 64, 88, 112 };
  for (int i = 0; i < 5; i++)
    dispIcon(MENU[i].spr, BOT[i] - 8, 112, UI.menuIdx == i);
}

/* ================= 活动（一页一个） ================= */
static void renderActivity() {
  dispClear(C_BG);
  if (UI.actSel >= ACT_N) UI.actSel = ACT_N - 1;
  const ActDef& a = ACTIVITIES[UI.actSel];

  uint8_t cd = 0xFF;
  if (strcmp(a.id, "sit") == 0) cd = CD_SIT;
  else if (strcmp(a.id, "train") == 0) cd = CD_TRAIN;
  else if (strcmp(a.id, "dig") == 0) cd = CD_DIG;
  else if (strcmp(a.id, "dungeon") == 0) cd = CD_DUNGEON;
  bool cool = (cd != 0xFF) && cooling(cd);

  dispTextC(a.name, 54, 4, cool ? C_DIM : C_GOLD_HI);
  char buf[24];
  snprintf(buf, sizeof(buf), "%d/%d", UI.actSel + 1, ACT_N);
  dispTextR(buf, 124, 4, C_DIM);

  dispTri(6, 46, -1, true);
  dispTri(111, 46, 1, true);
  pedestal56(a.spr);

  dispTextClip(a.desc, 6, 81, 116, HEXC(0xe8ddb8), (int)(millis() / 16 % 260));
  if (cool) {
    int left = G.cool[cd] - (int32_t)gameNowSec();
    if (left < 1) left = 1;
    snprintf(buf, sizeof(buf), "冷却中 %d 秒", left);
    dispTextC(buf, 64, 94, C_WARN);
  } else {
    dispTextC("可执行", 64, 94, C_GOLD);
  }
  dispRect(4, 104, 120, 1, C_LINE);
  dispTextClip(a.desc, 6, 108, 116, C_DIM, (int)(millis() / 20 % 220));
}

/* ================= 背包 / 商城 ================= */
static void renderList(uint8_t kind) {
  dispClear(C_BG);
  bool isShop = (kind == LIST_SHOP);
  dispTextC(isShop ? "商城·长安集市" : "背包·随身行囊", 44, 4, HEXC(0xd8c68a));
  char buf[20];
  snprintf(buf, sizeof(buf), "金%d", G.coins);
  dispTextR(buf, 124, 4, C_GOLD);

  uint8_t cat = isShop ? UI.shopCat : UI.bagCat;
  /* 12px 点阵放不下 6×2 字一行，改两行三列（与网页分类栏同义） */
  for (int i = 0; i < CAT_N; i++) {
    int col = i % 3, row = i / 3;
    int cx = 6 + col * 40, cy = 14 + row * 14;
    bool on = (i == cat);
    if (on) dispRect(cx - 2, cy - 1, 32, 14, UI.catMode ? HEXC(0x3a3f54) : HEXC(0x2e3345));
    dispText(CATS[i].name, cx + 4, cy, on ? (UI.catMode ? C_GOLD_HI : C_GOLD) : HEXC(0x5a6078));
  }
  if (UI.catMode) dispSelFrame(2, 11, 124, 30);
  dispRect(4, 42, 120, 1, C_LINE);

  uint8_t n = orderCount(kind);
  if (n == 0) {
    dispTextC(isShop ? "此类暂无商品" : "行囊里没有这类", 64, 60, C_DIM);
    return;
  }
  uint8_t& sel = selRef(kind);
  if (sel >= n) sel = n - 1;

  uint8_t start = 0;
  if (sel >= LIST_ROWS) start = (sel >= n - 1) ? (n > LIST_ROWS ? n - LIST_ROWS : 0) : sel - 1;
  if (start + LIST_ROWS > n) start = (n > LIST_ROWS) ? n - LIST_ROWS : 0;

  for (int i = 0; i < LIST_ROWS; i++) {
    uint8_t idx = start + i;
    if (idx >= n) break;
    uint8_t gid = orderAt(kind, idx);
    const GoodsDef& g = GOODS[gid];
    int y = 45 + i * 15;
    bool on = (idx == sel) && !UI.catMode;
    if (on) dispSelFrame(3, y - 1, 122, 15);

    bool owned = (g.type == GT_DECO) ? ((G.decos & ((gid == G_D_PLANT) ? 1 : (gid == G_D_SCREEN) ? 2 : 4)) != 0)
                                    : (G.bag[gid] > 0);
    dispSpr(g.spr, 6, y);
    uint16_t tc = itemColor(g);
    dispText(g.name, 25, y + 1, on ? C_GOLD_HI : (tc ? tc : (owned ? C_TEXT : HEXC(0x6b7488))));

    if (isShop) {
      snprintf(buf, sizeof(buf), "%d金", g.price);
      dispTextR(buf, 123, y + 1, G.coins >= g.price ? C_GOLD : HEXC(0x7a5a5a));
    } else if (g.type == GT_DECO) {
      dispTextR(owned ? "已布置" : "无", 123, y + 1, owned ? HEXC(0xb8e88a) : HEXC(0x5a6078));
    } else {
      snprintf(buf, sizeof(buf), "×%d", G.bag[gid]);
      dispTextR(buf, 123, y + 1, owned ? C_TEXT : HEXC(0x5a6078));
    }
  }

  /* 滚动条 */
  if (n > LIST_ROWS) {
    dispRect(125, 45, 2, 60, C_BTN);
    int h = 60 * LIST_ROWS / n;
    if (h < 8) h = 8;
    int top = 45 + (60 - h) * start / (n - LIST_ROWS > 0 ? n - LIST_ROWS : 1);
    dispRect(125, top, 2, h, C_DIM);
  }

  if (!UI.catMode) {
    uint8_t gid = orderAt(kind, sel);
    const GoodsDef& g = GOODS[gid];
    char line[48];
    snprintf(line, sizeof(line), "%s %s", g.name, itemEffect(g));
    dispRect(4, 104, 120, 1, C_LINE);
    dispTextClip(line, 6, 108, 116, C_TEXT, (int)(millis() / 18 % 240));
  }
}

/* ================= 物品详情 ================= */
static void renderItem() {
  dispClear(C_BG);
  uint8_t kind = UI.itemFrom;
  uint8_t n = orderCount(kind);
  if (n == 0) { UI.page = kind; return; }
  uint8_t& sel = selRef(kind);
  if (sel >= n) sel = n - 1;
  uint8_t gid = orderAt(kind, sel);
  const GoodsDef& g = GOODS[gid];

  dispTextC(g.name, 56, 4, itemColor(g) ? itemColor(g) : HEXC(0xd8c68a));
  char buf[24];
  snprintf(buf, sizeof(buf), "%d/%d", sel + 1, n);
  dispTextR(buf, 124, 4, C_DIM);
  if (g.type == GT_EQUIP && g.tier) {
    snprintf(buf, sizeof(buf), "品级 %s", TIER_NAME[g.tier]);
    dispTextC(buf, 64, 15, TIER_COLOR[g.tier]);
  }
  pedestal56(g.spr);
  dispTextClip(itemEffect(g), 6, 81, 116, HEXC(0xe8ddb8), (int)(millis() / 16 % 260));

  if (kind == LIST_SHOP) {
    snprintf(buf, sizeof(buf), "售价%d 持有%d", g.price, G.bag[gid]);
    dispTextC(buf, 64, 94, C_DIM);
  } else if (g.type == GT_EQUIP) {
    snprintf(buf, sizeof(buf), "%s %s", SLOT_NAME[g.slot], hasEquip(gid) ? "已装备" : "未装备");
    dispTextC(buf, 64, 94, C_DIM);
  } else if (g.type == GT_DECO) {
    dispTextC("已布置庭院", 64, 94, C_DIM);
  } else {
    snprintf(buf, sizeof(buf), "持有%d 卖价%d", G.bag[gid], g.sell);
    dispTextC(buf, 64, 94, C_DIM);
  }
  dispRect(4, 104, 120, 1, C_LINE);
  dispTextClip(g.desc, 6, 108, 116, C_DIM, (int)(millis() / 22 % 240));

  const char* act = "出售";
  if (kind == LIST_SHOP) act = (G.coins >= g.price) ? "A 购买" : "金币不足";
  else if (g.type == GT_EQUIP) act = hasEquip(gid) ? "A 卸下" : "A 装备";
  else if (g.type == GT_FOOD) act = "A 食用";
  else if (g.type == GT_MED) act = "A 服用";
  else if (g.type == GT_USE) act = "A 参悟";
  else if (g.type == GT_DECO) act = "已在庭院";
  else if (!g.sell) act = "不可出售";
  else { snprintf(buf, sizeof(buf), "A 卖出 %d 金", g.sell); act = buf; }
  dispTextC(act, 64, 114, C_GOLD_HI);
}

/* ================= 家园 ================= */
static void renderHome() {
  dispClear(C_BG);
  char buf[40];
  snprintf(buf, sizeof(buf), "家园·%s", HOME_NAMES[homeNameIdx()]);
  dispTextC(buf, 64, 5, HEXC(0xd8c68a));

  int gy = 72;
  dispRect(8, gy, 112, 6, HEXC(0x3a2f22));
  dispHouse(48, gy, !gameIsDay());
  /* 装饰 */
  if (G.decos & 1) dispSpr(SPR_D_PLANT, 14, gy - 16);
  if (G.decos & 2) dispSpr(SPR_D_SCREEN, 88, gy - 16);
  if (G.decos & 4) dispSpr(SPR_D_LAMP, 62, gy - 18);
  if (G.nannyTenth > 0) {
    dispRect(18, gy - 6, 4, 6, HEXC(0xd8b8a0));
    dispRect(17, gy - 10, 6, 4, HEXC(0x5a4638));
    dispRect(22, gy - 8, 5, 1, HEXC(0x8a6a40));
    dispText("保姆代管", 10, gy - 22, HEXC(0xb8e88a));
  }
  int mm = (int)((0.15f * G.homeLv) * 100 + 0.5f);
  snprintf(buf, sizeof(buf), "心情流失-%d%% 装饰%d/3", mm, decoCount());
  dispText(buf, 6, 80, C_TEXT);
  if (G.nannyTenth > 0) snprintf(buf, sizeof(buf), "保姆剩%d时辰", (G.nannyTenth + 9) / 10);
  else snprintf(buf, sizeof(buf), "保姆未雇 100金");
  dispText(buf, 6, 92, C_TEXT);
  if (G.homeLv < 4) snprintf(buf, sizeof(buf), "金%d 升级需%d", G.coins, (G.homeLv + 1) * 500);
  else snprintf(buf, sizeof(buf), "金%d 已至山庄", G.coins);
  dispText(buf, 6, 104, C_GOLD);

  /* 选项：升级 / 打扫 / 保姆 */
  uint8_t optN = 3;
  if (UI.homeSel >= optN) UI.homeSel = 0;
  static const char* LBL[3] = { "升级", "打扫", "保姆" };
  for (int i = 0; i < 3; i++) {
    int x = 6 + i * 42, y = 114, w = 38, h = 13;
    dispRect(x, y, w, h, (i == UI.homeSel) ? HEXC(0x3a3f54) : HEXC(0x22263a));
    if (i == UI.homeSel) dispSelFrame(x, y, w, h);
    dispTextC(LBL[i], x + w / 2, y + 1, (i == UI.homeSel) ? C_GOLD_HI : C_DIM);
  }
}

/* ================= 邮箱 ================= */
static void mailSummary(const MailEntry& m, char* out, int n) {
  if (m.reward == RW_EXP) snprintf(out, n, "修为+%d", m.amount);
  else if (m.reward == RW_COIN) snprintf(out, n, "金币+%d", m.amount);
  else snprintf(out, n, "玉露丸×%d", m.amount ? m.amount : 1);
}

static void mailText(const MailEntry& m, char* out, int n) {
  const MailSceneDef& sc = MAIL_SCENES[m.scene < MAIL_N ? m.scene : 0];
  uint8_t hr = (uint8_t)((m.ts / 3600UL) % 12);
  const char* rw = (m.reward == RW_EXP) ? "修为" : (m.reward == RW_COIN) ? "金币" : "玉露丸";
  snprintf(out, n, "你的%s，在%s于%s遇到了%s，%s，获得了%s%d。",
           titleName(), MAIL_HOURS[hr], sc.where, sc.who, sc.how, rw, m.amount);
}

static void renderMail() {
  dispClear(C_BG);
  if (G.mailN == 0) {
    dispTextC("邮箱", 64, 5, HEXC(0xd8c68a));
    dispTextC("暂无来信", 64, 52, C_DIM);
    dispTextC("奇遇会自动寄到邮箱", 64, 68, HEXC(0x79839a));
    return;
  }
  if (UI.mailSel >= G.mailN) UI.mailSel = 0;
  const MailEntry& m = G.mail[UI.mailSel];
  char buf[96];

  if (UI.mailDetail) {
    dispTextC("奇遇来信", 64, 3, HEXC(0xd8c68a));
    mailSummary(m, buf, sizeof(buf));
    dispTextC(buf, 64, 16, C_GOLD);
    dispRect(16, 28, 96, 1, C_LINE);

    mailText(m, buf, sizeof(buf));
    const char* lines[6];
    int nl = dispWrap(buf, 9, lines, 6);
    for (int i = 0; i < nl; i++) dispText(lines[i], 8, 34 + i * 14, HEXC(0xe8ddb8));
    return;
  }

  dispTextC("邮箱", 64, 3, HEXC(0xd8c68a));
  snprintf(buf, sizeof(buf), "%d封", G.mailN);
  dispTextR(buf, 124, 4, C_DIM);
  dispTri(6, 60, -1, G.mailN > 1);
  dispTri(111, 60, 1, G.mailN > 1);

  int bx = 40, by = 18;
  dispRect(bx - 2, by - 2, 52, 52, C_BTN);
  dispRect(bx - 2, by - 2, 52, 1, C_LINE);
  dispSprS(SPR_MAIL, bx + 10, by + 10, 32);

  mailSummary(m, buf, sizeof(buf));
  dispTextC(buf, 64, 76, C_GOLD);
  mailText(m, buf, sizeof(buf));
  dispTextClip(buf, 6, 92, 116, C_TEXT, (int)(millis() / 20 % 260));
  snprintf(buf, sizeof(buf), "%d/%d", UI.mailSel + 1, G.mailN);
  dispTextR(buf, 124, 116, C_DIM);
}

/* ================= 手札（3 页 + 详情 2 页） ================= */
static void renderStatus() {
  dispClear(C_BG);
  char buf[48];

  if (UI.notePage == 0 && UI.noteDetail) {
    dispTextC("侠士手札", 64, 3, HEXC(0xd8c68a));
    dispTextC(titleName(), 64, 17, C_GOLD_HI);
    dispRect(16, 32, 96, 1, C_LINE);

    const char* eqn = "";
    for (int i = 0; i < 3; i++) if (G.equip[i] != 0xFF) eqn = GOODS[G.equip[i]].name;
    static char l0[24], l1[24], l2[24], l3[24], l4[24], l5[24], l6[24], l7[24];
    snprintf(l0, 24, "体力%d 心情%d", G.hunger, G.mood);
    snprintf(l1, 24, "修为%d %s", G.exp, repName());
    snprintf(l2, 24, "金币%d", G.coins);
    snprintf(l3, 24, "家园%s 饰%d", HOME_NAMES[homeNameIdx()], decoCount());
    snprintf(l4, 24, "装备 %s", eqn[0] ? eqn : "无");
    snprintf(l5, 24, "%s", (G.flags & 0x02) ? "重伤 收益-40%" : (G.flags & 0x01) ? "中毒 收益-20%" : "状态康健");
    snprintf(l6, 24, "狼藉%d处", G.mess);
    snprintf(l7, 24, "连续登录%d天", G.loginStreak);
    const char* L[8] = { l0, l1, l2, l3, l4, l5, l6, l7 };
    uint8_t pg = UI.noteDetailPage;
    if (pg > 1) pg = 0;
    for (int i = 0; i < 4; i++) {
      int gi = pg * 4 + i;
      uint16_t c = (gi == 5 && (G.flags & 0x03)) ? C_WARN : C_TEXT;
      dispText(L[gi], 8, 40 + i * 18, c);
    }
    snprintf(buf, sizeof(buf), "%d/2", pg + 1);
    dispTextR(buf, 124, 116, C_DIM);
    return;
  }

  static const uint8_t TABSPR[3] = { SPR_BOOK, SPR_SCROLL, SPR_GATE };
  dispTextC(NOTE_TABS[UI.notePage], 54, 4, C_GOLD_HI);
  snprintf(buf, sizeof(buf), "%d/3", UI.notePage + 1);
  dispTextR(buf, 124, 4, C_DIM);
  dispTri(6, 46, -1, true);
  dispTri(111, 46, 1, true);
  pedestal56(TABSPR[UI.notePage]);

  if (UI.notePage == 0) {
    dispTextC("A 查看详情", 64, 90, C_DIM);
  } else if (UI.notePage == 1) {
    dispTextC("游戏日常", 64, 84, HEXC(0xe8ddb8));
    dispTextC("需联网同步 暂未接入", 64, 98, C_DIM);
    dispTextC("本机可看活动页", 64, 112, HEXC(0x79839a));
  } else {
    dispTextC("开服查询", 64, 86, HEXC(0xe8ddb8));
    dispTextC("功能预留", 64, 100, C_DIM);
    dispTextC("敬请期待", 64, 114, HEXC(0x79839a));
  }
}

/* ================= 设置 ================= */
static void renderSettings() {
  dispClear(C_BG);
  char buf[40];
  if (UI.setPage < 0) {
    dispTextC("系统设置", 64, 4, HEXC(0xd8c68a));
    for (int i = 0; i < SET_N; i++) {
      int y = 24 + i * 18;
      bool on = (i == UI.setIdx);
      if (on) dispRect(8, y - 2, 112, 15, HEXC(0x3a3f54));
      dispText(SET_MENU[i].label, 12, y, on ? C_GOLD_HI : C_TEXT);
      dispTextClip(SET_MENU[i].desc, 56, y + 1, 68, C_DIM, 0);
    }
    return;
  }
  const char* id = SET_MENU[UI.setIdx].id;
  if (strcmp(id, "ota") == 0) {
    dispTextC("系统升级", 64, 5, HEXC(0xd8c68a));
    snprintf(buf, sizeof(buf), "当前版本 v%s", APP_VERSION);
    dispText(buf, 10, 26, C_TEXT);
    if (UI.otaPhase == 0) dispText("A 检查更新", 10, 50, C_GOLD_HI);
    else if (UI.otaPhase == 1) { dispText("已是最新版本", 10, 50, HEXC(0xb8e88a)); dispText("A 再看一次", 10, 66, C_DIM); }
    else if (UI.otaPhase == 2) dispText("发现新版本", 10, 50, C_GOLD);
    else if (UI.otaPhase == 3) {
      dispText("写入中…", 10, 50, C_GOLD);
      dispRect(10, 70, 108, 8, C_BTN);
      dispRect(10, 70, 108 * UI.otaPct / 100, 8, HEXC(0xe8a04a));
    }
    dispText("B 返回", 10, 112, C_DIM);
  } else if (strcmp(id, "wifi") == 0) {
    dispTextC("WiFi连接", 64, 5, HEXC(0xd8c68a));
    dispText("本机无网络模块凭据", 10, 32, C_TEXT);
    dispText("请在固件里填写 WiFi", 10, 48, C_DIM);
    dispText("B 返回", 10, 112, C_DIM);
  } else if (strcmp(id, "ap") == 0) {
    dispTextC("热点模式", 64, 5, HEXC(0xd8c68a));
    dispText(UI.apOn ? "热点已开启" : "热点已关闭", 10, 36, UI.apOn ? HEXC(0xb8e88a) : C_DIM);
    dispText("A 切换", 10, 60, C_GOLD_HI);
    dispText("B 返回", 10, 112, C_DIM);
  } else if (strcmp(id, "sound") == 0) {
    dispTextC("音效", 64, 5, HEXC(0xd8c68a));
    dispText(UI.muted ? "音效已关闭" : "音效已开启", 10, 36, C_TEXT);
    dispText("A 切换", 10, 60, C_GOLD_HI);
    dispText("B 返回", 10, 112, C_DIM);
  } else {
    dispTextC("关于本机", 64, 5, HEXC(0xd8c68a));
    dispText("剑侠情缘·单机版三", 10, 24, C_GOLD_HI);
    snprintf(buf, sizeof(buf), "固件 v%s", APP_VERSION);
    dispText(buf, 10, 42, C_TEXT);
    dispText("ESP32-C3 + ST7735", 10, 58, C_DIM);
    snprintf(buf, sizeof(buf), "彩屏 %d 档 %s", dispGetMode() + 1, dispModeName(dispGetMode()));
    dispText(buf, 10, 74, C_DIM);
    dispText("长按▲8秒 校色", 10, 90, HEXC(0x79839a));
    dispText("B 返回", 10, 112, C_DIM);
  }
}

/* ================= 游戏厅 ================= */
static void gmReset(uint8_t id) {
  GameSt& s = UI.gm;
  memset(&s, 0, sizeof(s));
  s.id = id;
  s.popT = 0;
}

static void gameStart(uint8_t idx) {
  const GameDef& g = GAMES[idx];
  uint8_t cd = 0xFF;
  if (strcmp(g.id, "dice") == 0) cd = CD_G_DICE;
  else if (strcmp(g.id, "whack") == 0) cd = CD_G_WHACK;
  else if (strcmp(g.id, "fish") == 0) cd = CD_G_FISH;
  else if (strcmp(g.id, "memory") == 0) cd = CD_G_MEMORY;
  else if (strcmp(g.id, "race") == 0) cd = CD_G_RACE;
  if (cd != 0xFF && cooling(cd)) { toastf("%s还在冷却，歇口气", g.name); return; }

  if (idx == 0) {
    if (G.coins < 50) { toast("赌坊最低押 50 金币"); return; }
    gmReset(0);
    UI.gm.phase = 0;      // side
    UI.gm.side = 1;
    UI.gm.betIdx = 1;
    UI.gm.dice[0] = UI.gm.dice[1] = UI.gm.dice[2] = 1;
  } else if (idx == 1) {
    gmReset(1);
    UI.gm.phase = 0;      // play
    UI.gm.cur = 2; UI.gm.mole = -1; UI.gm.next = 13;
  } else if (idx == 2) {
    gmReset(2);
    UI.gm.phase = 0;      // run
    UI.gm.pos = 0; UI.gm.dir = 1; UI.gm.cast = 3;
    UI.gm.zoneL = 6 + random(0, 9); UI.gm.zoneW = 5;
  } else if (idx == 3) {
    gmReset(3);
    UI.gm.phase = 0;
    static const uint8_t F[6] = { 0, 1, 2, 0, 1, 2 };
    for (int i = 0; i < 6; i++) UI.gm.faces[i] = F[i];
    for (int i = 5; i > 0; i--) { int j = random(0, i + 1); uint8_t t = UI.gm.faces[i]; UI.gm.faces[i] = UI.gm.faces[j]; UI.gm.faces[j] = t; }
    UI.gm.first = -1;
  } else {
    if (G.coins < RACE_BETS[0]) { toast("赛马最低押 50 金"); return; }
    gmReset(4);
    UI.gm.phase = 0;      // pick
    UI.gm.winner = 0xFF;
  }
}

/* 一帧游戏动画 */
static void gameTickAnim() {
  GameSt& s = UI.gm;
  if (s.popT > 0) s.popT--;
  if (s.id == 0) {                        // 骰子
    if (s.phase == 1) {                   // roll
      s.t++;
      if (s.t % 4 == 0) for (int i = 0; i < 3; i++) s.dice[i] = 1 + random(0, 6);
      if (s.t > 30) {
        s.sum = s.dice[0] + s.dice[1] + s.dice[2];
        s.baozi = (s.dice[0] == s.dice[1] && s.dice[1] == s.dice[2]);
        s.win = !s.baozi && ((s.sum >= 11) == (s.side == 1));
        uint16_t bet = BETS[s.betIdx];
        float pen = (G.flags & 0x02) ? 0.6f : (G.flags & 0x01) ? 0.8f : 1.0f;
        s.pay = (uint16_t)(bet * pen * (hasEquip(G_T_JADE) ? 1.2f : 1.0f));
        if (s.pay < 1) s.pay = 1;
        if (s.win) {
          G.coins = (G.coins + s.pay > 60000) ? 60000 : G.coins + s.pay;
          G.exp += (uint16_t)(2 * pen + 0.5f);
          checkTitle();
        } else {
          G.coins = (G.coins > bet) ? G.coins - bet : 0;
        }
        s.phase = 2;                      // result
        gameSave();
        if (G.coins < 50) { toast("输光了…赌坊老板把你请了出去"); UI.gm.id = GAME_HUB; }
      }
    }
  } else if (s.id == 1) {                 // 打地鼠
    if (s.phase == 0) {
      s.t++;
      if (s.mole < 0 || s.t > s.next) {
        s.mole = random(0, 6);
        s.next = s.t + 13 + random(0, 15);
      }
    }
  } else if (s.id == 2) {                 // 钓鱼
    if (s.phase == 0) {
      s.pos += s.dir;
      if (s.pos >= 20) { s.pos = 20; s.dir = 0xFF; }
      if (s.pos == 0 && s.dir == 0xFF) s.dir = 1;
    }
  } else if (s.id == 3) {                 // 翻牌
    if (s.phase == 0 && s.wait > 0) {
      s.wait--;
      if (s.wait == 0) { for (int i = 0; i < 6; i++) s.open[i] = false; s.first = -1; }
    }
  } else if (s.id == 4) {                 // 赛马
    if (s.phase == 2) {
      s.t++;
      int fin = -1;
      for (int i = 0; i < 4; i++) {
        if (s.hs[i].pos >= 96) continue;
        if (s.hs[i].burst > 0) s.hs[i].burst--;
        else if (random(0, 1000) < 35) s.hs[i].burst = 9 + random(0, 12);
        float boost = s.hs[i].burst > 0 ? 1.85f : 1.0f;
        s.hs[i].pos += (HORSES[i].spd / 100.0f * 0.34f + random(0, 100) / 100.0f * 0.30f) * boost;
        if (s.hs[i].pos >= 96) { s.hs[i].pos = 96; if (fin < 0) fin = i; }
      }
      if (fin >= 0) {
        s.winner = fin;
        s.phase = 3;
        uint16_t bet = RACE_BETS[s.betIdx];
        float pen = (G.flags & 0x02) ? 0.6f : (G.flags & 0x01) ? 0.8f : 1.0f;
        if (fin == s.sel) {
          s.win = true;
          s.pay = (uint16_t)(bet * HORSES[fin].odds * pen * (hasEquip(G_T_JADE) ? 1.2f : 1.0f));
          if (s.pay < 1) s.pay = 1;
          G.coins = (G.coins + s.pay > 60000) ? 60000 : G.coins + s.pay;
          G.exp += 2;
          checkTitle();
        } else {
          s.win = false; s.pay = bet;
          G.coins = (G.coins > bet) ? G.coins - bet : 0;
        }
        setCool(CD_G_RACE, 45000);
        gameSave();
      }
    }
  }
}

static void drawHorse(int x, int y, const HorseDef& h, bool moving) {
  dispRect(x + 1, y + 2, 6, 3, h.body);
  dispRect(x + 6, y, 3, 3, h.body);
  dispPixel(x + 8, y + 1, HEXC(0x10131c));
  dispRect(x + 5, y, 2, 1, h.dark);
  dispRect(x, y + 1, 1, 3, h.dark);
  int ph = moving ? ((frameCounter() >> 2) & 1) : 0;
  dispRect(x + 2, y + 5, 1, 1 + ph, h.dark);
  dispRect(x + 5, y + 5, 1, 2 - ph, h.dark);
}

static void renderGameHub() {
  uint8_t sel = UI.gm.sel;
  if (sel >= GAME_N) sel = GAME_N - 1;
  const GameDef& g = GAMES[sel];
  uint8_t cd = 0xFF;
  if (strcmp(g.id, "dice") == 0) cd = CD_G_DICE;
  else if (strcmp(g.id, "whack") == 0) cd = CD_G_WHACK;
  else if (strcmp(g.id, "fish") == 0) cd = CD_G_FISH;
  else if (strcmp(g.id, "memory") == 0) cd = CD_G_MEMORY;
  else if (strcmp(g.id, "race") == 0) cd = CD_G_RACE;
  bool cool = (cd != 0xFF) && cooling(cd);

  dispTextC(g.name, 54, 4, cool ? C_DIM : C_GOLD_HI);
  char buf[24];
  snprintf(buf, sizeof(buf), "%d/%d", sel + 1, GAME_N);
  dispTextR(buf, 124, 4, C_DIM);
  dispTri(6, 46, -1, true);
  dispTri(111, 46, 1, true);
  pedestal56(g.spr);
  dispTextClip(g.desc, 6, 81, 116, HEXC(0xe8ddb8), (int)(millis() / 16 % 260));
  if (cool) {
    int left = G.cool[cd] - (int32_t)gameNowSec();
    if (left < 1) left = 1;
    snprintf(buf, sizeof(buf), "冷却 %d 秒", left);
    dispTextC(buf, 64, 94, C_WARN);
  } else {
    snprintf(buf, sizeof(buf), "金币 %d", G.coins);
    dispTextC(buf, 64, 94, C_GOLD);
  }
  dispRect(4, 104, 120, 1, C_LINE);
  dispTextC("A 进入  B 返回", 64, 108, C_DIM);
}

static void renderDice() {
  GameSt& s = UI.gm;
  dispTextC("黑市赌坊·押大小", 64, 4, HEXC(0xd8c68a));
  char buf[32];
  snprintf(buf, sizeof(buf), "金币 %d", G.coins);
  dispText(buf, 8, 18, C_GOLD);
  bool rolling = (s.phase == 1);
  for (int i = 0; i < 3; i++) {
    int dx = 22 + i * 30, dy = 34;
    dispRect(dx, dy, 22, 22, HEXC(0x2e3345));
    bool flip = rolling && (frameCounter() + i * 9) % 12 < 6;
    dispRect(dx + 1, dy + 1, 20, 20, flip ? HEXC(0x4a5270) : HEXC(0xe8ddc8));
    snprintf(buf, sizeof(buf), "%c", flip ? '?' : ('0' + s.dice[i]));
    dispTextC(buf, dx + 11, dy + 3, HEXC(0x262a38));
  }
  if (s.phase >= 1) { snprintf(buf, sizeof(buf), "点数 %d", s.sum); dispTextC(buf, 64, 58, C_TEXT); }

  int y1 = 74, y2 = 86;
  if (s.phase == 0) {
    if (s.side == 1) dispRect(10, y1 - 2, 108, 12, HEXC(0x3a3f54));
    if (s.side == 0) dispRect(10, y2 - 2, 108, 12, HEXC(0x3a3f54));
  }
  dispText("押大 11点以上", 14, y1, (s.phase == 0 && s.side == 1) ? C_GOLD_HI : C_DIM);
  dispText("押小 10点以下", 14, y2, (s.phase == 0 && s.side == 0) ? C_GOLD_HI : C_DIM);

  if (s.phase == 2) {
    if (s.baozi) { snprintf(buf, sizeof(buf), "豹子通吃 -%d", s.pay); dispTextC(buf, 64, 104, C_WARN); }
    else if (s.win) { snprintf(buf, sizeof(buf), "押中 +%d 修为+2", s.pay); dispTextC(buf, 64, 104, HEXC(0xb8e88a)); }
    else { snprintf(buf, sizeof(buf), "没押中 -%d", s.pay); dispTextC(buf, 64, 104, C_WARN); }
    dispTextC("A 再来  B 返回", 64, 117, C_DIM);
  } else if (s.phase == 0) {
    dispTextC("A 选边  B 返回", 64, 104, C_DIM);
    dispTextC("豹子三同庄家通吃", 64, 117, HEXC(0x8a8fa5));
  } else if (s.phase == 1) {
    snprintf(buf, sizeof(buf), "注额 %d 金", BETS[s.betIdx]);
    dispTextC(buf, 64, 104, C_GOLD);
    dispTextC("骰盅摇动中…", 64, 117, C_GOLD);
  }
}

static void renderWhack() {
  GameSt& s = UI.gm;
  dispTextC("打地鼠", 64, 4, HEXC(0xd8c68a));
  char buf[36];
  snprintf(buf, sizeof(buf), "敲中%d/10 赚%d金", s.hits, s.pay);
  dispText(buf, 8, 18, C_TEXT);
  for (int i = 0; i < 6; i++) {
    int hx = 22 + (i % 3) * 30, hy = 32 + (i / 3) * 26;
    dispRect(hx, hy + 10, 22, 8, HEXC(0x3a2f22));
    dispRect(hx + 3, hy + 11, 16, 4, HEXC(0x241c12));
    if (i == s.mole && s.phase == 0) {
      dispRect(hx + 5, hy, 12, 12, HEXC(0x8a5a34));
      dispRect(hx + 7, hy + 3, 2, 2, HEXC(0x262a38));
      dispRect(hx + 12, hy + 3, 2, 2, HEXC(0x262a38));
      dispRect(hx + 8, hy + 7, 6, 2, HEXC(0xe8a08a));
    }
    if (i == s.cur && s.phase == 0) dispSelFrame(hx - 2, hy - 2, 26, 24);
  }
  if (s.popT > 0) dispTextC(s.pop, 64, 90, s.pop[0] == '+' ? HEXC(0xb8e88a) : C_WARN);
  else {
    snprintf(buf, sizeof(buf), "敲中%d只 赚%d金", s.hits, s.pay);
    dispTextC(buf, 64, 96, HEXC(0xb8e88a));
    dispTextC("A 敲  B 退出", 64, 112, C_DIM);
  }
}

static void renderFish() {
  GameSt& s = UI.gm;
  dispTextC("池边垂钓", 64, 4, HEXC(0xd8c68a));
  char buf[36];
  snprintf(buf, sizeof(buf), "剩%d杆 收获%d金", s.cast, s.gain);
  dispText(buf, 8, 18, C_TEXT);
  dispRect(10, 30, 108, 26, HEXC(0x1d3a5e));
  for (int i = 0; i < 5; i++) dispRect(16 + i * 22, 34 + ((frameCounter() / 20 + i) % 3) * 6, 10, 1, HEXC(0x2a5a8e));
  int fx = 20 + (int)(frameCounter() * 0.4f) % 80;
  dispRect(fx, 46, 9, 3, HEXC(0x0f2438));
  dispRect(fx + 8, 46, 3, 3, HEXC(0x0f2438));

  dispRect(14, 68, 100, 12, HEXC(0x262a38));
  dispRect(14 + s.zoneL * 5, 68, s.zoneW * 5, 12, HEXC(0x3a7a3a));
  dispRect(14 + s.pos * 5, 68, 3, 12, C_GOLD_HI);

  if (s.popT > 0) dispTextC(s.pop, 64, 88, s.pop[0] == '+' ? HEXC(0xb8e88a) : C_WARN);
  else {
    snprintf(buf, sizeof(buf), "收获%d金", s.gain);
    dispTextC(buf, 64, 88, HEXC(0xb8e88a));
    dispTextC("A 收竿  B 退出", 64, 108, C_DIM);
  }
}

static void renderMemory() {
  GameSt& s = UI.gm;
  dispTextC("翻牌配对", 64, 4, HEXC(0xd8c68a));
  char buf[36];
  snprintf(buf, sizeof(buf), "配对%d/3 赚%d金", s.matched, s.pay);
  dispText(buf, 8, 18, C_TEXT);
  static const uint16_t FC[3] = { HEXC(0xe86a5a), HEXC(0x4a8db8), HEXC(0xe8c860) };
  static const char* FT[3] = { "剑", "书", "钱" };
  for (int i = 0; i < 6; i++) {
    int x = 18 + (i % 3) * 32, y = 30 + (i / 3) * 30;
    if (s.done[i]) {
      dispRect(x, y, 26, 26, C_PANEL);
      dispRect(x, y, 26, 2, C_LINE); dispRect(x, y + 24, 26, 2, C_LINE);
    } else if (s.open[i]) {
      dispRect(x, y, 26, 26, FC[s.faces[i]]);
      dispTextC(FT[s.faces[i]], x + 13, y + 6, HEXC(0xf5ecd0));
    } else {
      dispRect(x, y, 26, 26, HEXC(0x2e3a58));
      dispRect(x + 3, y + 3, 20, 20, HEXC(0x26314a));
      dispRect(x + 10, y + 10, 6, 6, HEXC(0x3a4a70));
    }
    if (i == s.cur && s.phase == 0) dispSelFrame(x - 2, y - 2, 30, 30);
  }
  if (s.popT > 0) dispTextC(s.pop, 64, 92, s.pop[0] == '+' ? HEXC(0xb8e88a) : C_WARN);
  else {
    snprintf(buf, sizeof(buf), "配对%d/3 金+%d", s.matched, s.pay);
    dispTextC(buf, 64, 96, HEXC(0xb8e88a));
    dispTextC("A 翻牌  B 退出", 64, 112, C_DIM);
  }
}

static void renderRace() {
  GameSt& s = UI.gm;
  dispTextC("马场", 64, 4, HEXC(0xd8c68a));
  char buf[32];
  snprintf(buf, sizeof(buf), "金%d", G.coins);
  dispText(buf, 5, 4, C_GOLD);

  if (s.phase == 0) {
    for (int i = 0; i < 4; i++) {
      int y = 26 + i * 21;
      bool on = (i == s.sel);
      if (on) { dispRect(6, y - 3, 116, 19, HEXC(0x26314a)); dispSelFrame(6, y - 3, 116, 19); }
      dispRect(14, y + 2, 8, 8, HORSES[i].body);
      dispRect(14, y + 2, 8, 2, HORSES[i].dark);
      dispText(HORSES[i].name, 28, y, on ? C_GOLD_HI : C_TEXT);
      snprintf(buf, sizeof(buf), "×%d", HORSES[i].odds);
      dispTextR(buf, 116, y, on ? C_GOLD : C_DIM);
    }
    dispTextC("赔率越高越冷门", 64, 112, HEXC(0x5a6078));
    return;
  }
  if (s.phase == 1) {
    const HorseDef& h = HORSES[s.sel];
    uint16_t bet = RACE_BETS[s.betIdx];
    dispRect(30, 30, 10, 10, h.body); dispRect(30, 30, 10, 3, h.dark);
    snprintf(buf, sizeof(buf), "%s ×%d", h.name, h.odds);
    dispTextC(buf, 64, 46, C_GOLD_HI);
    for (int i = 0; i < 4; i++) {
      int x = 12 + i * 26, on = (i == s.betIdx);
      dispRect(x, 66, 24, 16, on ? HEXC(0x3a3f54) : HEXC(0x22263a));
      if (on) dispSelFrame(x, 66, 24, 16);
      snprintf(buf, sizeof(buf), "%d", RACE_BETS[i]);
      dispTextC(buf, x + 12, 68, (RACE_BETS[i] <= G.coins) ? (on ? C_GOLD_HI : C_TEXT) : HEXC(0x5a6078));
    }
    snprintf(buf, sizeof(buf), "押中得%d金", (int)(bet * h.odds * (hasEquip(G_T_JADE) ? 1.2f : 1.0f)));
    dispTextC(buf, 64, 92, C_GOLD);
    dispTextC("A 开跑  B 返回", 64, 112, C_DIM);
    return;
  }

  for (int i = 0; i < 4; i++) {
    int y = 20 + i * 20;
    dispRect(4, y, 120, 17, (i % 2) ? HEXC(0x1a1f30) : HEXC(0x1e2338));
    if (i == s.sel) dispRect(4, y, 120, 1, HEXC(0x8a7440));
    dispRect(112, y, 2, 17, C_TEXT);
    for (int k = 2; k < 17; k += 4) dispRect(112, y + k, 2, 2, C_BG);
    drawHorse(9 + (int)(s.hs[i].pos * 0.95f), y + 4, HORSES[i], s.phase == 2);
    if (s.phase == 3 && i == s.winner) dispText("冠", 116, y + 2, C_GOLD);
  }
  if (s.phase == 2) {
    snprintf(buf, sizeof(buf), "押%s %d金", HORSES[s.sel].name, RACE_BETS[s.betIdx]);
    dispTextC(buf, 64, 104, C_GOLD);
  } else {
    dispRect(0, 100, SCR_W, 28, HEXC(0x0f1420));
    snprintf(buf, sizeof(buf), "%s 夺冠", HORSES[s.winner].name);
    dispTextC(buf, 64, 102, C_GOLD_HI);
    if (s.win) { snprintf(buf, sizeof(buf), "押中 +%d 金", s.pay); dispTextC(buf, 64, 115, HEXC(0xb8e88a)); }
    else { snprintf(buf, sizeof(buf), "没押中 -%d 金", s.pay); dispTextC(buf, 64, 115, C_WARN); }
  }
}

static void renderGame() {
  dispClear(C_BG);
  if (UI.gm.id == GAME_HUB) return renderGameHub();
  if (UI.gm.id == 0) return renderDice();
  if (UI.gm.id == 1) return renderWhack();
  if (UI.gm.id == 2) return renderFish();
  if (UI.gm.id == 3) return renderMemory();
  if (UI.gm.id == 4) return renderRace();
  UI.gm.id = GAME_HUB;
  renderGameHub();
}

/* ================= 渲染入口 ================= */
void uiInit() {
  memset(&UI, 0, sizeof(UI));
  UI.page = PG_BOOT;
  UI.menuIdx = 0;
  UI.itemFrom = LIST_BAG;
  UI.setPage = -1;
  UI.gm.id = GAME_HUB;
  UI.bagCat = 0; UI.shopCat = 0;
  UI.catMode = true;
  UI.apOn = false;
  UI.muted = false;
  UI.otaPhase = 0;
}

void uiRender() {
  switch (UI.page) {
    case PG_BOOT:     renderBoot(); break;
    case PG_MAIN:     renderMain(); break;
    case PG_ACTIVITY: renderActivity(); break;
    case PG_GAME:     renderGame(); break;
    case PG_LIST:     renderList(UI.itemFrom); break;
    case PG_ITEM:     renderItem(); break;
    case PG_HOME:     renderHome(); break;
    case PG_MAIL:     renderMail(); break;
    case PG_STATUS:   renderStatus(); break;
    case PG_SETTINGS: renderSettings(); break;
    default:          renderMain(); break;
  }
}

void uiFrameTick() {
  if (UI.page == PG_GAME) gameTickAnim();
}

/* ================= 交互 ================= */
static void openMenu(uint8_t idx) {
  const char* id = MENU[idx].id;
  if (strcmp(id, "activity") == 0) { UI.page = PG_ACTIVITY; UI.actSel = 0; }
  else if (strcmp(id, "game") == 0) { UI.page = PG_GAME; UI.gm.id = GAME_HUB; UI.gm.sel = 0; }
  else if (strcmp(id, "bag") == 0) { UI.page = PG_LIST; UI.itemFrom = LIST_BAG; UI.bagSel = 0; UI.catMode = true; }
  else if (strcmp(id, "shop") == 0) { UI.page = PG_LIST; UI.itemFrom = LIST_SHOP; UI.shopSel = 0; UI.catMode = true; }
  else if (strcmp(id, "mail") == 0) { UI.page = PG_MAIL; UI.mailSel = 0; UI.mailDetail = false; }
  else { UI.page = PG_STATUS; UI.notePage = 0; UI.noteDetail = false; }
}

void uiMoveSel(int8_t d) {
  switch (UI.page) {
    case PG_MAIN:
      UI.menuIdx = (uint8_t)((UI.menuIdx + d + 5) % 5);
      break;
    case PG_ACTIVITY:
      UI.actSel = (uint8_t)((UI.actSel + d + ACT_N) % ACT_N);
      break;
    case PG_GAME:
      if (UI.gm.id == GAME_HUB) {
        UI.gm.sel = (uint8_t)((UI.gm.sel + d + GAME_N) % GAME_N);
      } else if (UI.gm.id == 0) {                    // 骰子
        if (UI.gm.phase == 0) UI.gm.side = UI.gm.side > 0 ? 0 : 1;
        else if (UI.gm.phase == 1) UI.gm.betIdx = (uint8_t)((UI.gm.betIdx + d + 5) % 5);
      } else if (UI.gm.id == 1) {                    // 打地鼠
        if (UI.gm.phase == 0) UI.gm.cur = (uint8_t)((UI.gm.cur + d + 6) % 6);
      } else if (UI.gm.id == 3) {                    // 翻牌
        if (UI.gm.phase == 0) UI.gm.cur = (uint8_t)((UI.gm.cur + d + 6) % 6);
      } else if (UI.gm.id == 4) {                    // 赛马
        if (UI.gm.phase == 0) UI.gm.sel = (uint8_t)((UI.gm.sel + d + 4) % 4);
        else if (UI.gm.phase == 1) UI.gm.betIdx = (uint8_t)((UI.gm.betIdx + d + 4) % 4);
      }
      break;
    case PG_LIST: {
      uint8_t n = orderCount(UI.itemFrom);
      if (UI.catMode) {
        uint8_t& cat = (UI.itemFrom == LIST_SHOP) ? UI.shopCat : UI.bagCat;
        cat = (uint8_t)((cat + d + CAT_N) % CAT_N);
        selRef(UI.itemFrom) = 0;
      } else if (n > 0) {
        uint8_t& sel = selRef(UI.itemFrom);
        sel = (uint8_t)((sel + d + n) % n);
      }
      break;
    }
    case PG_ITEM: {
      uint8_t n = orderCount(UI.itemFrom);
      if (n > 0) {
        uint8_t& sel = selRef(UI.itemFrom);
        sel = (uint8_t)((sel + d + n) % n);
      }
      break;
    }
    case PG_HOME:
      UI.homeSel = (uint8_t)((UI.homeSel + d + 3) % 3);
      break;
    case PG_MAIL:
      if (!UI.mailDetail && G.mailN > 0)
        UI.mailSel = (uint8_t)((UI.mailSel + d + G.mailN) % G.mailN);
      break;
    case PG_STATUS:
      if (UI.noteDetail) {
        UI.noteDetailPage = (uint8_t)((UI.noteDetailPage + d + 2) % 2);
      } else {
        UI.notePage = (uint8_t)((UI.notePage + d + 3) % 3);
      }
      break;
    case PG_SETTINGS:
      if (UI.setPage < 0) UI.setIdx = (uint8_t)((UI.setIdx + d + SET_N) % SET_N);
      break;
    default: break;
  }
}

void uiConfirmA() {
  switch (UI.page) {
    case PG_BOOT:
      UI.page = PG_MAIN;
      toast("初入江湖，多多关照");
      gameSave();
      break;
    case PG_MAIN:
      openMenu(UI.menuIdx);
      break;
    case PG_ACTIVITY: {
      const ActDef& a = ACTIVITIES[UI.actSel];
      if (strcmp(a.id, "sit") == 0) doSit();
      else if (strcmp(a.id, "train") == 0) doTrain();
      else if (strcmp(a.id, "dig") == 0) doDig();
      else if (strcmp(a.id, "home") == 0) UI.page = PG_HOME;
      else UI.page = PG_HOME;   // 副本：先在副本页选本（简化为直接战 war）
      if (strcmp(a.id, "dungeon") == 0) { UI.page = PG_MAIN; startDungeon(0); }
      break;
    }
    case PG_GAME: {
      GameSt& s = UI.gm;
      if (s.id == GAME_HUB) { gameStart(s.sel); break; }
      if (s.id == 0) {
        if (s.phase == 0) { s.phase = 1; s.t = 0; }
        else if (s.phase == 1) {
          uint16_t bet = BETS[s.betIdx];
          if (G.coins < bet) { toast("金币不够押这一注"); break; }
          s.phase = 1;
        } else if (s.phase == 2) { s.phase = 0; s.sum = 0; s.t = 0; }
      } else if (s.id == 1) {
        if (s.phase == 0) {
          if (s.cur == s.mole) {
            s.hits++;
            int gain = 20 + random(0, 18);
            s.pay += gain;
            snprintf(s.pop, sizeof(s.pop), "+%d 金", gain);
            s.popT = 45;
            s.mole = -1;
            if (s.hits >= 10) {
              G.coins = (G.coins + s.pay > 60000) ? 60000 : G.coins + s.pay;
              s.phase = 1;
              setCool(CD_G_WHACK, 60000);
              gameSave();
            }
          } else {
            snprintf(s.pop, sizeof(s.pop), "打空了");
            s.popT = 30;
          }
        }
      } else if (s.id == 2) {
        if (s.phase == 0) {
          bool inZone = (s.pos >= s.zoneL && s.pos <= s.zoneL + s.zoneW);
          int gain = inZone ? (18 + random(0, 16)) : (4 + random(0, 6));
          s.gain += gain;
          s.good += inZone ? 1 : 0;
          snprintf(s.pop, sizeof(s.pop), "+%d 金", gain);
          s.popT = 45;
          s.zoneL = 2 + random(0, 15);
          if (--s.cast == 0) {
            G.coins = (G.coins + s.gain > 60000) ? 60000 : G.coins + s.gain;
            s.phase = 1;
            setCool(CD_G_FISH, 45000);
            gameSave();
          }
        }
      } else if (s.id == 3) {
        if (s.phase == 0 && !s.done[s.cur] && !s.open[s.cur] && s.wait == 0) {
          s.open[s.cur] = true;
          if (s.first < 0) s.first = (int8_t)s.cur;
          else {
            int a = s.first, b = s.cur;
            if (s.faces[a] == s.faces[b]) {
              s.done[a] = s.done[b] = true;
              s.matched++;
              s.pay += 40;
              snprintf(s.pop, sizeof(s.pop), "+40 金");
              s.popT = 40;
              s.first = -1;
              if (s.matched >= 3) {
                G.coins = (G.coins + s.pay + 60 > 60000) ? 60000 : G.coins + s.pay + 60;
                s.phase = 1;
                setCool(CD_G_MEMORY, 90000);
                gameSave();
              }
            } else {
              s.wait = 22;
            }
          }
        }
      } else if (s.id == 4) {
        if (s.phase == 0) s.phase = 1;
        else if (s.phase == 1) {
          if (G.coins < RACE_BETS[s.betIdx]) { toast("金币不够这注"); break; }
          s.phase = 2;
          for (int i = 0; i < 4; i++) { s.hs[i].pos = 0; s.hs[i].burst = 0; }
        } else if (s.phase == 3) { s.phase = 0; }
      }
      break;
    }
    case PG_LIST: {
      if (UI.catMode) { UI.catMode = false; break; }
      if (orderCount(UI.itemFrom) > 0) UI.page = PG_ITEM;
      break;
    }
    case PG_ITEM: {
      uint8_t n = orderCount(UI.itemFrom);
      if (n == 0) break;
      uint8_t gid = orderAt(UI.itemFrom, selRef(UI.itemFrom));
      const GoodsDef& g = GOODS[gid];
      if (UI.itemFrom == LIST_SHOP) buyGoods(gid);
      else if (g.type == GT_FOOD) eatGoods(gid);
      else if (g.type == GT_MED) useMed();
      else if (g.type == GT_USE) usePage(gid);
      else if (g.type == GT_EQUIP) equipToggle(gid);
      else if (g.type == GT_DECO) toast("已在庭院布置妥当");
      else if (g.sell) sellGoods(gid);
      break;
    }
    case PG_HOME:
      if (UI.homeSel == 0) doHomeUpgrade();
      else if (UI.homeSel == 1) doClean();
      else doNanny();
      break;
    case PG_MAIL:
      if (!UI.mailDetail && G.mailN > 0) UI.mailDetail = true;
      break;
    case PG_STATUS:
      if (UI.noteDetail) UI.noteDetailPage = (uint8_t)((UI.noteDetailPage + 1) % 2);
      else if (UI.notePage == 0) { UI.noteDetail = true; UI.noteDetailPage = 0; }
      else if (UI.notePage == 1) toast("日常需联网同步");
      else toast("开服查询预留");
      break;
    case PG_SETTINGS:
      if (UI.setPage < 0) {
        UI.setPage = (int8_t)UI.setIdx;
      } else {
        const char* id = SET_MENU[UI.setIdx].id;
        if (strcmp(id, "ota") == 0) {
          UI.otaPhase = (UI.otaPhase == 0) ? 1 : 0;
          if (UI.otaPhase == 1) toast("已是最新版本");
        } else if (strcmp(id, "ap") == 0) {
          UI.apOn = !UI.apOn;
          toast(UI.apOn ? "热点已开启" : "热点已关闭");
        } else if (strcmp(id, "sound") == 0) {
          UI.muted = !UI.muted;
          toast(UI.muted ? "音效已关闭" : "音效已开启");
        }
      }
      break;
    default: break;
  }
}

bool uiBack() {
  switch (UI.page) {
    case PG_BOOT: return true;
    case PG_MAIN: UI.page = PG_STATUS; UI.notePage = 0; UI.noteDetail = false; return true;
    case PG_ITEM: UI.page = PG_LIST; return true;
    case PG_LIST:
      if (!UI.catMode) { UI.page = PG_MAIN; return true; }
      UI.page = PG_MAIN; return true;
    case PG_GAME:
      if (UI.gm.id != GAME_HUB) { UI.gm.id = GAME_HUB; return true; }
      UI.page = PG_MAIN; return true;
    case PG_MAIL:
      if (UI.mailDetail) { UI.mailDetail = false; return true; }
      UI.page = PG_MAIN; return true;
    case PG_STATUS:
      if (UI.noteDetail) { UI.noteDetail = false; return true; }
      UI.page = PG_MAIN; return true;
    case PG_SETTINGS:
      if (UI.setPage >= 0) { UI.setPage = -1; return true; }
      UI.page = UI.settingsFrom; return true;
    default:
      UI.page = PG_MAIN; return true;
  }
}

void uiLongB() {
  if (UI.page == PG_SETTINGS) return;
  UI.settingsFrom = UI.page;
  UI.page = PG_SETTINGS;
  UI.setPage = -1;
  UI.setIdx = 0;
  toast("系统设置");
}

void uiTick() {
  applyTime(1.0f);
  checkDailyLogin();
}
