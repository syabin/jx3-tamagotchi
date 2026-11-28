/* input.h — 4 键输入（消抖 / 长按连滚 / 长按 B 呼出设置） */
#ifndef JX3_INPUT_H
#define JX3_INPUT_H
#include <Arduino.h>

void inpInit();
void inpPoll();     // 建议每 10ms 调用一次
void inpGambleTick();

#endif
