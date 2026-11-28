/* 把 index.html 的 128x128 canvas 调用录成 JSON，交给 render_frame.py 画成 PNG。
   用法：node web端设计/_test/render_frame.js [page] [out.json] */
const fs = require("fs"), vm = require("vm");
const page = process.argv[2] || "main";
const out = process.argv[3] || (__dirname + "/_frame.json");
const html = fs.readFileSync(__dirname + "/../../index.html", "utf8");
const script = html.match(/<script>([\s\S]*)<\/script>/)[1];

const cmds = [];
const state = { fillStyle: "#000", strokeStyle: "#000", font: "9px sans-serif", textBaseline: "top", globalAlpha: 1 };
const ctxStub = new Proxy(state, {
  get(t, k) {
    if (k === "fillRect") return (x, y, w, h) => cmds.push({ op: "rect", x, y, w, h, c: t.fillStyle, a: t.globalAlpha });
    if (k === "fillText") return (s, x, y) => cmds.push({ op: "text", s: String(s), x, y, c: t.fillStyle, font: t.font, a: t.globalAlpha });
    if (k === "measureText") return s => ({ width: String(s).length * 7 });
    if (k === "drawImage") return () => {};
    if (k in t) return t[k];
    return () => {};
  },
  set(t, k, v) { t[k] = v; return true; },
});
const canvasEl = { getContext: () => ctxStub, addEventListener() {}, width: 128, height: 128, style: {} };
function el(extra) {
  return Object.assign({
    _handlers: {}, addEventListener(ev, fn) { this._handlers[ev] = fn; },
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
  localStorage: { getItem: k => (k in store ? store[k] : null), setItem: (k, v) => { store[k] = String(v); }, removeItem: k => { delete store[k]; } },
  document: {
    getElementById: id => (id === "screen" ? canvasEl : (sandbox._els[id] = sandbox._els[id] || el())),
    querySelector: () => el(), body: { classList: { toggle() {}, contains: () => false } }, addEventListener() {},
  },
  window: { addEventListener() {} }, _els: {},
};
const ctx = vm.createContext(sandbox);
vm.runInContext(script, ctx, { filename: "index.html" });
vm.runInContext(`page = ${JSON.stringify(page)}; frame = 12; if (page === 'main') { S.mess = 0; S.poisoned = false; S.down = false; S.dungeonUntil = 0; S.homeLv = 0; } render();`, ctx);
fs.writeFileSync(out, JSON.stringify(cmds));
console.log("recorded " + cmds.length + " canvas ops -> " + out);
