/* 回归冒烟测试：最小 DOM/canvas stub + vm 跑 index.html 脚本。
   用法：node web端设计/_test/regress.js   （改 index.html 后先跑这个） */
const fs = require("fs"), vm = require("vm");
const assert = (cond, msg) => { if (!cond) { console.error("FAIL: " + msg); process.exitCode = 1; } else console.log("ok  - " + msg); };

const html = fs.readFileSync(__dirname + "/../../index.html", "utf8");
const script = html.match(/<script>([\s\S]*)<\/script>/)[1];

const ctxStub = new Proxy({
  font: "", fillStyle: "", textBaseline: "", strokeStyle: "", lineWidth: 1,
  measureText: s => ({ width: String(s).length * 8 }),
}, {
  get(t, k) { if (k in t) return t[k]; return () => {}; },   // moveTo/lineTo/stroke 等一律 no-op
  set(t, k, v) { t[k] = v; return true; },
});
const canvasEl = { getContext: () => ctxStub, addEventListener() {}, width: 128, height: 128, style: {} };
function el(extra) {
  return Object.assign({
    _handlers: {},
    addEventListener(ev, fn) { this._handlers[ev] = fn; },
    fire(ev) { if (this._handlers[ev]) this._handlers[ev]({ preventDefault() {} }); },
    onclick: null, onchange: null,
    classList: { toggle() {}, contains: () => false, add() {}, remove() {} },
    textContent: "", checked: false, value: "1", style: {},
  }, extra || {});
}
const store = {};
const sandbox = {
  console, Date, Math, JSON, Promise,
  fetch: () => Promise.reject(new Error("offline")),
  requestAnimationFrame: () => 0,
  setTimeout: () => 0, setInterval: () => 0, clearTimeout() {}, clearInterval() {},
  localStorage: {
    getItem: k => (k in store ? store[k] : null),
    setItem: (k, v) => { store[k] = String(v); },
    removeItem: k => { delete store[k]; },
  },
  document: {
    getElementById: id => (id === "screen" ? canvasEl : (sandbox._els[id] = sandbox._els[id] || el())),
    querySelector: () => el(),
    body: { classList: { toggle() {}, contains: () => false } },
    addEventListener() {},
  },
  window: { addEventListener() {} },
  _els: {},
};
const ctx = vm.createContext(sandbox);
vm.runInContext(script, ctx, { filename: "index.html" });
const g = code => vm.runInContext(code, ctx);

/* ---- 开机流程（v0.2.7：取消门派，boot → main） ---- */
assert(g("typeof render === 'function' && typeof confirmA === 'function'"), "脚本可求值，核心函数存在");
assert(g("page") === "boot", "新档进入开机画面");
g("confirmA()");
assert(g("page") === "main", "A 键从开机直接进入主界面（无门派选择）");
assert(g("typeof SECTS === 'undefined' && typeof PET_STYLE !== 'undefined'"), "SECTS 门派表已移除，改为固定配色");
assert(g("!('sect' in S)"), "存档中无 sect 字段");
assert(g("typeof BOOT_LOGO_URL === 'string' && BOOT_LOGO_URL.startsWith('data:image/png;base64,')"), "开机画面内嵌 SeasunGame logo");

/* ---- v8.0 随时呼出设置（长按 B）---- */
g("page = 'bag'; bagSel = 3; catMode = true; openSettings();");
assert(g("page") === "settings" && g("settingsFrom") === "bag", "任意页可呼出设置并记录来源");
g("setPage = 'about'; pressB();");
assert(g("page") === "settings" && g("setPage") === null, "设置子页 B 回设置菜单");
g("pressB();");
assert(g("page") === "bag" && g("bagSel") === 3 && g("catMode") === true, "设置 B 返回原页且状态不丢");
g("page = 'game'; gameSt = { phase: 'roll' }; openSettings(); pressB();");
assert(g("page") === "game" && g("gameSt.phase") === "roll", "摇骰中呼出设置再返回不丢状态");
g("page = 'main'; gameSt = null;");   // 恢复：直接回主界面，避免 confirmA 误入其它页

/* ---- v0.2.0 主界面：底部一排 4 个按钮 ---- */
assert(g("MENU.length") === 6, "菜单共 6 项（5 按钮 + 状态面板）");
assert(g("MENU.map(m=>m.id).join(',')") === "activity,game,bag,shop,mail,status",
  "菜单顺序 = 活动/游戏/背包/商城/邮箱 + status（不参与选中）");
assert(g("MENU.some(m => m.id === 'settings')") === false, "主界面无设置按钮（长按 B 呼出）");
assert(g("MENU.some(m => m.id === 'sit') && MENU.some(m => m.id === 'dig')") === false, "打坐/挖宝等行动已移入活动页");

/* ---- 渲染不抛异常（含异常状态/副本/狼藉）---- */
g("S.mess=3; S.poisoned=true; S.dungeonUntil=Date.now()+5000; render(); render();");
g("S.poisoned=false; S.down=true; S.dungeonUntil=0; render();");
g("S.down=false; S.mess=0; render();");
assert(true, "主界面渲染无异常（中毒/重伤/副本中/狼藉）");

/* ---- 导航与非常驻提示 ---- */
g("menuIdx = 0; moveSel(-1);");
assert(g("menuIdx") === 4, "▲ 从第 0 项左回绕到最后一个按钮 mail（状态面板不参与选中）");
g("moveSel(1);");
assert(g("menuIdx") === 0, "▼ 从 shop 回到第 0 项（不进状态面板）");
g("menuIdx = 4; moveSel(1);");
assert(g("menuIdx") === 0, "mail 右移回绕到第 0 项而非状态面板");
g("menuIdx = 0; moveSel(-1);");
assert(g("!!selHint && Date.now() < selHint.until"), "切换选中时出现短暂 selHint");
assert(g("selHint.until - Date.now()") > 1900 && g("selHint.until - Date.now()") <= 2000, "selHint 时长为 2 秒");

/* ---- 家园页：打扫并入 + 升级 + 保姆（v7.6 起保姆常驻选项）---- */
g("doAct('home')");
assert(g("page") === "home", "A 进家园页");
assert(g("homeOptions().join(',')") === "up,nanny", "无狼藉时 = 升级 + 保姆");
g("S.mess = 2;");
assert(g("homeOptions().join(',')") === "up,clean,nanny", "有狼藉时出现打扫选项");
g("moveSel(1)");
assert(g("homeSel") === 1, "▲▼ 切到打扫");
g("confirmA()");
assert(g("S.mess") === 0 && g("page") === "home", "打扫生效且留在家园页");
g("S.coins = 1000; S.homeLv = 0; homeSel = 0; confirmA();");
assert(g("S.homeLv") === 1 && g("S.coins") === 500, "升级扣款 500 并升到 1 级");
g("page = 'main'; render();");

/* ---- v7.6 装备系统（代替进化分支）---- */
g("S.bag.w_sword = 1; itemFrom = 'bag'; bagSel = BAG_ORDER.indexOf('w_sword'); page = 'item'; confirmA();");
assert(g("S.equip.weapon") === "w_sword", "背包可装备精铁剑到武器槽");
g("confirmA();");
assert(g("S.equip.weapon") === null, "再按 A 卸下装备");
g("confirmA(); page = 'item'; render();");
assert(g("hasEq('w_sword')") === true, "装备操作留在详情页且已生效");
/* 加成生效：木桩修为 12×1.5 = 18 */
g("S.exp = 0; S.cool = {}; S.hunger = 80; doAct('train');");
assert(g("S.exp") === 18, "精铁剑木桩修为 +50%（12→18）");
/* 槽位互斥：换武器应顶掉旧武器 */
g("S.cool = {}; S.bag.w_pick = 1; itemFrom = 'bag'; bagSel = BAG_ORDER.indexOf('w_pick'); page = 'item'; confirmA();");
assert(g("S.equip.weapon") === "w_pick", "同槽位换装顶掉旧装备");

/* ---- v7.6 装饰（购入即布置）---- */
g("S.coins = 1000; S.decos = {}; itemFrom = 'shop'; shopSel = SHOP_ORDER.indexOf('d_plant'); page = 'item'; confirmA();");
assert(g("S.decos.d_plant") === true && g("S.coins") === 880, "购入盆栽即布置庭院（扣款 120）");
assert(g("decoCount()") === 1, "装饰计数生效");
assert(g("Math.abs(moodDecayCut() - 0.05) < 1e-9"), "装饰心情减免生效");

/* ---- v7.6 保姆代管 ---- */
g("page = 'home'; doAct('home'); homeSel = homeOptions().indexOf('nanny'); S.coins = 500; confirmA();");
assert(g("S.nannyGH") === 8 && g("S.coins") === 400, "100 金雇佣保姆 8 时辰");
g("applyTime(60, 60, true);");   // 60 真实秒 ×60 速 = 1 游戏小时（silent 免随机事件干扰）
assert(g("S.nannyGH") === 7, "保姆合约随游戏时间倒扣");
g("S.mess = 2; applyTime(60, 60, true);");
assert(g("S.mess") === 0, "保姆自动洒扫");

/* ---- v7.6 每日登录奖励 ---- */
const c0 = g("S.coins");
g("S.lastLogin = null; S.loginStreak = 0; checkDailyLogin();");
assert(g("S.loginStreak") === 1 && g("S.coins") === c0 + 50, "首日登录奖励 50 金");
g("checkDailyLogin();");
assert(g("S.coins") === c0 + 50, "同日不重复发奖");

/* ---- v7.6 昼夜作息：22~6 时就寝封锁行动（v8.2 默认 ×1，按真实毫秒算）---- */
g("S.bornTs = Date.now() - 15 * 3600 * 1000;");   // 游戏时刻 = (15+8)%24 = 23 点
assert(g("isSleeping()") === true, "23 点判定为就寝时段");
g("S.cool = {}; doAct('dig');");
assert(g("!S.cool.dig"), "就寝时挖宝被封锁");
assert(String(g("canAct()")).indexOf("睡") >= 0, "canAct 提示就寝中");
g("render(); render();");   // 睡姿渲染不抛异常
g("S.bornTs = Date.now() - 5 * 3600 * 1000;");    // 回到 13 点
assert(g("isSleeping()") === false, "白天不就寝");

/* ---- v7.7 装备品级（三品级，商城同池） ---- */
assert(g("Object.keys(EQ_TIERS).join(',')") === "1,2,3", "装备分三品级（凡品/良品/珍品）");
assert(g("GOODS.w_sword.tier") === 1 && g("GOODS.a_robe.tier") === 2, "商城装备标注 tier（精铁剑凡品/道袍良品）");
assert(g("GOODS.a_robe.tier === 2 && GOODS.a_charm.tier === 2 && GOODS.t_jade.tier === 2"), "良品装备存在");
assert(g("giftEquipPool().every(id => GOODS[id] && GOODS[id].type === 'equip' && GOODS[id].price > 0)"),
  "签到鸿运当头装备池 = 商城同池（全是商城可购装备）");
assert(g("giftEquipPool().length") > g("Object.values(GOODS).filter(x=>x.type==='equip').length"),
  "装备池按品级加权（凡品权重更高）");

/* ---- v7.7 商城/背包分类展示 ---- */
assert(g("CATS.map(c=>c.id).join(',')") === "all,food,med,equip,deco,mat", "分类：全部/食品/丹药/装备/家具/材料");
assert(g("bagCat = 'equip'; filteredOrder('bag').every(id => GOODS[id].type === 'equip')"), "背包装备分类过滤正确");
assert(g("bagCat = 'mat'; filteredOrder('bag').some(id => id === 'xj_zt')"), "玄晶归入材料分类");
g("bagCat = 'food';");
assert(g("filteredOrder('bag').join(',')") === "baozi,fish,feast", "背包食品分类只含食物");
assert(g("bagCat = 'med'; filteredOrder('bag').join(',')") === "pill,page", "丹药分类含药品与参悟页");
assert(g("shopCat = 'mat'; filteredOrder('shop').length") === 0, "商城材料分类为空（商城无材料出售）");
g("bagCat = 'all'; shopCat = 'all';");
/* 分类导航 v8.1：进入先选分类栏 → A 进列表 → B 退回分类栏 → B 回主页 */
g("doAct('bag');");
assert(g("page") === "bag" && g("catMode") === true, "进背包先选中分类栏");
g("moveSel(1);");
assert(g("bagCat") === "food", "分类栏 ▼ 换到「食品」");
assert(g("bagSel") === 0, "换分类后列表选中重置");
g("confirmA();");
assert(g("catMode") === false, "分类栏 A 进入列表");
g("moveSel(1);");
assert(g("bagSel") === 1, "列表 ▲▼ 选物品");
g("pressB();");
assert(g("catMode") === true && g("page") === "bag", "列表 B 退回分类栏（反向操作）");
g("pressB();");
assert(g("page") === "main", "分类栏 B 才回主页");
g("doAct('shop');");
assert(g("catMode") === true, "进商城也先选中分类栏");
g("pressB();");
assert(g("page") === "main", "商城分类栏 B 回主页");
/* 详情页用过滤后的列表翻页 */
g("doAct('shop'); shopCat = 'deco'; shopSel = 0; confirmA(); confirmA(); render();");   // 第一次 A 进列表，第二次 A 进详情
assert(g("page === 'item' && itemFrom === 'shop'"), "商城家具分类可进详情");
g("pressB();");
g("doAct('shop'); shopCat = 'all'; shopSel = SHOP_ORDER.length - 1; confirmA(); confirmA(); render(); pressB();");
assert(g("shopCat") === "all", "详情页翻页不串分类");

/* ---- v7.7 玄晶（团本 0.1% 橙色品级，本赛季铸天玄晶） ---- */
assert(g("GOODS.xj_zt.name") === "铸天玄晶" && g("GOODS.xj_zt.type") === "xj", "本赛季玄晶 = 铸天玄晶（type xj）");
assert(g("itemColor(GOODS.xj_zt)") === g("XJ_COLOR"), "玄晶品级色为橙色");
assert(g("GOODS.xj_zt.sell") === 2500, "玄晶可卖 2500 金");
/* 强制掉落验证：劫持 Math.random 让首次必中 0.1% 掷骰 */
g("S.bag.xj_zt = 0; S.exp = 0; const _mr = Math.random; Math.random = () => 0.0005;" +
  "S.dungeonUntil = Date.now() - 1; S.dungeonKey = 'raid'; applyTime(0.001, 60, false); Math.random = _mr;");
assert(g("S.bag.xj_zt") === 1, "团本 0.1% 掉落玄晶（强中验证）");
assert(g("String(toast.text).indexOf('铸天玄晶') >= 0"), "玄晶掉落触发橙色播报");
g("S.bag.xj_zt = 0; S.dungeonUntil = Date.now() - 1; S.dungeonKey = 'war';" +
  "const _mr2 = Math.random; Math.random = () => 0.0005; applyTime(0.001, 60, false); Math.random = _mr2;");
assert(g("S.bag.xj_zt") === 0, "非团本不掉玄晶");

/* ---- v7.7 签到鸿运当头（与商城同池装备，非专属） ---- */
g("S.lastLogin = null; const _mr3 = Math.random; Math.random = () => 0.1; checkDailyLogin(); Math.random = _mr3;");
assert(g("Object.values(GOODS).some(x => x.type === 'equip' && (S.bag[x.id] || 0) > 0)"),
  "签到 30% 概率赠装备（命中商城同池装备）");

/* ---- v7.7 区服设置已移除 ---- */
assert(g("typeof SERVERS") === "undefined", "SERVERS 常量已移除");
assert(g("SET_MENU.some(m => m.id === 'server')") === false, "设置菜单无区服项");
assert(g("typeof S.server") === "undefined", "存档不再含 server 字段");

/* ---- v8.2 手札 B 返回 / 音效开关 / 默认速度 ---- */
assert(g("APP_VERSION") === "0.2.10", "版本号为语义化 0.2.10");

/* ---- v0.2.10：剑三化命名 + 最高识别度素材 ---- */
assert(g("TITLES.map(t=>t.t).join(',')") === "初闯江湖,江湖新秀,一方名侠,武林名宿,武林至尊", "称号表对齐剑三成就称谓");
assert(g("REPUTATIONS.map(r=>r.name).join(',')") === "中立,友好,尊敬,崇敬,崇拜", "声望五阶对齐剑三");
assert(g("reputationOf(700).name") === "崇拜" && g("reputationOf(0).name") === "中立", "声望按修为正确映射");
assert(g("HOME_NAMES.join(',')") === "荒地,茅舍,宅院,别院,山庄", "家园等级名对齐剑三宅院体系");
assert(g("DUNGEONS.card.name") === "秘境" && g("DUNGEONS.raid.name") === "团本", "副本类型名对齐剑三");
assert(g("typeof umbrella === 'undefined'"), "梅伞为 drawPet 内部绘制，不污染全局");
assert(html.includes('text("稻香村"') && html.includes("const wx = sx + 99"), "稻香村木牌与大水车已进主场景");
assert(html.includes("ux + 13, uy + 4") && html.includes("#e86a9a"), "梅纹伞保留右下缺角与梅花纹");
assert(html.includes("淡紫面纱") && html.includes("药箱（斜背）"), "陈月面纱与药箱已绘制");
assert(g("typeof applyPanelLook") === "undefined" && g("typeof QLUT") === "undefined", "v8.4 暖屏色域映射已撤销");
const src86 = require("fs").readFileSync(require("path").join(__dirname, "../..", "index.html"), "utf8");
assert(!/body\.qmode \.screen-frame \{ border-color: #8a8478/.test(src86), "v8.6 真机预览不再覆盖暖灰边框");
assert(!/body\.qmode \.ctl \{ background: radial-gradient\(circle at 35% 30%, #d8d4c8/.test(src86), "v8.6 真机预览不再覆盖暖灰按键");
assert(g("B_HOLD_MS") === 1500, "长按 B 呼出设置阈值为 1.5 秒");
assert(g("speedF") === 1, "默认时间流速 = 真实 ×1");
g("page = 'main'; pressB();");
assert(g("page") === "status", "主页 B 进手札");
g("confirmA();");
assert(g("page") === "status" && g("noteDetail") === true, "手札页 A 进入详情");
g("render();");
g("pressB();");
assert(g("page") === "status" && g("noteDetail") === false, "详情页 B 返回手札列表");
g("pressB();");
assert(g("page") === "main", "手札列表 B 返回主页");
/* v8.3 二次触发 bug：实体 B 的 pointerup 才返回，pointerleave 不得再返回一次 */
const btnB = sandbox._els["btnB"];
g("page = 'main';");
btnB.fire("pointerdown"); btnB.fire("pointerup");
assert(g("page") === "status", "实体 B 单次点击：主页进手札");
btnB.fire("pointerleave");
assert(g("page") === "status", "pointerleave 不二次触发返回（手札不被立刻退回）");
btnB.fire("pointerdown"); btnB.fire("pointerup"); btnB.fire("pointerleave");
assert(g("page") === "main", "实体 B 再点一次：手札退主页且不被二次拉回");
assert(g("SET_MENU.some(m => m.id === 'sound')") === true, "设置菜单含音效开关");
g("muted = false; page = 'settings'; setPage = 'sound'; confirmA();");
assert(g("muted") === true, "音效开关 A 切换为静音");
assert(g("localStorage.getItem('jx3mute')") === "1", "静音偏好已持久化");
g("confirmA();");
assert(g("muted") === false && g("localStorage.getItem('jx3mute')") === "0", "再按 A 恢复音效并持久化");
g("page = 'main'; setPage = null;");

/* ---- 各子页渲染无异常 ---- */
["status", "bag", "shop", "activity"].forEach(p => {
  g("doAct('" + p + "'); render(); pressB();");
});
g("openSettings(); render(); pressB();");
assert(g("page") === "main", "状态/背包/商城/活动/设置页进出正常");

/* ---- v0.2.0 活动页：行动执行 + 冷却 ---- */
g("doAct('activity'); render();");
assert(g("page") === "activity", "A 进活动页");
assert(g("activityRows()[0].id") === "sit", "活动页第一行 = 打坐");
assert(g("activityRows().some(r => r.id === 'dungeon')") === true && g("activityRows().some(r => r.kind === 'daily')") === false,
  "活动页含独立副本，今日日常已移出手札页且不再参与副本");
g("actSel = 0; S.mood = 10; confirmA();");
assert(g("S.mood") === 30, "活动页 A 执行打坐（心情+20）");
assert(g("cooling('sit')") === true, "打坐进入冷却");
assert(g("page") === "activity", "普通活动执行后留在活动页，不再自动回主页");
g("actSel = 3; confirmA();");
assert(g("page") === "home", "活动页 A 进家园页");
g("pressB();");
assert(g("page") === "main", "家园 B 回主页");
g("render();");

/* ---- v0.2.0 游戏厅：大厅导航 + 进入所选 ---- */
g("doAct('game'); render();");
assert(g("page") === "game" && g("gameSt.game") === "hub", "A 进游戏厅（大厅态）");
assert(g("GAMES.map(x=>x.id).join(',')") === "dice,whack,fish,memory,race", "游戏厅 5 个游戏（含赛马）");
assert(g("GAMES.every(x=>x.tip && x.tip.length > 8)") === true, "5 个游戏都有玩法小抄（单页详情的底部说明）");
assert(g("typeof gameIcon === 'function' && typeof triLR === 'function'") === true, "游戏厅单页图标与左右箭头可渲染");
g("moveSel(1); moveSel(1);");
assert(g("gameSt.sel") === 2, "大厅 ▶ 切到第 3 个游戏（钓鱼）");
g("moveSel(-1);");
assert(g("gameSt.sel") === 1, "大厅 ◀ 回到打地鼠");
g("confirmA(); render(); render();");
assert(g("gameSt.game") === "whack" && g("gameSt.phase") === "play", "A 进入所选游戏（打地鼠）");

/* ---- 打地鼠：敲中/敲空/结算 ---- */
g("gameSt.cur = gameSt.mole = 3; gameSt.hits = 9; gameSt.swings = 9; gameSt.pay = 0; confirmA();");
assert(g("gameSt.hits") === 10 && g("gameSt.phase") === "over" && g("S.coins") > 0, "第 10 只敲中 → 结算发钱并冷却");
assert(g("cooling('g_whack')") === true, "打地鼠进入冷却");
g("confirmA();");
assert(g("gameSt.game") === "hub", "结算后 A 回大厅");

/* ---- 0.2.5 手札三页 / 大图标 / 日常只查询 ---- */
g("page = 'status'; notePage = 0; moveSel(1);");
assert(g("notePage") === 1, "手札 ▲▼ 切到第 2 页：游戏日常");
g("render();");
assert(g("typeof noteIcon === 'function' && typeof actIcon === 'function'") === true, "三页页签与活动页的大图标可渲染");
g("moveSel(1);");
assert(g("notePage") === 2, "手札切到第 3 页：开服查询（预留）");
g("moveSel(1);");
assert(g("notePage") === 0, "手札三页循环回第一页");
g("notePage = 1; dailyData = { date: '2026-11-28', war: '大战' }; dailyPage = 0; page = 'status'; confirmA();");
assert(g("page") === "status", "手札游戏日常 A 只翻页/刷新，不再进副本");
g("dailyData = { date: '2026-11-28', war: '大战' }; dungeonPick = null;");
assert(g("dungeonActivity().key") === "war", "有日常时副本地名取自日常条目");
g("dailyData = null; dungeonPick = null;");
assert(g("Object.keys(DUNGEONS).includes(dungeonActivity().key)") === true, "无日常时副本随意挑一个");
g("dailyData = null; dungeonPick = null; page = 'game';");   // 恢复后续赛马测试依赖的 game 页

/* ---- v0.2.3 赛马：选马 → 定注 → 起跑 → 结算 ---- */
g("gameExit(); gameSt.sel = 4; S.coins = 1000; gameStart('race');");
assert(g("gameSt.game") === "race" && g("gameSt.phase") === "pick", "进入赛马：选马阶段");
g("moveSel(1);");
assert(g("gameSt.sel") === 1, "赛马 ▲▼ 换马");
g("confirmA();");
assert(g("gameSt.phase") === "bet", "赛马 A 进入定注");
const raceCoins = g("S.coins");
g("confirmA();");
assert(g("gameSt.phase") === "race", "赛马 A 起跑");
g("gameSt.hs[gameSt.sel].pos = RACE_LEN - 0.1; gameTick();");   // 让押中的马先冲线（留足一帧最小步长，避免随机不过线）
assert(g("gameSt.phase") === "result" && g("gameSt.winner") === g("gameSt.sel"), "先到终点者夺冠");
assert(g("S.coins") > raceCoins, "押中按赔率赔付");
assert(g("cooling('g_race')") === true, "赛马进入冷却");
g("confirmA(); gameExit();");

/* ---- 钓鱼：绿区收竿 ---- */
g("gameSt = null; doAct('game'); gameSt.sel = 2; confirmA();");
assert(g("gameSt.game") === "fish" && g("gameSt.cast") === 3, "进入钓鱼（3 杆）");
g("gameSt.pos = gameSt.zoneL; confirmA();");       // 指针停在绿区起点
assert(g("gameSt.good") === 1 && g("gameSt.cast") === 2, "绿区收竿命中（剩 2 杆）");
g("gameSt.pos = 0; gameSt.zoneL = 10; confirmA(); confirmA();");   // 两杆落空 → 3 杆打完自动结算
assert(g("gameSt.phase") === "over" && g("cooling('g_fish')") === true, "3 杆打完结算并冷却");

/* ---- 翻牌配对：翻两张同面即配对 ---- */
g("gameSt = null; doAct('game'); gameSt.sel = 3; confirmA();");
assert(g("gameSt.game") === "memory", "进入翻牌配对");
g("var fi = gameSt.faces.findIndex(f => f === gameSt.faces[gameSt.cur], gameSt.cur + 1); gameSt.cur = fi < 0 ? gameSt.cur : fi; gameSt.first = -1; gameSt.open = []; confirmA(); gameSt.cur = gameSt.faces.indexOf(gameSt.faces[gameSt.faces.length - 1]) === gameSt.cur ? 0 : 0;");
// 简化：直接翻 cur 与它同面的另一张
g("var c0 = gameSt.cur; var c1 = -1; for (var i2 = 0; i2 < 6; i2++) { if (i2 !== c0 && gameSt.faces[i2] === gameSt.faces[c0] && !gameSt.done[i2]) { c1 = i2; break; } }");
g("gameSt.first = -1; gameSt.open = []; gameSt.cur = c0; confirmA(); gameSt.cur = c1; confirmA();");
assert(g("gameSt.matched") === 1 && g("S.coins") > 0, "翻两张同面 → 配对成功发奖");
assert(g("gameSt.done[c0]") === true && g("gameSt.done[c1]") === true, "配对的牌置为已消");

/* ---- 押大小仍走老流程（大厅进入） ---- */
g("gameSt = null; doAct('game'); gameSt.sel = 0; confirmA();");
assert(g("gameSt.game") === "dice" && g("gameSt.phase") === "side", "进入押大小");
g("moveSel(1);");
assert(g("gameSt.side") === 0, "押大小 ▲▼ 切换押小");

/* ---- 统一像素精灵（v0.2.6）：42 枚 16×16 字符网格 ---- */
assert(g("Object.keys(SPR).length") >= 42, "统一精灵不少于 42 枚");
assert(g("Object.keys(SPR).every(k => SPR[k].r.length === 16 && SPR[k].r.every(r => r.length === 16))") === true,
  "所有精灵严格 16×16");
assert(g("MENU.every(m => !!SPR[m.id])") === true, "主菜单每个入口都有精灵");
assert(g("Object.keys(GOODS).every(id => !!SPR[id])") === true, "全部物品都有精灵");
assert(g("GAMES.every(game => !!SPR[game.id === 'fish' ? 'game_fish' : game.id])") === true, "全部小游戏都有精灵");
assert(g("ACTIVITIES.every(a => !!SPR[a.id])") === true, "全部活动都有精灵");
assert(g("NOTE_TABS.every(t => !!SPR[t.icon])") === true, "手札三页页签都有精灵");
assert(g("typeof drawSpr === 'function' && typeof itemIcon === 'function' && typeof gameIcon === 'function' && typeof noteIcon === 'function' && typeof actIcon === 'function'") === true,
  "五类图标入口统一到 drawSpr");
assert(g("(function(){ try { ['feed','baozi','dice','race','book','dungeon'].forEach((id,i)=>drawSpr(id,i*16,0,1)); itemIcon('baozi',0,0,3.5); gameIcon('fish',0,0,3.5); noteIcon('book',0,0,2.5); actIcon('dungeon',0,0,3.5); icon('bag',0,0,true); icon('shop',0,0,false); return 1; } catch(e) { return 0; } })()") === 1,
  "所有图标入口渲染无异常");

/* ---- v0.2.8 邮箱 ---- */
assert(g("typeof MAIL_SCENES !== 'undefined' && MAIL_SCENES.length >= 6"), "奇遇场景池 >= 6 种");
assert(g("Array.isArray(S.mail)"), "存档含 mail 数组");
g("S.mail = []; S.coins = 0; S.exp = 0;");
g("_mrMail = Math.random; Math.random = () => 0.1; rollEncounter(); Math.random = _mrMail;");
assert(g("S.mail.length") === 1 && (g("S.coins") > 0 || g("S.exp") > 0), "随机奇遇写入邮件并发放奖励");
assert(g("S.mail[0].t.includes('获得了') && S.mail[0].t.includes('遇到了')"), "邮件文案含遇到/获得叙事");
g("page = 'main'; menuIdx = 4; confirmA();");
assert(g("page") === "mail" && g("mailDetail") === false, "主页选邮箱 A 进邮箱列表");
g("confirmA();");
assert(g("mailDetail") === true, "邮箱列表 A 进详情");
g("render(); pressB();");
assert(g("mailDetail") === false && g("page") === "mail", "邮箱详情 B 回列表");
g("pressB();");
assert(g("page") === "main", "邮箱列表 B 回主页");
g("render();");

/* ---- v0.2.7 手札详情页 ---- */
g("page = 'status'; notePage = 0; noteDetail = false;");
g("confirmA();");
assert(g("noteDetail") === true && g("noteDetailPage") === 0, "手札首页 A 进详情第 1 页");
g("moveSel(1);");
assert(g("notePage") === 0 && g("noteDetailPage") === 1, "详情页左右翻到第 2 页");
g("confirmA();");
assert(g("noteDetailPage") === 0, "详情页 A 循环回第 1 页");
g("render();");
g("page = 'status'; notePage = 0; noteDetail = false; noteDetailPage = 0;");

/* ---- 存档 ---- */
g("save();");
assert(g("load()") === true, "存档可重新载入");

/* ---- 日常双通道 ---- */
assert(g("DAILY_SOURCES[0]") === "web端设计/data/daily.json" && g("DAILY_SOURCES.length") === 2,
  "日常数据双通道：预取 json 优先，直连 jx3api 兜底");

console.log(process.exitCode ? "\n存在失败项" : "\n全部通过");
