# clients/ue

UE 客户端实现目录。

## Version Control

1. UE 工程继续使用仓库 Git（Monorepo）。
2. UE 二进制资产（`*.uasset`、`*.umap` 等）通过 Git LFS 管理。
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
7. `SubmitPass`：封装 `C2S_Command(Pass)`。
8. `SubmitMove`：封装 `C2S_Command(Move)`。
9. `SubmitResign`：封装 `C2S_Command(Resign)`。
10. `PullOutboundMessages`：按玩家拉取 `S2C` outbox 消息（返回 UE 结构体，含消息类型与 `PayloadJson`）。
11. `DecodeJoinAckPayloadJson` / `DecodeCommandAckPayloadJson` / `DecodeErrorPayloadJson`：将常用确认/错误消息 JSON 解码为 UE 结构化视图。
12. `DecodeSnapshotPayloadJson` / `DecodeEventDeltaPayloadJson`：将状态同步消息 JSON 解码为 UE 结构化视图。
13. `TryParseJoinAckMessage` / `TryParseCommandAckMessage` / `TryParseErrorMessage`：按消息类型安全解析 outbox 确认与错误消息。
14. `TryParseSnapshotMessage` / `TryParseEventDeltaMessage`：按消息类型安全解析 outbox 状态同步消息。

当前阶段保持协议 JSON 透传，便于先打通网络/状态闭环，再逐步引入更强类型化的 UE ViewModel。

## Automation Test

1. `StupidChess.UE.CoreBridge.LocalFlow`：覆盖本地链路 `Join -> Commit/Reveal -> Move -> Resign -> AckError`，并验证 `TryParse*` 结构化解析接口。
2. 运行方式（UE 5.7 示例）：
   - `UnrealEditor-Cmd.exe StupidChessUE.uproject -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.LocalFlow; Quit" -unattended -nop4 -nosplash -NullRHI`
