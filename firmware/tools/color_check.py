#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""color_check.py — 固件颜色 ↔ 网页色 一致性核对

用途：排查"屏幕有色差但 logo 没色差"这类问题。
原理：
  1. 扫 firmware/config.h（以及 *.cpp/*.h 里的 HEXC(0xRRGGBB)），
     把每个颜色常量按 C 宏 HEXC 换成 RGB565，再反推回 RGB888；
  2. 从 index.html 提取全部 #rrggbb 网页色，为每个固件色找最近的网页色；
  3. 偏差 > TOL 的标 ⚠，说明这个色是"自己编的"而不是网页原色。

用法：python tools/color_check.py
"""
import re
import sys
import pathlib

HERE = pathlib.Path(__file__).resolve().parent
FW = HERE.parent
ROOT = FW.parent
TOL = 12          # 每通道允许偏差（565 量化误差约 ±4~8）


def hexc(v: int) -> int:
    """与 config.h 的 HEXC() 宏逐位一致。"""
    r = (v >> 16) & 0xF8
    g = (v >> 8) & 0xFC
    b = v & 0xF8
    return ((r << 8) | (g << 3) | (b >> 3)) & 0xFFFF


def back565(c: int):
    """RGB565 → RGB888（按位复制低位，和屏的还原方式一致）。"""
    r5 = (c >> 11) & 0x1F
    g6 = (c >> 5) & 0x3F
    b5 = c & 0x1F
    return ((r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2))


def dist(a, b) -> int:
    return max(abs(a[i] - b[i]) for i in range(3))


def load_web_colors():
    """index.html 里出现的全部 #rrggbb。"""
    txt = (ROOT / "index.html").read_text(encoding="utf-8")
    out = {}
    for m in re.finditer(r"#([0-9a-fA-F]{6})\b", txt):
        v = int(m.group(1), 16)
        out[m.group(1).lower()] = v
    return out


def load_fw_colors():
    """返回 [(来源:行, 名字, 源色hex 或 None, 565值)]"""
    found = []
    for f in ["config.h", "display.cpp", "ui.cpp"]:
        p = FW / f
        if not p.exists():
            continue
        for i, line in enumerate(p.read_text(encoding="utf-8").splitlines(), 1):
            # 常量名 = HEXC(0xRRGGBB)
            m = re.search(r"(C_[A-Z0-9_]+)\s*=\s*HEXC\(0x([0-9a-fA-F]{6})\)", line)
            if m:
                src = int(m.group(2), 16)
                found.append((f"{f}:{i}", m.group(1), src, hexc(src)))
                continue
            # 常量名 = 0xNNNN  （裸 565）
            m = re.search(r"(C_[A-Z0-9_]+)\s*=\s*0x([0-9a-fA-F]{4})\s*;", line)
            if m:
                found.append((f"{f}:{i}", m.group(1), None, int(m.group(2), 16)))
                continue
            # 行内 HEXC(0xRRGGBB) 直接当参数用
            for m in re.finditer(r"HEXC\(0x([0-9a-fA-F]{6})\)", line):
                src = int(m.group(1), 16)
                found.append((f"{f}:{i}", "HEXC()", src, hexc(src)))
    return found


def main():
    web = load_web_colors()
    fw = load_fw_colors()
    print(f"网页色 {len(web)} 个 / 固件色 {len(fw)} 处\n")

    bad = 0
    seen = set()
    for loc, name, src, c565 in fw:
        key = (name, src, c565)
        if key in seen:
            continue
        seen.add(key)
        r, g, b = back565(c565)
        got = (r << 16) | (g << 8) | b
        # 找最近的网页色
        best, bestd = None, 999
        for wh, wv in web.items():
            d = dist(((wv >> 16) & 255, (wv >> 8) & 255, wv & 255), (r, g, b))
            if d < bestd:
                best, bestd = wh, d
        flag = "  " if bestd <= TOL else "⚠ "
        if bestd > TOL:
            bad += 1
        # 如果源色本身在网页里存在，直接报它与还原色的差
        if src is not None:
            srccol = ((src >> 16) & 255, (src >> 8) & 255, src & 255)
            dsrc = dist(srccol, (r, g, b))
            note = f"源#{src:06x} 还原#{got:06x} Δ{dsrc}"
        else:
            note = f"裸565 0x{c565:04x} → #{got:06x}"
        print(f"{flag}{name:<10} {loc:<14} {note:<40} 最近网页色 #{best} Δ{bestd}")

    print(f"\n不合网页原色的处数：{bad}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
