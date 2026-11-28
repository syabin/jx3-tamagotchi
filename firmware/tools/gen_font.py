# -*- coding: utf-8 -*-
"""扫描固件源码里的全部非 ASCII 字符，生成 12×12 点阵字库 font_cn.h。

用法： python tools/gen_font.py
改了 config.h / ui.cpp 里的中文后重跑一次。
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "font_cn.h")

W = H = 12
BYTES = 24
FONTS = [
    r"C:\Windows\Fonts\simhei.ttf",
    r"C:\Windows\Fonts\msyh.ttc",
    r"C:\Windows\Fonts\simsun.ttc",
]

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("需要 Pillow：python -m pip install pillow")

font_path = next((p for p in FONTS if os.path.exists(p)), None)
if not font_path:
    sys.exit("找不到中文字体（simhei/msyh/simsun）")
font = ImageFont.truetype(font_path, 12, index=0)

chars = set()
for name in os.listdir(ROOT):
    if not name.endswith((".h", ".cpp", ".ino")):
        continue
    with open(os.path.join(ROOT, name), encoding="utf-8", errors="ignore") as f:
        for ch in f.read():
            if ord(ch) > 126 and not ch.isspace():
                chars.add(ch)
chars.update("·—…×℃？！：，。、（）【】《》「」／")


def glyph(ch):
    img = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(img)
    try:
        box = d.textbbox((0, 0), ch, font=font)
        tw, th = box[2] - box[0], box[3] - box[1]
        x = (W - tw) // 2 - box[0]
        y = (H - th) // 2 - box[1]
    except Exception:
        x, y = 0, 0
    d.text((x, y), ch, font=font, fill=255)
    px = img.load()
    out = bytearray()
    for row in range(H):
        bits = 0
        for col in range(W):
            if px[col, row] >= 92:
                bits |= 1 << (11 - col)
        out.append((bits >> 8) & 0xFF)
        out.append(bits & 0xFF)
    return bytes(out)


rows = [(ord(c), glyph(c), c) for c in sorted(chars, key=ord)]

with open(OUT, "w", encoding="utf-8", newline="\n") as f:
    f.write("/* 自动生成，请勿手改 —— 运行 tools/gen_font.py */\n")
    f.write("#pragma once\n#include <stdint.h>\n#include <pgmspace.h>\n\n")
    f.write("struct Glyph { uint16_t code; uint8_t data[%d]; };\n" % BYTES)
    f.write("static const Glyph FONT_TABLE[] PROGMEM = {\n")
    for code, data, ch in rows:
        hexs = ",".join("0x%02X" % b for b in data)
        f.write("  {0x%04X,{%s}}, /* %s */\n" % (code, hexs, ch))
    f.write("};\n")
    f.write("#define FONT_TABLE_N %d\n" % len(rows))

print("生成 %s：%d 个汉字字形，约 %.1f KB" % (OUT, len(rows), len(rows) * (BYTES + 2) / 1024))
