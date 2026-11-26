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

/* ---- 家园页：打扫并入 + 升级 ---- */
g("doAct('home')");
assert(g("page") === "home", "A 进家园页");
assert(g("homeOptions().join(',')") === "up", "无狼藉时只有升级选项");
g("S.mess = 2;");
assert(g("homeOptions().join(',')") === "up,clean", "有狼藉时出现打扫选项");
g("moveSel(1)");
assert(g("homeSel") === 1, "▲▼ 切到打扫");
g("confirmA()");
assert(g("S.mess") === 0 && g("page") === "home", "打扫生效且留在家园页");
g("S.coins = 1000; S.homeLv = 0; homeSel = 0; confirmA();");
assert(g("S.homeLv") === 1 && g("S.coins") === 500, "升级扣款 500 并升到 1 级");
g("page = 'main'; render();");

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
