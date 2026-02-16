# Progress

## Last Updated

1. 2026-02-16

## Current Milestone

1. 完成 UE 侧桥接可消费化（结构化解析 + 解析缓存），为蓝图绑定和消息派发层做准备。

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
11. UE CoreBridge 模块接入：
    - 新增 `StupidChessCoreBridge` 模块并挂入 `.uproject` 与 Targets。
    - 在 UE 模块内编译共享 `core/protocol/server` 源（桥接编译单元）。
    - 新增 `UStupidChessLocalMatchSubsystem`，暴露 `Join/PullSync/Ack` 与 outbox 拉取接口。
12. UE 编译兼容修复：`EStupidChessProtocolMessageType` 增加 `Unknown=0`，满足 UHT 对 `UENUM` 默认值的要求。
13. UE 编译修复：
    - `protocol/src/ProtocolCodec.cpp` 的内部 JSON 节点类型重命名为 `FProtocolJsonValue`，避免与 UE 全局 `FJsonValue` 冲突。
    - `UStupidChessLocalMatchSubsystem` 改为显式管理 `FStupidChessServerRuntime*` 生命周期，规避 UHT 生成单元中的不完整类型删除错误。
14. UE 命令提交封装：
    - `UStupidChessLocalMatchSubsystem` 新增 `SubmitCommitSetup/SubmitRevealSetup/SubmitPass`。
    - `RevealSetup` 支持蓝图摆子结构 `FStupidChessSetupPlacement` 输入，并统一编码到 `C2S_Command`。
15. UE 命令提交封装扩展：
    - `UStupidChessLocalMatchSubsystem` 新增 `SubmitMove/SubmitResign`。
    - 新增蓝图结构 `FStupidChessMoveCommand`，用于 `Move` 命令参数传递。
16. gateway 回归增强：
    - `ServerGatewayTests` 新增 battle phase 下 `Move` 与 `Resign` 路由用例。
    - 断言 `S2C_CommandAck + S2C_Snapshot + S2C_EventDelta` 广播序列与终局状态字段。
17. UE payload 解析层：
    - `UStupidChessLocalMatchSubsystem` 新增 `DecodeSnapshotPayloadJson/DecodeEventDeltaPayloadJson`。
    - 新增 `TryParseSnapshotMessage/TryParseEventDeltaMessage`，便于按消息类型安全解码。
    - 新增结构化视图模型：`FStupidChessSnapshotView`、`FStupidChessEventDeltaView` 等。
18. UE payload 解析层扩展：
    - 新增 `DecodeJoinAckPayloadJson/DecodeCommandAckPayloadJson/DecodeErrorPayloadJson`。
    - 新增 `TryParseJoinAckMessage/TryParseCommandAckMessage/TryParseErrorMessage`。
    - 新增结构化视图模型：`FStupidChessJoinAckView`、`FStupidChessCommandAckView`、`FStupidChessErrorView`。
19. UE bridge 自动化冒烟测试：
    - 新增 `StupidChess.UE.CoreBridge.LocalFlow` 自动化用例。
    - 覆盖 `Join -> Commit/Reveal -> Move -> Resign -> AckError` 全链路与 `TryParse*` 解析断言。
20. UE bridge 错误路径自动化测试：
    - 新增 `StupidChess.UE.CoreBridge.ErrorPaths` 自动化用例。
    - 覆盖本地参数校验拒绝、服务端拒绝命令、以及 JSON 解码失败路径。
21. UE bridge 解析缓存能力：
    - `UStupidChessLocalMatchSubsystem` 新增 `ResetParsedCache/ParseOutboundMessagesToCache` 与 `GetCached*` 接口。
    - `LocalFlow` 与 `ErrorPaths` 自动化用例新增缓存读取断言（确认/快照/增量/错误消息）。
22. UE bridge 消息派发入口：
    - 新增 `PullParseAndDispatchOutboundMessages`，支持一次调用完成拉取、缓存更新与事件分发。
    - 新增 `BlueprintAssignable` 事件：`OnJoinAckParsed/OnCommandAckParsed/OnErrorParsed/OnSnapshotParsed/OnEventDeltaParsed`。
    - 新增 `GetLastPulledMessages/GetLastPulledMessageCount`，便于表现层在事件回调后读取原始批次。
    - 自动化用例迁移到新入口，覆盖 `Join/Move/InvalidAck` 路径。
23. 终局协议与 UE 接入补齐：
    - `protocol/server` 新增 `FProtocolGameOverPayload` 与 `S2C_GameOver` 编解码/下发链路。
    - `UStupidChessLocalMatchSubsystem` 新增 `GameOver` 结构化解码、缓存读取与 `OnGameOverParsed` 派发事件。
    - `ServerGateway/UE` 自动化测试新增 `S2C_GameOver` 断言，覆盖认输终局路径。

## In Progress

1. 准备 UE 项目内的蓝图接线模板（GameInstance/Widget 订阅 `On*Parsed`，驱动 UI 状态更新）。

## Next Steps

1. 在 UE 工程内创建最小蓝图样例（加入房间、提交命令、订阅 `On*Parsed` 更新文本/棋盘状态）。
2. 在蓝图层为 `S2C_GameOver` 增加终局 UI 流程（弹窗/结算态/重开入口）。
3. 评估 JSON 与二进制协议切换策略（是否保留 JSON 调试通道）。

## Test Baseline

1. `ctest --preset vcpkg-debug-test --output-on-failure` 当前为全通过（37/37）。
2. `Build.bat StupidChessUEEditor Win64 Development ...` 当前编译通过（UE 5.7）。
3. `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.LocalFlow;Quit"` 当前通过（EXIT CODE: 0）。
4. `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.ErrorPaths;Quit"` 当前通过（EXIT CODE: 0）。
