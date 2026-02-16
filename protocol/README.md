# protocol

跨端共享协议定义目录。

## 当前内容

1. 基础消息信封：`FProtocolEnvelope`
2. 入局/命令确认：`FProtocolJoinPayload`、`FProtocolJoinAckPayload`、`FProtocolCommandAckPayload`
3. 快照与增量：`FProtocolSnapshotPayload`、`FProtocolEventDeltaPayload`
4. 终局信号：`FProtocolGameOverPayload`
5. C2S 重连控制：`FProtocolPullSyncPayload`、`FProtocolAckPayload`
6. 协议编解码：`ProtocolCodec::*`

## 设计说明

1. `S2C_Snapshot` 只表达当前视角可见棋盘状态，不包含对手隐藏真值。
2. `S2C_EventDelta` 以 `RequestedAfterSequence` 和 `LatestSequence` 进行断线续拉。
3. `S2C_GameOver` 用于终局事件触发，最终状态仍以 `S2C_Snapshot` 为权威。
4. 当前默认序列化为 JSON（`ProtocolCodec`），后续可替换为二进制编码。
5. DTO 的组包职责在 `server` 侧 `FProtocolMapper`，避免规则层与协议层直接耦合。
6. 当前 transport 骨架由 `FServerTransportAdapter` 负责发送到 sink/outbox，便于后续替换为真实网络层。
