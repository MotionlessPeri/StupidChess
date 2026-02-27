# Progress

## Last Updated

1. 2026-02-26

## Purpose

1. 本文件只维护“全局总览 + 导航索引”。
2. 详细进展按工作流维护在 `docs/progress/*.md`。

## Workstream Index

1. Core/Server/Protocol：`docs/progress/CoreServerProtocol.md`
2. UnrealMCP 能力建设：`docs/progress/UnrealMCP.md`
3. UE Prototype：`docs/progress/UEPrototype.md`
4. 历史明细归档：`docs/progress/LegacyProgressLog.md`

## Current Milestone

1. Prototype A：完成信息不对等展示口径验证并稳定回归。
2. Prototype B：进入“双客户端 + 本地 server + 真实 transport”最小闭环规划与实现。

## In Progress

1. Prototype A 表面职业映射回归修复验证（PIE 场景：`Bootstrap Scrambled + Show Red/Black View`）。
2. 将 UE 原型入口固化为稳定调试入口，减少手工 Level Blueprint 连接。
3. 规划 Prototype B 的最小实现拆分（transport、会话、双客户端联调脚手架）。

## Next Steps

1. 完成 Prototype A 回归验证并提交单独修复闭环。
2. 启动 Prototype B 第一阶段：本地 server + 两客户端连接握手（Join/Pull/Ack）。
3. 在 Prototype B 中复用 Prototype A 的“玩家视图/严格视图”口径做集成回归。

## Test Baseline

1. `ctest --preset vcpkg-debug-test --output-on-failure`（当前基线 39/39）。
2. `Build.bat StupidChessUEEditor Win64 Development ...`（UE 5.7 冷编译通过）。
3. UE 自动化：`LocalFlow / ErrorPaths / IncrementalPull` 通过。
