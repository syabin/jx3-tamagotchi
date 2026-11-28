/* display.cpp — ST7735S 128x128 绘制实现
 *
 * 关键设计：所有绘制先写进一块 128*128*2 = 32KB 的整帧缓存（GFXcanvas16），
 * 每帧结束时一次性 push 给屏幕。这样屏幕永远不会看到"画了一半"的中间状态，
 * 彻底消除逐行重绘造成的滚动、撕裂、闪烁。硬件 SPI 40MHz 推一帧约 8ms。
 */
#include "display.h"
#include "game.h"
#include "font_cn.h"
#include "sprites.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Preferences.h>

/* Adafruit_ST7735 把 MADCTL 常量藏在 .cpp 里，这里补一份 */
#define CMD_MADCTL   0x36
#define MADCTL_MY    0x80
#define MADCTL_MX    0x40
#define MADCTL_BGR   0x08

/* 派生一层，把 protected 的 setColRowStart 暴露出来做 GRAM 偏移校正 */
class TFT_JX3 : public Adafruit_ST7735 {
public:
  TFT_JX3(int8_t cs, int8_t dc, int8_t rst) : Adafruit_ST7735(cs, dc, rst) {}
  void setOffset(int8_t c, int8_t r) { setColRowStart(c, r); }
};

/* 三参数构造 = 硬件 SPI（SCL/SDA 走 GPIO4/6 的 FSPI 默认线） */
static TFT_JX3 tft(PIN_CS, PIN_DC, PIN_RES);
static GFXcanvas16 canvas(SCR_W, SCR_H);   // 32KB 帧缓存

static uint8_t dispMode = DISP_MODE_DEFAULT;

static void applyDispMode() {
  tft.setRotation(0);
  uint8_t madctl = MADCTL_MX | MADCTL_MY;
  if (!(dispMode & 0x01)) madctl |= MADCTL_BGR;
  tft.sendCommand(CMD_MADCTL, &madctl, 1);
  tft.invertDisplay((dispMode & 0x02) != 0);
  tft.setOffset(DISP_OFF_X, DISP_OFF_Y);
}

void dispInit() {
  /* 注意：GPIO5 恰好是 C3 的 FSPI 默认 MISO，而这块屏把 GPIO5 当背光 BLK 用。
   * 所以必须显式 begin 并指定 MISO = -1，否则 SPI 会去抢 GPIO5，背光被拉低、屏发暗发绿。 */
  SPI.begin(PIN_SCL, -1, PIN_SDA, -1);

  pinMode(PIN_BLK, OUTPUT);
#if BLK_DUTY >= 255
  digitalWrite(PIN_BLK, BLK_ACTIVE_LOW ? LOW : HIGH);
#else
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PIN_BLK, 0);
  ledcWrite(0, BLK_ACTIVE_LOW ? (255 - BLK_DUTY) : BLK_DUTY);
#endif

  tft.initR(INITR_144GREENTAB);
  tft.setSPISpeed(SPI_HZ);

  Preferences p;
  if (p.begin(NVS_NAMESPACE, true)) {
    if (p.isKey(DISP_MODE_KEY)) dispMode = p.getUChar(DISP_MODE_KEY, DISP_MODE_DEFAULT) & 0x03;
    p.end();
  }
  applyDispMode();

  canvas.fillScreen(C_BG);
  dispFlush();
}

uint8_t dispGetMode() { return dispMode; }

void dispSetMode(uint8_t m, bool persist) {
  dispMode = m & 0x03;
  applyDispMode();
  if (persist) {
    Preferences p;
    if (p.begin(NVS_NAMESPACE, false)) { p.putUChar(DISP_MODE_KEY, dispMode); p.end(); }
  }
}

void dispCycleMode() { dispSetMode((dispMode + 1) & 0x03, true); }

const char* dispModeName(uint8_t m) {
  static const char* N[4] = { "BGR", "RGB", "BGR INV", "RGB INV" };
  return N[m & 0x03];
}

void dispFlush() {
  uint16_t* buf = canvas.getBuffer();
  if (!buf) return;                      // 极端情况下堆分配失败，宁可不刷也不崩
  tft.drawRGBBitmap(0, 0, buf, SCR_W, SCR_H);
}

/* ---------------- 显示校准模式 ----------------
 * 长按 ▲ 3 秒进入。八色块 + 九级灰阶梯，用来一眼判断：
 *   红块发蓝 / 蓝块发红      → 色序（BGR 位）错
 *   白块发黑 / 黑块发白      → 反显（invert）错
 *   灰阶梯断层、跳色、偏紫    → 位深/伽马问题，不是色序能修的
 * 操作：▲ 换一档（不保存） / A 确定这一档（写 NVS） / B 直接退出
 */
static void waitRel(uint8_t pin, uint32_t maxMs = 4000) {
  uint32_t t = millis();
  while (digitalRead(pin) == LOW && millis() - t < maxMs) delay(10);
}

static void drawCard() {
  canvas.fillScreen(C_BG);

  canvas.fillRect(0, 0, SCR_W, 14, C_PANEL);
  canvas.drawFastHLine(0, 14, SCR_W, C_GOLD);
  int x = 4, y = 1;
  for (const char* p = dispModeName(dispMode); *p; p++, x += 6)
    canvas.drawChar(x, y, *p, C_GOLD_HI, C_PANEL, 1);

  static const uint16_t SW[8] = {
    0xF800, 0x07E0, 0x001F, 0xFFE0,   // 红 绿 蓝 黄
    0x07FF, 0xF81F, 0xFFFF, 0x0000    // 青 品 白 黑
  };
  for (int i = 0; i < 8; i++) {
    int cx = 5 + (i % 4) * 31, cy = 17 + (i / 4) * 30;
    canvas.fillRect(cx, cy, 26, 26, SW[i]);
    canvas.drawRect(cx, cy, 26, 26, C_DIM);
  }

  /* 九级灰阶梯 */
  for (int i = 0; i < 9; i++) {
    uint8_t v = (uint8_t)(i * 255 / 8);
    uint16_t g = (uint16_t)(((v * 31 / 255) << 11) | ((v * 63 / 255) << 5) | (v * 31 / 255));
    canvas.fillRect(2 + i * 14, 78, 13, 9, g);
  }
  canvas.drawFastHLine(0, 89, SCR_W, C_DARK);

  dispTextC(TXT_CAL_A, 64, 93, C_TEXT);
  dispTextC(TXT_CAL_B, 64, 109, C_DIM);
  dispFlush();
}

void dispCalibrate() {
  for (;;) {
    drawCard();
    delay(60);                              // 消抖

    if (digitalRead(PIN_K1) == LOW) {       // ▲ 换下一档
      dispSetMode((dispMode + 1) & 0x03, false);
      waitRel(PIN_K1);
      continue;
    }
    if (digitalRead(PIN_K3) == LOW) {       // A 确定并保存
      dispSetMode(dispMode, true);
      waitRel(PIN_K3);
      return;
    }
    if (digitalRead(PIN_K4) == LOW) {       // B 退出不保存
      waitRel(PIN_K4);
      return;
    }
  }
}

/* 这两个对帧缓存无意义，保留空实现以兼容调用点 */
void dispBegin() {}
void dispEnd()   {}

void dispClear(uint16_t c) { canvas.fillScreen(c); }
void dispRect(int x, int y, int w, int h, uint16_t c) { canvas.fillRect(x, y, w, h, c); }
void dispFrame(int x, int y, int w, int h, uint16_t c) { canvas.drawRect(x, y, w, h, c); }
void dispPixel(int x, int y, uint16_t c) { canvas.drawPixel(x, y, c); }

/* ---------- 点阵文字 ---------- */
static const Glyph* findGlyph(uint16_t code) {
  int lo = 0, hi = FONT_TABLE_N - 1;
  while (lo <= hi) {
    int mid = (lo + hi) >> 1;
    uint16_t c = FONT_TABLE[mid].code;
    if (c == code) return &FONT_TABLE[mid];
    if (c < code) lo = mid + 1; else hi = mid - 1;
  }
  return NULL;
}

int textWidth(const char* s) {
  int w = 0;
  if (!s) return 0;
  for (const uint8_t* p = (const uint8_t*)s; *p; ) {
    uint16_t cp = *p;
    int len = 1;
    if (cp >= 0xC0) {
      if ((cp & 0xE0) == 0xC0 && p[1]) { cp = ((cp & 0x1F) << 6) | (p[1] & 0x3F); len = 2; }
      else if ((cp & 0xF0) == 0xE0 && p[1] && p[2]) {
        cp = ((cp & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); len = 3;
      }
    }
    w += (cp < 127) ? 6 : 12;
    p += len;
  }
  return w;
}

static void drawOne(uint16_t cp, int x, int y, uint16_t c) {
  const Glyph* g = findGlyph(cp);
  if (!g) return;
  int cols = (cp < 127) ? 6 : 12;
  for (int row = 0; row < 12; row++) {
    uint16_t bits = ((uint16_t)g->data[row * 2] << 8) | g->data[row * 2 + 1];
    for (int col = 0; col < cols; col++) {
      if (bits & (1 << (11 - col))) canvas.drawPixel(x + col, y + row, c);
    }
  }
}

void dispText(const char* s, int x, int y, uint16_t c) {
  if (!s) return;
  dispBegin();
  for (const uint8_t* p = (const uint8_t*)s; *p; ) {
    uint16_t cp = *p; int len = 1;
    if (cp >= 0xC0) {
      if ((cp & 0xE0) == 0xC0 && p[1]) { cp = ((cp & 0x1F) << 6) | (p[1] & 0x3F); len = 2; }
      else if ((cp & 0xF0) == 0xE0 && p[1] && p[2]) {
        cp = ((cp & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); len = 3;
      }
    }
    drawOne(cp, x, y, c);
    x += (cp < 127) ? 6 : 12;
    p += len;
  }
  dispEnd();
}
void dispTextC(const char* s, int cx, int y, uint16_t c) { dispText(s, cx - textWidth(s) / 2, y, c); }
void dispTextR(const char* s, int rx, int y, uint16_t c) { dispText(s, rx - textWidth(s), y, c); }
void dispNum(int n, int x, int y, uint16_t c) {
  char b[16]; snprintf(b, sizeof(b), "%d", n); dispTextR(b, x, y, c);
}

void dispBar(int x, int y, int w, int h, int val, uint16_t c) {
  if (val < 0) val = 0; if (val > 100) val = 100;
  dispRect(x, y, w, h, C_PANEL2);
  dispRect(x + 1, y + 1, (w - 2) * val / 100, h - 2, c);
  dispFrame(x, y, w, h, C_DIM);
}
void dispSelFrame(int x, int y, int w, int h) {
  dispFrame(x, y, w, h, C_GOLD);
  dispRect(x, y, 3, 1, C_GOLD_HI); dispRect(x, y, 1, 3, C_GOLD_HI);
  dispRect(x + w - 3, y, 3, 1, C_GOLD_HI); dispRect(x + w - 1, y, 1, 3, C_GOLD_HI);
  dispRect(x, y + h - 1, 3, 1, C_GOLD_HI); dispRect(x, y + h - 3, 1, 3, C_GOLD_HI);
  dispRect(x + w - 3, y + h - 1, 3, 1, C_GOLD_HI); dispRect(x + w - 1, y + h - 3, 1, 3, C_GOLD_HI);
}

/* ---------- 统一 16×16 像素精灵（1px 精度，与网页版同源） ----------
   sprites.h 由 gen_sprites.py 从同一份字符网格生成；off=2 时取中心 12×12 供列表缩略。 */
static void dispSpr(const uint16_t (*tbl)[256], int sid, int x, int y, int scale, int off) {
  for (int row = off; row < 16 - off; row++) {
    for (int col = off; col < 16 - off; col++) {
      uint16_t c = pgm_read_word(&tbl[sid][row * 16 + col]);
      if (!c) continue;
      dispRect(x + (col - off) * scale, y + (row - off) * scale, scale, scale, c);
    }
  }
}

/* ---------- 菜单图标：圆牌底 + 1px 精灵 ---------- */
void dispIcon(uint8_t id, int x, int y, bool selected) {
  if (id >= 9) return;   /* v0.2.8：第 9 格邮箱 */
  dispRect(x, y, 24, 24, selected ? C_PANEL2 : C_PANEL);
  dispRect(x + 2, y,      20, 1, C_GOLD);
  dispRect(x + 2, y + 23, 20, 1, C_GOLD);
  dispRect(x,     y + 2,   1, 20, C_GOLD);
  dispRect(x + 23,y + 2,   1, 20, C_GOLD);
  dispSpr(selected ? SPR_MENU_SEL : SPR_MENU, FW_MENU_ID[id], x + 4, y + 4, 1, 0);
}

/* ---------- 物品图标：16×16 精灵；s=1 取中心 12×12，s>=4 画 48×48 大图 ---------- */
void dispItemIcon(uint8_t idx, int x, int y, uint8_t s) {
  if (idx >= 19) return;
  if (s <= 1) dispSpr(SPR_ITEM, idx, x, y, 1, 2);
  else        dispSpr(SPR_ITEM, idx, x, y, s >= 4 ? 3 : 2, 0);
}

/* ---------- 像素小人：成年陈月（黑长直 / 白纱 / 紫面纱 / 梅纹破伞 / 药箱） ---------- */
void dispPet(int x, int y, uint8_t pose, uint8_t sectIdx) {
  (void)sectIdx;  /* 取消门派，固定陈月配色 */
  SectDef sd = PET_STYLE;
  uint16_t robe = sd.robe, trim = sd.trim, skin = C_SKIN, hair = 0x18C6;
  uint16_t gold = C_GOLD, white = C_WHITE;
  #define P(dx,dy,w,h,col) dispRect(x+(dx)*2, y+(dy)*2, (w)*2, (h)*2, col)
  const bool day = gameHour() > 6 && gameHour() < 18;
  const uint16_t sky = day ? 0x74AA : 0x0821;
  const uint16_t CAN = 0xD6B8, CAN2 = 0xCBF0, RIB = 0xB990, OUT = 0x3028;

  /* 梅纹淡紫伞：右下撕缺一瓣，画在人后，脸不被遮 */
  if (pose != 3 && pose != 8) {
    P(18,0,6,1,OUT); P(16,1,10,1,CAN2); P(15,2,12,1,CAN);
    P(14,3,14,1,CAN2); P(14,4,14,1,CAN); P(15,5,12,1,CAN2);
    P(16,6,9,1,CAN); P(17,7,6,1,CAN2);
    P(21,4,3,1,sky); P(20,5,4,1,sky); P(18,6,4,1,sky); P(17,7,3,1,sky); // 缺角
    P(20,4,1,1,OUT); P(19,5,1,1,OUT); P(17,6,1,1,OUT);                   // 毛边
    P(19,1,1,5,RIB); P(16,2,1,3,RIB); P(22,2,1,3,RIB);                   // 伞骨
    P(17,3,2,2,0xE86A); P(17,4,1,1,white); P(21,2,2,2,0xE86A);           // 梅花
    P(20,8,1,6,0x8A60); P(19,13,1,3,0x8A60);                             // 伞柄
  }

  if (pose == 3) {  // 重伤：佝偻站立 + 星星
    P(3,11,11,5,robe); P(12,9,5,5,skin); P(12,8,5,2,hair);
    P(13,10,1,1,hair); P(15,10,1,1,hair); P(13,11,3,1,0x8030);
    P(6,9,5,2,trim); P(5,14,4,3,robe);
    P(14,4,2,2,C_GOLD_HI); P(17,6,1,1,C_GOLD_HI);
    return;
  }
  if (pose == 8) {  // 就寝：躺下 + Z
    P(2,13,12,4,robe); P(13,11,5,4,skin); P(13,10,5,2,hair);
    P(17,6,3,1,0x9CD3); P(19,7,2,1,0x9CD3); P(17,8,3,1,0x9CD3);
    return;
  }

  /* 花瓣裙 + 黑长直 + 脸：先裙后身，避免裙边盖住衣襟 */
  P(5,15,10,1,CAN2); P(4,16,12,1,0xD6B8); P(3,17,14,1,CAN2);
  P(6,10,8,6,robe); P(8,12,4,5,trim);
  P(6,7,8,5,skin);                                     // 脸
  P(4,5,2,10,hair); P(14,5,2,10,hair);                 // 黑长直到肩
  P(5,2,10,1,white); P(6,3,8,1,0xCBF0);               // 白纱抹额
  P(6,8,8,2,0xD6B8); P(6,8,8,1,0xB990);               // 淡紫面纱
  P(7,6,1,1,hair); P(12,6,1,1,hair);                   // 眼
  P(13,2,1,1,0xE86A);                                  // 梅纹发饰
  /* 斜背药箱：白底红十字 */
  P(2,14,4,4,0x8A60); P(2,14,4,1,0xB080);
  P(3,15,2,2,white); P(4,15,1,2,0xD2A8); P(3,16,2,1,0xD2A8);
  P(7,18,2,2,0x3028); P(13,18,2,2,0x3028);            // 腿靴

  if (pose == 2) {            // 中毒：绿脸
    P(6,7,8,5,0x3688); P(6,8,8,2,0xD6B8);
    P(7,6,1,1,hair); P(12,6,1,1,hair);
    P(15,2,2,2,0x3661);
  }
  if (pose == 1) {            // 吃东西：张口 + 饭碗
    P(8,10,4,1,0x8030); P(15,8,4,4,white);
  }
  if (pose == 4) {            // 副本：举剑
    P(16,2,1,10,0x9CD3); P(15,5,3,1,gold);
  }
  if (pose == 5) {            // 打坐
    P(5,16,10,3,robe); P(7,10,6,6,robe);
  }
  if (pose == 6) {            // 木桩
    P(16,4,1,12,0x8A40); P(14,6,4,1,0x9CD3);
  }
  if (pose == 7) {            // 挖宝
    P(17,6,1,9,0x8A40); P(15,14,5,1,0x9CD3);
  }
  #undef P
}

/* ---------- 场景：稻香村（木牌 / 梯田 / 大水车 / 溪桥） ---------- */
void dispScene(bool night, uint16_t frame) {
  if (night) {
    dispRect(0, 18, 128, 64, 0x0821);
    for (int i = 0; i < 11; i++) {
      int sx = (i * 37 + 5) % 124, sy = 22 + ((i * 19) % 22);
      uint16_t c = ((frame / 8 + i) % 7 == 0) ? C_GOLD_HI : C_DIM;
      dispPixel(sx, sy, c);
    }
    dispRect(100, 24, 10, 10, 0xF7DE); dispRect(104, 22, 8, 12, 0x0821);
  } else {
    dispRect(0, 18, 128, 64, 0x9CD3);   /* 稻香村晴天 */
    dispRect(108, 24, 9, 9, 0xFFF5);
    for (int i = 0; i < 2; i++) {
      int cx = 16 + i * 52 + (frame / 16) % 44;
      dispRect(cx, 30, 14, 2, 0x74AA);
      dispRect(cx + 3, 28, 9, 2, 0xD8E8);
    }
  }
  /* 稻香村木牌：最强地标识别 */
  dispRect(4, 19, 44, 14, night ? 0x2018 : 0x4228);
  dispRect(5, 20, 42, 12, night ? 0x3A2C20 : 0x8A60);
  dispRect(5, 20, 42, 1, night ? 0x4A3828 : 0xC9A8);
  dispRect(5, 31, 42, 1, night ? 0x181008 : 0x3A2418);
  dispText("稻香村", 8, 20, night ? C_DIM : C_GOLD_HI);
  dispRect(9, 33, 1, 4, 0x6A40); dispRect(42, 33, 1, 4, 0x6A40);
  /* 梯田：四层宽田带，少量稻苗 */
  for (int i = 0; i < 4; i++) {
    int ty = 56 + i * 5, tw = 96 - i * 10, tx = 6 + i * 6;
    dispRect(tx, ty, tw, 1, night ? 0x1020 : 0x3E5A34);
    dispRect(tx + 1, ty + 1, tw - 2, 3, night ? 0x1428 : 0x7A9E5A);
    dispRect(tx + 1, ty + 1, tw - 2, 1, night ? 0x182C : 0x9EBB68);
    for (int sp = 12; sp < tw - 10; sp += 18)
      dispRect(tx + sp, ty, 1, 2, night ? 0x243226 : 0xC8D878);
  }
  /* 远村茅屋 */
  dispRect(20, 62, 12, 5, night ? 0x1A1E28 : 0x8A7A5A);
  dispRect(24, 63, 3, 4, night ? 0x0D1218 : 0x4A3A2A);
  dispRect(18, 60, 16, 2, night ? 0x121820 : 0x6A8A5A);
  dispRect(20, 58, 12, 2, night ? 0x16201C : 0x7A9E6A);
  /* 大水车：8 根粗辐 + 水斗 + 加粗外圈 */
  int wx = 104, wy = 64, wr = 11;
  float wa = frame / 100.0f;
  dispRect(wx - 16, wy + 7, 18, 2, night ? 0x2A2018 : 0x6A4A2A); // 引水槽
  for (int i = 0; i < 8; i++) {
    float a = wa + i * 0.7854f;
    int ex = wx + (int)(cosf(a) * wr), ey = wy + (int)(sinf(a) * wr);
    for (int k = 0; k <= 9; k++)
      dispRect(wx + (ex - wx) * k / 9, wy + (ey - wy) * k / 9, 2, 2,
               night ? 0x2E2418 : 0x7A5A34);
    dispRect(ex - 2, ey - 1, 5, 3, night ? 0x201810 : 0x5A3A22);
    dispRect(ex - 1, ey, 3, 1, night ? 0x4A3A2A : 0xC9A86A);
  }
  for (int i = 0; i < 24; i++) {
    float a = i * 0.2618f;
    dispRect(wx + (int)(cosf(a) * wr), wy + (int)(sinf(a) * wr), 2, 2,
             night ? 0x403224 : 0xB08A52);
  }
  dispRect(wx - 3, wy - 3, 7, 7, night ? 0x201810 : 0x5A3A22);
  dispRect(wx - 2, wy - 2, 5, 5, night ? 0x342A20 : 0x8A6A40);
  dispRect(wx - 1, wy - 1, 3, 3, night ? 0x4A3A2A : 0xC9A86A);
  /* 溪流 + 拱桥 */
  uint16_t water = night ? 0x1838 : 0x5A9EB8, wave = night ? 0x2A50 : 0x8AC8D8;
  dispRect(0, 79, 128, 4, water);
  dispRect(0, 79, 128, 1, wave);
  for (int i = 0; i < 9; i++)
    dispRect((i * 13 + frame / 6) % 124, 80 + (i % 2), 4, 1, wave);
  for (int i = 0; i < 12; i++) {
    int arch = (int)(sinf(i / 11.0f * 3.1416f) * 3);
    dispRect(22 + i * 3, 79 - arch, 4, 2, night ? 0x342A20 : 0x8A6A40);
    dispRect(22 + i * 3, 78 - arch, 4, 1, night ? 0x4A3A2A : 0xB08A52);
  }
  /* 荷花池 */
  dispRect(2, 80, 22, 3, night ? 0x142C3C : 0x4A8A9E);
  dispRect(5, 81, 6, 2, night ? 0x1A3428 : 0x4A8A52);
  dispRect(14, 82, 6, 2, night ? 0x1A3428 : 0x4A8A52);
  dispRect(7, 79, 2, 2, 0xE86A9A); dispRect(16, 80, 2, 2, 0xE86A9A);
}

void dispHouse(int x0, int y0) {
  dispRect(x0, y0 - 16, 32, 16, 0x6A28);
  dispRect(x0 - 3, y0 - 20, 38, 4, 0xB4C0);
  dispRect(x0 + 13, y0 - 10, 7, 10, 0x8A40);
  dispRect(x0 + 5, y0 - 13, 5, 5, C_GOLD);
  dispRect(x0 + 23, y0 - 13, 5, 5, C_GOLD);
  dispRect(x0, y0, 32, 2, 0x4228);
}
void dispDecos(int gy) {
  dispRect(24, gy - 14, 14, 14, 0x8A40);
  dispRect(26, gy - 12, 10, 10, 0xD4A0);
  dispRect(88, gy - 3, 7, 3, 0x8A40);
  dispRect(89, gy - 8, 5, 5, 0x2428);
  dispRect(103, gy - 11, 1, 11, 0x8A40);
  dispRect(101, gy - 16, 5, 6, C_GOLD_HI);
}
