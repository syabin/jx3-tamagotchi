/* display.h — ST7735S 绘制层（点阵中文 / 图标 / 像素小人 / 场景） */
#ifndef JX3_DISPLAY_H
#define JX3_DISPLAY_H

#include <Arduino.h>
#include "config.h"

void dispInit();
void dispFlush();                 // 把整帧缓存推给屏幕（每帧调用一次）
void dispClear(uint16_t c = C_BG);
void dispRect(int x, int y, int w, int h, uint16_t c);
void dispFrame(int x, int y, int w, int h, uint16_t c);
void dispPixel(int x, int y, uint16_t c);
void dispBegin();
void dispEnd();

/* 显示模式（色序 / 反显）—— 长按 ▲ 循环切换 */
uint8_t dispGetMode();
void    dispSetMode(uint8_t m, bool persist);
void    dispCycleMode();
const char* dispModeName(uint8_t m);
void    dispCalibrate();                 // 八色卡校准模式（阻塞，按 A 保存 / B 退出）

int  textWidth(const char* s);
void dispText(const char* s, int x, int y, uint16_t c);
void dispTextC(const char* s, int cx, int y, uint16_t c);
void dispTextR(const char* s, int rx, int y, uint16_t c);
void dispNum(int n, int x, int y, uint16_t c);          // 数字右对齐辅助
void dispBar(int x, int y, int w, int h, int val, uint16_t c);
void dispSelFrame(int x, int y, int w, int h);

/* 8 个菜单图标（暗色圆牌 + 金环 + 金色纹样） */
void dispIcon(uint8_t id, int x, int y, bool selected);

/* 物品图：s=1 缩略（12px），s=3/4 详情大图 */
void dispItemIcon(uint8_t idx, int x, int y, uint8_t s);

/* 像素小人：pose 0=idle 1=eat 2=sick 3=down 4=dungeon 5=sit 6=train 7=dig 8=sleep */
void dispPet(int x, int y, uint8_t pose, uint8_t sectIdx);

/* 场景 */
void dispScene(bool night, uint16_t frame);
void dispHouse(int x0, int y0);
void dispDecos(int gy);

#endif /* JX3_DISPLAY_H */
