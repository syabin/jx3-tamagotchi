/* ============================================================
 * 剑网三拓麻歌子 · ESP32-C3 实体机固件
 * config.h — 引脚 / 全量数值 / 全量文案（与网页 v8.3 对齐）
 *
 * 硬件：ESP32-C3 Super Mini + 1.44" 128x128 ST7735S（板载 4 键）
 * 屏丝印：K1 K2 K3 K4 BLK CS DC RES SDA SCL VCC GND
 *
 * 【接线速查】详见 README.md
 *   VCC→3V3  GND→GND  SCL→GPIO4  SDA→GPIO6  CS→GPIO7
 *   DC→GPIO10  RES→GPIO3  BLK→GPIO5
 *   K1(▲)→GPIO0  K2(▼)→GPIO1  K3(A)→GPIO20  K4(B)→GPIO21
 *   GPIO2 / GPIO8 / GPIO9 是启动配置脚，默认不占用。
 *
 * 修改中文文案后，运行 `python tools/gen_font.py` 重新生成 font_cn.h。
 * ============================================================ */
#ifndef JX3_CONFIG_H
#define JX3_CONFIG_H

#include <stdint.h>

/* ---------------- 引脚 ---------------- */
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

#define PIN_BUZZER  -1   // 无源蜂鸣器：若接了请改成空闲脚并设 ENABLE_BUZZER=1
#define ENABLE_BUZZER 0

/* ---------------- 屏幕 ---------------- */
#define SCR_W   128
#define SCR_H   128
#define SPI_HZ  27000000      // 硬件 SPI 时钟。面包板 + 长杜邦线建议 27MHz，飞线很短可试 40000000

/* 显示模式：bit0=1 用 RGB 色序（不置 MADCTL 的 BGR 位）
 *           bit1=1 反显 invertDisplay
 * 开机后任意页面「长按 ▲ 3 秒」循环切换 4 档，切换时会先弹一张八色自检卡，
 * 红/绿/蓝 三块显示正确的那一档就是对的，选中后自动存入 NVS。 */
#define DISP_MODE_DEFAULT  0
#define DISP_MODE_KEY      "dmod2"
#define DISP_HOLD_MS       3000

/* GRAM 偏移（1.44" 128x128 绿标为 2/1）。整屏偏出几个像素时调这里。 */
#define DISP_OFF_X  2
#define DISP_OFF_Y  1

/* 背光亮度 0–255。1.44" TN 全亮时蓝溢明显又刺眼，200 左右观感最接近网页版。
 * 若你的模块背光是低电平点亮，把 BLK_ACTIVE_LOW 改 1。 */
#define BLK_DUTY         200
#define BLK_ACTIVE_LOW   0

/* ---------------- 时序 ---------------- */
#define TICK_MS           1000
#define FRAME_MS          33      // ~30fps，对齐 ST7735 实际刷新
#define TOAST_MS          2800
#define HINT_MS           2000
#define FX_EAT_MS         2400
#define FX_MED_MS         2600
#define FX_USE_MS         2200
#define OFFLINE_MAX_GH    4
#define SAVE_INTERVAL_MS  15000
#define KEY_DEBOUNCE_MS   40      // 电平必须稳定这么久才算数（机械按键抖动 5~20ms）
#define KEY_RELEASE_LOCK_MS 300   // 长按动作后，松手再加这么久静默，吞掉松手回弹
#define KEY_HOLD_MS       1500    // v8.3：长按 B 1.5 秒呼出设置
#define KEY_REPEAT_WAIT   480
#define KEY_REPEAT_RATE   110

/* ---------------- 版本 / 存档 ---------------- */
#define APP_VERSION       "8.4"
#define NVS_NAMESPACE     "jx3tama"
#define NVS_KEY           "save"
#define NVS_TIME_KEY      "clock"
#define SAVE_VERSION      3

/* ---------------- 颜色 RGB565 ---------------- */
/* 网页版色值 + TN 屏补偿：
 * 1.44" TN 的蓝通道透过率偏高、背光又是冷白，直接照搬网页版的"冷墨色"会整屏泛蓝，
 * 所以背景去掉蓝调改中性暖黑，文字由冷白改暖白，金色提饱和。 */
#define C_BG        0x1081   /* #121110 中性暖黑（网页 #141824 去蓝） */
#define C_PANEL     0x18C2   /* #1c1a18 面板 */
#define C_PANEL2    0x2103   /* #26221e 面板浅一档 */
#define C_GOLD      0xDDEE   /* #e3c078 主金（比网页版更实） */
#define C_GOLD_HI   0xF758   /* #f8ecc8 亮金高光 */
#define C_TEXT      0xDEFA   /* #e6e2d8 主文字 暖白 */
#define C_DIM       0x8C50   /* #8f8d88 次要文字 中性灰 */
#define C_DARK      0x3164   /* #332e28 深边框 */
#define C_RED       0xD2A8   /* #d8554a */
#define C_GREEN     0x552B   /* #5aa860 */
#define C_ORANGE    0xDCA8   /* #e0964a */
#define C_PURPLE    0xBBD4   /* #c07aa8 */
#define C_BLUE      0x53B7   /* #5a78c0 */
#define C_WHITE     0xEF7B   /* #f5f0e4 */
#define C_CYAN      0x07FF
#define C_SKIN      0xEE33   /* #f0c8a0 */

/* ---------------- 枚举 ---------------- */
enum GoodType {
  T_FOOD, T_MED, T_USE, T_MAT, T_COLL, T_EQUIP, T_XJ, T_DECO
};
enum EquipSlot { SLOT_NONE = 0, SLOT_WEAPON = 1, SLOT_BODY = 2, SLOT_TREA = 3 };
enum GoodsCat  { CAT_ALL = 0, CAT_FOOD, CAT_MED, CAT_EQUIP, CAT_DECO, CAT_MAT };
enum PageId {
  PG_BOOT, PG_SECT_UNUSED, PG_MAIN, PG_BAG, PG_SHOP, PG_ITEM, PG_HOME,
  PG_DAILY, PG_STATUS, PG_GAME, PG_MAIL, PG_SETTINGS
};
enum ActPhase { PH_SIDE, PH_BET, PH_ROLL, PH_RESULT };
enum FxKind   { FX_NONE, FX_EAT, FX_MED, FX_USE };

/* ---------------- 称号 / 声望 / 家园（v0.2.10 全面对齐剑网三） ---------------- */
struct SectDef { uint16_t robe; uint16_t trim; uint16_t accent; };
static const SectDef PET_STYLE = { 0xF7DE, 0xFFFF, 0xCBF0 };   // 陈月：白衣 / 白纱 / 淡紫
struct TitleDef { int exp; const char* name; };
static const TitleDef TITLES[5] = {
  { 0,   "初闯江湖" },
  { 50,  "江湖新秀" },
  { 150, "一方名侠" },
  { 350, "武林名宿" },
  { 700, "武林至尊" },
};
/* 剑三声望五阶：中立 → 友好 → 尊敬 → 崇敬 → 崇拜 */
struct RepDef { int exp; const char* name; };
static const RepDef REPUTATIONS[5] = {
  { 0,   "中立" },
  { 50,  "友好" },
  { 150, "尊敬" },
  { 350, "崇敬" },
  { 700, "崇拜" },
};
static const char* const HOME_NAMES[5] = { "荒地", "茅舍", "宅院", "别院", "山庄" };

/* ---------------- 物品目录（19 件，与 v8.3 完全一致） ---------------- */
struct GoodDef {
  const char* name;
  uint8_t  type;      // GoodType
  uint8_t  slot;      // EquipSlot
  uint8_t  tier;      // 1凡品 2良品 3珍品
  uint8_t  cat;       // GoodsCat
  int8_t   gain;      // 食物回体力
  int8_t   mood;      // 食物回心情
  int8_t   exp;       // 残页修为
  int16_t  price;     // 商城价，0=不卖
  int16_t  sell;      // 卖价，0=不可卖
  uint8_t  cd;        // 食用冷却（秒）
  const char* eff;    // 短标签
};
static const GoodDef GOODS[19] = {
  /*  0 */ { "包子",     T_FOOD,  0, 0, CAT_FOOD,  15, 5,  0,  80,    0, 12, "体力+15 心情+5" },
  /*  1 */ { "蒸鱼",     T_FOOD,  0, 0, CAT_FOOD,  30, 5,  0, 150,    0, 25, "体力+30 心情+5" },
  /*  2 */ { "烧尾宴",   T_FOOD,  0, 0, CAT_FOOD,  50, 5,  0, 300,    0, 60, "体力+50 心情+5" },
  /*  3 */ { "玉露丸",   T_MED,   0, 0, CAT_MED,    0, 0,  0, 200,    0,  0, "解毒 治重伤" },
  /*  4 */ { "剑意残页", T_USE,   0, 0, CAT_MED,    0, 0,  3,   0,    0,  0, "参悟 修为+3" },
  /*  5 */ { "铁矿石",   T_MAT,   0, 0, CAT_MAT,    0, 0,  0,   0,   60,  0, "可卖60金" },
  /*  6 */ { "剑穗",     T_COLL,  0, 0, CAT_MAT,    0, 0,  0,   0,  150,  0, "可卖150金" },
  /*  7 */ { "精铁",     T_MAT,   0, 0, CAT_MAT,    0, 0,  0,   0,  200,  0, "可卖200金" },
  /*  8 */ { "神兵残片", T_COLL,  0, 0, CAT_MAT,    0, 0,  0,   0,  500,  0, "可卖500金" },
  /*  9 */ { "铸天玄晶", T_XJ,    0, 0, CAT_MAT,    0, 0,  0,   0, 2500,  0, "赛季至宝" },
  /* 10 */ { "精铁剑",   T_EQUIP, 1, 1, CAT_EQUIP,  0, 0,  0, 300,    0,  0, "武器 木桩+50%" },
  /* 11 */ { "镔铁镐",   T_EQUIP, 1, 1, CAT_EQUIP,  0, 0,  0, 260,    0,  0, "武器 挖宝+50%" },
  /* 12 */ { "云锦道袍", T_EQUIP, 2, 2, CAT_EQUIP,  0, 0,  0, 280,    0,  0, "护身 心情-20%" },
  /* 13 */ { "平安符",   T_EQUIP, 2, 2, CAT_EQUIP,  0, 0,  0, 200,    0,  0, "护身 副本胜率+15%" },
  /* 14 */ { "翡翠扳指", T_EQUIP, 3, 2, CAT_EQUIP,  0, 0,  0, 350,    0,  0, "饰品 赌坊+20%" },
  /* 15 */ { "酒葫芦",   T_EQUIP, 1, 3, CAT_EQUIP,  0, 0,  0, 150,    0,  0, "饰品 打坐修为+2" },
  /* 16 */ { "幽兰盆栽", T_DECO,  0, 0, CAT_DECO,   0, 0,  0, 120,    0,  0, "装饰 心情-5%" },
  /* 17 */ { "云纹屏风", T_DECO,  0, 0, CAT_DECO,   0, 0,  0, 260,    0,  0, "装饰 心情-8%" },
  /* 18 */ { "长明灯",   T_DECO,  0, 0, CAT_DECO,   0, 0,  0, 200,    0,  0, "装饰 心情体力-6%" },
};
#define GOODS_COUNT 19

/* 索引（避免各处写魔法数字） */
enum GOOD_IDX {
  IDX_BAOZI = 0, IDX_FISH, IDX_FEAST, IDX_PILL, IDX_PAGE, IDX_IRON,
  IDX_TASSEL, IDX_STEEL, IDX_SHARD, IDX_XJ,
  IDX_WSWORD, IDX_WPICK, IDX_AROBE, IDX_ACHARM, IDX_TJADE, IDX_TGOURD,
  IDX_DPLANT, IDX_DSCREEN, IDX_DLAMP
};
/* 商城顺序（不含不可购买物） */
static const uint8_t SHOP_ORDER[13] = {
  0,1,2,3,10,11,12,13,14,15,16,17,18
};
#define SHOP_COUNT 13
/* 背包顺序 = GOODS 自然顺序 */
static const uint8_t BAG_ORDER[GOODS_COUNT] = {
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18
};

/* 分类 */
static const char* const CAT_LABELS[6] = { "全部", "食品", "丹药", "装备", "家具", "材料" };

/* 装饰 */
static const uint8_t DECO_LIST[3]  = { IDX_DPLANT, IDX_DSCREEN, IDX_DLAMP };
static const uint8_t DECO_MOOD[3]  = { 5, 8, 6 };   // 百分比整数

/* 赌坊注档 */
static const int BETS[5] = { 50, 100, 200, 500, 1000 };
#define BET_COUNT 5

/* ---------------- 副本掉落表 ---------------- */
struct DropDef { uint8_t idx; uint8_t pct; };   // pct: 0..100，用整数避免浮点误差
struct DungeonDef {
  const char* name;
  int coinMin, coinMax;
  int expMin,  expMax;
  uint8_t dropN;
  DropDef drops[4];
};
static const DungeonDef DUNGEONS[4] = {
  { "大战",     250, 250, 18, 24, 3, { {IDX_IRON,40}, {IDX_BAOZI,30}, {IDX_PAGE,10}, {0,0} } },
  { "阵营战场", 280, 280, 15, 20, 2, { {IDX_PILL,8},  {255,25},      {0,0},       {0,0} } },  // 255=金币+100
  { "秘境日常", 260, 260, 16, 22, 3, { {IDX_FISH,20}, {IDX_PILL,10},  {IDX_TASSEL,5},{0,0} } },
  { "团本周本", 400, 450, 25, 33, 4, { {IDX_STEEL,35},{IDX_PILL,15},  {IDX_TASSEL,12},{IDX_SHARD,2} } },
};

/* ---------------- 菜单 ---------------- */
static const char* const MENU_LABELS[9] = {
  "日常", "背包", "商城", "打坐", "木桩", "挖宝", "小游戏", "家园", "邮箱"
};
static const char* const MENU_DESCS[9] = {
  "今日剑网三日常", "查看行囊物品", "购买食材装备", "心情+20 修为+3",
  "修为+12 耗体力", "挖宝得金币", "黑市赌坊押大小", "升级宅子打扫", "奇遇来信"
};

/* ---------------- 文案（字库从这些字符串提取） ---------------- */
/* 奇遇邮箱场景文案（scene 0..7，与网页 MAIL_SCENES 对应） */
static const char* const MAIL_WHERE[8] = {
  "长安城外", "藏剑山庄", "稻香村口", "洛阳街头", "万花谷中", "江南客栈", "苍山洱海", "天策校场"
};
static const char* const MAIL_WHO[8] = {
  "云游道人", "斑驳古剑", "老药农", "江湖艺人", "受伤灵狐", "遗落钱袋", "金色锦鲤", "守营老兵"
};
static const char* const MAIL_HOW[8] = {
  "论道半局", "擦拭端详", "帮忙搬筐", "看了一场戏", "替它包扎", "等了半日", "投饼喂养", "陪练几时"
};
static const char* const TXT_MAIL      = "邮箱";
static const char* const TXT_NO_MAIL   = "暂无来信";
static const char* const TXT_MAIL_HINT = "奇遇自动寄到邮箱";
static const char* const TXT_BOOT_A   = "剑网三";
static const char* const TXT_BOOT_B   = "剑侠情缘·单机版三";
static const char* const TXT_PRESS_A  = "按A开始";
static const char* const TXT_HOLD_UP  = "长按K1调色";
static const char* const TXT_CAL_A    = "上键换档A确定";
static const char* const TXT_CAL_B    = "B退出";
/* v0.2.7：TXT_SECT 已移除 */
static const char* const TXT_BAG      = "随身行囊";
static const char* const TXT_SHOP     = "长安集市";
static const char* const TXT_HOME     = "家园";
static const char* const TXT_DAILY    = "今日日常";
static const char* const TXT_STATUS   = "侠士手札";
static const char* const TXT_GAME     = "黑市赌坊";
static const char* const TXT_SETTINGS = "系统设置";
static const char* const TXT_ITEM     = "物品详情";

static const char* const TXT_HUNGER   = "体力";
static const char* const TXT_MOOD     = "心情";
static const char* const TXT_EXP      = "修为";
static const char* const TXT_COINS    = "金币";
static const char* const TXT_TITLE    = "称号";
static const char* const TXT_REPUTE   = "声望";
static const char* const TXT_NEXT     = "晋升";
static const char* const TXT_STATE    = "状态";
static const char* const TXT_HOMELV   = "宅邸";
static const char* const TXT_DAY      = "天";
static const char* const TXT_HOUR     = "时辰";
static const char* const TXT_COND_OK  = "康健";
static const char* const TXT_COND_POI = "中毒 -20%";
static const char* const TXT_COND_DOW = "重伤 -40%";
static const char* const TXT_SLEEP    = "就寝中";
static const char* const TXT_NANNY    = "保姆";

static const char* const TXT_ALL      = "全部";
static const char* const TXT_OWNED    = "持有";
static const char* const TXT_PRICE    = "价";
static const char* const TXT_TIER     = "品级";
static const char* const TXT_EQUIPED  = "已穿戴";
static const char* const TXT_EQUIP    = "穿戴";
static const char* const TXT_UNEQUIP  = "卸下";
static const char* const TXT_SELL     = "卖出";
static const char* const TXT_EAT      = "吃掉";
static const char* const TXT_USE      = "使用";
static const char* const TXT_COOL     = "冷却中";
static const char* const TXT_NOITEM   = "没有此物";
static const char* const TXT_NOCOIN   = "金币不足";
static const char* const TXT_BACK     = "返回";
static const char* const TXT_CONFIRM  = "确认清档?";
static const char* const TXT_YES      = "是";
static const char* const TXT_NO       = "否";
static const char* const TXT_ON       = "开";
static const char* const TXT_OFF      = "关";

static const char* const TXT_UP       = "升级";
static const char* const TXT_CLEAN    = "打扫";
static const char* const TXT_HIRE     = "保姆100金";
static const char* const TXT_COST     = "升级需";
static const char* const TXT_MAXLV    = "已满级";
static const char* const TXT_MESS     = "狼藉";
static const char* const TXT_DECO     = "装饰";
static const char* const TXT_LEFT     = "剩";

static const char* const TXT_SIDE     = "押大 11点以上";
static const char* const TXT_SMALL    = "押小 10点以下";
static const char* const TXT_BET      = "注额";
static const char* const TXT_BAOZI    = "豹子 庄家通吃";
static const char* const TXT_WIN      = "押中";
static const char* const TXT_LOSE     = "未押中";
static const char* const TXT_ROLLING  = "骰盅摇动中";
static const char* const TXT_PLAY     = "再来一局";
static const char* const TXT_LEAVE    = "离开";

static const char* const TXT_SET_SPEED= "时间流速";
static const char* const TXT_SET_SOUND= "音效开关";
static const char* const TXT_SET_SAVE = "手动存档";
static const char* const TXT_SET_RESET= "重新开号";
static const char* const TXT_SET_ABOUT= "关于本机";
static const char* const TXT_SPEED_1  = "真实";
static const char* const TXT_SPEED_10 = "真机";
static const char* const TXT_SPEED_60 = "体验";
static const char* const TXT_SPEED_600= "极速";
static const char* const TXT_ABOUT_FW = "固件";
static const char* const TXT_ABOUT_DEV= "ESP32-C3";
static const char* const TXT_ABOUT_SCR= "ST7735 128x128";
static const char* const TXT_HOLD_SET = "长按B设置";

/* 提示短句 */
static const char* const MSG_EAT      = "气力恢复";
static const char* const MSG_CURED    = "满血复活";
static const char* const MSG_SIT      = "打坐 心境澄明";
static const char* const MSG_TRAIN    = "练武 修为+";
static const char* const MSG_DIG      = "挖到 金币+";
static const char* const MSG_STONE    = "一块石头";
static const char* const MSG_CLEANOK  = "打扫完毕";
static const char* const MSG_DUNGEON  = "副本通关";
static const char* const MSG_WIPE     = "团灭了";
static const char* const MSG_POISON   = "中毒了 -20%";
static const char* const MSG_DOWN     = "重伤 -40%";
static const char* const MSG_RECOVER  = "伤势痊愈";
static const char* const MSG_LEVELUP  = "称号晋升";
static const char* const MSG_SLEEP    = "夜深了 侠士就寝";
static const char* const MSG_WAKE     = "天亮了 侠士起身";
static const char* const MSG_NANNYEND = "保姆合约到期";
static const char* const MSG_HIREOK   = "已雇保姆8时辰";
static const char* const MSG_UPHOME   = "家园升级";
static const char* const MSG_EQON     = "装备";
static const char* const MSG_EQOFF    = "卸下";
static const char* const MSG_BUYOK    = "购得";
static const char* const MSG_SELLOK   = "卖出 得金";
static const char* const MSG_DAILY    = "每日登录";
static const char* const MSG_LUCKY    = "鸿运当头";
static const char* const MSG_SAVED    = "已存档";
static const char* const MSG_RESET    = "已重新开号";
static const char* const MSG_NOFOOD   = "先吃点东西";
static const char* const MSG_COOL     = "还没缓过来";
static const char* const MSG_NOMONEY  = "本钱不够";
static const char* const MSG_XJ       = "橙光乍现 玄晶降临";
static const char* const MSG_DECO     = "已布置庭院";

/* 日常（固件离线显示固定分类，联网时由 daily.cpp 覆盖） */
static const char* const D_WAR    = "大战";
static const char* const D_BATTLE = "阵营战场";
static const char* const D_CARD   = "秘境日常";
static const char* const D_RAID   = "团本周本";
static const char* const D_OTHER  = "其他活动";
static const char* const D_EMPTY  = "暂无数据";
static const char* const D_OFFLINE= "离线模式";
static const char* const D_ENTER  = "按A进副本";

/* 品级名 */
static const char* const TIER_NAMES[4] = { "", "凡品", "良品", "珍品" };
static const char* const SLOT_NAMES[4] = { "", "武器", "护身", "饰品" };

#endif /* JX3_CONFIG_H */
