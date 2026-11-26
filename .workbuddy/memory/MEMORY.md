# 项目长期约定（用户明确要求，必须遵守）

## 双仓库发布流程（2026-11-26 定，「后面都这么做」）

- **完整项目**存 https://github.com/syabin/Tamagotchi （远端名 `tamagotchi`，main 上游）——**不开 GitHub Pages**。
- **网页**另外单独存一份到 https://github.com/syabin/jx3-tamagotchi （远端名 `origin`），该仓库开 GitHub Pages，用于在线试玩。
- 每次改动：先 commit → push 到 `tamagotchi`；若网页（index.html / version.json / 设定集等）有更新，**同时** push 到 `origin` 让 Pages 生效。
- 推送命令沿用踩坑解法：`env -u http_proxy -u https_proxy -u HTTP_PROXY -u HTTPS_PROXY -u ALL_PROXY -u all_proxy GIT_TERMINAL_PROMPT=0 git -c http.schannelCheckRevoke=false push <remote> main`。
- Pages 生效约 40~90 秒；CDN 缓存 max-age=600，验证时加 `?t=时间戳`。

## 其他固定约定

- 数值改动先改 `design/设定集.md` 再同步代码；版本号三处同步（`APP_VERSION` / `version.json` / 设定集头部）。
- 日常数据（v7.3 双通道定稿）：①网页端读 Actions 预取的同源 `data/daily.json`（`.github/workflows/fetch-daily.yml` 每小时刷新，零跨域零代理零服务器，走客户端网络）；②`data/daily.json` 不存在时（ESP32 主机端）自动直连 jx3api；③全失败用 localStorage 缓存（`jx3daily`）兜底。**不用任何公共 CORS 代理**：r.jina.ai 是海外服务（用户明确不用，且 2026-11-26 实测成功率仅 1/4）、allorigins/codetabs 全灭、corsproxy.io 要 key、cors.lol 限流。

## 干净发布流程（v7.2 起生效，jx3-tamagotchi 不带 .workbuddy）

- 本地分支 `pages` = main + 一条「剔除 .workbuddy」提交（5dd2239）；origin（jx3-tamagotchi）的 main 只收 pages 分支。
- 发布用独立 worktree `D:/workspace/Tamagotchi-pages`（checkout pages），**主工作区永远停在 main，不要在主工作区切 pages**（否则 .workbuddy 未跟踪/跟踪冲突）。
- 每次发布网页三步（在 Tamagotchi-pages 目录执行）：
  1. `git merge main`（.workbuddy 若报 modify/delete 冲突：`git rm -r -f .workbuddy` 解决——该目录里的副本可随时从 tamagotchi 仓恢复）
  2. 若 merge 把 .workbuddy 带回为已跟踪：`git rm -r --cached .workbuddy && git commit -m "deploy: 剔除 .workbuddy"`；残留未跟踪文件可删（从 tamagotchi 可恢复）
  3. `git push origin pages:main`
- Tamagotchi 仓（tamagotchi 远端）收 main 全量（含 .workbuddy 日志）；jx3-tamagotchi 仓（origin 远端）只收网页+README，不含日志。
