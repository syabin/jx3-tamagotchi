#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""轻量 C++ 静态检查：去注释/字符串后检查括号平衡、字符串闭合、本地 include。
用法：python tools/check.py
不能替代真正的编译器，但能抓到写固件时最常见的低级错误。
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
files = [f for f in os.listdir(ROOT) if f.endswith(('.h', '.cpp'))]
errors = []

for name in files:
    path = os.path.join(ROOT, name)
    src = open(path, encoding='utf-8', errors='replace').read()
    # 去注释与字符串/字符字面量，但保留换行与位置
    clean = re.sub(r'/\*.*?\*/', lambda m: re.sub(r'[^\n]', ' ', m.group()), src, flags=re.S)
    clean = re.sub(r'//[^\n]*', '', clean)
    clean = re.sub(r'"(?:\\.|[^"\\])*"', lambda m: re.sub(r'[^\n]', ' ', m.group()), clean)
    clean = re.sub(r"'(?:\\.|[^'\\])*'", lambda m: re.sub(r'[^\n]', ' ', m.group()), clean)

    # 括号平衡
    pairs = {')': '(', ']': '[', '}': '{'}
    stack = []
    line = 1
    for ch in clean:
        if ch == '\n':
            line += 1
        elif ch in '([{':
            stack.append((ch, line))
        elif ch in ')]}':
            if not stack or stack[-1][0] != pairs[ch]:
                errors.append(f'{name}:{line} 括号不匹配 {ch}')
            else:
                stack.pop()
    if stack:
        errors.append(f'{name} 未闭合括号: {stack[-3:]}')

    # 字符串闭合（在去掉注释后的文本上按行粗查）
    for i, ln in enumerate(clean.splitlines(), 1):
        if ln.count('"') % 2 == 1 and not ln.lstrip().startswith('#'):
            errors.append(f'{name}:{i} 双引号数量为奇数（可能被截断）')

    # 本地 include 存在性（__has_include 保护的模板文件允许缺失）
    guarded = '__has_include' in src
    for inc in re.findall(r'#include\s+"([^"]+)"', src):
        if not os.path.exists(os.path.join(ROOT, inc)) and not (guarded and 'credentials' in inc):
            errors.append(f'{name}: #include "{inc}" 文件不存在')

# 函数声明 / 调用粗查（先去注释，避免把说明文字里的词当函数调用）
def strip_comments(s):
    s = re.sub(r'/\*.*?\*/', '', s, flags=re.S)
    return re.sub(r'//[^\n]*', '', s)

allsrc = '\n'.join(strip_comments(open(os.path.join(ROOT, f), encoding='utf-8', errors='replace').read())
                   for f in files)
declared = set(re.findall(r'\b(?:void|bool|int|float|uint8_t|uint16_t|uint32_t|const char\*|char\*)\s+([a-zA-Z_]\w*)\s*\(', allsrc))
calls = set(re.findall(r'\b([a-zA-Z_]\w*)\s*\(', allsrc))
known = declared | {
    'if','for','while','switch','sizeof','return','delay','pinMode','digitalRead','digitalWrite',
    'millis','random','randomSeed','min','max','abs','ceilf','lroundf','fmodf','memset','memcpy',
    'malloc','free','snprintf','strncpy','strncmp','sscanf','mktime','time','configTime','delay',
    'dispRect','dispText','dispTextC','dispTextR','dispNum','dispItemIcon','dispPet','dispIcon',
    'dispScene','dispHouse','dispDecos','dispSelFrame','dispFrame','dispPixel','dispClear',
    'dispBegin','dispEnd','dispInit','textWidth','uiGoto','uiRender','uiToastOverlay','uiDoMenu',
    'uiItemAction','uiFiltered','inpInit','inpPoll','inpGambleTick','netInit','netSyncClock',
    'gameInit','gameNew','gameLoad','gameSave','gameTick','gameAdvance','nowEpoch','setClock',
    'gameHour','isSleeping','actPenalty','condText','titleIndex','titleName','bagCount','bagTotal',
    'hasEq','decoCount','moodDecayCutPct','hungerDecayCutPct','toast','hint','checkTitle','emitTone',
    'actEat','actUseMed','actUsePage','actEquipToggle','actSell','actBuy','actSit','actTrain',
    'actDig','actClean','actUpgradeHome','actNanny','actDungeon','gambleRoll','checkDailyLogin',
    'gambleCap','parseFloat','String','begin','end','getUInt','putUInt','getBytes','putBytes',
    'findGlyph','ledcAttachPin','ledcDetachPin','ledcWrite','ledcWriteTone','yield','printf',
    '__has_include','else','tft','Q','P',
    'getBytesLength','initR','setRotation','fillScreen','startWrite','endWrite','writePixel',
    'drawRect','drawPixel','fillRect','mode','status','begin','getString','esp_random',
    'pgm_read_word','dispSpr',
    # 外部库 / 强转 / 数学函数：不是本项目要声明的函数
    'Adafruit_ST7735','TFT_JX3','canvas','cosf','sinf','uint16_t',
    'drawChar','drawFastHLine','drawRGBBitmap','getBuffer','getUChar',
    'invertDisplay','isKey','ledcSetup','putUChar','sendCommand',
    'setColRowStart','setSPISpeed'
}
suspect = sorted(c for c in calls - known if c not in ('Q', 'P', 'SECTS'))
if suspect:
    errors.append('可疑未声明调用: ' + ', '.join(suspect[:30]))

if errors:
    print('检查未通过：')
    for e in errors:
        print('  -', e)
    sys.exit(1)
print(f'静态检查通过：{len(files)} 个源文件，括号/引号/include/函数调用均无异常')
