/* net.cpp — WiFi + NTP（可选；留空凭据则完全离线运行） */
#include "net.h"
#include "game.h"
#include <WiFi.h>
#include <time.h>

bool NET_ONLINE = false;
char NET_SSID[32] = "";

#if __has_include("wifi_credentials.h")
  #include "wifi_credentials.h"
#else
  #define WIFI_SSID ""
  #define WIFI_PASS ""
#endif

void netInit() {
  if (!WIFI_SSID || !WIFI_SSID[0]) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) delay(200);
  NET_ONLINE = (WiFi.status() == WL_CONNECTED);
  if (NET_ONLINE) {
    strncpy(NET_SSID, WIFI_SSID, sizeof(NET_SSID) - 1);
    netSyncClock();
  }
}

void netSyncClock() {
  if (!NET_ONLINE) return;
  /* UTC+8，NTP 服务器用国内可达的阿里云 */
  configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp1.aliyun.com");
  time_t now = 0;
  for (int i = 0; i < 20 && now < 1700000000L; i++) {
    delay(250);
    time(&now);
  }
  if (now >= 1700000000L) setClock((uint32_t)now);
}
