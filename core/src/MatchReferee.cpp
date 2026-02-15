#include "CoreRules/MatchReferee.h"

#include <utility>

namespace
{
FCommandResult BuildAcceptedResult()
{
    return FCommandResult{true, {}, {}};
}

FCommandResult BuildRejectedResult(std::string ErrorCode, std::string ErrorMessage)
{
    return FCommandResult{false, std::move(ErrorCode), std::move(ErrorMessage)};
}

ESide GetOppositeSide(ESide Side)
{
    return Side == ESide::Red ? ESide::Black : ESide::Red;
}
}

FMatchReferee::FMatchReferee(FRuleConfig InRuleConfig)
    : RuleConfig(InRuleConfig)
{
    ResetNewMatch();
}

void FMatchReferee::ResetNewMatch()
{
    GameState = FGameState{};
    GameState.Phase = EGamePhase::SetupCommit;
    GameState.CurrentTurn = ESide::Red;
    GameState.Result = EGameResult::Ongoing;
    GameState.EndReason = EEndReason::None;
    GameState.PassCount = 0;
    GameState.TurnIndex = 0;
}

const FGameState& FMatchReferee::GetState() const noexcept
{
    return GameState;
}

FCommandResult FMatchReferee::ApplyCommit(const FSetupCommit& Commit)
{
    if (GameState.Phase != EGamePhase::SetupCommit)
    {
        return BuildRejectedResult("ERR_INVALID_PHASE", "Commit is only allowed in SetupCommit phase.");
    }

    if (Commit.Side == ESide::Red)
    {
        GameState.bRedCommitted = true;
    }
    else
    {
        GameState.bBlackCommitted = true;
    }

    if (GameState.bRedCommitted && GameState.bBlackCommitted)
    {
        GameState.Phase = EGamePhase::SetupReveal;
    }

    return BuildAcceptedResult();
}

FCommandResult FMatchReferee::ApplyReveal(const FSetupPlain& SetupPlain)
{
    if (GameState.Phase != EGamePhase::SetupReveal)
    {
        return BuildRejectedResult("ERR_INVALID_PHASE", "Reveal is only allowed in SetupReveal phase.");
    }

    if (SetupPlain.Side == ESide::Red)
    {
        GameState.bRedRevealed = true;
    }
    else
    {
        GameState.bBlackRevealed = true;
    }

    if (GameState.bRedRevealed && GameState.bBlackRevealed)
    {
        GameState.Phase = EGamePhase::Battle;
        GameState.CurrentTurn = ESide::Red;
    }

    return BuildAcceptedResult();
}

FCommandResult FMatchReferee::ApplyCommand(const FPlayerCommand& Command)
{
    if (GameState.Phase != EGamePhase::Battle)
    {
        return BuildRejectedResult("ERR_INVALID_PHASE", "Battle command is not allowed in current phase.");
    }

    if (GameState.Result != EGameResult::Ongoing)
    {
        return BuildRejectedResult("ERR_GAME_OVER", "Game already ended.");
    }

    if (Command.Side != GameState.CurrentTurn)
    {
        return BuildRejectedResult("ERR_NOT_YOUR_TURN", "It is not the player's turn.");
    }

    switch (Command.CommandType)
    {
    case ECommandType::Pass:
        if (!CanPass(Command.Side))
        {
            return BuildRejectedResult("ERR_PASS_NOT_ALLOWED", "Pass is not allowed now.");
        }

        ++GameState.PassCount;
        ++GameState.TurnIndex;

        if (RuleConfig.bDoublePassIsDraw && GameState.PassCount >= 2)
        {
            GameState.Result = EGameResult::Draw;
            GameState.EndReason = EEndReason::DoublePassDraw;
            GameState.Phase = EGamePhase::GameOver;
        }
        else
        {
            GameState.CurrentTurn = GetOppositeSide(GameState.CurrentTurn);
        }
        return BuildAcceptedResult();

    case ECommandType::Resign:
        GameState.Result = Command.Side == ESide::Red ? EGameResult::BlackWin : EGameResult::RedWin;
        GameState.EndReason = EEndReason::Resign;
        GameState.Phase = EGamePhase::GameOver;
        ++GameState.TurnIndex;
        return BuildAcceptedResult();

    case ECommandType::Move:
        GameState.PassCount = 0;
        GameState.CurrentTurn = GetOppositeSide(GameState.CurrentTurn);
        ++GameState.TurnIndex;
        return BuildAcceptedResult();

    default:
        return BuildRejectedResult("ERR_UNSUPPORTED_COMMAND", "Command is not implemented in skeleton.");
    }
}

std::vector<FMoveAction> FMatchReferee::GenerateLegalMoves(ESide Side) const
{
    if (GameState.Phase != EGamePhase::Battle)
    {
        return {};
    }

    if (Side != GameState.CurrentTurn)
    {
        return {};
    }

    // Skeleton stage: move generation is not implemented yet.
    return {};
}

bool FMatchReferee::CanPass(ESide Side) const
{
    if (!RuleConfig.bAllowPassWhenNoLegalMove)
    {
        return false;
    }

    if (GameState.Phase != EGamePhase::Battle)
    {
        return false;
    }

    if (Side != GameState.CurrentTurn)
    {
        return false;
    }

    return GenerateLegalMoves(Side).empty();
}
