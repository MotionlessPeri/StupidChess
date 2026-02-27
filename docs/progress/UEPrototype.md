# Progress - UE Prototype

## Last Updated

1. 2026-02-26

## Scope

1. `StupidChessCoreBridge` 的 UE 原型交互验证。
2. Battle/Setup 原型与信息不对等验证（Prototype A）。
3. 为 Prototype B（网络通信原型）做前置准备。

## Status

1. Battle Prototype v1 可交互闭环已打通。
2. Setup Prototype v1（手动摆子 -> Commit/Reveal -> Battle）可用。
3. Prototype A（单棋盘红黑视角切换 + 严格视图）已接入。
4. 当前在处理 Prototype A 的“表面职业显示口径”回归问题并做验证。

## Completed Highlights

1. Battle Prototype：
   - `Bootstrap Battle / Bootstrap Scrambled / Pull Both / Pass Current / Black Resign`
   - 棋盘点选提交走子 + 状态文本回显（Ack/Snapshot/EventDelta/GameOver）
2. Setup Prototype：
   - `Bootstrap Setup / Submit Setup / Undo Setup`
   - 合法起始位摆子、提交后进入 Battle
3. Prototype A：
   - `Show Red View / Show Black View / Toggle Strict View`
   - 严格视图下对手未公开实际职业显示为 `？`
4. UI 可用性：
   - 侧栏按钮前置，避免日志过长导致按钮不可见

## Current Focus

1. 表面职业显示口径回归修复（对应旧条目 78 的二次修复）：
   - 改为按合法起始位模板坐标直算表面职业（红方 canonical，黑方 Y 镜像）
   - 不依赖运行时 `SetupStandardPlacements*` 扫描结果
2. 在 PIE 下确认 `Bootstrap Scrambled + Show Red/Black View` 的展示一致性。

## Open Items

1. Prototype B：双客户端 + 本地 server + 真实 transport 最小闭环。
2. 将当前原型入口固化为稳定调试入口（减少手工 Level Blueprint 连接）。
3. 根据 Prototype A 验证结果，确定最终“玩家视图 vs 调试视图”展示策略。

## Source

1. 历史明细见 `docs/progress/LegacyProgressLog.md`（条目 68+ 为主）。
