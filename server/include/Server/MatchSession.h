#pragma once

#include "CoreRules/MatchReferee.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using FMatchId = uint64_t;
using FPlayerId = uint64_t;

struct FMatchJoinRequest
{
    FMatchId MatchId = 0;
    FPlayerId PlayerId = 0;
};

struct FMatchJoinResponse
{
    bool bAccepted = false;
    ESide AssignedSide = ESide::Red;
    std::string ErrorMessage;
};

struct FPlayerPieceView
{
    FPieceId PieceId = 0;
    ESide Side = ESide::Red;
    ERoleType VisibleRole = ERoleType::Pawn;
    FBoardPos Pos{};
    bool bAlive = false;
    bool bFrozen = false;
    bool bRevealed = false;
};

struct FMatchPlayerView
{
    ESide ViewerSide = ESide::Red;
    EGamePhase Phase = EGamePhase::SetupCommit;
    ESide CurrentTurn = ESide::Red;
    int32_t PassCount = 0;
    EGameResult Result = EGameResult::Ongoing;
    EEndReason EndReason = EEndReason::None;
    uint64_t TurnIndex = 0;
    std::vector<FPlayerPieceView> Pieces;
};

enum class EMatchEventType : uint8_t
{
    PlayerJoined,
    SetupCommitted,
    SetupRevealed,
    MoveApplied,
    PassApplied,
    ResignApplied,
    CommandRejected,
    GameOver
};

struct FMatchEventRecord
{
    uint64_t Sequence = 0;
    uint64_t TurnIndex = 0;
    EMatchEventType EventType = EMatchEventType::PlayerJoined;
    FPlayerId ActorPlayerId = 0;
    std::string ErrorCode;
    std::string Description;
};

class FInMemoryMatchSession
{
public:
    explicit FInMemoryMatchSession(FMatchId InMatchId);

    FMatchJoinResponse Join(const FMatchJoinRequest& Request);
    FCommandResult SubmitCommand(FPlayerId PlayerId, const FPlayerCommand& Command);

    const FGameState& GetState() const noexcept;
    FMatchPlayerView GetPlayerView(FPlayerId PlayerId) const;
    std::vector<FMatchEventRecord> PullEvents(FPlayerId PlayerId, uint64_t AfterSequence) const;
    uint64_t GetLatestEventSequence() const noexcept;

private:
    void AppendEvent(EMatchEventType EventType, FPlayerId ActorPlayerId, std::string Description, std::string ErrorCode = {});
    std::optional<ESide> GetPlayerSide(FPlayerId PlayerId) const;

private:
    FMatchId MatchId = 0;
    FMatchReferee MatchReferee;
    std::unordered_map<FPlayerId, ESide> PlayerSides;
    std::vector<FMatchEventRecord> EventLog;
    uint64_t NextEventSequence = 1;
};
