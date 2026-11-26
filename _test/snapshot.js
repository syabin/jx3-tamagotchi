/* ASCII 布局快照（临时工具）：把 fillRect 记到 128×128 网格里目检 */
const fs = require("fs"), vm = require("vm");
const script = fs.readFileSync(__dirname + "/../index.html", "utf8").match(/<script>([\s\S]*)<\/script>/)[1];
const grid = Array.from({ length: 128 }, () => Array(128).fill(" "));
const ctxStub = new Proxy(
  { measureText: s => ({ width: String(s).length * 8 }) },
  {
    get(t, k) {
      if (k in t) return t[k];
      return (x, y, w, h) => {
        if (k !== "fillRect") return;
        const CH = { "#141824": " ", "#2a3a5e": ".", "#0d0f18": ",", "#3a2f22": "=", "#1a1e2c": "P", "#22263a": "i", "#262a38": "i", "#2e3345": "f", "#d8c68a": "G", "#3a3f54": "h", "#e8c860": "$", "#20263a": " ", "#161a28": "_" };
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
vm.runInContext("confirmA(); confirmA(); S.mess = 2; S.coins = 888; S.mood = 80; render();", ctx);
console.log("+" + "-".repeat(128) + "+");
console.log(grid.map(r => "|" + r.join("") + "|").join("\n"));
console.log("+" + "-".repeat(128) + "+");
