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
   - 统一 `JoinAck/CommandAck/Snapshot/EventDelta` 的字段口径。

## 约束

1. 服务端是权威判定源；客户端不做最终规则裁决。
2. `PullPlayerSync` 返回值必须仅包含观察方可见数据。
3. 重连恢复以 `Sequence` 为准，保证事件单调递增与可重放。
