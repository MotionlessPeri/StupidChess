# clients/ue

UE 客户端实现目录。

## Version Control

1. UE 工程继续使用仓库 Git（Monorepo）。
2. UE 二进制资产（`*.uasset`、`*.umap` 等）当前通过 Git 普通对象管理，并在 `.gitattributes` 中标记为 `binary`。
3. `Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/` 等生成目录不入库。

## Runtime Modules

1. `StupidChessUE`：UE 主游戏模块（表现层）。
2. `StupidChessCoreBridge`：桥接模块，在 UE 内编译并托管 `core/protocol/server` 共享 C++ 源。

## Bridge Entry

`StupidChessCoreBridge` 提供 `UStupidChessLocalMatchSubsystem`（`UGameInstanceSubsystem`）作为最小接入入口：

1. `ResetLocalServer`：重置本地权威服务与消息状态。
2. `JoinLocalMatch`：发送 `C2S_Join`。
3. `PullLocalSync`：发送 `C2S_PullSync`。
4. `AckLocalEvents`：发送 `C2S_Ack`。
5. `SubmitCommitSetup`：封装 `C2S_Command(CommitSetup)`。
6. `SubmitRevealSetup`：封装 `C2S_Command(RevealSetup)`，支持摆子列表传入。
7. `BuildStandardSetupPlacements`：返回标准 16 子摆放（红黑自动镜像），用于快速接 `RevealSetup`。
8. `SubmitPass`：封装 `C2S_Command(Pass)`。
9. `SubmitMove`：封装 `C2S_Command(Move)`。
10. `SubmitResign`：封装 `C2S_Command(Resign)`。
11. `PullOutboundMessages`：按玩家拉取 `S2C` outbox 消息（返回 UE 结构体，含消息类型与 `PayloadJson`）。
12. `ResetParsedCache`：清空 subsystem 内部最近一次结构化解析缓存。
13. `ParseOutboundMessagesToCache`：批量解析 outbox 消息并缓存最近一次 `JoinAck/CommandAck/Error/Snapshot/EventDelta/GameOver`。
14. `GetCachedJoinAck` / `GetCachedCommandAck` / `GetCachedError`：读取缓存确认/错误消息视图。
15. `GetCachedSnapshot` / `GetCachedEventDelta` / `GetCachedGameOver`：读取缓存状态同步与终局视图。
16. `PullParseAndDispatchOutboundMessages`：按玩家拉取 outbox、更新解析缓存并触发 Blueprint 事件分发。
17. `GetLastPulledMessages` / `GetLastPulledMessageCount`：读取最近一次拉取的原始消息批次。
18. `DecodeJoinAckPayloadJson` / `DecodeCommandAckPayloadJson` / `DecodeErrorPayloadJson`：将常用确认/错误消息 JSON 解码为 UE 结构化视图。
19. `DecodeSnapshotPayloadJson` / `DecodeEventDeltaPayloadJson` / `DecodeGameOverPayloadJson`：将状态同步与终局消息 JSON 解码为 UE 结构化视图。
20. `TryParseJoinAckMessage` / `TryParseCommandAckMessage` / `TryParseErrorMessage`：按消息类型安全解析 outbox 确认与错误消息。
21. `TryParseSnapshotMessage` / `TryParseEventDeltaMessage` / `TryParseGameOverMessage`：按消息类型安全解析 outbox 状态同步与终局消息。

`FStupidChessGameOverView` 除终局基础字段外，额外提供 UI 友好字段：

1. `bIsDraw`：是否和局。
2. `WinnerSide`：赢家阵营（`Red/Black`，无赢家时为 `-1`）。

### Event Dispatch

`UStupidChessLocalMatchSubsystem` 暴露以下 `BlueprintAssignable` 事件，供 UI/流程图直接订阅：

1. `OnJoinAckParsed`
2. `OnCommandAckParsed`
3. `OnErrorParsed`
4. `OnSnapshotParsed`
5. `OnEventDeltaParsed`
6. `OnGameOverParsed`

当前阶段保持协议 JSON 透传，便于先打通网络/状态闭环，再逐步引入更强类型化的 UE ViewModel。

最小蓝图接线模板见：`clients/ue/BlueprintQuickStart.md`。

## Automation Test

1. `StupidChess.UE.CoreBridge.LocalFlow`：覆盖本地链路 `Join -> Commit/Reveal -> Move -> Resign -> AckError`，并验证 `TryParse* + ParsedCache` 结构化解析接口。
2. `StupidChess.UE.CoreBridge.ErrorPaths`：覆盖非法命令/非法 payload 路径（本地校验拒绝、服务端拒绝、JSON 解码失败），并验证错误消息缓存解析。
3. 运行方式（UE 5.7 示例）：
   - `UnrealEditor-Cmd.exe StupidChessUE.uproject -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.LocalFlow; Quit" -unattended -nop4 -nosplash -NullRHI -culture=en`
   - `UnrealEditor-Cmd.exe StupidChessUE.uproject -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.ErrorPaths; Quit" -unattended -nop4 -nosplash -NullRHI -culture=en`
