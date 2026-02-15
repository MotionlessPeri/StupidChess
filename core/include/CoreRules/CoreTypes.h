#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class ESide : uint8_t
{
    Red,
    Black
};

enum class ERoleType : uint8_t
{
    King,
    Advisor,
    Elephant,
    Horse,
    Rook,
    Cannon,
    Pawn
};

enum class EPieceState : uint8_t
{
    HiddenSurface,
    RevealedActual
};

enum class EGamePhase : uint8_t
{
    SetupCommit,
    SetupReveal,
    Battle,
    GameOver
};

enum class EGameResult : uint8_t
{
    Ongoing,
    RedWin,
    BlackWin,
    Draw
};

enum class EEndReason : uint8_t
{
    None,
    Checkmate,
    Resign,
    Timeout,
    DoublePassDraw,
    RuleViolation
};

enum class ECommandType : uint8_t
{
    CommitSetup,
    RevealSetup,
    Move,
    Pass,
    Resign
};

using FPieceId = uint16_t;

struct FBoardPos
{
    int8_t X = -1;
    int8_t Y = -1;

    bool IsValid() const noexcept
    {
        return X >= 0 && X < 9 && Y >= 0 && Y < 10;
    }

    bool operator==(const FBoardPos& Other) const noexcept = default;
};

struct FRuleConfig
{
    bool bRevealOnFirstCapture = true;
    bool bRevealCapturedRole = true;
    bool bFreezeIfIllegalAfterReveal = true;
    bool bAllowPassWhenNoLegalMove = true;
    bool bDoublePassIsDraw = true;
};

struct FPieceState
{
    FPieceId PieceId = 0;
    ESide Side = ESide::Red;
    ERoleType ActualRole = ERoleType::Pawn;
    ERoleType SurfaceRole = ERoleType::Pawn;
    EPieceState PieceState = EPieceState::HiddenSurface;
    FBoardPos Pos{};
    bool bAlive = true;
    bool bFrozen = false;
    bool bHasCaptured = false;
};

struct FMoveAction
{
    FPieceId PieceId = 0;
    FBoardPos From{};
    FBoardPos To{};
    std::optional<FPieceId> CapturedPieceId;
};

struct FSetupPlacement
{
    FPieceId PieceId = 0;
    FBoardPos TargetPos{};
};

struct FSetupPlain
{
    ESide Side = ESide::Red;
    std::vector<FSetupPlacement> Placements;
    std::string Nonce;
};

struct FSetupCommit
{
    ESide Side = ESide::Red;
    std::string HashHex;
};

struct FPlayerCommand
{
    ECommandType CommandType = ECommandType::Move;
    ESide Side = ESide::Red;
    std::optional<FMoveAction> Move;
    std::optional<FSetupCommit> SetupCommit;
    std::optional<FSetupPlain> SetupPlain;
};

struct FCommandResult
{
    bool bAccepted = false;
    std::string ErrorCode;
    std::string ErrorMessage;
};

struct FGameState
{
    EGamePhase Phase = EGamePhase::SetupCommit;
    ESide CurrentTurn = ESide::Red;
    std::array<std::optional<FPieceId>, 90> BoardCells{};
    std::vector<FPieceState> Pieces;
    bool bRedCommitted = false;
    bool bBlackCommitted = false;
    bool bRedRevealed = false;
    bool bBlackRevealed = false;
    int32_t PassCount = 0;
    EGameResult Result = EGameResult::Ongoing;
    EEndReason EndReason = EEndReason::None;
    uint64_t TurnIndex = 0;
};
