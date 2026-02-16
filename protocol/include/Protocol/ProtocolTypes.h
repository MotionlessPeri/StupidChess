#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class EProtocolMessageType : uint16_t
{
    C2S_Join = 100,
    C2S_Command = 101,
    C2S_Ping = 102,
    C2S_PullSync = 103,
    C2S_Ack = 104,

    S2C_JoinAck = 200,
    S2C_CommandAck = 201,
    S2C_Snapshot = 202,
    S2C_EventDelta = 203,
    S2C_GameOver = 204,
    S2C_Error = 205
};

struct FProtocolEnvelope
{
    EProtocolMessageType MessageType = EProtocolMessageType::C2S_Ping;
    uint64_t Sequence = 0;
    std::string MatchId;
    std::string PayloadJson;
};

struct FProtocolJoinPayload
{
    uint64_t MatchId = 0;
    uint64_t PlayerId = 0;
};

struct FProtocolMovePayload
{
    uint16_t PieceId = 0;
    int32_t FromX = -1;
    int32_t FromY = -1;
    int32_t ToX = -1;
    int32_t ToY = -1;
    bool bHasCapturedPieceId = false;
    uint16_t CapturedPieceId = 0;
};

struct FProtocolSetupPlacementPayload
{
    uint16_t PieceId = 0;
    int32_t X = -1;
    int32_t Y = -1;
};

struct FProtocolSetupCommitPayload
{
    int32_t Side = 0;
    std::string HashHex;
};

struct FProtocolSetupPlainPayload
{
    int32_t Side = 0;
    std::string Nonce;
    std::vector<FProtocolSetupPlacementPayload> Placements;
};

struct FProtocolCommandPayload
{
    uint64_t PlayerId = 0;
    int32_t CommandType = 0;
    int32_t Side = 0;

    bool bHasMove = false;
    FProtocolMovePayload Move{};

    bool bHasSetupCommit = false;
    FProtocolSetupCommitPayload SetupCommit{};

    bool bHasSetupPlain = false;
    FProtocolSetupPlainPayload SetupPlain{};
};

struct FProtocolPullSyncPayload
{
    uint64_t PlayerId = 0;
    bool bHasAfterSequenceOverride = false;
    uint64_t AfterSequenceOverride = 0;
};

struct FProtocolAckPayload
{
    uint64_t PlayerId = 0;
    uint64_t Sequence = 0;
};

struct FProtocolJoinAckPayload
{
    bool bAccepted = false;
    int32_t AssignedSide = 0;
    std::string ErrorCode;
    std::string ErrorMessage;
};

struct FProtocolCommandAckPayload
{
    bool bAccepted = false;
    std::string ErrorCode;
    std::string ErrorMessage;
};

struct FProtocolPieceSnapshot
{
    uint16_t PieceId = 0;
    int32_t Side = 0;
    int32_t VisibleRole = 0;
    int32_t X = -1;
    int32_t Y = -1;
    bool bAlive = false;
    bool bFrozen = false;
    bool bRevealed = false;
};

struct FProtocolSnapshotPayload
{
    int32_t ViewerSide = 0;
    int32_t Phase = 0;
    int32_t CurrentTurn = 0;
    int32_t PassCount = 0;
    int32_t Result = 0;
    int32_t EndReason = 0;
    uint64_t TurnIndex = 0;
    uint64_t LastEventSequence = 0;
    std::vector<FProtocolPieceSnapshot> Pieces;
};

struct FProtocolEventRecordPayload
{
    uint64_t Sequence = 0;
    uint64_t TurnIndex = 0;
    int32_t EventType = 0;
    uint64_t ActorPlayerId = 0;
    std::string ErrorCode;
    std::string Description;
};

struct FProtocolEventDeltaPayload
{
    uint64_t RequestedAfterSequence = 0;
    uint64_t LatestSequence = 0;
    std::vector<FProtocolEventRecordPayload> Events;
};

struct FProtocolErrorPayload
{
    std::string ErrorMessage;
};
