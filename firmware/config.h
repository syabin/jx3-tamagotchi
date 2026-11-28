#pragma once
#include <Arduino.h>
#include "sprites.h"

/* ==========================================================================
   剑侠情缘·单机版三 —— ESP32-C3 + ST7735 128×128
   固件与网页版 0.2.11 一比一：同一套 16×16 精灵、同一套数据表、同一套页面。
   ========================================================================== */

#define APP_VERSION   "0.2.11"
#define SCR_W         128
#define SCR_H         128

/* 网页里的 "#rrggbb" 直接搬过来：HEXC(0x5a86a8) */
#define HEXC(v) ((uint16_t)(((((v) >> 16) & 0xF8) << 8) | ((((v) >> 8) & 0xFC) << 3) | (((v) & 0xF8) >> 3)))

/* ---------------- 引脚（沿用已调通的接法） ----------------
   VCC→3V3  GND→GND  SCL→GPIO4  SDA→GPIO6  CS→GPIO7
   DC→GPIO10  RES→GPIO3  BLK→GPIO5
   K1(▲)→GPIO0  K2(▼)→GPIO1  K3(A)→GPIO20  K4(B)→GPIO21
   GPIO2/8/9 是启动配置脚，不占用。 */
#define PIN_SCL   4
#define PIN_SDA   6
#define PIN_CS    7
#define PIN_DC    10
#define PIN_RES   3
#define PIN_BLK   5
#define PIN_K1    0     // ▲
#define PIN_K2    1     // ▼
#define PIN_K3    20    // A
#define PIN_K4    21    // B
#define PIN_BUZZER  -1
#define ENABLE_BUZZER 0

#define SPI_HZ            27000000
#define DISP_MODE_DEFAULT 0
#define DISP_MODE_KEY     "dmod2"
#define DISP_OFF_X        2
#define DISP_OFF_Y        1
#define BLK_DUTY          200
#define BLK_ACTIVE_LOW    0

#define TICK_MS           1000
#define FRAME_MS          33
#define TOAST_MS          2800
#define HINT_MS           2000
#define FX_EAT_MS         2400
#define FX_MED_MS         2600
#define FX_USE_MS         2200
#define SAVE_INTERVAL_MS  15000
#define KEY_DEBOUNCE_MS   40
#define KEY_RELEASE_LOCK_MS 300
#define KEY_REPEAT_WAIT   480
#define KEY_REPEAT_RATE   110

#define NVS_NAMESPACE     "jx3tama"
#define NVS_KEY           "save"
#define NVS_TIME_KEY      "clock"

/* ---------------- 通用色（含 1.44" TN 屏补偿） ----------------
   TN 蓝通道透过率偏高，背景去蓝改暖黑、文字用暖白、金色提饱和；
   场景部分仍按网页原色 HEXC()，保证稻香村的青绿不被压掉。 */
static const uint16_t C_BG      = 0x1081;   // #121110 暖黑
static const uint16_t C_BAR     = 0x18C2;   // #1c1a18 面板
static const uint16_t C_LINE    = 0x3164;   // #332e28 深边框
static const uint16_t C_GOLD    = 0xDDEE;   // #e3c078
static const uint16_t C_GOLD_HI = 0xF758;   // #f8ecc8
static const uint16_t C_TEXT    = 0xDEFA;   // #e6e2d8
static const uint16_t C_DIM     = 0x8C50;   // #8f8d88
static const uint16_t C_WARN    = 0xD2A8;   // #d8554a
static const uint16_t C_BTN     = 0x2103;   // #26221e
static const uint16_t C_PANEL   = 0x18C2;
static const uint16_t C_PANEL2  = 0x2103;
static const uint16_t C_WHITE   = 0xEF7B;   // #f5f0e4
static const uint16_t C_SKIN    = 0xEE33;   // #f0c8a0
static const uint16_t C_HAIR    = 0x18C6;
static const uint16_t C_PURPLE  = HEXC(0xc9a8e0);
static const uint16_t C_GREEN   = HEXC(0x7ec8a0);
static const uint16_t C_BLUE    = HEXC(0x7ec8f0);
static const uint16_t C_VIOLET  = HEXC(0xc89af0);
static const uint16_t C_ORANGE  = HEXC(0xff9a3c);
static const uint16_t C_RED     = HEXC(0xd8554a);
static const uint16_t C_DARK    = 0x3164;

/* ---------------- 页面 ---------------- */
enum PageId : uint8_t {
  PG_BOOT, PG_MAIN, PG_ACTIVITY, PG_GAME, PG_LIST, PG_ITEM,
  PG_HOME, PG_MAIL, PG_STATUS, PG_SETTINGS
};

/* ---------------- 称号 / 声望 / 家园 ---------------- */
struct TitleDef { uint16_t exp; const char* name; };
static const TitleDef TITLES[5] = {
  {   0, "初闯江湖" },
  {  50, "江湖新秀" },
  { 150, "一方名侠" },
  { 350, "武林名宿" },
  { 700, "武林至尊" },
};
struct RepDef { uint16_t exp; const char* name; uint16_t color; };
static const RepDef REPUTATIONS[5] = {
  {   0, "中立", HEXC(0x9aa3b8) },
  {  50, "友好", HEXC(0x7ec8a0) },
  { 150, "尊敬", HEXC(0x7ec8f0) },
  { 350, "崇敬", HEXC(0xc89af0) },
  { 700, "崇拜", HEXC(0xf0c860) },
};
static const char* const HOME_NAMES[5] = { "荒地", "茅舍", "宅院", "别院", "山庄" };

/* ---------------- 物品 ---------------- */
enum GoodsType : uint8_t { GT_FOOD, GT_MED, GT_USE, GT_MAT, GT_COLL, GT_EQUIP, GT_XJ, GT_DECO };
enum : uint8_t { SL_NONE, SL_WEAPON, SL_BODY, SL_TREA };

struct GoodsDef {
  const char* id;
  const char* name;
  uint8_t type;
  uint8_t gain;      // 食物：体力
  uint8_t mood;      // 食物：心情
  uint8_t exp;       // 参悟：修为
  uint16_t price;    // 商城价（0 = 不卖）
  uint16_t sell;     // 卖价（0 = 不能卖）
  uint8_t cd;        // 食用冷却秒
  uint8_t slot;      // 装备槽
  uint8_t tier;      // 1 凡品 2 良品 3 珍品
  uint8_t spr;       // 精灵 id
  const char* eff;
  const char* desc;
};

enum : uint8_t {
  G_BAOZI, G_FISH, G_FEAST, G_PILL, G_PAGE, G_IRON, G_TASSEL, G_STEEL, G_SHARD,
  G_XJ_ZT, G_W_SWORD, G_W_PICK, G_A_ROBE, G_A_CHARM, G_T_JADE, G_T_GOURD,
  G_D_PLANT, G_D_SCREEN, G_D_LAMP, GOODS_COUNT
};

static const GoodsDef GOODS[GOODS_COUNT] = {
  { "baozi",  "包子",     GT_FOOD, 15, 5, 0,  80,   0, 12, SL_NONE, 0, SPR_BAOZI,  "",                      "刚出笼的白面馒头，管饱。" },
  { "fish",   "蒸鱼",     GT_FOOD, 30, 5, 0, 150,   0, 25, SL_NONE, 0, SPR_FISH,   "",                      "江上现捞，姜丝去腥。" },
  { "feast",  "烧尾宴",   GT_FOOD, 50, 5, 0, 300,   0, 60, SL_NONE, 0, SPR_FEAST,  "",                      "长安大酒楼的头牌席面。" },
  { "pill",   "玉露丸",   GT_MED,   0, 0, 0, 200,   0,  0, SL_NONE, 0, SPR_PILL,   "",                      "九花玉露，解毒疗伤。" },
  { "page",   "剑意残页", GT_USE,   0, 0, 3,   0,   0,  0, SL_NONE, 0, SPR_PAGE,   "",                      "残页上剑意未尽，可参悟。" },
  { "iron",   "铁矿石",   GT_MAT,   0, 0, 0,   0,  60,  0, SL_NONE, 0, SPR_IRON,   "",                      "寻常矿石，铁匠收。" },
  { "tassel", "剑穗",     GT_COLL,  0, 0, 0,   0, 150,  0, SL_NONE, 0, SPR_TASSEL, "",                      "旧剑上的穗子，有人好这口。" },
  { "steel",  "精铁",     GT_MAT,   0, 0, 0,   0, 200,  0, SL_NONE, 0, SPR_STEEL,  "",                      "锻造良材，重而沉手。" },
  { "shard",  "神兵残片", GT_COLL,  0, 0, 0,   0, 500,  0, SL_NONE, 0, SPR_SHARD,  "",                      "紫品残片，藏家千金求。" },
  { "xj_zt",  "铸天玄晶", GT_XJ,    0, 0, 0,   0, 2500, 0, SL_NONE, 0, SPR_XJ_ZT,  "",                      "本赛季团本至宝，橙光灼灼，藏家梦寐。" },
  { "w_sword","精铁剑",   GT_EQUIP, 0, 0, 0, 300,   0,  0, SL_WEAPON, 1, SPR_W_SWORD, "武器 · 木桩修为+50%", "百炼成钢，练武事半功倍。" },
  { "w_pick", "镔铁镐",   GT_EQUIP, 0, 0, 0, 260,   0,  0, SL_WEAPON, 1, SPR_W_PICK,  "武器 · 挖宝金币+50%", "掘地如泥，寻宝事半功倍。" },
  { "a_robe", "云锦道袍", GT_EQUIP, 0, 0, 0, 280,   0,  0, SL_BODY,   2, SPR_A_ROBE,  "护身 · 心情流失-20%", "绣云御寒，心平气和。" },
  { "a_charm","平安符",   GT_EQUIP, 0, 0, 0, 200,   0,  0, SL_BODY,   2, SPR_A_CHARM, "护身 · 副本胜率+15%", "出入平安，逢凶化吉。" },
  { "t_jade", "翡翠扳指", GT_EQUIP, 0, 0, 0, 350,   0,  0, SL_TREA,   2, SPR_T_JADE,  "饰品 · 赌坊赢利+20%", "运筹帷幄，手气更旺。" },
  { "t_gourd","酒葫芦",   GT_EQUIP, 0, 0, 0, 150,   0,  0, SL_TREA,   1, SPR_T_GOURD, "饰品 · 打坐修为+2",   "酒后吐真言，参悟更透。" },
  { "d_plant","幽兰盆栽", GT_DECO,  0, 0, 0, 120,   0,  0, SL_NONE, 0, SPR_D_PLANT, "装饰 · 心情流失-5%",  "清心雅趣，暗香浮动。" },
  { "d_screen","云纹屏风",GT_DECO,  0, 0, 0, 260,   0,  0, SL_NONE, 0, SPR_D_SCREEN,"装饰 · 心情流失-8%",  "遮风挡煞，庭院生辉。" },
  { "d_lamp", "长明灯",   GT_DECO,  0, 0, 0, 200,   0,  0, SL_NONE, 0, SPR_D_LAMP,  "装饰 · 心情-6% 体力-6%", "彻夜长明，宅院安稳。" },
};

static const uint8_t BAG_ORDER[GOODS_COUNT] = {
  G_BAOZI, G_FISH, G_FEAST, G_PILL, G_PAGE, G_IRON, G_TASSEL, G_STEEL, G_SHARD, G_XJ_ZT,
  G_W_SWORD, G_W_PICK, G_A_ROBE, G_A_CHARM, G_T_JADE, G_T_GOURD, G_D_PLANT, G_D_SCREEN, G_D_LAMP
};
#define SHOP_N 13
static const uint8_t SHOP_ORDER[SHOP_N] = {
  G_BAOZI, G_FISH, G_FEAST, G_PILL, G_W_SWORD, G_W_PICK, G_A_ROBE,
  G_A_CHARM, G_T_JADE, G_T_GOURD, G_D_PLANT, G_D_SCREEN, G_D_LAMP
};

/* 分类：全部 / 食品 / 丹药 / 装备 / 家具 / 材料 */
#define CAT_N 6
struct CatDef { const char* name; uint8_t type; };
static const CatDef CATS[CAT_N] = {
  { "全部", 0xFF }, { "食品", GT_FOOD }, { "丹药", GT_MED },
  { "装备", GT_EQUIP }, { "家具", GT_DECO }, { "材料", GT_MAT },
};
static inline uint8_t catOfType(uint8_t t) {
  if (t == GT_FOOD) return GT_FOOD;
  if (t == GT_MED || t == GT_USE) return GT_MED;
  if (t == GT_EQUIP) return GT_EQUIP;
  if (t == GT_DECO) return GT_DECO;
  return GT_MAT;
}

/* 装备品级名 */
static const char* const TIER_NAME[4] = { "", "凡品", "良品", "珍品" };
static const uint16_t TIER_COLOR[4] = { 0, HEXC(0x9aa3b8), HEXC(0x7ec8f0), HEXC(0xc89af0) };
static const char* const SLOT_NAME[4] = { "", "武器", "护身", "饰品" };

/* ---------------- 主界面按钮（前 5 个参与选择，状态走 B 键） ---------------- */
#define MENU_N 6
struct MenuDef { const char* id; const char* label; const char* desc; uint8_t spr; };
static const MenuDef MENU[MENU_N] = {
  { "activity", "活动", "打坐·木桩·挖宝·家园·副本", SPR_ACTIVITY },
  { "game",     "游戏", "游戏厅 · 左右选小游戏",     SPR_GAME },
  { "bag",      "背包", "查看行囊物品",              SPR_BAG },
  { "shop",     "商城", "购买食材和药品",            SPR_SHOP },
  { "mail",     "邮箱", "查看奇遇来信",              SPR_MAIL },
  { "status",   "状态", "侠士手札",                  SPR_STATUS },
};

/* ---------------- 活动（5 项大图标） ---------------- */
#define ACT_N 5
struct ActDef { const char* id; const char* name; const char* desc; uint8_t spr; };
static const ActDef ACTIVITIES[ACT_N] = {
  { "sit",     "打坐", "吐纳调息 · 心情+20 修为+3",  SPR_SIT },
  { "train",   "木桩", "勤练剑法 · 修为+12 耗体力",  SPR_TRAIN },
  { "dig",     "挖宝", "铁锹寻宝 · 金币入袋",        SPR_DIG },
  { "home",    "家园", "升级宅子 · 打扫 · 保姆",     SPR_HOME },
  { "dungeon", "副本", "大战·战场·秘境·团本",        SPR_DUNGEON },
};

/* ---------------- 副本 ---------------- */
#define DNG_N 4
struct DngDef {
  const char* key; const char* name;
  uint16_t coinMin, coinMax, expMin, expMax;
};
static const DngDef DUNGEONS[DNG_N] = {
  { "war",    "大战",     250, 250, 18, 24 },
  { "battle", "阵营战场", 280, 280, 15, 20 },
  { "card",   "秘境",     260, 260, 16, 22 },
  { "raid",   "团本",     400, 450, 25, 33 },
};

/* 副本额外掉落：gid（0xFF = 金币×100）+ 千分比概率，与网页 extra 表一致 */
#define DROP_COIN 0xFF
struct DropDef { uint8_t gid; uint16_t permille; };
static const DropDef DROPS[DNG_N][4] = {
  { { G_IRON, 400 }, { G_BAOZI, 300 }, { G_PAGE, 100 }, { 0xFF, 0 } },
  { { DROP_COIN, 250 }, { G_PILL, 80 }, { 0xFF, 0 }, { 0xFF, 0 } },
  { { G_FISH, 200 }, { G_PILL, 100 }, { G_TASSEL, 50 }, { 0xFF, 0 } },
  { { G_STEEL, 350 }, { G_PILL, 150 }, { G_TASSEL, 120 }, { G_SHARD, 20 } },
};

/* ---------------- 小游戏 ---------------- */
#define GAME_N 5
struct GameDef { const char* id; const char* name; const char* desc; uint8_t spr; uint32_t cdMs; };
static const GameDef GAMES[GAME_N] = {
  { "dice",   "押大小",  "黑市赌坊 · 猜大小",   SPR_DICE,    0 },
  { "whack",  "打地鼠",  "眼疾手快 · 敲中赚钱", SPR_WHACK,   60000 },
  { "fish",   "钓鱼",    "看准时机 · 收竿",     SPR_GAME_FISH, 45000 },
  { "memory", "翻牌配对","记住牌面 · 两两配对", SPR_MEMORY,  90000 },
  { "race",   "赛马",    "四驹争先 · 押中翻倍", SPR_RACE,    45000 },
};
static const uint16_t BETS[5] = { 50, 100, 200, 500, 1000 };
static const uint16_t RACE_BETS[4] = { 50, 100, 200, 500 };

struct HorseDef { const char* name; uint16_t body, dark; uint8_t odds; uint8_t spd; };
static const HorseDef HORSES[4] = {
  { "赤兔", HEXC(0xd0603e), HEXC(0x8a3a22), 2, 116 },
  { "的卢", HEXC(0xe8e2d0), HEXC(0x9a9482), 3, 108 },
  { "乌骓", HEXC(0x5a6382), HEXC(0x2a3048), 5, 100 },
  { "绝影", HEXC(0x8a6ab8), HEXC(0x5a3f88), 8,  92 },
};

/* ---------------- 奇遇邮件 ---------------- */
#define MAIL_N 8
enum : uint8_t { RW_EXP, RW_COIN, RW_ITEM };
struct MailSceneDef {
  const char* where; const char* who; const char* how;
  uint8_t reward; uint8_t minv; uint8_t maxv; uint8_t item;
};
static const MailSceneDef MAIL_SCENES[MAIL_N] = {
  { "长安城外", "一位云游道人",     "论道半局",         RW_EXP,  12, 28, 0 },
  { "藏剑山庄", "一把锈迹斑斑的古剑", "擦拭端详良久",    RW_EXP,  15, 32, 0 },
  { "稻香村口", "挑担的老药农",     "帮他把药筐搬回家", RW_COIN, 20, 55, 0 },
  { "洛阳街头", "卖艺的江湖人",     "驻足看了一场好戏", RW_COIN, 25, 60, 0 },
  { "万花谷",   "受伤的灵狐",       "替它包扎了伤口",   RW_ITEM,  0,  0, G_PILL },
  { "江南客栈", "遗忘在桌上的钱袋", "等了半日无人来领", RW_COIN, 30, 80, 0 },
  { "苍山洱海", "一尾金色锦鲤",     "投了些饼屑喂养",   RW_EXP,  10, 24, 0 },
  { "天策府",   "校场上的老兵",     "陪他练了几个时辰", RW_COIN, 35, 70, 0 },
};
static const char* const MAIL_HOURS[12] = {
  "子时", "丑时", "寅时", "卯时", "辰时", "巳时",
  "午时", "未时", "申时", "酉时", "戌时", "亥时"
};

/* ---------------- 手札页签 ---------------- */
#define NOTE_N 3
static const char* const NOTE_TABS[NOTE_N] = { "侠士手札", "游戏日常", "开服查询" };

/* ---------------- 设置 ---------------- */
#define SET_N 5
struct SetDef { const char* id; const char* label; const char* desc; };
static const SetDef SET_MENU[SET_N] = {
  { "ota",   "系统升级", "在线检查更新" },
  { "wifi",  "WiFi连接", "连接无线网络" },
  { "ap",    "热点模式", "手机传更新包" },
  { "sound", "音效",     "按键提示音开关" },
  { "about", "关于本机", "版本与状态" },
};

/* ---------------- 冷却 id ---------------- */
enum : uint8_t {
  CD_SIT, CD_TRAIN, CD_DIG, CD_DUNGEON, CD_EAT,
  CD_G_DICE, CD_G_WHACK, CD_G_FISH, CD_G_MEMORY, CD_G_RACE,
  CD_N
};

/* ---------------- 参数（与网页一致） ---------------- */
#define OFFLINE_MAX_GH   4       // 离线最多结算 4 游戏时辰
#define MAIL_MAX         8       // 固件邮箱只留最近 8 封
#define LIST_ROWS        4       // 背包/商城列表每屏 4 行
#define B_HOLD_MS     1500       // 长按 B 呼出设置
#define SAVE_MAGIC  0x4A583354UL // "JX3T"
#define SAVE_VER         3

/* 异常状态收益系数：中毒 0.8 / 重伤 0.6（不封锁行动） */
#define PEN_POISON 80
#define PEN_DOWN   60
