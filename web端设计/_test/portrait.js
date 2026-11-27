/* 人物放大字符快照（临时工具）：单独渲染 drawPet 各姿态，按 1px 逐像素采样目视检查 */
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

/* 1px 采样字符映射（v0.2.2 细化人物色板） */
const CH = {
  "#2b2b3a": "@", "#e6e7f2": "H", "#c2c5da": "h", "#ffffff": "*", "#d4d6ea": "+",
  "#fce3d3": "o", "#eec9b4": "d", "#f2a3ad": "b", "#f6c3cb": "p",
  "#24378f": "E", "#4366cf": "e", "#9db8ff": "w",
  "#7b8ad4": "B", "#6675bf": "8", "#a3b0e8": "b", "#e8c860": "G",
  "#8c3a48": "M", "#e88a94": "m", "#c96b74": "m", "#6a8a4a": "x",
  "#4a8db8": "R", "#3a6f94": "r", "#e8eef5": "T", "#bfe3ff": "c",
  "#a53a3a": "R", "#7e2b2b": "r", "#ffb0a0": "c",
  "#d16a9a": "R", "#a94f7c": "r", "#f5e5f2": "T", "#ffc0e0": "c",
  "#a08040": "R", "#7e6630": "r", "#e8d8b0": "T", "#e8c890": "c",
  "#3a3a52": "l", "#23232f": "l", "#8a5a2a": "W", "#b8c0d0": "s",
  "#f5e9b8": "+", "#9ad0e8": "~", "#d8d8e0": "U", "#e8d8a0": "f",
  "#b8d88a": "v", "#e8f0ff": "/", "#f5ecd0": "+",
  "#4a7a3a": "n", "#e8d840": "y", "#2e4a24": "N",
  "#7a5a34": "P", "#8a6a40": "P", "#6a4a2a": "P",
};
function draw(pose, label, sect) {
  grid = [];
  vm.runInContext("frame=100; S.sect=" + JSON.stringify(sect || "cy") + "; drawPet(" + JSON.stringify(pose) + ", 4, 8, 3);", ctx);
  console.log("\n=== " + label + " (" + (sect || "cy") + ") ===");
  for (let y = 0; y < 76; y++) {
    let row = "";
    for (let x = 0; x < 56; x++) row += CH[grid[y] && grid[y][x]] || " ";
    if (row.trim()) console.log(row.replace(/\s+$/, ""));
  }
}
["idle", "eat", "sit", "sleep", "down", "sick", "train", "dig", "dungeon"].forEach(p => draw(p, p));
draw("idle", "idle-天策", "tc");
