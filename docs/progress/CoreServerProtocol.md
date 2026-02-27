# Progress - Core/Server/Protocol

## Last Updated

1. 2026-02-26

## Scope

1. `core/` 规则引擎能力与一致性。
2. `server/` 权威会话、网关与消息分发。
3. `protocol/` DTO 与编解码。

## Status

1. 核心规则与会话链路已达 MVP 可用（含回归基线）。
2. 协议链路已支持 `Join/Command/PullSync/Ack/Snapshot/EventDelta/GameOver/Error`。
3. 当前主要缺口是“真实网络 transport 原型（Prototype B）”。

## Completed Highlights

1. 规则骨架与关键裁定回归已就绪：`Commit/Reveal`、合法行棋、`Pass`、首次吃子切换与冻结。
2. `FInMemoryMatchSession` + `FInMemoryMatchService` 已打通多房间、玩家绑定、增量游标与续拉逻辑。
3. `ProtocolCodec` 与 `ServerGateway` 已覆盖 C2S/S2C 主流程。
4. `FProtocolMapper` 与 transport adapter 已打通服务内模型到协议消息映射。
5. `S2C_GameOver` 链路已完整接入并有自动化覆盖。

## Regression Baseline

1. `ctest --preset vcpkg-debug-test --output-on-failure`（当前基线 39/39）。
2. 网关层与 UE bridge 自动化用例已覆盖 `LocalFlow/ErrorPaths/IncrementalPull`。

## Open Items

1. Prototype B：双客户端 + 本地 server + 真实 transport 最小闭环。
2. 重连与会话恢复在真实 transport 场景下的端到端验证。
3. 回放一致性用例扩展（跨端可重放验证）。

## Source

1. 历史明细见 `docs/progress/LegacyProgressLog.md`（条目 1-25 为主）。
