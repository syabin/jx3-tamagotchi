/**
 * jx3-tamagotchi · 日常数据 CORS 代理（Cloudflare Worker）
 *
 * 背景：jx3api 的 /active/calendar 不返回 Access-Control-Allow-Origin，
 *      浏览器直连会被跨域拦掉；公共代理（r.jina.ai / allorigins / codetabs）
 *      时好时坏。自己挂一个 Worker 最稳，还能顺手做 10 分钟边缘缓存。
 *
 * 部署（免费额度足够）：
 *   1. 打开 https://dash.cloudflare.com/ → Workers & Pages → Create Worker
 *   2. 把本文件内容整体粘进去，Deploy
 *   3. 拿到地址形如 https://jx3-daily.你的账号.workers.dev
 *   4. 回到 index.html，把 DAILY_PROXY 改成：
 *        const DAILY_PROXY = "https://jx3-daily.你的账号.workers.dev/?url=";
 *      再发布一次即可（前端会自动 encodeURIComponent 目标地址）
 *
 * 调用示例：GET /?url=https%3A%2F%2Fwww.jx3api.com%2Factive%2Fcalendar%3Fmode%3Dday
 */

const CORS = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Methods": "GET,OPTIONS",
  "Access-Control-Allow-Headers": "*",
  "Access-Control-Max-Age": "86400",
};

const ALLOW_HOSTS = ["jx3api.com", "www.jx3api.com", "api.jx3api.com"];

export default {
  async fetch(request) {
    if (request.method === "OPTIONS") return new Response(null, { headers: CORS });

    const u = new URL(request.url);
    const target = u.searchParams.get("url");
    if (!target) {
      return new Response(JSON.stringify({ error: "missing ?url=" }), {
        status: 400,
        headers: { ...CORS, "Content-Type": "application/json; charset=utf-8" },
      });
    }

    let t;
    try { t = new URL(target); } catch (e) {
      return new Response(JSON.stringify({ error: "bad url" }), {
        status: 400, headers: { ...CORS, "Content-Type": "application/json; charset=utf-8" },
      });
    }
    if (!ALLOW_HOSTS.includes(t.hostname)) {
      return new Response(JSON.stringify({ error: "host not allowed" }), {
        status: 403, headers: { ...CORS, "Content-Type": "application/json; charset=utf-8" },
      });
    }

    const upstream = await fetch(t.toString(), {
      cf: { cacheTtl: 600, cacheEverything: true },
      headers: { "User-Agent": "jx3-tamagotchi/7.0" },
    });
    const body = await upstream.text();

    return new Response(body, {
      status: upstream.status,
      headers: {
        ...CORS,
        "Content-Type": "application/json; charset=utf-8",
        "Cache-Control": "public, max-age=600",
      },
    });
  },
};
