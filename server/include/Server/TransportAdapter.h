#pragma once

#include "Protocol/ProtocolTypes.h"
#include "Server/MatchService.h"
#include "Server/ProtocolMapper.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct FOutboundProtocolMessage
{
    FPlayerId PlayerId = 0;
    uint64_t ServerSequence = 0;
    FProtocolEnvelope Envelope{};
    std::optional<FProtocolJoinAckPayload> JoinAck;
    std::optional<FProtocolCommandAckPayload> CommandAck;
    std::optional<FProtocolSnapshotPayload> Snapshot;
    std::optional<FProtocolEventDeltaPayload> EventDelta;
    std::string ErrorMessage;
};

class IServerMessageSink
{
public:
    virtual ~IServerMessageSink() = default;
    virtual void Send(const FOutboundProtocolMessage& Message) = 0;
};

class FInMemoryServerMessageSink final : public IServerMessageSink
{
public:
    void Send(const FOutboundProtocolMessage& Message) override;

    const std::vector<FOutboundProtocolMessage>& GetAllMessages() const noexcept;
    std::vector<FOutboundProtocolMessage> PullMessages(FPlayerId PlayerId, uint64_t AfterServerSequence = 0) const;
    void Clear();

private:
    std::vector<FOutboundProtocolMessage> Messages;
};

class FServerTransportAdapter
{
public:
    explicit FServerTransportAdapter(FInMemoryMatchService* InMatchService, IServerMessageSink* InMessageSink);

    bool HandleJoinRequest(const FProtocolJoinPayload& JoinPayload);
    bool HandlePlayerCommand(FPlayerId PlayerId, const FPlayerCommand& Command);
    bool HandlePullSync(FPlayerId PlayerId, std::optional<uint64_t> AfterSequenceOverride = std::nullopt);
    bool HandleAck(FPlayerId PlayerId, uint64_t Sequence);

    uint64_t GetNextServerSequence() const noexcept;

private:
    void SendJoinAck(FPlayerId PlayerId, FMatchId MatchId, const FProtocolJoinAckPayload& Payload);
    void SendCommandAck(FPlayerId PlayerId, FMatchId MatchId, const FProtocolCommandAckPayload& Payload);
    void SendSnapshotAndDelta(FPlayerId PlayerId, const FMatchSyncResponse& SyncResponse);
    void SendError(FPlayerId PlayerId, FMatchId MatchId, std::string ErrorMessage);

    FOutboundProtocolMessage BuildMessageBase(FPlayerId PlayerId, FMatchId MatchId, EProtocolMessageType MessageType);

private:
    FInMemoryMatchService* MatchService = nullptr;
    IServerMessageSink* MessageSink = nullptr;
    uint64_t NextServerSequence = 1;
};
