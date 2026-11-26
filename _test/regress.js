/* 回归冒烟测试：最小 DOM/canvas stub + vm 跑 index.html 脚本。
   用法：node _test/regress.js   （改 index.html 后先跑这个） */
const fs = require("fs"), vm = require("vm");
const assert = (cond, msg) => { if (!cond) { console.error("FAIL: " + msg); process.exitCode = 1; } else console.log("ok  - " + msg); };

const html = fs.readFileSync(__dirname + "/../index.html", "utf8");
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
    addEventListener() {}, onclick: null, onchange: null,
    classList: { toggle() {}, contains: () => false, add() {}, remove() {} },
    textContent: "", checked: false, value: "60", style: {},
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

/* ---- 开机流程 ---- */
assert(g("typeof render === 'function' && typeof confirmA === 'function'"), "脚本可求值，核心函数存在");
assert(g("page") === "boot", "新档进入开机画面");
g("confirmA()");
assert(g("page") === "sect", "A 键进入门派选择");
g("confirmA()");
assert(g("page") === "main", "选门派后进入主界面");

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

/* ---- v7.4 新菜单布局 ---- */
assert(g("MENU.length") === 10, "菜单共 10 项（8 模块 + 设置 + 状态）");
assert(g("MENU.map(m=>m.id).join(',')") === "daily,bag,shop,sit,train,dig,game,home,settings,status",
  "菜单顺序 = 上4(daily,bag,shop,sit) + 下4(train,dig,game,home) + settings + status");
assert(g("MENU.some(m => m.id === 'clean')") === false, "打扫不再作为独立菜单项");

/* ---- 渲染不抛异常（含异常状态/副本/狼藉）---- */
g("S.mess=3; S.poisoned=true; S.dungeonUntil=Date.now()+5000; render(); render();");
g("S.poisoned=false; S.down=true; S.dungeonUntil=0; render();");
g("S.down=false; S.mess=0; render();");
assert(true, "主界面渲染无异常（中毒/重伤/副本中/狼藉）");

/* ---- 导航与非常驻提示 ---- */
g("menuIdx = 0; moveSel(-1);");
assert(g("menuIdx % MENU.length") === 9, "▲ 从第 0 项回绕到最后一项（状态面板）");
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

/* ---- v7.6 昼夜作息：22~6 时就寝封锁行动 ---- */
g("S.bornTs = Date.now() - 15 * 3600 * 1000 / 60;");   // 游戏时刻 = (15+8)%24 = 23 点
assert(g("isSleeping()") === true, "23 点判定为就寝时段");
g("S.cool = {}; doAct('dig');");
assert(g("!S.cool.dig"), "就寝时挖宝被封锁");
assert(String(g("canAct()")).indexOf("睡") >= 0, "canAct 提示就寝中");
g("render(); render();");   // 睡姿渲染不抛异常
g("S.bornTs = Date.now() - 5 * 3600 * 1000 / 60;");    // 回到 13 点
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
/* 分类导航：▲▼ 换分类，A/B 退出 */
g("doAct('bag'); bagSel = 0; catMode = false; moveSel(-1);");
assert(g("catMode") === true, "▲ 顶格上抬进入分类标签行");
g("moveSel(1);");
assert(g("bagCat") === "food", "分类行 ▼ 换到「食品」");
assert(g("bagSel") === 0, "换分类后列表选中重置");
g("confirmA();");
assert(g("catMode") === false, "A 键确认退出分类行");
g("moveSel(-1); pressB();");
assert(g("catMode") === false && g("page") === "bag", "分类行 B 退回列表而非主页");
g("pressB();");
assert(g("page") === "main", "列表页 B 回主页");
/* 详情页用过滤后的列表翻页 */
g("doAct('shop'); shopCat = 'deco'; shopSel = 0; confirmA(); render();");
assert(g("page === 'item' && itemFrom === 'shop'"), "商城家具分类可进详情");
g("pressB();");
g("doAct('shop'); shopCat = 'all'; shopSel = SHOP_ORDER.length - 1; confirmA(); render(); pressB();");
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

/* ---- 各子页渲染无异常 ---- */
["status", "bag", "shop", "daily", "settings"].forEach(p => {
  g("doAct('" + p + "'); render(); pressB();");
});
assert(g("page") === "main", "状态/背包/商城/日常/设置页进出正常");

/* ---- 存档 ---- */
g("save();");
assert(g("load()") === true, "存档可重新载入");

/* ---- 日常双通道 ---- */
assert(g("DAILY_SOURCES[0]") === "data/daily.json" && g("DAILY_SOURCES.length") === 2,
  "日常数据双通道：预取 json 优先，直连 jx3api 兜底");

console.log(process.exitCode ? "\n存在失败项" : "\n全部通过");
