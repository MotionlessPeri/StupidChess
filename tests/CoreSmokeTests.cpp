#include "CoreRules/MatchReferee.h"

#include <array>

#include <gtest/gtest.h>

namespace
{
struct FSetupSlot
{
    FBoardPos Pos{};
};

constexpr std::array<FSetupSlot, 16> RedSetupSlots = {{
    {{0, 0}},
    {{1, 0}},
    {{2, 0}},
    {{3, 0}},
    {{4, 0}},
    {{5, 0}},
    {{6, 0}},
    {{7, 0}},
    {{8, 0}},
    {{1, 2}},
    {{7, 2}},
    {{0, 3}},
    {{2, 3}},
    {{4, 3}},
    {{6, 3}},
    {{8, 3}},
}};

FSetupPlain BuildStandardSetup(ESide Side)
{
    FSetupPlain Setup{};
    Setup.Side = Side;
    Setup.Nonce = Side == ESide::Red ? "RedNonce" : "BlackNonce";
    Setup.Placements.reserve(16);

    const int32_t BasePieceId = Side == ESide::Red ? 0 : 16;
    for (int32_t Index = 0; Index < 16; ++Index)
    {
        FBoardPos Pos = RedSetupSlots[Index].Pos;
        if (Side == ESide::Black)
        {
            Pos.Y = static_cast<int8_t>(9 - Pos.Y);
        }

        Setup.Placements.push_back(FSetupPlacement{
            static_cast<FPieceId>(BasePieceId + Index),
            Pos});
    }

    return Setup;
}

void StartBattle(FMatchReferee& MatchReferee)
{
    FCommandResult CommitRed = MatchReferee.ApplyCommit({ESide::Red, ""});
    FCommandResult CommitBlack = MatchReferee.ApplyCommit({ESide::Black, ""});
    ASSERT_TRUE(CommitRed.bAccepted);
    ASSERT_TRUE(CommitBlack.bAccepted);
    ASSERT_EQ(MatchReferee.GetState().Phase, EGamePhase::SetupReveal);

    FCommandResult RevealRed = MatchReferee.ApplyReveal(BuildStandardSetup(ESide::Red));
    FCommandResult RevealBlack = MatchReferee.ApplyReveal(BuildStandardSetup(ESide::Black));
    ASSERT_TRUE(RevealRed.bAccepted);
    ASSERT_TRUE(RevealBlack.bAccepted);
    ASSERT_EQ(MatchReferee.GetState().Phase, EGamePhase::Battle);
}
}

TEST(CoreSmokeTests, ShouldStartInSetupCommit)
{
    FMatchReferee MatchReferee;

    const FGameState& InitialState = MatchReferee.GetState();
    EXPECT_EQ(InitialState.Phase, EGamePhase::SetupCommit);
    EXPECT_EQ(InitialState.CurrentTurn, ESide::Red);
    EXPECT_EQ(InitialState.Result, EGameResult::Ongoing);
}

TEST(CoreSmokeTests, ShouldRejectPassWhenLegalMovesExist)
{
    FMatchReferee MatchReferee;
    StartBattle(MatchReferee);

    const std::vector<FMoveAction> RedMoves = MatchReferee.GenerateLegalMoves(ESide::Red);
    ASSERT_FALSE(RedMoves.empty());

    FPlayerCommand PassCommand{};
    PassCommand.CommandType = ECommandType::Pass;
    PassCommand.Side = ESide::Red;

    FCommandResult PassResult = MatchReferee.ApplyCommand(PassCommand);
    EXPECT_FALSE(PassResult.bAccepted);
    EXPECT_EQ(PassResult.ErrorCode, "ERR_PASS_NOT_ALLOWED");
}

TEST(CoreSmokeTests, ShouldAdvanceTurnAfterLegalMove)
{
    FMatchReferee MatchReferee;
    StartBattle(MatchReferee);

    const std::vector<FMoveAction> RedMoves = MatchReferee.GenerateLegalMoves(ESide::Red);
    ASSERT_FALSE(RedMoves.empty());

    FPlayerCommand MoveCommand{};
    MoveCommand.CommandType = ECommandType::Move;
    MoveCommand.Side = ESide::Red;
    MoveCommand.Move = RedMoves.front();

    FCommandResult MoveResult = MatchReferee.ApplyCommand(MoveCommand);
    EXPECT_TRUE(MoveResult.bAccepted);
    EXPECT_EQ(MatchReferee.GetState().CurrentTurn, ESide::Black);
    EXPECT_EQ(MatchReferee.GetState().TurnIndex, 1);
}
