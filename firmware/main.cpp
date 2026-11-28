/* main.cpp — ESP32-C3 拓麻歌子固件入口 */
#include <Arduino.h>
#include "config.h"
#include "game.h"
#include "display.h"
#include "ui.h"
#include "input.h"
#include "net.h"

void setup() {
  /* 关键：GPIO20/21 是 C3 的 UART0 RX/TX，而我们拿它们当 K3/K4 按键。
   * 所以串口显式不绑定任何引脚（-1,-1），否则 UART 会把 20/21 抢走导致按键失灵。 */
  Serial.begin(115200, SERIAL_8N1, -1, -1);
  delay(50);
  randomSeed((unsigned long)esp_random());

  netInit();        // 没填 WiFi 凭据时立即返回，不影响后续
  gameInit();
  dispInit();
  inpInit();

  Serial.printf("JX3 Tamagotchi FW %s | save=%d\n", APP_VERSION, G_hasSave ? 1 : 0);
}

void loop() {
  static uint32_t lastTick = 0, lastFrame = 0, lastSave = 0;
  uint32_t now = millis();

  inpPoll();
  inpGambleTick();

  if (now - lastTick >= TICK_MS) { lastTick = now; gameTick(); }
  if (now - lastFrame >= FRAME_MS) { lastFrame = now; uiRender(); dispFlush(); }
  if (now - lastSave >= SAVE_INTERVAL_MS) { lastSave = now; gameSave(); }

  yield();
}
