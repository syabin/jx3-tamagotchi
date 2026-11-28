/* net.h — 可选 WiFi / NTP 校时（不联网也能完整游玩） */
#ifndef JX3_NET_H
#define JX3_NET_H
#include <Arduino.h>

extern bool NET_ONLINE;
extern char NET_SSID[32];

void netInit();          // 有凭据才连，最多等 8 秒，失败静默继续
void netSyncClock();     // NTP 校准（影响昼夜与每日签到）

#endif
