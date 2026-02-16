# server

联网对战权威服务端。

## 当前骨架

1. `FInMemoryMatchSession`
   - 单房间裁判封装。
   - 负责 `Join`、命令提交、玩家视角投影、事件日志追加。
   - 事件支持 `Sequence` 游标增量拉取。
2. `FInMemoryMatchService`
   - 多房间管理与玩家绑定。
   - 玩家只允许绑定一个房间。
   - 支持 `AckPlayerEvents` 游标确认与 `PullPlayerSync` 断线重连增量恢复。
3. `FProtocolMapper`
   - 将 `MatchService/MatchSession` 内部模型映射为 `protocol` DTO。
   - 统一 `JoinAck/CommandAck/Snapshot/EventDelta/GameOver` 的字段口径。
4. `FServerTransportAdapter`
   - 处理 Join/Command/PullSync/Ack 请求入口。
   - 统一下发 `S2C_JoinAck/S2C_CommandAck/S2C_Snapshot/S2C_EventDelta/S2C_GameOver/S2C_Error`。
   - 当局面进入 `GameOver` 时，在同步消息后追加 `S2C_GameOver`。
   - `FInMemoryServerMessageSink` 提供测试与本地验证用 outbox。
5. `FServerGateway`
   - 接收 `ProtocolEnvelope`（或 JSON），解码 `C2S` payload 并路由到 transport adapter。
   - 当前支持 `C2S_Join/C2S_Command/C2S_PullSync/C2S_Ack/C2S_Ping`。

## 约束

1. 服务端是权威判定源；客户端不做最终规则裁决。
2. `PullPlayerSync` 返回值必须仅包含观察方可见数据。
3. 重连恢复以 `Sequence` 为准，保证事件单调递增与可重放。
