# Blueprint Quick Start

本文件给出一个最小蓝图接线模板，目标是先打通本地权威链路，不依赖棋盘表现层。

## 1. 准备

1. 在 UE 项目中创建一个调试 `Widget Blueprint`，例如 `WBP_LocalMatchDebug`。
2. 在 Widget 中添加几个按钮和日志文本（`Multi-Line Text`）：
   - `BtnJoin`
   - `BtnCommitReveal`
   - `BtnRedMove`
   - `BtnBlackResign`
   - `BtnPullRed`
   - `BtnPullBlack`
3. 在 Widget 蓝图里增加变量：
   - `MatchId`（`int64`，默认 `900`）
   - `RedPlayerId`（`int64`，默认 `10001`）
   - `BlackPlayerId`（`int64`，默认 `10002`）
   - `LocalSubsystem`（`StupidChessLocalMatchSubsystem` 对象引用）

## 2. 初始化与事件绑定

1. `Event Construct`：
   - 调用 `Get Game Instance Subsystem`，类型选 `StupidChessLocalMatchSubsystem`，保存到 `LocalSubsystem`。
2. 在 `LocalSubsystem` 上绑定以下事件（`Assign`）：
   - `OnJoinAckParsed`
   - `OnCommandAckParsed`
   - `OnErrorParsed`
   - `OnSnapshotParsed`
   - `OnEventDeltaParsed`
   - `OnGameOverParsed`
3. 每个事件回调里将关键字段追加到日志文本，先确保链路可见。

## 3. 按钮接线

1. `BtnJoin`：
   - `ResetLocalServer`
   - `JoinLocalMatch(MatchId, RedPlayerId)`
   - `JoinLocalMatch(MatchId, BlackPlayerId)`
   - `PullParseAndDispatchOutboundMessages(RedPlayerId)`
   - `PullParseAndDispatchOutboundMessages(BlackPlayerId)`

2. `BtnCommitReveal`：
   - 红方：
     - `SubmitCommitSetup(MatchId, RedPlayerId, Red, "")`
     - `BuildStandardSetupPlacements(Red)`
     - `SubmitRevealSetup(MatchId, RedPlayerId, Red, "R", RedPlacements)`
   - 黑方：
     - `SubmitCommitSetup(MatchId, BlackPlayerId, Black, "")`
     - `BuildStandardSetupPlacements(Black)`
     - `SubmitRevealSetup(MatchId, BlackPlayerId, Black, "B", BlackPlacements)`
   - 之后分别 `PullParseAndDispatchOutboundMessages(RedPlayerId/BlackPlayerId)`

3. `BtnRedMove`（测试用）：
   - 组一个 `FStupidChessMoveCommand`：
     - `PieceId=11, From(0,3), To(0,4), bHasCapturedPieceId=false`
   - 调用 `SubmitMove(MatchId, RedPlayerId, Red, Move)`
   - 再分别 `PullParseAndDispatchOutboundMessages(RedPlayerId/BlackPlayerId)`

4. `BtnBlackResign`：
   - `SubmitResign(MatchId, BlackPlayerId, Black)`
   - 再分别 `PullParseAndDispatchOutboundMessages(RedPlayerId/BlackPlayerId)`
   - 在 `OnGameOverParsed` 里读：
     - `Result`
     - `EndReason`
     - `bIsDraw`
     - `WinnerSide`

5. `BtnPullRed` / `BtnPullBlack`：
   - 分别调用一次 `PullParseAndDispatchOutboundMessages(PlayerId)`，用于手动补拉。

## 4. 调试建议

1. 优先用 `OnErrorParsed` 查看失败原因，不要在蓝图侧猜测规则判定。
2. 终局 UI 推荐以 `OnGameOverParsed` 触发，以 `Snapshot` 字段作为最终状态显示来源。
3. 若 Rider Unit Test 出现 `Aborted` 但日志显示成功，优先在运行参数中加 `-culture=en`。
