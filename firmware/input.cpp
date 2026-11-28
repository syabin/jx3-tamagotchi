/* input.cpp — 4 键输入：▲ ▼ A B
 * ▲▼ 左右选择（带长按连发）；A 确认；B 短按返回 / 长按 1.5 秒进设置；
 * 主界面长按 ▲ 8 秒进入八色卡校色。
 */
#include "input.h"
#include "ui.h"
#include "display.h"

struct Btn {
  uint8_t pin;
  bool stable;         // 去抖后状态（true = 按下）
  bool lastRaw;
  uint32_t tChange;
  uint32_t tDown;
  uint32_t tRepeat;
  bool holdFired;
};

static Btn BTN_U = { PIN_K1, false, false, 0, 0, 0, false };
static Btn BTN_D = { PIN_K2, false, false, 0, 0, 0, false };
static Btn BTN_A = { PIN_K3, false, false, 0, 0, 0, false };
static Btn BTN_B = { PIN_K4, false, false, 0, 0, 0, false };

static bool rawDown(uint8_t pin) { return digitalRead(pin) == LOW; }

void inpInit() {
  pinMode(PIN_K1, INPUT_PULLUP);
  pinMode(PIN_K2, INPUT_PULLUP);
  pinMode(PIN_K3, INPUT_PULLUP);
  pinMode(PIN_K4, INPUT_PULLUP);
}

bool inpAnyPressed() { return rawDown(PIN_K1) || rawDown(PIN_K2) || rawDown(PIN_K3) || rawDown(PIN_K4); }

/* 返回 -1 松开 / 0 无 / 1 按下 */
static int step(Btn& b, uint32_t now) {
  bool raw = rawDown(b.pin);
  if (raw != b.lastRaw) { b.lastRaw = raw; b.tChange = now; return 0; }
  if (now - b.tChange < KEY_DEBOUNCE_MS || raw == b.stable) return 0;
  b.stable = raw;
  if (raw) { b.tDown = now; b.tRepeat = now + KEY_REPEAT_WAIT; b.holdFired = false; return 1; }
  return -1;
}

void inpPoll() {
  uint32_t now = millis();
  int e1 = step(BTN_U, now);
  int e2 = step(BTN_D, now);
  int e3 = step(BTN_A, now);
  int e4 = step(BTN_B, now);

  /* ▲ ▼：按下即动，长按连发 */
  if (e1 == 1) { uiMoveSel(-1); BTN_U.tRepeat = now + KEY_REPEAT_WAIT; }
  else if (BTN_U.stable) {
    if ((int32_t)(now - BTN_U.tRepeat) >= 0) { uiMoveSel(-1); BTN_U.tRepeat = now + KEY_REPEAT_RATE; }
    if (UI.page == PG_MAIN && !BTN_U.holdFired && now - BTN_U.tDown > 8000) {
      BTN_U.holdFired = true;
      dispCalibrate();
    }
  }

  if (e2 == 1) { uiMoveSel(1); BTN_D.tRepeat = now + KEY_REPEAT_WAIT; }
  else if (BTN_D.stable && (int32_t)(now - BTN_D.tRepeat) >= 0) {
    uiMoveSel(1);
    BTN_D.tRepeat = now + KEY_REPEAT_RATE;
  }

  /* A：按下沿触发一次 */
  if (e3 == 1) uiConfirmA();

  /* B：短按返回，长按 1.5 秒进设置（松手不再触发返回） */
  if (e4 == 1) { BTN_B.tDown = now; BTN_B.holdFired = false; }
  if (BTN_B.stable && !BTN_B.holdFired && now - BTN_B.tDown >= B_HOLD_MS) {
    BTN_B.holdFired = true;
    uiLongB();
  }
  if (e4 == -1) {
    if (!BTN_B.holdFired && BTN_B.tDown && now - BTN_B.tDown < B_HOLD_MS) uiBack();
    BTN_B.holdFired = false;
  }
}
