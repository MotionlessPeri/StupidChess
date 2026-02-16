#include "StupidChessLocalMatchSubsystem.h"

#include "Protocol/ProtocolCodec.h"
#include "Protocol/ProtocolTypes.h"
#include "Server/MatchService.h"
#include "Server/ServerGateway.h"
#include "Server/TransportAdapter.h"

#include <charconv>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>

namespace
{
EProtocolMessageType ToProtocolMessageType(EStupidChessProtocolMessageType MessageType)
{
    switch (MessageType)
    {
    case EStupidChessProtocolMessageType::C2S_Join:
        return EProtocolMessageType::C2S_Join;
    case EStupidChessProtocolMessageType::C2S_Command:
        return EProtocolMessageType::C2S_Command;
    case EStupidChessProtocolMessageType::C2S_Ping:
        return EProtocolMessageType::C2S_Ping;
    case EStupidChessProtocolMessageType::C2S_PullSync:
        return EProtocolMessageType::C2S_PullSync;
    case EStupidChessProtocolMessageType::C2S_Ack:
        return EProtocolMessageType::C2S_Ack;
    case EStupidChessProtocolMessageType::S2C_JoinAck:
        return EProtocolMessageType::S2C_JoinAck;
    case EStupidChessProtocolMessageType::S2C_CommandAck:
        return EProtocolMessageType::S2C_CommandAck;
    case EStupidChessProtocolMessageType::S2C_Snapshot:
        return EProtocolMessageType::S2C_Snapshot;
    case EStupidChessProtocolMessageType::S2C_EventDelta:
        return EProtocolMessageType::S2C_EventDelta;
    case EStupidChessProtocolMessageType::S2C_GameOver:
        return EProtocolMessageType::S2C_GameOver;
    case EStupidChessProtocolMessageType::S2C_Error:
        return EProtocolMessageType::S2C_Error;
    default:
        return EProtocolMessageType::S2C_Error;
    }
}

EStupidChessProtocolMessageType ToBridgeMessageType(EProtocolMessageType MessageType)
{
    switch (MessageType)
    {
    case EProtocolMessageType::C2S_Join:
        return EStupidChessProtocolMessageType::C2S_Join;
    case EProtocolMessageType::C2S_Command:
        return EStupidChessProtocolMessageType::C2S_Command;
    case EProtocolMessageType::C2S_Ping:
        return EStupidChessProtocolMessageType::C2S_Ping;
    case EProtocolMessageType::C2S_PullSync:
        return EStupidChessProtocolMessageType::C2S_PullSync;
    case EProtocolMessageType::C2S_Ack:
        return EStupidChessProtocolMessageType::C2S_Ack;
    case EProtocolMessageType::S2C_JoinAck:
        return EStupidChessProtocolMessageType::S2C_JoinAck;
    case EProtocolMessageType::S2C_CommandAck:
        return EStupidChessProtocolMessageType::S2C_CommandAck;
    case EProtocolMessageType::S2C_Snapshot:
        return EStupidChessProtocolMessageType::S2C_Snapshot;
    case EProtocolMessageType::S2C_EventDelta:
        return EStupidChessProtocolMessageType::S2C_EventDelta;
    case EProtocolMessageType::S2C_GameOver:
        return EStupidChessProtocolMessageType::S2C_GameOver;
    case EProtocolMessageType::S2C_Error:
        return EStupidChessProtocolMessageType::S2C_Error;
    default:
        return EStupidChessProtocolMessageType::S2C_Error;
    }
}
}

struct FStupidChessServerRuntime
{
    FInMemoryMatchService MatchService;
    FInMemoryServerMessageSink MessageSink;
    FServerTransportAdapter TransportAdapter;
    FServerGateway ServerGateway;

    FStupidChessServerRuntime()
        : TransportAdapter(&MatchService, &MessageSink)
        , ServerGateway(&TransportAdapter)
    {
    }
};

void UStupidChessLocalMatchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ResetLocalServer();
}

void UStupidChessLocalMatchSubsystem::Deinitialize()
{
    delete ServerRuntime;
    ServerRuntime = nullptr;
    Super::Deinitialize();
}

void UStupidChessLocalMatchSubsystem::ResetLocalServer()
{
    delete ServerRuntime;
    ServerRuntime = new FStupidChessServerRuntime();
    NextClientSequence = 1;
    ResetParsedCache();
}

bool UStupidChessLocalMatchSubsystem::JoinLocalMatch(int64 MatchId, int64 PlayerId)
{
    if (MatchId <= 0 || PlayerId <= 0 || ServerRuntime == nullptr)
    {
        return false;
    }

    const FProtocolJoinPayload Payload{
        static_cast<uint64_t>(MatchId),
        static_cast<uint64_t>(PlayerId)};

    std::string PayloadJson;
    if (!ProtocolCodec::EncodeJoinPayload(Payload, PayloadJson))
    {
        return false;
    }

    return ProcessEnvelope(EStupidChessProtocolMessageType::C2S_Join, MatchId, PayloadJson);
}

bool UStupidChessLocalMatchSubsystem::PullLocalSync(
    int64 MatchId,
    int64 PlayerId,
    bool bHasAfterSequenceOverride,
    int64 AfterSequenceOverride)
{
    if (MatchId <= 0 || PlayerId <= 0 || AfterSequenceOverride < 0 || ServerRuntime == nullptr)
    {
        return false;
    }

    FProtocolPullSyncPayload Payload{};
    Payload.PlayerId = static_cast<uint64_t>(PlayerId);
    Payload.bHasAfterSequenceOverride = bHasAfterSequenceOverride;
    Payload.AfterSequenceOverride = bHasAfterSequenceOverride ? static_cast<uint64_t>(AfterSequenceOverride) : 0;

    std::string PayloadJson;
    if (!ProtocolCodec::EncodePullSyncPayload(Payload, PayloadJson))
    {
        return false;
    }

    return ProcessEnvelope(EStupidChessProtocolMessageType::C2S_PullSync, MatchId, PayloadJson);
}

bool UStupidChessLocalMatchSubsystem::AckLocalEvents(int64 MatchId, int64 PlayerId, int64 Sequence)
{
    if (MatchId <= 0 || PlayerId <= 0 || Sequence < 0 || ServerRuntime == nullptr)
    {
        return false;
    }

    const FProtocolAckPayload Payload{
        static_cast<uint64_t>(PlayerId),
        static_cast<uint64_t>(Sequence)};

    std::string PayloadJson;
    if (!ProtocolCodec::EncodeAckPayload(Payload, PayloadJson))
    {
        return false;
    }

    return ProcessEnvelope(EStupidChessProtocolMessageType::C2S_Ack, MatchId, PayloadJson);
}

bool UStupidChessLocalMatchSubsystem::SubmitCommitSetup(int64 MatchId, int64 PlayerId, EStupidChessSide Side, const FString& HashHex)
{
    if (MatchId <= 0 || PlayerId <= 0 || ServerRuntime == nullptr)
    {
        return false;
    }

    FProtocolCommandPayload CommandPayload{};
    CommandPayload.PlayerId = static_cast<uint64_t>(PlayerId);
    CommandPayload.CommandType = static_cast<int32_t>(ECommandType::CommitSetup);
    CommandPayload.Side = ToProtocolSide(Side);
    CommandPayload.bHasSetupCommit = true;
    CommandPayload.SetupCommit.Side = CommandPayload.Side;
    CommandPayload.SetupCommit.HashHex = TCHAR_TO_UTF8(*HashHex);
    return SubmitCommandPayload(MatchId, CommandPayload);
}

bool UStupidChessLocalMatchSubsystem::SubmitRevealSetup(
    int64 MatchId,
    int64 PlayerId,
    EStupidChessSide Side,
    const FString& Nonce,
    const TArray<FStupidChessSetupPlacement>& Placements)
{
    if (MatchId <= 0 || PlayerId <= 0 || ServerRuntime == nullptr)
    {
        return false;
    }

    FProtocolCommandPayload CommandPayload{};
    CommandPayload.PlayerId = static_cast<uint64_t>(PlayerId);
    CommandPayload.CommandType = static_cast<int32_t>(ECommandType::RevealSetup);
    CommandPayload.Side = ToProtocolSide(Side);
    CommandPayload.bHasSetupPlain = true;
    CommandPayload.SetupPlain.Side = CommandPayload.Side;
    CommandPayload.SetupPlain.Nonce = TCHAR_TO_UTF8(*Nonce);
    CommandPayload.SetupPlain.Placements.reserve(static_cast<size_t>(Placements.Num()));

    for (const FStupidChessSetupPlacement& Placement : Placements)
    {
        if (Placement.PieceId < 0 || Placement.PieceId > 65535)
        {
            return false;
        }

        CommandPayload.SetupPlain.Placements.push_back(FProtocolSetupPlacementPayload{
            static_cast<uint16_t>(Placement.PieceId),
            Placement.X,
            Placement.Y});
    }

    return SubmitCommandPayload(MatchId, CommandPayload);
}

bool UStupidChessLocalMatchSubsystem::SubmitPass(int64 MatchId, int64 PlayerId, EStupidChessSide Side)
{
    if (MatchId <= 0 || PlayerId <= 0 || ServerRuntime == nullptr)
    {
        return false;
    }

    FProtocolCommandPayload CommandPayload{};
    CommandPayload.PlayerId = static_cast<uint64_t>(PlayerId);
    CommandPayload.CommandType = static_cast<int32_t>(ECommandType::Pass);
    CommandPayload.Side = ToProtocolSide(Side);
    return SubmitCommandPayload(MatchId, CommandPayload);
}

bool UStupidChessLocalMatchSubsystem::SubmitMove(int64 MatchId, int64 PlayerId, EStupidChessSide Side, const FStupidChessMoveCommand& Move)
{
    if (MatchId <= 0 || PlayerId <= 0 || ServerRuntime == nullptr)
    {
        return false;
    }

    if (Move.PieceId < 0 || Move.PieceId > 65535)
    {
        return false;
    }

    if (Move.bHasCapturedPieceId && (Move.CapturedPieceId < 0 || Move.CapturedPieceId > 65535))
    {
        return false;
    }

    FProtocolCommandPayload CommandPayload{};
    CommandPayload.PlayerId = static_cast<uint64_t>(PlayerId);
    CommandPayload.CommandType = static_cast<int32_t>(ECommandType::Move);
    CommandPayload.Side = ToProtocolSide(Side);
    CommandPayload.bHasMove = true;
    CommandPayload.Move.PieceId = static_cast<uint16_t>(Move.PieceId);
    CommandPayload.Move.FromX = Move.FromX;
    CommandPayload.Move.FromY = Move.FromY;
    CommandPayload.Move.ToX = Move.ToX;
    CommandPayload.Move.ToY = Move.ToY;
    CommandPayload.Move.bHasCapturedPieceId = Move.bHasCapturedPieceId;
    CommandPayload.Move.CapturedPieceId = Move.bHasCapturedPieceId ? static_cast<uint16_t>(Move.CapturedPieceId) : 0;
    return SubmitCommandPayload(MatchId, CommandPayload);
}

bool UStupidChessLocalMatchSubsystem::SubmitResign(int64 MatchId, int64 PlayerId, EStupidChessSide Side)
{
    if (MatchId <= 0 || PlayerId <= 0 || ServerRuntime == nullptr)
    {
        return false;
    }

    FProtocolCommandPayload CommandPayload{};
    CommandPayload.PlayerId = static_cast<uint64_t>(PlayerId);
    CommandPayload.CommandType = static_cast<int32_t>(ECommandType::Resign);
    CommandPayload.Side = ToProtocolSide(Side);
    return SubmitCommandPayload(MatchId, CommandPayload);
}

TArray<FStupidChessOutboundMessage> UStupidChessLocalMatchSubsystem::PullOutboundMessages(int64 PlayerId, int64 AfterServerSequence) const
{
    TArray<FStupidChessOutboundMessage> Result;
    if (PlayerId <= 0 || AfterServerSequence < 0 || ServerRuntime == nullptr)
    {
        return Result;
    }

    const std::vector<FOutboundProtocolMessage> Messages = ServerRuntime->MessageSink.PullMessages(
        static_cast<FPlayerId>(PlayerId),
        static_cast<uint64_t>(AfterServerSequence));

    Result.Reserve(static_cast<int32>(Messages.size()));
    for (const FOutboundProtocolMessage& Message : Messages)
    {
        FStupidChessOutboundMessage OutMessage{};
        OutMessage.PlayerId = static_cast<int64>(Message.PlayerId);
        OutMessage.ServerSequence = static_cast<int64>(Message.ServerSequence);
        OutMessage.MatchId = ParseMatchId(Message.Envelope.MatchId);
        OutMessage.MessageType = ToBridgeMessageType(Message.Envelope.MessageType);
        OutMessage.PayloadJson = UTF8_TO_TCHAR(Message.Envelope.PayloadJson.c_str());
        OutMessage.ErrorMessage = UTF8_TO_TCHAR(Message.ErrorMessage.c_str());
        Result.Add(MoveTemp(OutMessage));
    }

    return Result;
}

void UStupidChessLocalMatchSubsystem::ClearOutboundMessages()
{
    if (ServerRuntime != nullptr)
    {
        ServerRuntime->MessageSink.Clear();
    }
}

void UStupidChessLocalMatchSubsystem::ResetParsedCache()
{
    bHasCachedJoinAck = false;
    bHasCachedCommandAck = false;
    bHasCachedError = false;
    bHasCachedSnapshot = false;
    bHasCachedEventDelta = false;
    CachedJoinAck = FStupidChessJoinAckView{};
    CachedCommandAck = FStupidChessCommandAckView{};
    CachedError = FStupidChessErrorView{};
    CachedSnapshot = FStupidChessSnapshotView{};
    CachedEventDelta = FStupidChessEventDeltaView{};
}

int32 UStupidChessLocalMatchSubsystem::ParseOutboundMessagesToCache(const TArray<FStupidChessOutboundMessage>& Messages)
{
    int32 ParsedCount = 0;

    for (const FStupidChessOutboundMessage& Message : Messages)
    {
        switch (Message.MessageType)
        {
        case EStupidChessProtocolMessageType::S2C_JoinAck:
        {
            FStupidChessJoinAckView ParsedJoinAck{};
            if (TryParseJoinAckMessage(Message, ParsedJoinAck))
            {
                CachedJoinAck = MoveTemp(ParsedJoinAck);
                bHasCachedJoinAck = true;
                ++ParsedCount;
            }
            break;
        }
        case EStupidChessProtocolMessageType::S2C_CommandAck:
        {
            FStupidChessCommandAckView ParsedCommandAck{};
            if (TryParseCommandAckMessage(Message, ParsedCommandAck))
            {
                CachedCommandAck = MoveTemp(ParsedCommandAck);
                bHasCachedCommandAck = true;
                ++ParsedCount;
            }
            break;
        }
        case EStupidChessProtocolMessageType::S2C_Error:
        {
            FStupidChessErrorView ParsedError{};
            if (TryParseErrorMessage(Message, ParsedError))
            {
                CachedError = MoveTemp(ParsedError);
                bHasCachedError = true;
                ++ParsedCount;
            }
            break;
        }
        case EStupidChessProtocolMessageType::S2C_Snapshot:
        {
            FStupidChessSnapshotView ParsedSnapshot{};
            if (TryParseSnapshotMessage(Message, ParsedSnapshot))
            {
                CachedSnapshot = MoveTemp(ParsedSnapshot);
                bHasCachedSnapshot = true;
                ++ParsedCount;
            }
            break;
        }
        case EStupidChessProtocolMessageType::S2C_EventDelta:
        {
            FStupidChessEventDeltaView ParsedEventDelta{};
            if (TryParseEventDeltaMessage(Message, ParsedEventDelta))
            {
                CachedEventDelta = MoveTemp(ParsedEventDelta);
                bHasCachedEventDelta = true;
                ++ParsedCount;
            }
            break;
        }
        default:
            break;
        }
    }

    return ParsedCount;
}

bool UStupidChessLocalMatchSubsystem::DecodeJoinAckPayloadJson(const FString& PayloadJson, FStupidChessJoinAckView& OutJoinAck) const
{
    FProtocolJoinAckPayload DecodedPayload{};
    const std::string JsonUtf8 = TCHAR_TO_UTF8(*PayloadJson);
    if (!ProtocolCodec::DecodeJoinAckPayload(JsonUtf8, DecodedPayload))
    {
        return false;
    }

    OutJoinAck.bAccepted = DecodedPayload.bAccepted;
    OutJoinAck.AssignedSide = DecodedPayload.AssignedSide;
    OutJoinAck.ErrorCode = UTF8_TO_TCHAR(DecodedPayload.ErrorCode.c_str());
    OutJoinAck.ErrorMessage = UTF8_TO_TCHAR(DecodedPayload.ErrorMessage.c_str());
    return true;
}

bool UStupidChessLocalMatchSubsystem::DecodeCommandAckPayloadJson(const FString& PayloadJson, FStupidChessCommandAckView& OutCommandAck) const
{
    FProtocolCommandAckPayload DecodedPayload{};
    const std::string JsonUtf8 = TCHAR_TO_UTF8(*PayloadJson);
    if (!ProtocolCodec::DecodeCommandAckPayload(JsonUtf8, DecodedPayload))
    {
        return false;
    }

    OutCommandAck.bAccepted = DecodedPayload.bAccepted;
    OutCommandAck.ErrorCode = UTF8_TO_TCHAR(DecodedPayload.ErrorCode.c_str());
    OutCommandAck.ErrorMessage = UTF8_TO_TCHAR(DecodedPayload.ErrorMessage.c_str());
    return true;
}

bool UStupidChessLocalMatchSubsystem::DecodeErrorPayloadJson(const FString& PayloadJson, FStupidChessErrorView& OutError) const
{
    FProtocolErrorPayload DecodedPayload{};
    const std::string JsonUtf8 = TCHAR_TO_UTF8(*PayloadJson);
    if (!ProtocolCodec::DecodeErrorPayload(JsonUtf8, DecodedPayload))
    {
        return false;
    }

    OutError.ErrorMessage = UTF8_TO_TCHAR(DecodedPayload.ErrorMessage.c_str());
    return true;
}

bool UStupidChessLocalMatchSubsystem::DecodeSnapshotPayloadJson(const FString& PayloadJson, FStupidChessSnapshotView& OutSnapshot) const
{
    FProtocolSnapshotPayload DecodedPayload{};
    const std::string JsonUtf8 = TCHAR_TO_UTF8(*PayloadJson);
    if (!ProtocolCodec::DecodeSnapshotPayload(JsonUtf8, DecodedPayload))
    {
        return false;
    }

    OutSnapshot.ViewerSide = DecodedPayload.ViewerSide;
    OutSnapshot.Phase = DecodedPayload.Phase;
    OutSnapshot.CurrentTurn = DecodedPayload.CurrentTurn;
    OutSnapshot.PassCount = DecodedPayload.PassCount;
    OutSnapshot.Result = DecodedPayload.Result;
    OutSnapshot.EndReason = DecodedPayload.EndReason;
    OutSnapshot.TurnIndex = static_cast<int64>(DecodedPayload.TurnIndex);
    OutSnapshot.LastEventSequence = static_cast<int64>(DecodedPayload.LastEventSequence);
    OutSnapshot.Pieces.Reset();
    OutSnapshot.Pieces.Reserve(static_cast<int32>(DecodedPayload.Pieces.size()));

    for (const FProtocolPieceSnapshot& Piece : DecodedPayload.Pieces)
    {
        FStupidChessPieceSnapshot OutPiece{};
        OutPiece.PieceId = static_cast<int32>(Piece.PieceId);
        OutPiece.Side = Piece.Side;
        OutPiece.VisibleRole = Piece.VisibleRole;
        OutPiece.X = Piece.X;
        OutPiece.Y = Piece.Y;
        OutPiece.bAlive = Piece.bAlive;
        OutPiece.bFrozen = Piece.bFrozen;
        OutPiece.bRevealed = Piece.bRevealed;
        OutSnapshot.Pieces.Add(MoveTemp(OutPiece));
    }

    return true;
}

bool UStupidChessLocalMatchSubsystem::DecodeEventDeltaPayloadJson(const FString& PayloadJson, FStupidChessEventDeltaView& OutEventDelta) const
{
    FProtocolEventDeltaPayload DecodedPayload{};
    const std::string JsonUtf8 = TCHAR_TO_UTF8(*PayloadJson);
    if (!ProtocolCodec::DecodeEventDeltaPayload(JsonUtf8, DecodedPayload))
    {
        return false;
    }

    OutEventDelta.RequestedAfterSequence = static_cast<int64>(DecodedPayload.RequestedAfterSequence);
    OutEventDelta.LatestSequence = static_cast<int64>(DecodedPayload.LatestSequence);
    OutEventDelta.Events.Reset();
    OutEventDelta.Events.Reserve(static_cast<int32>(DecodedPayload.Events.size()));

    for (const FProtocolEventRecordPayload& Event : DecodedPayload.Events)
    {
        FStupidChessEventRecordView OutEvent{};
        OutEvent.Sequence = static_cast<int64>(Event.Sequence);
        OutEvent.TurnIndex = static_cast<int64>(Event.TurnIndex);
        OutEvent.EventType = Event.EventType;
        OutEvent.ActorPlayerId = static_cast<int64>(Event.ActorPlayerId);
        OutEvent.ErrorCode = UTF8_TO_TCHAR(Event.ErrorCode.c_str());
        OutEvent.Description = UTF8_TO_TCHAR(Event.Description.c_str());
        OutEventDelta.Events.Add(MoveTemp(OutEvent));
    }

    return true;
}

bool UStupidChessLocalMatchSubsystem::TryParseSnapshotMessage(const FStupidChessOutboundMessage& Message, FStupidChessSnapshotView& OutSnapshot) const
{
    if (Message.MessageType != EStupidChessProtocolMessageType::S2C_Snapshot)
    {
        return false;
    }

    return DecodeSnapshotPayloadJson(Message.PayloadJson, OutSnapshot);
}

bool UStupidChessLocalMatchSubsystem::TryParseEventDeltaMessage(const FStupidChessOutboundMessage& Message, FStupidChessEventDeltaView& OutEventDelta) const
{
    if (Message.MessageType != EStupidChessProtocolMessageType::S2C_EventDelta)
    {
        return false;
    }

    return DecodeEventDeltaPayloadJson(Message.PayloadJson, OutEventDelta);
}

bool UStupidChessLocalMatchSubsystem::TryParseJoinAckMessage(const FStupidChessOutboundMessage& Message, FStupidChessJoinAckView& OutJoinAck) const
{
    if (Message.MessageType != EStupidChessProtocolMessageType::S2C_JoinAck)
    {
        return false;
    }

    return DecodeJoinAckPayloadJson(Message.PayloadJson, OutJoinAck);
}

bool UStupidChessLocalMatchSubsystem::TryParseCommandAckMessage(const FStupidChessOutboundMessage& Message, FStupidChessCommandAckView& OutCommandAck) const
{
    if (Message.MessageType != EStupidChessProtocolMessageType::S2C_CommandAck)
    {
        return false;
    }

    return DecodeCommandAckPayloadJson(Message.PayloadJson, OutCommandAck);
}

bool UStupidChessLocalMatchSubsystem::TryParseErrorMessage(const FStupidChessOutboundMessage& Message, FStupidChessErrorView& OutError) const
{
    if (Message.MessageType != EStupidChessProtocolMessageType::S2C_Error)
    {
        return false;
    }

    return DecodeErrorPayloadJson(Message.PayloadJson, OutError);
}

bool UStupidChessLocalMatchSubsystem::GetCachedJoinAck(FStupidChessJoinAckView& OutJoinAck) const
{
    if (!bHasCachedJoinAck)
    {
        return false;
    }

    OutJoinAck = CachedJoinAck;
    return true;
}

bool UStupidChessLocalMatchSubsystem::GetCachedCommandAck(FStupidChessCommandAckView& OutCommandAck) const
{
    if (!bHasCachedCommandAck)
    {
        return false;
    }

    OutCommandAck = CachedCommandAck;
    return true;
}

bool UStupidChessLocalMatchSubsystem::GetCachedError(FStupidChessErrorView& OutError) const
{
    if (!bHasCachedError)
    {
        return false;
    }

    OutError = CachedError;
    return true;
}

bool UStupidChessLocalMatchSubsystem::GetCachedSnapshot(FStupidChessSnapshotView& OutSnapshot) const
{
    if (!bHasCachedSnapshot)
    {
        return false;
    }

    OutSnapshot = CachedSnapshot;
    return true;
}

bool UStupidChessLocalMatchSubsystem::GetCachedEventDelta(FStupidChessEventDeltaView& OutEventDelta) const
{
    if (!bHasCachedEventDelta)
    {
        return false;
    }

    OutEventDelta = CachedEventDelta;
    return true;
}

int64 UStupidChessLocalMatchSubsystem::GetNextClientSequence() const
{
    return static_cast<int64>(NextClientSequence);
}

int64 UStupidChessLocalMatchSubsystem::GetNextServerSequence() const
{
    if (ServerRuntime == nullptr)
    {
        return 0;
    }

    return static_cast<int64>(ServerRuntime->TransportAdapter.GetNextServerSequence());
}

bool UStupidChessLocalMatchSubsystem::SubmitCommandPayload(int64 MatchId, const FProtocolCommandPayload& CommandPayload)
{
    std::string PayloadJson;
    if (!ProtocolCodec::EncodeCommandPayload(CommandPayload, PayloadJson))
    {
        return false;
    }

    return ProcessEnvelope(EStupidChessProtocolMessageType::C2S_Command, MatchId, PayloadJson);
}

int32 UStupidChessLocalMatchSubsystem::ToProtocolSide(EStupidChessSide Side)
{
    switch (Side)
    {
    case EStupidChessSide::Red:
        return static_cast<int32>(ESide::Red);
    case EStupidChessSide::Black:
        return static_cast<int32>(ESide::Black);
    default:
        return static_cast<int32>(ESide::Red);
    }
}

bool UStupidChessLocalMatchSubsystem::ProcessEnvelope(
    EStupidChessProtocolMessageType MessageType,
    int64 MatchId,
    const std::string& PayloadJson)
{
    if (MatchId <= 0 || ServerRuntime == nullptr)
    {
        return false;
    }

    FProtocolEnvelope Envelope{};
    Envelope.MessageType = ToProtocolMessageType(MessageType);
    Envelope.Sequence = NextClientSequence++;
    Envelope.MatchId = std::to_string(MatchId);
    Envelope.PayloadJson = PayloadJson;
    return ServerRuntime->ServerGateway.ProcessEnvelope(Envelope);
}

int64 UStupidChessLocalMatchSubsystem::ParseMatchId(const std::string& MatchIdText)
{
    uint64_t ParsedValue = 0;
    const char* Begin = MatchIdText.data();
    const char* End = Begin + MatchIdText.size();
    const std::from_chars_result ParseResult = std::from_chars(Begin, End, ParsedValue);
    if (ParseResult.ec != std::errc() || ParseResult.ptr != End)
    {
        return 0;
    }

    return static_cast<int64>(ParsedValue);
}
