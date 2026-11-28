#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""从固件源码里扫描所有可见字符，生成 12x12 点阵字库 font_cn.h。

用法：python tools/gen_font.py
改了 config.h / ui.cpp 里的中文后重新跑一次即可。
字体优先 simhei.ttf（黑体，小字号最清晰），其次 msyh.ttc。
"""
import os
import re
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "font_cn.h")

W, H = 12, 12
BYTES = 24          # 12 行 x 2 字节
FONTS = [
    r"C:\Windows\Fonts\simhei.ttf",
    r"C:\Windows\Fonts\msyh.ttc",
    r"C:\Windows\Fonts\simsun.ttc",
]

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    raise SystemExit("需要 Pillow：python -m pip install pillow")

font_path = next((p for p in FONTS if os.path.exists(p)), None)
if not font_path:
    raise SystemExit("找不到中文字体")

font = ImageFont.truetype(font_path, 12, index=0)

# 扫描源码中的中文与常用符号；ASCII 直接按 32..126 全量生成
chars = set(chr(c) for c in range(32, 127))
for name in os.listdir(ROOT):
    if not name.endswith((".h", ".cpp")):
        continue
    with open(os.path.join(ROOT, name), "r", encoding="utf-8", errors="ignore") as f:
        for ch in f.read():
            if ord(ch) > 126 and not ch.isspace():
                chars.add(ch)
# 常用标点兜底
chars.update("·—…×℃？！：，。、（）【】《》「」")


def glyph(ch):
    """返回 24 字节：每行两字节，高 12 位对应 12 列。
    ASCII 只占前 6 列（bit 11..6），绘制时步进 6px；中文占 12 列，步进 12px。"""
    ascii_char = ord(ch) < 127
    gw = 6 if ascii_char else W
    img = Image.new("L", (gw, H), 0)
    d = ImageDraw.Draw(img)
    try:
        box = d.textbbox((0, 0), ch, font=font)
        tw, th = box[2] - box[0], box[3] - box[1]
        x = (gw - tw) // 2 - box[0]
        y = (H - th) // 2 - box[1]
    except Exception:
        x, y = 0, 0
    d.text((x, y), ch, font=font, fill=255)
    px = img.load()
    out = bytearray()
    for row in range(H):
        bits = 0
        for col in range(gw):
            if px[col, row] >= 96:
                bits |= 1 << (11 - col)
        out.append((bits >> 8) & 0xFF)
        out.append(bits & 0xFF)
    return bytes(out)


rows = []
for ch in sorted(chars, key=ord):
    rows.append((ord(ch), glyph(ch), ch))

with open(OUT, "w", encoding="utf-8") as f:
    f.write("/* 自动生成，请勿手改 —— 运行 tools/gen_font.py */\n")
    f.write("#ifndef JX3_FONT_CN_H\n#define JX3_FONT_CN_H\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write("struct Glyph { uint16_t code; uint8_t data[%d]; };\n" % BYTES)
    f.write("static const Glyph FONT_TABLE[] PROGMEM = {\n")
    for code, data, ch in rows:
        hexs = ",".join("0x%02X" % b for b in data)
        label = ch if (32 <= code < 127) else "0x%04X" % code
        f.write("  {0x%04X, {%s}},  /* %s */\n" % (code, hexs, label))
    f.write("};\n#define FONT_TABLE_N %d\n" % len(rows))
    f.write("#endif\n")

print("生成 %s：%d 个字形，约 %.1f KB" % (OUT, len(rows), len(rows) * (BYTES + 2) / 1024))
