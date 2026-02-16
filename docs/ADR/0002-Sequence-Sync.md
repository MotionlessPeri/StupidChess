# ADR-0002: Sequence Cursor Sync For Reconnect

- Status: Accepted
- Date: 2026-02-16

## Context

联网 PvP 需要支持断线重连，客户端必须以确定性方式补齐断线期间事件。  
当前规则引擎已具备事件序列输出，但缺少服务层的游标确认与重连语义约束。

## Decision

1. 服务端事件日志采用房间内单调递增 `Sequence`。
2. 增量拉取接口以 `AfterSequence` 为游标，返回 `Events(Sequence > AfterSequence)`。
3. 服务层保存每个玩家的 `LastAckedSequence`：
   - `PullPlayerSync` 默认使用该游标；
   - 允许传入 override 游标用于全量重拉。
4. `Ack` 必须满足：
   - 不回退（`Ack >= LastAckedSequence`）；
   - 不越界（`Ack <= LatestSequence`）。
5. 协议层定义 `Snapshot/EventDelta` DTO，携带 `RequestedAfterSequence` 与 `LatestSequence`。

## Rationale

1. 统一游标语义可避免不同客户端实现各自定义“从哪开始补”。
2. `Ack + Delta` 能减少全量快照频率，降低带宽与重连抖动。
3. 规则层与网络层职责清晰：规则负责判定，服务层负责同步一致性。

## Consequences

1. 服务端需要维护玩家绑定状态与游标状态。
2. 客户端需持久化最近处理完成的 `Sequence`，用于重连恢复。
3. 协议变更需要保持向后兼容策略（后续通过版本字段管理）。

## Decision Update (2026-02-16)

1. 新增 `FServerTransportAdapter` 作为服务入口：
   - 处理 `Join/Command/PullSync/Ack`；
   - 统一下发 `S2C_JoinAck/S2C_CommandAck/S2C_Snapshot/S2C_EventDelta/S2C_Error`。
2. `Join` 成功后立即下发一次 `Snapshot + EventDelta`，保证客户端首帧可见状态与事件游标对齐。
3. 命令被接受后，按同房玩家视角分别下发 `Snapshot + EventDelta`，避免前端自行推导规则结论。
4. 新增 `FServerGateway + ProtocolCodec`：
   - 负责 `C2S_Join/C2S_Command/C2S_PullSync/C2S_Ack` 的解码与路由；
   - 统一 `ProtocolEnvelope.PayloadJson` 的编解码入口，替代临时调试字符串。
