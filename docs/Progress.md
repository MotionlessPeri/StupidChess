# Progress

## Last Updated

1. 2026-02-16

## Current Milestone

1. 完成联网 PvP 服务端骨架（会话、房间、事件增量同步、协议映射），进入客户端接入准备阶段。

## Completed

1. `core` 规则骨架与关键回归测试：`Commit/Reveal`、合法行棋、`Pass` 裁定、首次吃子切换与冻结。
2. `server` 会话层：`FInMemoryMatchSession`（玩家加入、命令提交、视角投影、事件日志）。
3. `server` 房间层：`FInMemoryMatchService`（多房间、玩家绑定、`Ack` 游标、断线续拉/全量重拉）。
4. `protocol` DTO：`JoinAck/CommandAck/Snapshot/EventDelta`。
5. `server` 映射层：`FProtocolMapper`（内部模型到协议 DTO）。
6. 文档与 ADR 已同步：`RuleSpec`、`Architecture`、`InterfaceSpec`、`ADR-0002`。

## In Progress

1. 规划 server transport adapter（把 `ProtocolMapper` 结果组包为可传输消息）。

## Next Steps

1. 增加 transport 层接口骨架（连接、收发、会话绑定）。
2. 补齐 `S2C_Snapshot/S2C_EventDelta` 发送路径示例与测试。
3. 启动 UE 客户端 facade 的 `Join/Sync/Ack` 对接骨架。

## Test Baseline

1. `ctest --preset vcpkg-debug-test --output-on-failure` 当前为全通过（24/24）。
