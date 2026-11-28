/* 剑侠情缘·单机版三 —— ESP32-C3 + ST7735 128×128
 * 与网页版 0.2.11 一比一：同一套精灵、同一套数据表、同一套页面与玩法。
 *
 * 接线见 README.md。改中文文案后运行 python tools/gen_font.py 重新生成字库。
 */
#include <Arduino.h>
#include "config.h"
#include "game.h"
#include "display.h"
#include "ui.h"
#include "input.h"

void setup() {
  /* GPIO20/21 被用作 K3/K4，串口不绑定引脚，否则 UART 会抢走按键 */
  Serial.begin(115200, SERIAL_8N1, -1, -1);
  delay(50);
  randomSeed(esp_random());

  gameInit();
  dispInit();
  uiInit();
  inpInit();

  if (!G_hasSave) {
    gameNew();
    UI.page = PG_BOOT;
  } else {
    UI.page = PG_MAIN;
  }
  Serial.printf("JX3 Tamagotchi %s | save=%d\n", APP_VERSION, G_hasSave ? 1 : 0);
}

void loop() {
  static uint32_t lastTick = 0, lastFrame = 0, lastSave = 0;
  uint32_t now = millis();

  inpPoll();

  if (now - lastTick >= TICK_MS) { lastTick = now; uiTick(); }
  if (now - lastFrame >= FRAME_MS) {
    lastFrame = now;
    frameTick();
    uiFrameTick();
    uiRender();
    dispFlush();
  }
  if (now - lastSave >= SAVE_INTERVAL_MS) { lastSave = now; gameSave(); }

  delay(1);
}
