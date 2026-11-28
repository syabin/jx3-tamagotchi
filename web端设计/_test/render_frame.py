#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""把 render_frame.js 录下的 canvas 操作画成 PNG（需 Pillow）。
用法：python web端设计/_test/render_frame.py [in.json] [out.png]"""
import json
import os
import re
import sys
from PIL import Image, ImageDraw, ImageFont

src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "_frame.json")
dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), "_frame.png")
cmds = json.load(open(src, encoding="utf-8"))
img = Image.new("RGB", (128, 128), (20, 24, 36))
d = ImageDraw.Draw(img)
fonts = {}

def color(c, a=1):
    if isinstance(c, str) and c.startswith("#"):
        h = c[1:]
        if len(h) == 3:
            h = "".join(x * 2 for x in h)
        r, g, b = (int(h[i:i + 2], 16) for i in (0, 2, 4))
    elif isinstance(c, (list, tuple)):
        r, g, b = c[:3]
    else:
        r, g, b = (20, 24, 36)
    if a < 1:
        r, g, b = (int(r * a + 20 * (1 - a)), int(g * a + 24 * (1 - a)), int(b * a + 36 * (1 - a)))
    return (r, g, b)

for op in cmds:
    c = color(op.get("c", "#141824"), op.get("a", 1))
    if op["op"] == "rect":
        x, y, w, h = (round(op[k]) for k in ("x", "y", "w", "h"))
        d.rectangle([x, y, x + w - 1, y + h - 1], fill=c)
    elif op["op"] == "text":
        m = re.search(r"(\d+)px", op.get("font", ""))
        size = int(m.group(1)) if m else 9
        key = size
        if key not in fonts:
            path = r"C:\Windows\Fonts\simhei.ttf"
            fonts[key] = ImageFont.truetype(path, size) if os.path.exists(path) else ImageFont.load_default()
        d.text((round(op["x"]), round(op["y"])), op["s"], fill=c, font=fonts[key], anchor="lt")
img.resize((512, 512), Image.NEAREST).save(dst)
print("saved", dst)
