/* 人物放大字符快照（临时工具）：单独渲染 drawPet 各姿态，按 2px 块采样目视检查 */
const fs = require("fs"), vm = require("vm");
const script = fs.readFileSync(__dirname + "/../../index.html", "utf8").match(/<script>([\s\S]*)<\/script>/)[1];
let grid = [];
const ctxStub = new Proxy(
  { measureText: s => ({ width: String(s).length * 8 }) },
  {
    get(t, k) {
      if (k in t) return t[k];
      return (x, y, w, h) => {
        if (k !== "fillRect") return;
        x = Math.round(x); y = Math.round(y);
        for (let yy = y; yy < y + h; yy++) for (let xx = x; xx < x + w; xx++) {
          while (grid.length <= yy) grid.push([]);
          grid[yy][xx] = t.fillStyle;
        }
      };
    },
    set(t, k, v) { t[k] = v; return true; },
  }
);
function el() { return { addEventListener() {}, onclick: null, onchange: null, classList: { toggle() {}, contains: () => false }, textContent: "", checked: false, value: "60", style: {} }; }
const store = {};
const sandbox = {
  console, Date, Math, JSON, Promise,
  fetch: () => Promise.reject(new Error("x")),
  requestAnimationFrame: () => 0,
  setTimeout: () => 0, setInterval: () => 0, clearTimeout() {}, clearInterval() {},
  localStorage: { getItem: k => store[k] ?? null, setItem: (k, v) => { store[k] = String(v); }, removeItem: k => { delete store[k]; } },
  document: {
    getElementById: id => (id === "screen" ? { getContext: () => ctxStub, addEventListener() {}, width: 128, height: 128 } : (sandbox._els[id] = sandbox._els[id] || el())),
    querySelector: () => el(), body: { classList: { toggle() {}, contains: () => false } }, addEventListener() {},
  },
  window: { addEventListener() {} }, _els: {},
};
const ctx = vm.createContext(sandbox);
vm.runInContext(script, ctx);

const CH = {
  "#2b2b3a": "@", "#e6e7f2": "H", "#c2c5da": "h", "#ffffff": "*",
  "#fce3d3": "o", "#f2a3ad": "b", "#24378f": "E", "#4366cf": "e",
  "#bcd4ff": ".", "#7b8ad4": "B", "#6675bf": "8", "#e8c860": "G",
  "#4a8db8": "R", "#e8eef5": "T", "#3a3a52": "l", "#a53a3a": "R",
  "#8c3a48": "M", "#c96b74": "m", "#6a8a4a": "x", "#8a5a2a": "w",
  "#b8c0d0": "s", "#f5e9b8": "+", "#9ad0e8": "~", "#d8d8e0": "U",
  "#e8d8a0": "f",
};
function draw(pose, label, sect) {
  grid = [];
  vm.runInContext("frame=100; S.sect=" + JSON.stringify(sect || "cy") + "; S.sect=" + JSON.stringify(sect || "cy") + "; drawPet(" + JSON.stringify(pose) + ", 4, 8);", ctx);
  console.log("\n=== " + label + " (" + (sect || "cy") + ") ===");
  for (let y = 2; y < 46; y += 2) {
    let row = "";
    for (let x = 0; x < 40; x += 2) row += CH[grid[y] && grid[y][x]] || " ";
    console.log(row);
  }
}
["idle", "eat", "sit", "sleep", "dungeon"].forEach(p => draw(p, p));
