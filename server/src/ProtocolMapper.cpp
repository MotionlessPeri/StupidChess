#include "Server/ProtocolMapper.h"

namespace
{
int32_t ToInt(ESide Value)
{
    return static_cast<int32_t>(Value);
}

int32_t ToInt(ERoleType Value)
{
    return static_cast<int32_t>(Value);
}

int32_t ToInt(EGamePhase Value)
{
    return static_cast<int32_t>(Value);
}

int32_t ToInt(EGameResult Value)
{
    return static_cast<int32_t>(Value);
}

int32_t ToInt(EEndReason Value)
{
    return static_cast<int32_t>(Value);
}

int32_t ToInt(EMatchEventType Value)
{
    return static_cast<int32_t>(Value);
}
}

FProtocolJoinAckPayload FProtocolMapper::BuildJoinAckPayload(const FMatchJoinResponse& JoinResponse)
{
    FProtocolJoinAckPayload Payload{};
    Payload.bAccepted = JoinResponse.bAccepted;
    Payload.AssignedSide = ToInt(JoinResponse.AssignedSide);
    Payload.ErrorCode = JoinResponse.bAccepted ? std::string{} : "ERR_JOIN_REJECTED";
    Payload.ErrorMessage = JoinResponse.ErrorMessage;
    return Payload;
}

FProtocolCommandAckPayload FProtocolMapper::BuildCommandAckPayload(const FCommandResult& CommandResult)
{
    FProtocolCommandAckPayload Payload{};
    Payload.bAccepted = CommandResult.bAccepted;
    Payload.ErrorCode = CommandResult.ErrorCode;
    Payload.ErrorMessage = CommandResult.ErrorMessage;
    return Payload;
}

FProtocolSnapshotPayload FProtocolMapper::BuildSnapshotPayload(const FMatchPlayerView& View, uint64_t LastEventSequence)
{
    FProtocolSnapshotPayload Payload{};
    Payload.ViewerSide = ToInt(View.ViewerSide);
    Payload.Phase = ToInt(View.Phase);
    Payload.CurrentTurn = ToInt(View.CurrentTurn);
    Payload.PassCount = View.PassCount;
    Payload.Result = ToInt(View.Result);
    Payload.EndReason = ToInt(View.EndReason);
    Payload.TurnIndex = View.TurnIndex;
    Payload.LastEventSequence = LastEventSequence;
    Payload.Pieces.reserve(View.Pieces.size());

    for (const FPlayerPieceView& Piece : View.Pieces)
    {
        Payload.Pieces.push_back(FProtocolPieceSnapshot{
            Piece.PieceId,
            ToInt(Piece.Side),
            ToInt(Piece.VisibleRole),
            Piece.Pos.X,
            Piece.Pos.Y,
            Piece.bAlive,
            Piece.bFrozen,
            Piece.bRevealed});
    }

    return Payload;
}

FProtocolEventDeltaPayload FProtocolMapper::BuildEventDeltaPayload(const FMatchSyncResponse& SyncResponse)
{
    FProtocolEventDeltaPayload Payload{};
    Payload.RequestedAfterSequence = SyncResponse.RequestedAfterSequence;
    Payload.LatestSequence = SyncResponse.LatestSequence;
    Payload.Events.reserve(SyncResponse.Events.size());

    for (const FMatchEventRecord& Event : SyncResponse.Events)
    {
        Payload.Events.push_back(FProtocolEventRecordPayload{
            Event.Sequence,
            Event.TurnIndex,
            ToInt(Event.EventType),
            Event.ActorPlayerId,
            Event.ErrorCode,
            Event.Description});
    }

    return Payload;
}

FProtocolSyncBundle FProtocolMapper::BuildSyncBundle(const FMatchSyncResponse& SyncResponse)
{
    FProtocolSyncBundle Bundle{};
    Bundle.Snapshot = BuildSnapshotPayload(SyncResponse.View, SyncResponse.LatestSequence);
    Bundle.EventDelta = BuildEventDeltaPayload(SyncResponse);
    return Bundle;
}
