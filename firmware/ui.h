/* ui.h — 页面状态与渲染 */
#ifndef JX3_UI_H
#define JX3_UI_H

#include <Arduino.h>
#include "config.h"

struct UIState {
  uint8_t page;          // PageId
  uint8_t menuIdx;       // 主页 0..8（8=邮箱）
  uint8_t catIdx;        // 分类 0..5
  bool    catMode;       // 分类栏激活（v8.1 两级导航）
  uint8_t listSel;       // 列表行选中
  uint8_t itemFrom;      // 0=背包 1=商城
  uint8_t homeSel;
  uint8_t dailySel;
  uint8_t setIdx;
  uint8_t setPage;       // 0=列表 1=流速 2=音效 3=存档 4=清档 5=关于
  uint8_t settingsFrom;  // 长按 B 前的页面
  uint8_t confirmSel;    // 清档确认 0=否 1=是
  uint8_t mailSel;       // 邮箱选中
  bool    mailDetail;    // 邮箱详情
  uint16_t frame;
};
extern UIState U;

/* 过滤后的列表：返回数量，idx 写入物品索引数组 */
int  uiFiltered(bool shop, uint8_t* out, int maxN);
void uiRender();
void uiToastOverlay();

/* 页面切换辅助 */
void uiGoto(uint8_t page);
void uiDoMenu();
void uiItemAction();

#endif /* JX3_UI_H */
