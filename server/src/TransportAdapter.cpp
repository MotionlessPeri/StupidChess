#include "Server/TransportAdapter.h"

#include <sstream>

namespace
{
std::string EscapeJsonString(const std::string& Text)
{
    std::string Escaped;
    Escaped.reserve(Text.size());
    for (const char Ch : Text)
    {
        if (Ch == '\\' || Ch == '"')
        {
            Escaped.push_back('\\');
        }
        Escaped.push_back(Ch);
    }

    return Escaped;
}
}

void FInMemoryServerMessageSink::Send(const FOutboundProtocolMessage& Message)
{
    Messages.push_back(Message);
}

const std::vector<FOutboundProtocolMessage>& FInMemoryServerMessageSink::GetAllMessages() const noexcept
{
    return Messages;
}

std::vector<FOutboundProtocolMessage> FInMemoryServerMessageSink::PullMessages(FPlayerId PlayerId, uint64_t AfterServerSequence) const
{
    std::vector<FOutboundProtocolMessage> Result;
    for (const FOutboundProtocolMessage& Message : Messages)
    {
        if (Message.PlayerId == PlayerId && Message.ServerSequence > AfterServerSequence)
        {
            Result.push_back(Message);
        }
    }

    return Result;
}

void FInMemoryServerMessageSink::Clear()
{
    Messages.clear();
}

FServerTransportAdapter::FServerTransportAdapter(FInMemoryMatchService* InMatchService, IServerMessageSink* InMessageSink)
    : MatchService(InMatchService)
    , MessageSink(InMessageSink)
{
}

bool FServerTransportAdapter::HandleJoinRequest(const FProtocolJoinPayload& JoinPayload)
{
    if (MatchService == nullptr || MessageSink == nullptr)
    {
        return false;
    }

    const FMatchJoinRequest JoinRequest{
        JoinPayload.MatchId,
        JoinPayload.PlayerId};

    const FMatchJoinResponse JoinResponse = MatchService->JoinMatch(JoinRequest);
    SendJoinAck(JoinPayload.PlayerId, JoinPayload.MatchId, FProtocolMapper::BuildJoinAckPayload(JoinResponse));
    if (!JoinResponse.bAccepted)
    {
        return false;
    }

    return HandlePullSync(JoinPayload.PlayerId);
}

bool FServerTransportAdapter::HandlePlayerCommand(FPlayerId PlayerId, const FPlayerCommand& Command)
{
    if (MatchService == nullptr || MessageSink == nullptr)
    {
        return false;
    }

    const std::optional<FMatchId> MatchId = MatchService->FindPlayerMatch(PlayerId);
    if (!MatchId.has_value())
    {
        SendError(PlayerId, 0, "Player is not bound to any match.");
        return false;
    }

    const FCommandResult Result = MatchService->SubmitPlayerCommand(PlayerId, Command);
    SendCommandAck(PlayerId, MatchId.value(), FProtocolMapper::BuildCommandAckPayload(Result));
    if (!Result.bAccepted)
    {
        return false;
    }

    const std::vector<FPlayerId> PlayersInMatch = MatchService->GetPlayersInMatch(MatchId.value());
    for (const FPlayerId MatchPlayerId : PlayersInMatch)
    {
        const FMatchSyncResponse SyncResponse = MatchService->PullPlayerSync(MatchPlayerId);
        if (!SyncResponse.bAccepted)
        {
            SendError(MatchPlayerId, MatchId.value(), SyncResponse.ErrorMessage);
            continue;
        }

        SendSnapshotAndDelta(MatchPlayerId, SyncResponse);
    }

    return true;
}

bool FServerTransportAdapter::HandlePullSync(FPlayerId PlayerId, std::optional<uint64_t> AfterSequenceOverride)
{
    if (MatchService == nullptr || MessageSink == nullptr)
    {
        return false;
    }

    const FMatchSyncResponse SyncResponse = MatchService->PullPlayerSync(PlayerId, AfterSequenceOverride);
    if (!SyncResponse.bAccepted)
    {
        SendError(PlayerId, SyncResponse.MatchId, SyncResponse.ErrorMessage);
        return false;
    }

    SendSnapshotAndDelta(PlayerId, SyncResponse);
    return true;
}

bool FServerTransportAdapter::HandleAck(FPlayerId PlayerId, uint64_t Sequence)
{
    if (MatchService == nullptr || MessageSink == nullptr)
    {
        return false;
    }

    const std::optional<FMatchId> MatchId = MatchService->FindPlayerMatch(PlayerId);
    if (!MatchId.has_value())
    {
        SendError(PlayerId, 0, "Player is not bound to any match.");
        return false;
    }

    if (MatchService->AckPlayerEvents(PlayerId, Sequence))
    {
        return true;
    }

    SendError(PlayerId, MatchId.value(), "Ack sequence is invalid.");
    return false;
}

uint64_t FServerTransportAdapter::GetNextServerSequence() const noexcept
{
    return NextServerSequence;
}

void FServerTransportAdapter::SendJoinAck(FPlayerId PlayerId, FMatchId MatchId, const FProtocolJoinAckPayload& Payload)
{
    FOutboundProtocolMessage Message = BuildMessageBase(PlayerId, MatchId, EProtocolMessageType::S2C_JoinAck);
    Message.JoinAck = Payload;
    Message.Envelope.PayloadJson = BuildPayloadJson(Payload);
    MessageSink->Send(Message);
}

void FServerTransportAdapter::SendCommandAck(FPlayerId PlayerId, FMatchId MatchId, const FProtocolCommandAckPayload& Payload)
{
    FOutboundProtocolMessage Message = BuildMessageBase(PlayerId, MatchId, EProtocolMessageType::S2C_CommandAck);
    Message.CommandAck = Payload;
    Message.Envelope.PayloadJson = BuildPayloadJson(Payload);
    MessageSink->Send(Message);
}

void FServerTransportAdapter::SendSnapshotAndDelta(FPlayerId PlayerId, const FMatchSyncResponse& SyncResponse)
{
    const FProtocolSyncBundle Bundle = FProtocolMapper::BuildSyncBundle(SyncResponse);

    FOutboundProtocolMessage SnapshotMessage = BuildMessageBase(PlayerId, SyncResponse.MatchId, EProtocolMessageType::S2C_Snapshot);
    SnapshotMessage.Snapshot = Bundle.Snapshot;
    SnapshotMessage.Envelope.PayloadJson = BuildPayloadJson(Bundle.Snapshot);
    MessageSink->Send(SnapshotMessage);

    FOutboundProtocolMessage DeltaMessage = BuildMessageBase(PlayerId, SyncResponse.MatchId, EProtocolMessageType::S2C_EventDelta);
    DeltaMessage.EventDelta = Bundle.EventDelta;
    DeltaMessage.Envelope.PayloadJson = BuildPayloadJson(Bundle.EventDelta);
    MessageSink->Send(DeltaMessage);
}

void FServerTransportAdapter::SendError(FPlayerId PlayerId, FMatchId MatchId, std::string ErrorMessage)
{
    FOutboundProtocolMessage Message = BuildMessageBase(PlayerId, MatchId, EProtocolMessageType::S2C_Error);
    Message.ErrorMessage = std::move(ErrorMessage);
    Message.Envelope.PayloadJson = "{\"error\":\"" + EscapeJsonString(Message.ErrorMessage) + "\"}";
    MessageSink->Send(Message);
}

FOutboundProtocolMessage FServerTransportAdapter::BuildMessageBase(FPlayerId PlayerId, FMatchId MatchId, EProtocolMessageType MessageType)
{
    FOutboundProtocolMessage Message{};
    Message.PlayerId = PlayerId;
    Message.ServerSequence = NextServerSequence++;
    Message.Envelope = FProtocolEnvelope{
        MessageType,
        Message.ServerSequence,
        std::to_string(MatchId),
        {}};
    return Message;
}

std::string FServerTransportAdapter::BuildPayloadJson(const FProtocolJoinAckPayload& Payload) const
{
    std::ostringstream Stream;
    Stream << "{\"accepted\":" << (Payload.bAccepted ? "true" : "false")
           << ",\"assignedSide\":" << Payload.AssignedSide
           << ",\"errorCode\":\"" << EscapeJsonString(Payload.ErrorCode) << "\""
           << ",\"errorMessage\":\"" << EscapeJsonString(Payload.ErrorMessage) << "\"}";
    return Stream.str();
}

std::string FServerTransportAdapter::BuildPayloadJson(const FProtocolCommandAckPayload& Payload) const
{
    std::ostringstream Stream;
    Stream << "{\"accepted\":" << (Payload.bAccepted ? "true" : "false")
           << ",\"errorCode\":\"" << EscapeJsonString(Payload.ErrorCode) << "\""
           << ",\"errorMessage\":\"" << EscapeJsonString(Payload.ErrorMessage) << "\"}";
    return Stream.str();
}

std::string FServerTransportAdapter::BuildPayloadJson(const FProtocolSnapshotPayload& Payload) const
{
    std::ostringstream Stream;
    Stream << "{\"phase\":" << Payload.Phase
           << ",\"turnIndex\":" << Payload.TurnIndex
           << ",\"lastEventSequence\":" << Payload.LastEventSequence
           << ",\"piecesCount\":" << Payload.Pieces.size() << "}";
    return Stream.str();
}

std::string FServerTransportAdapter::BuildPayloadJson(const FProtocolEventDeltaPayload& Payload) const
{
    std::ostringstream Stream;
    Stream << "{\"requestedAfter\":" << Payload.RequestedAfterSequence
           << ",\"latest\":" << Payload.LatestSequence
           << ",\"eventsCount\":" << Payload.Events.size() << "}";
    return Stream.str();
}
