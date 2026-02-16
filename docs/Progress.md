# Progress

## Last Updated

1. 2026-02-16

## Current Milestone

1. 完成联网 PvP 服务端消息闭环（会话、房间、协议编解码、gateway、transport adapter），等待 UE 项目创建后接入。

## Completed

1. `core` 规则骨架与关键回归测试：`Commit/Reveal`、合法行棋、`Pass` 裁定、首次吃子切换与冻结。
2. `server` 会话层：`FInMemoryMatchSession`（玩家加入、命令提交、视角投影、事件日志）。
3. `server` 房间层：`FInMemoryMatchService`（多房间、玩家绑定、`Ack` 游标、断线续拉/全量重拉）。
4. `protocol` DTO：`JoinAck/CommandAck/Snapshot/EventDelta`。
5. `server` 映射层：`FProtocolMapper`（内部模型到协议 DTO）。
6. 文档与 ADR 已同步：`RuleSpec`、`Architecture`、`InterfaceSpec`、`ADR-0002`。
7. `server` transport 骨架：`FServerTransportAdapter + FInMemoryServerMessageSink`（Join/Command/PullSync/Ack 下发路径）。
8. `protocol` 编解码：`ProtocolCodec`（Envelope/Join/Command/PullSync/Ack/Snapshot/EventDelta/Error）。
9. `server` 网关层：`FServerGateway`（`C2S` 消息解码与路由）。
10. UE 版本控制基线：补充 UE 生成物 `.gitignore` 与内容资产 `.gitattributes`（Git LFS）规则。

## In Progress

1. 等待你确认 UE 项目目录清单后，开始 UE 客户端 `Join/Sync/Ack` 最小接入。

## Next Steps

1. 你创建 UE 项目后，接入 `Join/Sync/Ack` 最小闭环。
2. 增加 “命令后双端同步一致” 的回放式集成测试。
3. 评估 JSON 与二进制协议切换策略（是否保留 JSON 调试通道）。

## Test Baseline

1. `ctest --preset vcpkg-debug-test --output-on-failure` 当前为全通过（35/35）。
