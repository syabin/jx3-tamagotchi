#pragma once
#include <Arduino.h>
#include "config.h"

void dispInit();
void dispFlush();
void dispClear(uint16_t c = C_BG);
void dispRect(int x, int y, int w, int h, uint16_t c);
void dispFrame(int x, int y, int w, int h, uint16_t c);
void dispPixel(int x, int y, uint16_t c);

/* 文字：汉字 12×12 点阵，ASCII 5×7（宽 6）。y 为顶部基线，与网页 text() 一致 */
int  textW(const char* s);
void dispText(const char* s, int x, int y, uint16_t c);
void dispTextC(const char* s, int cx, int y, uint16_t c);
void dispTextR(const char* s, int rx, int y, uint16_t c);
void dispTextClip(const char* s, int x, int y, int w, uint16_t c, int offsetPx);
int  dispWrap(const char* s, int maxChars, const char** out, int maxLines);

void dispSpr(uint8_t id, int x, int y);            // 16×16 精灵
void dispSprS(uint8_t id, int x, int y, uint8_t size);  // 缩放到 size×size（最近邻）
void dispIcon(uint8_t sprId, int x, int y, bool sel);  // 圆牌 + 精灵
void dispPet(uint8_t pose, int ox, int oy);        // 0 idle 1 eat 2 sick 3 down 4 dungeon 5 sit 6 train 7 dig 8 sleep
void dispScene(bool night, uint32_t frame);
void dispHouse(int x0, int y0, bool night);
void dispMess(uint8_t n);
void dispSelFrame(int x, int y, int w, int h);
void dispTri(int x, int y, int dir, bool on);      // 左右切换三角

uint8_t dispGetMode();
void    dispSetMode(uint8_t m, bool persist);
void    dispCycleMode();
const char* dispModeName(uint8_t m);
void    dispCalibrate();
