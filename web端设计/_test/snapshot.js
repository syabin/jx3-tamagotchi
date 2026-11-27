/* ASCII 布局快照（临时工具）：把 fillRect 记到 128×128 网格里目检 */
const fs = require("fs"), vm = require("vm");
const script = fs.readFileSync(__dirname + "/../../index.html", "utf8").match(/<script>([\s\S]*)<\/script>/)[1];
const grid = Array.from({ length: 128 }, () => Array(128).fill(" "));
const ctxStub = new Proxy(
  { measureText: s => ({ width: String(s).length * 8 }) },
  {
    get(t, k) {
      if (k in t) return t[k];
      return (x, y, w, h) => {
        if (k !== "fillRect") return;
        const CH = { "#141824": " ", "#2a3a5e": ".", "#0d0f18": ",", "#3a2f22": "=", "#1a1e2c": "P", "#22263a": "i", "#262a38": "i", "#2e3345": "f", "#d8c68a": "G", "#3a3f54": "h", "#e8c860": "$", "#20263a": " ", "#161a28": "_", "#31436c": "`", "#1f2c48": "m", "#0a0c14": "!", "#11131f": ",", "#2a3d2a": "g", "#5a4632": "|", "#6a5438": "|", "#3a5a34": "v", "#4a5a80": "c", "#3e4d70": "c", "#f5e9b8": "O", "#e8d890": "o", "#d8d8e8": "O", "#1d3a5e": "~",
          /* v0.2.1 像素级场景/人物新色 */
          "#25345a": ".", "#0a0c15": ",", "#39415e": "-", "#5a6488": "+",
          "#5a6a92": "c", "#1e2438": "c", "#181d30": "c", "#131728": "c",
          "#cfd8ea": "*", "#8a94b8": ".", "#2a3350": "~", "#f0f0f8": "*", "#b8b8cc": ".",
          "#4a5a84": "O", "#f0e2a0": "O", "#1c2438": "b",
          "#26365a": "M", "#0c0f1a": "M",
          "#2e4a28": "T", "#3f6a38": "t", "#c04858": "R",
          "#4a6e40": "v", "#5a7e4a": "v", "#4a3c2a": ":", "#2e2418": "=",
          "#e86a6a": "r", "#f5d858": "y", "#e88ab8": "p", "#f5e9b8": "O",
          "#7a6a4c": "s", "#8a7a5a": "s", "#6e5838": "|",
          /* 人物色 */
          "#2b2b3a": "L", "#e6e7f2": "H", "#c2c5da": "h", "#ffffff": "W", "#d4d6ea": "+",
          "#fce3d3": "S", "#eec9b4": "s", "#f2a3ad": "B", "#f6c3cb": "B",
          "#24378f": "E", "#4366cf": "e", "#9db8ff": "e", "#7b8ad4": "K", "#6675bf": "k",
          "#8c3a48": "u", "#e88a94": "u", "#c96b74": "u",
          "#4a8db8": "Q", "#3a6f94": "q", "#e8eef5": "Q", "#bfe3ff": "w",
          "#3a3a52": "l", "#23232f": "l", "#a3b0e8": "K",
          "#8a6a40": "|", "#c9b070": "G", "#b8c0d0": "I", "#8a5a2a": "|",
          "#d85a4a": "r", "#f0a060": "y", "#f0d878": "y", "#8a6a44": "A", "#6e5434": "a",
          "#6a5a3a": "A", "#544730": "a", "#a04030": "V", "#7e3026": "v", "#4a3a26": "V", "#3a2e1e": "v",
          "#d8c68a": "G", "#b8a468": "g", "#e88f2a": "Y", "#e86a5a": "r", "#f5ecd0": "O",
          "#cfd4e8": "w", "#b8e88a": "n", "#d8e88a": "n", "#8a92ad": "w", "#6a5a3a": "A", "#8a7a52": "s",
          /* 底部按钮图标色 */
          "#7a6a3e": "-", "#1c2130": "i", "#332b1c": "i", "#10131c": " " };
        const ch = CH[t.fillStyle] || "?";
        x = Math.max(0, Math.round(x)); y = Math.max(0, Math.round(y));
        for (let yy = y; yy < Math.min(128, y + h); yy++)
          for (let xx = x; xx < Math.min(128, x + w); xx++) grid[yy][xx] = ch;
      };
    },
    set(t, k, v) { t[k] = v; return true; },
  }
);
function el() {
  return { addEventListener() {}, onclick: null, onchange: null, classList: { toggle() {}, contains: () => false }, textContent: "", checked: false, value: "60", style: {} };
}
const store = {};
const sandbox = {
  console, Date, Math, JSON, Promise,
  fetch: () => Promise.reject(new Error("x")),
  requestAnimationFrame: () => 0,
  setTimeout: () => 0, setInterval: () => 0, clearTimeout() {}, clearInterval() {},
  localStorage: { getItem: k => store[k] ?? null, setItem: (k, v) => { store[k] = String(v); }, removeItem: k => { delete store[k]; } },
  document: {
    getElementById: id => (id === "screen" ? { getContext: () => ctxStub, addEventListener() {}, width: 128, height: 128 } : (sandbox._els[id] = sandbox._els[id] || el())),
    querySelector: () => el(),
    body: { classList: { toggle() {}, contains: () => false } },
    addEventListener() {},
  },
  window: { addEventListener() {} },
  _els: {},
};
const ctx = vm.createContext(sandbox);
vm.runInContext(script, ctx);
vm.runInContext("page = 'main'; S.mess = 2; S.coins = 888; S.mood = 80; frame = 100; render();", ctx);
console.log("+" + "-".repeat(128) + "+");
console.log(grid.map(r => "|" + r.join("") + "|").join("\n"));
console.log("+" + "-".repeat(128) + "+");
/* 夜景：bornTs 回拨 12 分钟游戏时（speedF=60 → 12 小时）→ 20 点夜色（未就寝，可见流萤） */
for (const r of grid) r.fill(" ");
vm.runInContext("S.bornTs = Date.now() - 720000; frame = 130; render();", ctx);
console.log("=== NIGHT (20:00) ===");
console.log("+" + "-".repeat(128) + "+");
console.log(grid.map(r => "|" + r.join("") + "|").join("\n"));
console.log("+" + "-".repeat(128) + "+");
