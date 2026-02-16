#include "Server/MatchSession.h"

#include <algorithm>
#include <sstream>

FInMemoryMatchSession::FInMemoryMatchSession(FMatchId InMatchId)
    : MatchId(InMatchId)
{
}

void FInMemoryMatchSession::AppendEvent(EMatchEventType EventType, FPlayerId ActorPlayerId, std::string Description, std::string ErrorCode)
{
    EventLog.push_back(FMatchEventRecord{
        NextEventSequence++,
        MatchReferee.GetState().TurnIndex,
        EventType,
        ActorPlayerId,
        std::move(ErrorCode),
        std::move(Description)});
}

FMatchJoinResponse FInMemoryMatchSession::Join(const FMatchJoinRequest& Request)
{
    const auto ExistingIt = PlayerSides.find(Request.PlayerId);
    if (ExistingIt != PlayerSides.end())
    {
        return {true, ExistingIt->second, {}};
    }

    if (PlayerSides.size() >= 2)
    {
        return {false, ESide::Red, "Match already has two players."};
    }

    const ESide AssignedSide = PlayerSides.empty() ? ESide::Red : ESide::Black;
    PlayerSides.emplace(Request.PlayerId, AssignedSide);

    std::ostringstream Stream;
    Stream << "Player joined as " << (AssignedSide == ESide::Red ? "Red" : "Black");
    AppendEvent(EMatchEventType::PlayerJoined, Request.PlayerId, Stream.str());

    return {true, AssignedSide, {}};
}

FCommandResult FInMemoryMatchSession::SubmitCommand(FPlayerId PlayerId, const FPlayerCommand& Command)
{
    const std::optional<ESide> PlayerSide = GetPlayerSide(PlayerId);
    if (!PlayerSide.has_value())
    {
        FCommandResult NotJoinedResult{false, "ERR_PLAYER_NOT_IN_MATCH", "Player has not joined this match."};
        AppendEvent(EMatchEventType::CommandRejected, PlayerId, NotJoinedResult.ErrorMessage, NotJoinedResult.ErrorCode);
        return NotJoinedResult;
    }

    FPlayerCommand NormalizedCommand = Command;
    NormalizedCommand.Side = PlayerSide.value();
    const EGamePhase PhaseBefore = MatchReferee.GetState().Phase;

    FCommandResult Result{};
    switch (NormalizedCommand.CommandType)
    {
    case ECommandType::CommitSetup:
        if (NormalizedCommand.SetupCommit.has_value())
        {
            NormalizedCommand.SetupCommit->Side = PlayerSide.value();
            Result = MatchReferee.ApplyCommit(NormalizedCommand.SetupCommit.value());
        }
        else
        {
            Result = {false, "ERR_INVALID_PAYLOAD", "Commit command missing setup commit payload."};
        }
        break;
    case ECommandType::RevealSetup:
        if (NormalizedCommand.SetupPlain.has_value())
        {
            NormalizedCommand.SetupPlain->Side = PlayerSide.value();
            Result = MatchReferee.ApplyReveal(NormalizedCommand.SetupPlain.value());
        }
        else
        {
            Result = {false, "ERR_INVALID_PAYLOAD", "Reveal command missing setup plain payload."};
        }
        break;
    default:
        Result = MatchReferee.ApplyCommand(NormalizedCommand);
        break;
    }

    if (!Result.bAccepted)
    {
        AppendEvent(EMatchEventType::CommandRejected, PlayerId, Result.ErrorMessage, Result.ErrorCode);
        return Result;
    }

    switch (NormalizedCommand.CommandType)
    {
    case ECommandType::CommitSetup:
        AppendEvent(EMatchEventType::SetupCommitted, PlayerId, "Setup committed");
        break;
    case ECommandType::RevealSetup:
        AppendEvent(EMatchEventType::SetupRevealed, PlayerId, "Setup revealed");
        break;
    case ECommandType::Move:
        AppendEvent(EMatchEventType::MoveApplied, PlayerId, "Move applied");
        break;
    case ECommandType::Pass:
        AppendEvent(EMatchEventType::PassApplied, PlayerId, "Pass applied");
        break;
    case ECommandType::Resign:
        AppendEvent(EMatchEventType::ResignApplied, PlayerId, "Resign applied");
        break;
    default:
        break;
    }

    if (PhaseBefore != EGamePhase::GameOver && MatchReferee.GetState().Phase == EGamePhase::GameOver)
    {
        AppendEvent(EMatchEventType::GameOver, PlayerId, "Game over");
    }

    return Result;
}

const FGameState& FInMemoryMatchSession::GetState() const noexcept
{
    return MatchReferee.GetState();
}

FMatchPlayerView FInMemoryMatchSession::GetPlayerView(FPlayerId PlayerId) const
{
    FMatchPlayerView View{};
    const FGameState& State = MatchReferee.GetState();

    const std::optional<ESide> PlayerSide = GetPlayerSide(PlayerId);
    View.ViewerSide = PlayerSide.value_or(ESide::Red);
    View.Phase = State.Phase;
    View.CurrentTurn = State.CurrentTurn;
    View.PassCount = State.PassCount;
    View.Result = State.Result;
    View.EndReason = State.EndReason;
    View.TurnIndex = State.TurnIndex;
    View.Pieces.reserve(State.Pieces.size());

    for (const FPieceState& Piece : State.Pieces)
    {
        const bool bOwnPiece = PlayerSide.has_value() && Piece.Side == PlayerSide.value();
        const bool bPublicRevealed = Piece.PieceState == EPieceState::RevealedActual;
        const ERoleType VisibleRole = (bOwnPiece || bPublicRevealed) ? Piece.ActualRole : Piece.SurfaceRole;

        View.Pieces.push_back(FPlayerPieceView{
            Piece.PieceId,
            Piece.Side,
            VisibleRole,
            Piece.Pos,
            Piece.bAlive,
            Piece.bFrozen,
            bPublicRevealed});
    }

    return View;
}

std::vector<FMatchEventRecord> FInMemoryMatchSession::PullEvents(FPlayerId PlayerId, uint64_t AfterSequence) const
{
    if (!GetPlayerSide(PlayerId).has_value())
    {
        return {};
    }

    std::vector<FMatchEventRecord> Result;
    for (const FMatchEventRecord& Event : EventLog)
    {
        if (Event.Sequence > AfterSequence)
        {
            Result.push_back(Event);
        }
    }
    return Result;
}

uint64_t FInMemoryMatchSession::GetLatestEventSequence() const noexcept
{
    return NextEventSequence == 0 ? 0 : NextEventSequence - 1;
}

std::optional<ESide> FInMemoryMatchSession::GetPlayerSide(FPlayerId PlayerId) const
{
    const auto It = PlayerSides.find(PlayerId);
    if (It == PlayerSides.end())
    {
        return std::nullopt;
    }

    return It->second;
}
