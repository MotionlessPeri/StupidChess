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
17. `PullParseAndDispatchOutboundMessagesIncremental`：按玩家维护增量游标并拉取新消息，避免重复回调噪声。
18. `ResetPullCursor` / `GetPullCursor`：重置或读取增量拉取游标（支持按玩家或全量重置）。
19. `GetCachedCommandAckDebugString` / `GetCachedGameOverDebugString`：返回缓存确认/终局消息的调试摘要字符串（供蓝图快速打印关键字段）。
20. `GetLastPulledMessages` / `GetLastPulledMessageCount`：读取最近一次拉取的原始消息批次。
21. `DecodeJoinAckPayloadJson` / `DecodeCommandAckPayloadJson` / `DecodeErrorPayloadJson`：将常用确认/错误消息 JSON 解码为 UE 结构化视图。
22. `DecodeSnapshotPayloadJson` / `DecodeEventDeltaPayloadJson` / `DecodeGameOverPayloadJson`：将状态同步与终局消息 JSON 解码为 UE 结构化视图。
23. `TryParseJoinAckMessage` / `TryParseCommandAckMessage` / `TryParseErrorMessage`：按消息类型安全解析 outbox 确认与错误消息。
24. `TryParseSnapshotMessage` / `TryParseEventDeltaMessage` / `TryParseGameOverMessage`：按消息类型安全解析 outbox 状态同步与终局消息。

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

## Battle Prototype (C++ Runtime Widget)

`StupidChessCoreBridge` 现提供一个原型阶段可直接使用的运行时棋盘 Widget（纯 C++ 动态构建，不依赖手工搭 90 个格子）：

1. `UStupidChessBattlePrototypeWidget`
   - 运行时动态生成 `9x10` 棋盘格（可点击）
   - 侧栏按钮：`Bootstrap Battle / Pull Both / Pass Current / Black Resign`
   - 基于 `UStupidChessLocalMatchSubsystem` 的本地权威链路更新 UI（`Snapshot / EventDelta / Ack / GameOver`）
   - 支持“先选起点格，再点终点格”提交 `SubmitMove`
2. `UStupidChessBattlePrototypeBlueprintLibrary::ShowBattlePrototypeWidget`
   - 蓝图一键创建并 `AddToViewport`
   - 适合在 `Level Blueprint -> BeginPlay` 直接调用

### 最小使用方式（Prototype）

1. 在 `Level Blueprint` 的 `BeginPlay` 调用：
   - `ShowBattlePrototypeWidget(WorldContext=self)`
2. 进入 PIE 后点击 `Bootstrap Battle`
   - 自动执行 `ResetLocalServer -> Join(red/black) -> Commit/Reveal(red/black) -> Pull`
   - 进入战斗阶段后即可点击棋盘格尝试走子
3. 若要观察盲摆差异，点击 `Bootstrap Scrambled`
   - 仍使用合法初始摆位集合，但将棋子与位置随机映射（每方独立）
   - 更容易出现“可见职业 != 实际职业”的情况
4. 点击规则：
   - 第一次点击：选择当前回合方棋子
   - 第二次点击：作为目标格提交 `Move`
   - 点击已选中格：取消选择
5. 棋盘格文本（当前调试样式）：
   - 第一行：`R/B + PieceId`
   - 第二行：`<可见职业汉字>/<实际职业汉字> <Flags>`（所有棋子都固定显示 `可见/实际`）
   - `Flags`：`公` 表示该子已公开实际职业，`冻` 表示冻结，`-` 表示无标记

说明：

1. 当前是 Battle Prototype，目标是验证交互闭环与状态刷新，不是最终美术/完整玩法 UI。
2. 走子合法性仍由服务端权威裁定；非法操作请看侧栏 `Ack/Error` 文本。

## Automation Test

1. `StupidChess.UE.CoreBridge.LocalFlow`：覆盖本地链路 `Join -> Commit/Reveal -> Move -> Resign -> AckError`，并验证 `TryParse* + ParsedCache` 结构化解析接口。
2. `StupidChess.UE.CoreBridge.ErrorPaths`：覆盖非法命令/非法 payload 路径（本地校验拒绝、服务端拒绝、JSON 解码失败），并验证错误消息缓存解析。
3. `StupidChess.UE.CoreBridge.IncrementalPull`：覆盖按玩家增量游标拉取（首拉、空拉、增量拉取、重置游标重拉）。
4. 运行方式（UE 5.7 示例）：
   - `UnrealEditor-Cmd.exe StupidChessUE.uproject -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.LocalFlow; Quit" -unattended -nop4 -nosplash -NullRHI -culture=en`
   - `UnrealEditor-Cmd.exe StupidChessUE.uproject -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.ErrorPaths; Quit" -unattended -nop4 -nosplash -NullRHI -culture=en`
   - `UnrealEditor-Cmd.exe StupidChessUE.uproject -ExecCmds="Automation RunTests StupidChess.UE.CoreBridge.IncrementalPull; Quit" -unattended -nop4 -nosplash -NullRHI -culture=en`
