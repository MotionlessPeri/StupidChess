# protocol

跨端共享协议定义目录。

## 当前内容

1. 基础消息信封：`FProtocolEnvelope`
2. 入局/命令确认：`FProtocolJoinPayload`、`FProtocolJoinAckPayload`、`FProtocolCommandAckPayload`
3. 快照与增量：`FProtocolSnapshotPayload`、`FProtocolEventDeltaPayload`

## 设计说明

1. `S2C_Snapshot` 只表达当前视角可见棋盘状态，不包含对手隐藏真值。
2. `S2C_EventDelta` 以 `RequestedAfterSequence` 和 `LatestSequence` 进行断线续拉。
3. 当前仅定义 DTO，不绑定具体序列化协议（JSON/二进制后续在 adapter 决策）。
4. DTO 的组包职责在 `server` 侧 `FProtocolMapper`，避免规则层与协议层直接耦合。
