/* display.cpp — ST7735S 128×128 绘制层
 *
 * 整帧缓存（GFXcanvas16 32KB）→ 每帧一次推送，消除撕裂/滚动。
 * 所有坐标与网页版 128×128 画布一一对应，颜色用 HEXC() 直接写网页色值。
 */
#include "display.h"
#include "game.h"
#include "font_cn.h"
#include "sprites.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Preferences.h>

#define CMD_MADCTL   0x36
#define MADCTL_MY    0x80
#define MADCTL_MX    0x40
#define MADCTL_BGR   0x08

class TFT_JX3 : public Adafruit_ST7735 {
public:
  TFT_JX3(int8_t cs, int8_t dc, int8_t rst) : Adafruit_ST7735(cs, dc, rst) {}
  void setOffset(int8_t c, int8_t r) { setColRowStart(c, r); }
};

static TFT_JX3 tft(PIN_CS, PIN_DC, PIN_RES);
static GFXcanvas16 canvas(SCR_W, SCR_H);
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
  /* GPIO5 是 C3 FSPI 默认 MISO，而屏把它当背光 BLK：必须显式 MISO=-1 */
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

void dispFlush() {
  uint16_t* buf = canvas.getBuffer();
  if (!buf) return;
  tft.drawRGBBitmap(0, 0, buf, SCR_W, SCR_H);
}

uint8_t dispGetMode() { return dispMode; }
void dispSetMode(uint8_t m, bool persist) {
  dispMode = m & 0x03;
  applyDispMode();
  if (persist) { Preferences p; if (p.begin(NVS_NAMESPACE, false)) { p.putUChar(DISP_MODE_KEY, dispMode); p.end(); } }
}
void dispCycleMode() { dispSetMode((dispMode + 1) & 0x03, true); }
const char* dispModeName(uint8_t m) {
  static const char* N[4] = { "BGR", "RGB", "BGR INV", "RGB INV" };
  return N[m & 0x03];
}

void dispClear(uint16_t c) { canvas.fillScreen(c); }
void dispRect(int x, int y, int w, int h, uint16_t c) {
  if (w <= 0 || h <= 0) return;
  canvas.fillRect(x, y, w, h, c);
}
void dispFrame(int x, int y, int w, int h, uint16_t c) { canvas.drawRect(x, y, w, h, c); }
void dispPixel(int x, int y, uint16_t c) { canvas.drawPixel(x, y, c); }

/* ================= 文字 ================= */
static const Glyph* findGlyph(uint16_t code) {
  int lo = 0, hi = FONT_TABLE_N - 1;
  while (lo <= hi) {
    int mid = (lo + hi) >> 1;
    uint16_t c = pgm_read_word(&FONT_TABLE[mid].code);
    if (c == code) return &FONT_TABLE[mid];
    if (c < code) lo = mid + 1; else hi = mid - 1;
  }
  return NULL;
}

/* UTF-8 解码：返回码点，p 前进 */
static uint16_t utf8Next(const char*& p) {
  uint8_t c = (uint8_t)*p;
  if (c == 0) return 0;
  if (c < 0x80) { p++; return c; }
  if ((c & 0xE0) == 0xC0) { uint16_t v = ((c & 0x1F) << 6) | (p[1] & 0x3F); p += 2; return v; }
  if ((c & 0xF0) == 0xE0) { uint16_t v = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3; return v; }
  p++; return c;
}

static void drawHZ(uint16_t code, int x, int y, uint16_t c) {
  const Glyph* g = findGlyph(code);
  if (!g) {                     // 缺字画一个空心框，便于发现漏字
    dispFrame(x + 1, y + 1, 10, 10, C_DIM);
    return;
  }
  for (int r = 0; r < 12; r++) {
    uint16_t bits = ((uint16_t)pgm_read_byte(&g->data[r * 2]) << 8) | pgm_read_byte(&g->data[r * 2 + 1]);
    if (!bits) continue;
    int b = 0;
    while (b < 12) {
      if (bits & (0x800 >> b)) {
        int s = b;
        while (b < 12 && (bits & (0x800 >> b))) b++;
        canvas.fillRect(x + s, y + r, b - s, 1, c);
      } else b++;
    }
  }
}

int textW(const char* s) {
  int w = 0;
  if (!s) return 0;
  while (*s) {
    uint16_t cp = utf8Next(s);
    if (!cp) break;
    w += (cp < 0x80) ? 6 : 12;
  }
  return w;
}

void dispText(const char* s, int x, int y, uint16_t c) {
  if (!s) return;
  int cx = x;
  while (*s) {
    uint16_t cp = utf8Next(s);
    if (!cp) break;
    if (cp < 0x80) {
      canvas.drawChar(cx, y + 3, (char)cp, c, c, 1);
      cx += 6;
    } else {
      drawHZ(cp, cx, y, c);
      cx += 12;
    }
  }
}

void dispTextC(const char* s, int cx, int y, uint16_t c) { dispText(s, cx - textW(s) / 2, y, c); }
void dispTextR(const char* s, int rx, int y, uint16_t c) { dispText(s, rx - textW(s), y, c); }

/* 跑马灯：超宽时按 offsetPx 横向滚动 */
void dispTextClip(const char* s, int x, int y, int w, uint16_t c, int offsetPx) {
  if (!s) return;
  int tw = textW(s);
  int off = (tw <= w) ? 0 : (offsetPx % (tw + 12));
  int cx = x - off;
  const char* p = s;
  while (*p) {
    uint16_t cp = utf8Next(p);
    if (!cp) break;
    int cw = (cp < 0x80) ? 6 : 12;
    if (cx + cw > x && cx < x + w) {
      if (cp < 0x80) canvas.drawChar(cx, y + 3, (char)cp, c, c, 1);
      else drawHZ(cp, cx, y, c);
    }
    cx += cw;
    if (tw > w && cx > x + w) break;
  }
}

/* 按字数折行（与网页 wrapText 一致：纯字符切分，简单可靠） */
int dispWrap(const char* s, int maxChars, const char** out, int maxLines) {
  static const char* buf[8];
  static char store[8][48];
  int n = 0, ci = 0, si = 0;
  const char* p = s;
  while (*p && n < maxLines && n < 8) {
    if (*p == '\n') { store[n][si] = 0; out[n] = store[n]; n++; si = 0; ci = 0; p++; continue; }
    uint16_t cp = utf8Next(p);
    int need = (cp < 0x80) ? 1 : 3;
    if (ci >= maxChars || si + need >= 47) {
      store[n][si] = 0; out[n] = store[n]; n++; si = 0; ci = 0;
      if (n >= maxLines) break;
    }
    if (cp < 0x80) store[n][si++] = (char)cp;
    else {
      store[n][si++] = (char)(0xE0 | (cp >> 12));
      store[n][si++] = (char)(0x80 | ((cp >> 6) & 0x3F));
      store[n][si++] = (char)(0x80 | (cp & 0x3F));
    }
    ci++;
  }
  if (si > 0 && n < maxLines && n < 8) { store[n][si] = 0; out[n] = store[n]; n++; }
  (void)buf;
  return n;
}

/* ================= 精灵 / 图标 ================= */
void dispSpr(uint8_t id, int x, int y) {
  if (id >= SPR_COUNT) return;
  const uint16_t* s = (const uint16_t*)pgm_read_ptr(&SPR_TAB[id]);
  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 16; col++) {
      uint16_t c = pgm_read_word(&s[row * 16 + col]);
      if (c) canvas.drawPixel(x + col, y + row, c);
    }
  }
}

/* 最近邻放大：16×16 → size×size（大图台座用） */
void dispSprS(uint8_t id, int x, int y, uint8_t size) {
  if (id >= SPR_COUNT || size == 0) return;
  const uint16_t* s = (const uint16_t*)pgm_read_ptr(&SPR_TAB[id]);
  for (int ty = 0; ty < size; ty++) {
    int sy = ty * 16 / size;
    for (int tx = 0; tx < size; tx++) {
      uint16_t c = pgm_read_word(&s[sy * 16 + tx * 16 / size]);
      if (c) canvas.drawPixel(x + tx, y + ty, c);
    }
  }
}

/* 圆牌（网页 icon() 的 ROWW 造型）+ 精灵 */
void dispIcon(uint8_t sprId, int x, int y, bool sel) {
  static const uint8_t ROWW[16] = { 2, 8, 10, 12, 14, 14, 16, 16, 16, 16, 14, 14, 12, 10, 8, 2 };
  uint16_t rim = sel ? C_GOLD : HEXC(0x7a6a3e);
  uint16_t disc = sel ? HEXC(0x332b1c) : HEXC(0x1c2130);
  for (int dy = 0; dy < 16; dy++) {
    int w = ROWW[dy];
    int x0 = x + 8 - w / 2;
    dispRect(x0, y + dy, w, 1, rim);
    if (w >= 8) dispRect(x0 + 1, y + dy, w - 2, 1, disc);
  }
  dispSpr(sprId, x, y);
}

void dispSelFrame(int x, int y, int w, int h) {
  dispFrame(x, y, w, h, C_GOLD);
  dispRect(x, y, 3, 2, C_GOLD_HI); dispRect(x + w - 3, y, 3, 2, C_GOLD_HI);
  dispRect(x, y + h - 2, 3, 2, C_GOLD_HI); dispRect(x + w - 3, y + h - 2, 3, 2, C_GOLD_HI);
}

void dispTri(int x, int y, int dir, bool on) {
  uint16_t c = on ? C_GOLD_HI : HEXC(0x3a4158);
  for (int i = 0; i < 9; i++) {
    int L = 3 + 2 * (4 - abs(i - 4));
    if (dir < 0) dispRect(x + 11 - L, y + i, L, 1, c);
    else         dispRect(x, y + i, L, 1, c);
  }
}

/* ================= 陈月（与网页 drawPet 逐格一致，u=2） ================= */
static int g_ox, g_oy, g_u = 2;
#define P(x, y, w, h, c)  dispRect(g_ox + (x) * g_u, g_oy + (y) * g_u, (w) * g_u, (h) * g_u, c)

/* 头部位图：18 行 × 20 列，字符含义见 PET_PAL */
static const char* const PET_HEAD[18] = {
  "....OOOOOOOOOOOO....",
  "..OOWWWWWWWWWWWWOO..",
  ".OWWWWWWWWWWWWWWWWO.",
  "OSWHHHHHHHHHHHHHHWSO",
  "OSSHHHHHHHHHHHHHHSSO",
  "OSSHHMMMMMMMMMMHHSSO",
  "OSSgMMMMMMMMMMMMgSSO",
  "OSSggMMMMMMMMMMggSSO",
  "OSSgFFFFFFFFFFFFgSSO",
  "OSSFFeWWeFFeWWeFFSSO",
  "OSSFFEWWEFFEWWEFFSSO",
  "OSSFFeWWeFFeWWeFFSSO",
  "OSSfBBFFFFFFFFBBfSSO",
  "OSSFFFFFFmmFFFFFFSSO",
  "OSSfFFFFFFFFFFFFfSSO",
  ".OOSSFFFFFFFFFFSSOO.",
  "....OOOOOOOOOOOO....",
  "......OOOOOOOO......",
};

static uint16_t headPal(char ch) {
  switch (ch) {
    case 'O': return HEXC(0x2b2b3a);
    case 'H': return HEXC(0x2e2a38);
    case 'S': return HEXC(0x1e1c28);
    case 'L': return HEXC(0x4a4658);
    case 'M': return HEXC(0x3a3648);
    case 'F': return HEXC(0xfce3d3);
    case 'f': return HEXC(0xeec9b4);
    case 'B': return HEXC(0xe8a8b8);
    case 'E': return HEXC(0x4a3a6a);
    case 'e': return HEXC(0x7a6aa8);
    case 'W': return HEXC(0xffffff);
    case 'w': return HEXC(0xc9a8e0);
    case 'm': return HEXC(0xc96b74);
    case 'g': return HEXC(0xd8cce8);
    default:  return 0;
  }
}

static void petHead(int hx, int hy, uint8_t mood) {
#define Q(x, y, w, h, c) P(hx + (x), hy + (y), (w), (h), (c))
  const uint16_t WH = HEXC(0xffffff), BOW2 = HEXC(0xe0d8e8), HA = HEXC(0x2e2a38),
                 HL = HEXC(0x4a4658), SK = HEXC(0xfce3d3), OL = HEXC(0x2b2b3a),
                 E1 = HEXC(0x4a3a6a), E2 = HEXC(0x7a6aa8), BOW = HEXC(0xf5f0f8);
  /* 位图（同行同色合并成段） */
  for (int y = 0; y < 18; y++) {
    const char* row = PET_HEAD[y];
    int runStart = 0;
    char cur = 0;
    for (int x = 0; x <= 20; x++) {
      char ch = (x < 20) ? row[x] : 0;
      if (ch != cur) {
        if (cur && cur != '.') {
          uint16_t col = headPal(cur);
          if (col) Q(runStart, y, x - runStart, 1, col);
        }
        runStart = x; cur = ch;
      }
    }
  }
  /* 白纱抹额 + 黑长直 + 垂纱 */
  Q(2, 1, 16, 1, WH); Q(3, 2, 14, 1, BOW2); Q(4, 3, 12, 1, WH);
  Q(1, 6, 2, 11, HA); Q(17, 6, 2, 11, HA);
  Q(1, 8, 1, 8, HL); Q(18, 8, 1, 8, HL);
  Q(2, 5, 1, 7, WH); Q(17, 5, 1, 7, WH);
  Q(2, 5, 1, 1, BOW2); Q(17, 5, 1, 1, BOW2);
  int sway = (frameCounter() / 40) % 2;
  Q(sway ? 1 : 2, 12, 1, 3, WH); Q(sway ? 18 : 17, 12, 1, 3, WH);

  /* 眼 */
  if (mood == 3 || mood == 8) {                 // closed / sleep
    Q(5, 8, 4, 3, SK); Q(11, 8, 4, 3, SK);
    Q(5, 8, 1, 1, OL); Q(6, 9, 2, 1, OL); Q(8, 8, 1, 1, OL);
    Q(11, 8, 1, 1, OL); Q(12, 9, 2, 1, OL); Q(14, 8, 1, 1, OL);
  } else if (mood == 1) {                       // happy
    Q(5, 8, 4, 3, SK); Q(11, 8, 4, 3, SK);
    Q(5, 9, 1, 1, OL); Q(6, 8, 2, 1, OL); Q(8, 9, 1, 1, OL);
    Q(11, 9, 1, 1, OL); Q(12, 8, 2, 1, OL); Q(14, 9, 1, 1, OL);
  } else if (mood == 2 || mood == 4) {          // hurt / sick
    uint16_t et = (mood == 4) ? HEXC(0x6a8a4a) : E2;
    Q(5, 8, 4, 3, SK); Q(11, 8, 4, 3, SK);
    Q(5, 9, 4, 2, E1); Q(5, 9, 4, 1, et);
    Q(11, 9, 4, 2, E1); Q(11, 9, 4, 1, et);
    Q(6, 10, 1, 1, WH); Q(12, 10, 1, 1, WH);
  }
  /* 嘴 */
  if (mood == 1) { Q(8, 12, 4, 1, HEXC(0x8c3a48)); Q(9, 12, 2, 1, HEXC(0xe88a94)); }
  else if (mood == 5) {                          // eat
    Q(9, 12, 2, 1, SK); Q(8, 12, 4, 1, HEXC(0x8c3a48));
    if ((frameCounter() / 8) % 2) Q(8, 13, 4, 1, HEXC(0x8c3a48));
  } else if (mood == 2 || mood == 4) { Q(9, 12, 2, 1, SK); Q(9, 12, 2, 1, HEXC(0x8c3a48)); }
  /* 淡紫面纱（只露双眼） */
  Q(3, 11, 14, 3, HEXC(0xd8c4e8));
  Q(3, 11, 14, 1, HEXC(0xb89ad4));
  Q(3, 13, 14, 1, HEXC(0xc4aae0));
  for (int vx = 5; vx <= 15; vx += 5) Q(vx, 12, 1, 1, HEXC(0xf0e6f8));
  if (mood == 1 || mood == 5) Q(9, 12, 2, 1, HEXC(0x8c3a48));
  /* 白纱结 + 梅花发饰 */
  Q(1, 6, 2, 1, WH); Q(0, 7, 4, 1, WH); Q(0, 8, 4, 1, BOW2);
  Q(1, 7, 2, 1, BOW); Q(1, 8, 1, 1, HEXC(0xc9a8e0));
  Q(15, 2, 1, 1, HEXC(0xe86a9a)); Q(16, 3, 1, 1, HEXC(0xf5a8c8)); Q(15, 4, 1, 1, HEXC(0xe86a9a));
#undef Q
}

static void petBody(int by) {
  const uint16_t ROBE = HEXC(0xf0ecf5), TRIM = HEXC(0xffffff), DARK = HEXC(0xb8a8d0),
                 SK = HEXC(0xfce3d3), GOLD = HEXC(0xe8c860), SKIRT = HEXC(0xc9a8e0),
                 SKIRT2 = HEXC(0xd9c0ea), GAUZE = HEXC(0xf5f0fa);
  P(4, by, 12, 1, TRIM);
  P(8, by + 1, 4, 1, TRIM);
  P(7, by + 2, 1, 1, TRIM); P(12, by + 2, 1, 1, TRIM);
  P(9, by + 3, 2, 1, TRIM);
  P(4, by + 1, 12, 7, ROBE);
  P(4, by + 1, 1, 7, DARK); P(15, by + 1, 1, 7, DARK);
  P(9, by + 6, 2, 1, GOLD);
  P(2, by + 1, 2, 5, ROBE); P(16, by + 1, 2, 5, ROBE);
  P(2, by + 1, 2, 1, TRIM); P(16, by + 1, 2, 1, TRIM);
  P(2, by + 5, 2, 1, TRIM); P(16, by + 5, 2, 1, TRIM);
  P(2, by + 6, 1, 1, SK); P(17, by + 6, 1, 1, SK);
  P(4, by + 8, 12, 1, TRIM);
  P(9, by + 9, 2, 1, GOLD);
  /* 花瓣裙 + 白纱罩裙 */
  P(5, by + 9, 10, 1, SKIRT);
  P(3, by + 10, 14, 1, SKIRT2);
  P(2, by + 11, 16, 1, SKIRT);
  P(4, by + 9, 2, 3, GAUZE); P(14, by + 9, 2, 3, GAUZE);
  P(6, by + 11, 2, 2, HEXC(0x3a3a52)); P(12, by + 11, 2, 2, HEXC(0x3a3a52));
  P(6, by + 13, 2, 1, HEXC(0x23232f)); P(12, by + 13, 2, 1, HEXC(0x23232f));
  /* 药箱 */
  P(1, by + 7, 4, 4, HEXC(0x8a5a34)); P(1, by + 7, 4, 1, HEXC(0xb88a52));
  P(2, by + 8, 2, 2, HEXC(0xf0e6d6)); P(3, by + 8, 1, 2, HEXC(0xc04050)); P(2, by + 9, 2, 1, HEXC(0xc04050));
  P(5, by + 2, 1, 6, HEXC(0xa88c60));
}

static void petUmbrella(int ux, int uy) {
  const uint16_t OUT = HEXC(0x4a3a58), CAN = HEXC(0xd8c2e8), CAN2 = HEXC(0xc4a8dc), RIB = HEXC(0x8f6fae);
  uint16_t sky = gameIsDay() ? HEXC(0x7aa0b8) : HEXC(0x0d0f18);
  static const uint8_t ROWS[8][2] = { {5, 7}, {3, 11}, {2, 13}, {1, 15}, {0, 15}, {1, 14}, {2, 13}, {3, 11} };
  for (int r = 0; r < 8; r++) {
    int rx = ROWS[r][0], rw = ROWS[r][1];
    P(ux + rx, uy + r, rw, 1, (r % 2) ? CAN : CAN2);
    P(ux + rx, uy + r, 1, 1, OUT);
    P(ux + rx + rw - 1, uy + r, 1, 1, OUT);
  }
  /* 右下撕缺一瓣（用天空色挖） */
  P(ux + 13, uy + 4, 3, 1, sky);
  P(ux + 11, uy + 5, 4, 1, sky);
  P(ux + 9, uy + 6, 5, 1, sky);
  P(ux + 7, uy + 7, 5, 1, sky);
  P(ux + 12, uy + 4, 1, 1, OUT); P(ux + 10, uy + 5, 1, 1, OUT); P(ux + 8, uy + 6, 1, 1, OUT);
  /* 伞骨 */
  P(ux + 7, uy + 1, 1, 6, RIB);
  P(ux + 3, uy + 2, 1, 4, RIB); P(ux + 11, uy + 2, 1, 3, RIB);
  P(ux + 1, uy + 4, 1, 2, RIB);
  /* 梅花纹 */
  static const uint8_t BLOSSOM[3][2] = { {5, 3}, {11, 2}, {8, 5} };
  for (int i = 0; i < 3; i++) {
    int bx = BLOSSOM[i][0], by = BLOSSOM[i][1];
    P(ux + bx, uy + by, 1, 1, HEXC(0xf5d0e0));
    P(ux + bx - 1, uy + by, 1, 1, HEXC(0xe86a9a)); P(ux + bx + 1, uy + by, 1, 1, HEXC(0xe86a9a));
    P(ux + bx, uy + by - 1, 1, 1, HEXC(0xe86a9a)); P(ux + bx, uy + by + 1, 1, 1, HEXC(0xe86a9a));
  }
  P(ux + 7, uy + 8, 1, 5, HEXC(0x7a5a3a));
  P(ux + 6, uy + 13, 1, 3, HEXC(0x7a5a3a));
  P(ux + 5, uy + 16, 1, 2, HEXC(0x7a5a3a));
}

void dispPet(uint8_t pose, int ox, int oy) {
  g_ox = ox; g_oy = oy; g_u = 2;
  int bob = (pose == 0) ? ((frameCounter() / 30) % 2) : 0;
  g_oy = oy + bob * g_u;

  if (pose == 8) {                                   // 就寝
    petHead(1, 10, 8);
    P(3, 27, 14, 2, HEXC(0xf0ecf5)); P(2, 29, 16, 2, HEXC(0xffffff));
    return;
  }
  if (pose == 3) {                                   // 重伤
    petHead(1, 3, 2);
    P(4, 20, 12, 7, HEXC(0xf0ecf5)); P(5, 20, 10, 1, HEXC(0xffffff));
    P(4, 27, 12, 2, HEXC(0x3a3a52)); P(4, 29, 12, 1, HEXC(0x23232f));
    int a = frameCounter() / 12;
    dispRect(g_ox + (11 + (int)(cos(a * 0.1f) * 7)) * g_u, g_oy + (1 + (int)(sin(a * 0.1f) * 2)) * g_u,
             3, 3, C_GOLD_HI);
    return;
  }
  if (pose == 2) {                                   // 中毒
    petHead(1, 5, 4);
    P(4, 22, 12, 5, HEXC(0xf0ecf5)); P(5, 22, 10, 1, HEXC(0xffffff));
    P(3, 27, 14, 2, HEXC(0xf0ecf5)); P(4, 29, 12, 1, HEXC(0x3a3a52));
    return;
  }
  if (pose == 5) {                                   // 打坐
    petHead(0, 8, 3);
    P(6, 23, 8, 2, HEXC(0xf0ecf5)); P(6, 23, 8, 1, HEXC(0xffffff));
    P(3, 25, 14, 3, HEXC(0xf0ecf5));
    P(3, 25, 1, 3, HEXC(0xffffff)); P(16, 25, 1, 3, HEXC(0xffffff));
    P(4, 28, 12, 2, HEXC(0x3a3a52));
    return;
  }
  if (pose == 1) {                                   // 吃饭
    petUmbrella(16, 0);
    petHead(0, 0, 5); petBody(17);
    int bo = (frameCounter() / 8) % 2;
    P(19, 22, 6, 3, HEXC(0xd8d8e0)); P(20, 25, 4, 1, HEXC(0xd8d8e0));
    if (bo) { P(20, 19, 2, 2, HEXC(0xe8d8a0)); P(22, 18, 1, 1, HEXC(0xe8d8a0)); }
    return;
  }
  if (pose == 6) {                                   // 打木桩
    int wx = g_ox + 22 * g_u, wy = g_oy + 5 * g_u;
    dispRect(wx, wy + g_u, 7 * g_u, 25 * g_u, HEXC(0x7a5a34));
    dispRect(wx + g_u, wy, 6 * g_u, g_u, HEXC(0x8a6a40));
    dispRect(wx - g_u, wy + 4 * g_u, 9 * g_u, 2 * g_u, HEXC(0x6a4a2a));
    dispRect(wx - g_u, wy + 14 * g_u, 9 * g_u, 2 * g_u, HEXC(0x6a4a2a));
    int punch = (frameCounter() / 10) % 2;
    petHead(0, 0, 1); petBody(17);
    if (punch) { P(16, 19, 7, 2, HEXC(0xfce3d3)); P(22, 19, 2, 2, HEXC(0xffffff)); }
    else { P(16, 21, 5, 2, HEXC(0xfce3d3)); }
    P(15, 18, 3, 2, HEXC(0x8a5a2a)); P(16, 20, 1, 6, HEXC(0xb8c0d0));
    if (punch) dispRect(g_ox + 3 * g_u, g_oy + g_u, 2, 3, HEXC(0x9ad0e8));
    return;
  }
  if (pose == 7) {                                   // 挖宝
    int sw = (frameCounter() / 8) % 2;
    petHead(0, 2, 1); petBody(19);
    if (sw) { P(15, 16, 2, 6, HEXC(0x8a6a40)); P(14, 13, 4, 3, HEXC(0xb8c0d0)); }
    else { P(15, 23, 2, 7, HEXC(0x8a6a40)); P(14, 29, 4, 3, HEXC(0xb8c0d0)); P(6, 30, 8, 2, HEXC(0x5a4a30)); }
    return;
  }
  if (pose == 4) {                                   // 副本
    int mx = g_ox + 21 * g_u + ((frameCounter() / 14) % 2) * g_u, my = g_oy + 3 * g_u;
    dispRect(mx, my + 3 * g_u, 11 * g_u, 7 * g_u, HEXC(0x4a7a3a));
    dispRect(mx + g_u, my, 9 * g_u, 3 * g_u, HEXC(0x4a7a3a));
    dispRect(mx + 2 * g_u, my + g_u, g_u, g_u, HEXC(0xe8d840));
    dispRect(mx + 6 * g_u, my + g_u, g_u, g_u, HEXC(0xe8d840));
    dispRect(mx - g_u, my + g_u, g_u, 2 * g_u, HEXC(0x2e4a24));
    dispRect(mx + 11 * g_u, my + g_u, g_u, 2 * g_u, HEXC(0x2e4a24));
    int slash = (frameCounter() / 6) % 2;
    petHead(0, 0, 1); petBody(17);
    if (slash) { P(16, 19, 7, 2, HEXC(0xffffff)); dispRect(g_ox + 20 * g_u, g_oy + 12 * g_u, 10, 3, HEXC(0xe8f0ff)); }
    else { P(16, 21, 5, 2, HEXC(0xfce3d3)); }
    return;
  }
  /* idle */
  int blink = (frameCounter() % 160) < 8;
  petUmbrella(16, 0);
  petHead(0, 0, blink ? 3 : 0);
  petBody(17);
}
#undef P

/* ================= 稻香村场景 ================= */
#define SX 4
#define SY 27
#define SW 120
#define SH 71
#define GY (SY + SH - 9)

void dispScene(bool night, uint32_t f) {
  bool day = !night;
  /* 外框 */
  dispRect(SX - 2, SY - 2, SW + 4, SH + 4, HEXC(0x20263a));
  dispRect(SX - 1, SY - 1, SW + 2, SH + 2, HEXC(0x39415e));
  dispPixel(SX - 1, SY - 1, HEXC(0x5a6488)); dispPixel(SX + SW, SY - 1, HEXC(0x5a6488));
  dispPixel(SX - 1, SY + SH, HEXC(0x5a6488)); dispPixel(SX + SW, SY + SH, HEXC(0x5a6488));

  /* 天空三段 + 抖动过渡 */
  uint16_t sky0 = day ? HEXC(0x5a86a8) : HEXC(0x0a0c15);
  uint16_t sky1 = day ? HEXC(0x7aa0b8) : HEXC(0x0d0f18);
  uint16_t sky2 = day ? HEXC(0x9ec0c8) : HEXC(0x11131f);
  dispRect(SX, SY, SW, 12, sky0);
  dispRect(SX, SY + 12, SW, 14, sky1);
  dispRect(SX, SY + 26, SW, 10, sky2);
  for (int i = 0; i < 30; i++) dispRect(SX + (i * 9) % SW, SY + 12 + (i % 2), 1, 1, sky0);
  for (int i = 0; i < 26; i++) dispRect(SX + (i * 11 + 5) % SW, SY + 26 + (i % 2), 1, 1, sky1);

  /* 云 */
  uint16_t cT = day ? HEXC(0xf0f4f0) : HEXC(0x1e2438);
  uint16_t cM = day ? HEXC(0xe0e8e4) : HEXC(0x181d30);
  uint16_t cB = day ? HEXC(0xc8d4d0) : HEXC(0x131728);
  struct CloudPos { int x, y; bool big; };
  CloudPos cl[3] = {
    { SX + 12 + (int)fmod(f * 0.15, SW - 34), 26, true },
    { SX + 6  + (int)fmod(f * 0.09, SW - 24), 38, false },
    { SX + 30 + (int)fmod(f * 0.06, SW - 44), 33, false },
  };
  for (int i = 0; i < 3; i++) {
    int w = cl[i].big ? 20 : 14, cx = cl[i].x, cy = cl[i].y;
    dispRect(cx, cy + 2, w, 3, cM);
    dispRect(cx + 2, cy, 6, 3, cM);
    dispRect(cx + 8, cy + 1, 5, 2, cM);
    dispRect(cx + 3, cy, 4, 1, cT);
    dispRect(cx + 9, cy + 1, 3, 1, cT);
    dispRect(cx, cy + 5, w - 2, 1, cB);
  }

  /* 稻香村木牌 */
  dispRect(SX + 8, SY + 3, 34, 11, HEXC(0x5a3a22));
  dispRect(SX + 9, SY + 4, 32, 9, HEXC(0x8a6a40));
  dispRect(SX + 9, SY + 4, 32, 1, HEXC(0xc9a86a));
  dispRect(SX + 9, SY + 12, 32, 1, HEXC(0x3a2418));
  dispText("稻香村", SX + 12, SY + 4, C_GOLD_HI);
  dispRect(SX + 14, SY + 14, 1, 4, HEXC(0x6a4a2a));
  dispRect(SX + 36, SY + 14, 1, 4, HEXC(0x6a4a2a));

  if (!day) {
    static const uint8_t ST[15][2] = {
      {18,24},{30,22},{44,25},{58,21},{70,24},{84,22},{94,27},{26,32},
      {50,30},{78,33},{100,25},{38,38},{64,40},{88,42},{108,32}
    };
    for (int i = 0; i < 15; i++) {
      uint32_t tw = (f + i * 17) % 120;
      int x = ST[i][0], y = ST[i][1];
      if (tw < 80) dispPixel(x, y, HEXC(0xcfd8ea));
      if (tw < 20) {
        dispPixel(x - 1, y, HEXC(0x8a94b8)); dispPixel(x + 1, y, HEXC(0x8a94b8));
        dispPixel(x, y - 1, HEXC(0x8a94b8)); dispPixel(x, y + 1, HEXC(0x8a94b8));
      }
    }
    if (f % 400 > 380) {
      int mp = f % 400 - 380;
      dispPixel(96 + mp, 22 + mp / 2, HEXC(0xe8f0ff));
      dispPixel(97 + mp, 21 + mp / 2, HEXC(0x8a94b8));
    }
    dispRect(100, 22, 12, 12, HEXC(0x2a3350));
    dispRect(102, 24, 8, 8, HEXC(0xd8d8e8));
    dispRect(104, 26, 4, 4, HEXC(0x0d0f18));
    dispRect(103, 25, 2, 1, HEXC(0xf0f0f8));
    dispPixel(107, 30, HEXC(0xb8b8cc));
  } else {
    dispRect(102, 22, 12, 12, HEXC(0xe8d890));
    dispRect(104, 24, 8, 8, HEXC(0xfff5d8));
    dispRect(106, 26, 4, 4, HEXC(0xf5e9b8));
    if (f % 60 < 45) {
      dispRect(107, 20, 2, 2, HEXC(0xf0e2a0)); dispRect(107, 34, 2, 2, HEXC(0xf0e2a0));
      dispRect(100, 27, 2, 2, HEXC(0xf0e2a0)); dispRect(114, 27, 2, 2, HEXC(0xf0e2a0));
    }
    int bx = (int)fmod(f * 0.35, 170) - 25;
    int by = 28 + (int)(sinf(f / 24.0f) * 3);
    int wf = (f / 8) % 2;
    dispPixel(bx, by, HEXC(0x1c2438));
    dispPixel(bx + 2, by, HEXC(0x1c2438));
    dispPixel(bx + 1, by - wf, HEXC(0x1c2438));
    dispPixel(bx + 14, by + 4, HEXC(0x1c2438));
    dispPixel(bx + 16, by + 4, HEXC(0x1c2438));
    dispPixel(bx + 15, by + 4 - wf, HEXC(0x1c2438));
  }

  /* 远山 + 梯田 + 远村 */
  uint16_t mtnF = day ? HEXC(0x5a7e6a) : HEXC(0x141c28);
  uint16_t mtnN = day ? HEXC(0x4a6a58) : HEXC(0x0e1520);
  dispRect(SX, 60, SW, GY - 60, mtnF);
  dispRect(SX + 8, 52, 30, GY - 52, mtnF);
  dispRect(SX + 60, 54, 46, GY - 54, mtnF);

  for (int i = 0; i < 4; i++) {
    int ty = 64 + i * 5, tw2 = 96 - i * 10, tx = SX + 8 + i * 6;
    dispRect(tx, ty, tw2, 1, day ? HEXC(0x3e5a34) : HEXC(0x101a14));
    dispRect(tx + 1, ty + 1, tw2 - 2, 3, day ? HEXC(0x7a9e5a) : HEXC(0x1c2820));
    dispRect(tx + 1, ty + 1, tw2 - 2, 1, day ? HEXC(0x9ebb68) : HEXC(0x243226));
    dispRect(tx + 1, ty + 3, tw2 - 2, 1, day ? HEXC(0x668848) : HEXC(0x162018));
    for (int sp = 12; sp < tw2 - 10; sp += 18) {
      dispRect(tx + sp, ty, 1, 2, day ? HEXC(0xc8d878) : HEXC(0x2c3a28));
      dispRect(tx + sp + 1, ty - 1, 1, 2, day ? HEXC(0xd8e898) : HEXC(0x344430));
    }
  }
  /* 远村茅屋 */
  static const uint8_t VH[2][2] = { {26, 66}, {84, 64} };
  for (int i = 0; i < 2; i++) {
    int hx = VH[i][0], hy = VH[i][1];
    dispRect(hx + 2, hy, 8, 4, day ? HEXC(0x8a7a5a) : HEXC(0x1a1e28));
    dispRect(hx + 5, hy + 1, 2, 3, day ? HEXC(0x4a3a2a) : HEXC(0x0d1218));
    dispRect(hx, hy - 2, 12, 2, day ? HEXC(0x6a8a5a) : HEXC(0x121820));
    dispRect(hx + 2, hy - 4, 8, 2, day ? HEXC(0x7a9e6a) : HEXC(0x16201c));
    dispRect(hx + 4, hy - 5, 4, 1, day ? HEXC(0x8eae7a) : HEXC(0x1a2420));
    if (day && f % 80 < 50)
      dispPixel(hx + 6 + (int)((f / 40) % 2), hy - 8 - (int)((f / 60) % 3), HEXC(0xd8e0dc));
  }

  /* 近景矮丘 */
  dispRect(SX - 2, 74, 34, GY - 74, mtnN); dispRect(SX + 6, 68, 16, GY - 68, mtnN);
  dispRect(SX + 36, 78, 26, GY - 78, mtnN); dispRect(SX + 44, 72, 12, GY - 72, mtnN);
  dispRect(SX + 62, 76, 46, GY - 76, mtnN); dispRect(SX + 72, 70, 16, GY - 70, mtnN);

  /* 地面 */
  dispRect(SX, GY, SW, 3, HEXC(0x3a5a34));
  dispRect(SX, GY + 3, SW, 1, HEXC(0x2a3d2a));
  dispRect(SX, GY + 4, SW, 5, HEXC(0x3a2f22));
  dispRect(SX, GY + 6, SW, 1, HEXC(0x2e2418));
  for (int i = 0; i < 8; i++) dispRect(SX + (i * 23 + 8) % SW, GY + 5 + (i % 2), 2, 1, HEXC(0x4a3c2a));
  for (int i = 0; i < 16; i++) {
    int gx = SX + (i * 9 + 3) % SW;
    int gw = (int)((f + i * 7) / 30) % 2;
    dispRect(gx, GY - 1 - gw, 1, 1 + gw, (i % 3) ? HEXC(0x4a6e40) : HEXC(0x5a7e4a));
  }
  /* 小花 + 石径 */
  static const uint8_t FL[4] = { 26, 46, 90, 110 };
  for (int i = 0; i < 4; i++) {
    int fx = FL[i];
    uint16_t fc = (i == 0) ? HEXC(0xe86a6a) : (i == 1) ? HEXC(0xf5d858) : (i == 2) ? HEXC(0xe88ab8) : HEXC(0xe86a6a);
    dispRect(fx + 1, GY - 3, 1, 3, HEXC(0x3a5a34));
    dispRect(fx, GY - 5, 3, 2, fc);
    dispPixel(fx + 1, GY - 4, HEXC(0xf5e9b8));
  }
  static const uint8_t PATH[3][2] = { {24, 1}, {32, 2}, {42, 1} };
  for (int i = 0; i < 3; i++) {
    dispRect(PATH[i][0], GY + PATH[i][1], 4, 2, HEXC(0x7a6a4c));
    dispRect(PATH[i][0], GY + PATH[i][1], 4, 1, HEXC(0x8a7a5a));
  }

  /* 大水车 */
  int wx = SX + 99, wy = GY - 20, wr = 15;
  float wa = f / 110.0f;
  dispRect(wx - 18, wy + 8, 20, 2, day ? HEXC(0x6a4a2a) : HEXC(0x2a2018));
  dispRect(wx - 18, wy + 10, 2, GY - wy - 10, day ? HEXC(0x5a3a22) : HEXC(0x201810));
  dispRect(wx + 12, wy + 6, 2, GY - wy - 6, day ? HEXC(0x5a3a22) : HEXC(0x201810));
  for (int i = 0; i < 8; i++) {
    float a2 = wa + i * 0.7853982f;
    int ex = wx + (int)(cosf(a2) * wr), ey = wy + (int)(sinf(a2) * wr);
    for (int k = 0; k <= 11; k++)
      dispRect(wx + (ex - wx) * k / 11, wy + (ey - wy) * k / 11, 2, 2,
               day ? HEXC(0x7a5a34) : HEXC(0x2e2418));
    dispRect(ex - 2, ey - 1, 5, 3, day ? HEXC(0x5a3a22) : HEXC(0x201810));
    dispRect(ex - 1, ey, 3, 1, day ? HEXC(0xc9a86a) : HEXC(0x4a3a2a));
  }
  for (int i = 0; i < 30; i++) {
    float a2 = i * 0.2094395f;
    dispRect(wx + (int)(cosf(a2) * wr), wy + (int)(sinf(a2) * wr), 2, 2,
             day ? HEXC(0xb08a52) : HEXC(0x403224));
  }
  dispRect(wx - 3, wy - 3, 7, 7, day ? HEXC(0x5a3a22) : HEXC(0x201810));
  dispRect(wx - 2, wy - 2, 5, 5, day ? HEXC(0x8a6a40) : HEXC(0x342a20));
  dispRect(wx - 1, wy - 1, 3, 3, day ? HEXC(0xc9a86a) : HEXC(0x4a3a2a));

  /* 溪流 + 拱桥 */
  uint16_t water = day ? HEXC(0x5a9eb8) : HEXC(0x1a3448);
  uint16_t wave = day ? HEXC(0x8ac8d8) : HEXC(0x2a5068);
  dispRect(SX + 2, GY + 2, 118, 6, water);
  dispRect(SX + 2, GY + 2, 118, 1, wave);
  for (int i = 0; i < 11; i++) {
    int xx = SX + 6 + (int)((i * 9 + (f / 6) % 9) % 108);
    dispRect(xx, GY + 3 + (i % 3), 4, 1, wave);
  }
  int bx0 = SX + 24, by0 = GY - 1;
  for (int i = 0; i < 13; i++) {
    int arch = (int)(sinf(i / 12.0f * PI) * 3);
    dispRect(bx0 + i * 3, by0 - arch, 4, 2, day ? HEXC(0x8a6a40) : HEXC(0x342a20));
    dispRect(bx0 + i * 3, by0 - arch - 1, 4, 1, day ? HEXC(0xb08a52) : HEXC(0x4a3a2a));
    if (i % 4 == 0) dispRect(bx0 + i * 3, by0 - arch - 4, 1, 4, day ? HEXC(0x6a4a2a) : HEXC(0x2a2018));
  }
  dispRect(bx0, by0 - 5, 39, 1, day ? HEXC(0x6a4a2a) : HEXC(0x2a2018));

  /* 荷花池 */
  dispRect(SX + 4, GY + 2, 20, 6, day ? HEXC(0x4a8a9e) : HEXC(0x142c3c));
  static const uint8_t LOTUS[3][2] = { {7, 3}, {15, 5}, {21, 2} };
  for (int i = 0; i < 3; i++) {
    int lx = LOTUS[i][0], ly = LOTUS[i][1];
    dispRect(SX + lx, GY + ly, 7, 2, day ? HEXC(0x4a8a52) : HEXC(0x1a3428));
    dispPixel(SX + lx + 3, GY + ly - 1, day ? HEXC(0x6aa862) : HEXC(0x244436));
    if (i != 1) {
      dispRect(SX + lx + 2, GY + ly - 3, 2, 2, HEXC(0xe88ab8));
      dispPixel(SX + lx + 3, GY + ly - 4, HEXC(0xf5c8d8));
      dispPixel(SX + lx + 1, GY + ly - 2, HEXC(0xd86a9a));
    }
  }
  /* 草丛 */
  static const uint8_t GS[2][2] = { {48, 3}, {88, 2} };
  for (int i = 0; i < 2; i++) {
    int gx = GS[i][0], gy = GY - GS[i][1];
    dispRect(gx, gy, 5, 3, HEXC(0x2e4a28));
    dispRect(gx + 1, gy - 1, 3, 1, HEXC(0x2e4a28));
    dispPixel(gx + 1, gy, HEXC(0x3f6a38));
    dispPixel(gx + 3, gy + 1, HEXC(0xc04858));
  }
  /* 左侧老树 */
  dispRect(9, 78, 3, GY - 78, HEXC(0x5a4632));
  dispRect(9, 78, 1, GY - 78, HEXC(0x6a5438));
  dispRect(4, 70, 13, 8, HEXC(0x2e4a28));
  dispRect(6, 67, 9, 4, HEXC(0x2e4a28));
  dispRect(3, 72, 15, 4, HEXC(0x2e4a28));
  dispRect(6, 68, 4, 2, HEXC(0x3f6a38));
  dispRect(11, 71, 3, 2, HEXC(0x3f6a38));
  dispRect(5, 76, 2, 1, HEXC(0xc04858));
  dispRect(12, 74, 2, 1, HEXC(0xc04858));

  /* 夜间流萤 */
  if (!day) {
    for (int i = 0; i < 3; i++) {
      int fx = SX + 24 + (int)((sinf(f / 40.0f + i * 2.1f) * 0.5f + 0.5f) * 72);
      int fy = GY - 16 + (int)(sinf(f / 30.0f + i * 1.7f) * 6);
      if ((f + i * 23) % 60 < 40) dispPixel(fx, fy, HEXC(0xd8e88a));
    }
  }
}
#undef SX
#undef SY
#undef SW
#undef SH
#undef GY

/* 家园小屋（随等级变大） */
void dispHouse(int x0, int y0, bool night) {
  uint8_t lv = G.homeLv;
  if (lv == 0) return;
  int w = 12 + lv * 5, h = 7 + lv * 3;
  uint16_t wall = lv >= 3 ? HEXC(0x8a6a44) : HEXC(0x6a5a3a);
  uint16_t wallD = lv >= 3 ? HEXC(0x6e5434) : HEXC(0x544730);
  uint16_t roofC = lv >= 4 ? HEXC(0xd8c68a) : (lv >= 2 ? HEXC(0xa04030) : HEXC(0x4a3a26));
  uint16_t roofD = lv >= 4 ? HEXC(0xb8a468) : (lv >= 2 ? HEXC(0x7e3026) : HEXC(0x3a2e1e));
  dispRect(x0 + 1, y0 - h, w, h, wall);
  for (int i = 2; i < w - 1; i += 3) dispRect(x0 + 1 + i, y0 - h + 1, 1, h - 2, wallD);
  dispRect(x0 + 1, y0 - 1, w, 1, wallD);
  for (int i = 0; i < 3; i++) dispRect(x0 - 2 + i, y0 - h - 3 + i, w + 4 - i * 2, 1, roofC);
  dispRect(x0 + 2, y0 - h - 1, w - 4, 1, roofD);
  dispRect(x0 - 3, y0 - h - 4, w + 6, 1, roofC);
  dispRect(x0 + w / 2 - 2, y0 - h - 4, 2, 1, lv >= 4 ? HEXC(0xf0e2a0) : C_GOLD);
  dispRect(x0 + w / 2 - 1, y0 - 5, 3, 5, HEXC(0x2e2519));
  dispPixel(x0 + w / 2 + 1, y0 - 3, HEXC(0xc9b070));
  if (w >= 14) {
    dispRect(x0 + 2, y0 - h + 2, 3, 3, night ? HEXC(0xf0d878) : HEXC(0x3a3a52));
    dispRect(x0 + 2, y0 - h + 2, 3, 1, HEXC(0x5a4632));
  }
  if (lv >= 3) { dispRect(x0 + w - 3, y0 - h - 7, 1, 4, HEXC(0x5a4a32)); dispRect(x0 + w - 2, y0 - h - 7, 4, 2, C_GOLD); }
  if (night && lv >= 2) {
    dispRect(x0 + w - 1, y0 - h + 1, 1, 2, HEXC(0x5a4a32));
    dispRect(x0 + w, y0 - h + 3, 2, 3, HEXC(0xd85a4a));
    dispRect(x0 + w, y0 - h + 4, 2, 1, HEXC(0xf0a060));
  }
}

void dispMess(uint8_t n) {
  for (int i = 0; i < n; i++) {
    int x = 50 + i * 15;
    dispRect(x, 91, 8, 4, HEXC(0x6a5a3a));
    dispRect(x + 1, 89, 6, 2, HEXC(0x8a7a52));
    dispRect(x + 2, 87, 3, 2, HEXC(0x8a7a52));
  }
}

/* ================= 八色卡校准 ================= */
static void waitRel(uint8_t pin, uint32_t maxMs = 4000) {
  uint32_t t = millis();
  while (digitalRead(pin) == LOW && millis() - t < maxMs) delay(10);
}

static void drawCard() {
  canvas.fillScreen(C_BG);
  canvas.fillRect(0, 0, SCR_W, 14, C_PANEL);
  canvas.drawFastHLine(0, 14, SCR_W, C_GOLD);
  dispText(dispModeName(dispMode), 4, 1, C_GOLD_HI);
  static const uint16_t SW8[8] = { 0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F, 0xFFFF, 0x0000 };
  for (int i = 0; i < 8; i++) {
    int cx = 5 + (i % 4) * 31, cy = 17 + (i / 4) * 30;
    canvas.fillRect(cx, cy, 26, 26, SW8[i]);
    canvas.drawRect(cx, cy, 26, 26, C_DIM);
  }
  for (int i = 0; i < 9; i++) {
    uint8_t v = (uint8_t)(i * 255 / 8);
    uint16_t g = (uint16_t)(((v * 31 / 255) << 11) | ((v * 63 / 255) << 5) | (v * 31 / 255));
    canvas.fillRect(2 + i * 14, 78, 13, 9, g);
  }
  canvas.drawFastHLine(0, 89, SCR_W, C_DARK);
  dispTextC("A 确定此色彩", 64, 93, C_TEXT);
  dispTextC("▲ 换档   B 退出", 64, 108, C_DIM);
  dispFlush();
}

void dispCalibrate() {
  for (;;) {
    drawCard();
    delay(60);
    if (digitalRead(PIN_K1) == LOW) { dispSetMode((dispMode + 1) & 0x03, false); waitRel(PIN_K1); continue; }
    if (digitalRead(PIN_K3) == LOW) { dispSetMode(dispMode, true); waitRel(PIN_K3); return; }
    if (digitalRead(PIN_K4) == LOW) { waitRel(PIN_K4); return; }
  }
}
