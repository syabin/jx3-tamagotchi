# -*- coding: utf-8 -*-
"""从 index.html 的 SPR 表生成固件 sprites.h（16x16 RGB565，0x0000 = 透明）。

用法： python tools/gen_sprites.py
输出： sprites.h（枚举 SPR_xx + PROGMEM 数组 + 名称表）
"""
import re, json, os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "sprites.h")
# 源码可能在 <工作区>/jx3-tamagotchi/index.html，也可能固件已复制进仓库
# <工作区>/jx3-tamagotchi/firmware/ 下（此时 index.html 在上一级）
_CAND = [
    os.path.join(os.path.dirname(ROOT), "jx3-tamagotchi", "index.html"),
    os.path.join(os.path.dirname(ROOT), "index.html"),
    os.path.join(ROOT, "..", "index.html"),
]
INDEX = next((p for p in _CAND if os.path.exists(p)), _CAND[0])

PAL = {
    ".": None, "o": "#0b0e16", "D": "#202638", "d": "#6a6e7a", "k": "#10131c",
    "m": "#4a8db8", "l": "#7cb8d8", "h": "#d8eaf5",
    "g": "#e8c860", "y": "#e8d8a0", "w": "#f2f5fa",
    "s": "#f2c9a0", "r": "#c04050", "O": "#e88040",
    "c": "#4ab88a", "p": "#9a6ad8", "n": "#8a5a34",
    "b": "#2a3048", "G": "#3a7a4a",
}


def rgb565(hexcol):
    h = hexcol.lstrip("#")
    r, g, b = int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def load_spr():
    src = open(INDEX, encoding="utf-8").read()
    m = re.search(r"const SPR = \{(.*?)\n\};", src, re.S)
    if not m:
        raise SystemExit("index.html 里找不到 SPR 表")
    body = m.group(1)
    out = []
    # 每个条目形如：  id: {p:{...}, r:["...", ...]},
    for mm in re.finditer(r"^\s{2}([a-z0-9_]+):\s*(\{.*?\})\s*,?\s*$", body, re.M):
        sid, js = mm.group(1), mm.group(2)
        js = re.sub(r'([{,])\s*([A-Za-z_]\w*)\s*:', r'\1"\2":', js)   # JS 裸键 → JSON
        try:
            obj = json.loads(js)
        except Exception as e:
            raise SystemExit("%s 解析失败：%s" % (sid, e))
        out.append((sid, obj))
    return out


def main():
    sprs = load_spr()
    if not sprs:
        raise SystemExit("没有解析到精灵")
    ids = [s for s, _ in sprs]
    lines = []
    lines.append("// 由 tools/gen_sprites.py 自动生成，请勿手改（源：index.html 的 SPR 表）")
    lines.append("#pragma once")
    lines.append("#include <stdint.h>")
    lines.append("#include <pgmspace.h>")
    lines.append("")
    lines.append("enum : uint8_t {")
    for i, sid in enumerate(ids):
        lines.append("  SPR_%s = %d," % (sid.upper(), i))
    lines.append("  SPR_COUNT = %d" % len(ids))
    lines.append("};")
    lines.append("")
    lines.append("static const char* const SPR_NAME[SPR_COUNT] = {")
    lines.append("  " + ", ".join('"%s"' % s for s in ids))
    lines.append("};")
    lines.append("")

    for sid, obj in sprs:
        pal = dict(PAL)
        pal.update(obj.get("p") or {})
        rows = obj["r"]
        if len(rows) != 16:
            raise SystemExit("%s 行数 %d != 16" % (sid, len(rows)))
        vals = []
        for y, row in enumerate(rows):
            row = row.ljust(16, ".")
            for x, ch in enumerate(row[:16]):
                col = pal.get(ch)
                vals.append(0 if col is None else rgb565(col))
        lines.append("static const uint16_t SPR_%s_[256] PROGMEM = {" % sid.upper())
        for i in range(0, 256, 16):
            lines.append("  " + ",".join("0x%04X" % v for v in vals[i:i + 16]) + ",")
        lines.append("};")
        lines.append("")

    lines.append("static const uint16_t* const SPR_TAB[SPR_COUNT] PROGMEM = {")
    for sid in ids:
        lines.append("  SPR_%s_," % sid.upper())
    lines.append("};")
    lines.append("")
    open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines))
    print("生成 %s：%d 枚精灵，约 %.1f KB" % (OUT, len(ids), len(ids) * 512 / 1024))


if __name__ == "__main__":
    main()
