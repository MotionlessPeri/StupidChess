#include "CoreRules/MatchReferee.h"

#include <algorithm>
#include <array>
#include <optional>

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

std::array<FPieceId, 16> BuildDefaultPieceOrder(ESide Side)
{
    std::array<FPieceId, 16> PieceOrder{};
    const int32_t BasePieceId = Side == ESide::Red ? 0 : 16;
    for (int32_t Index = 0; Index < 16; ++Index)
    {
        PieceOrder[Index] = static_cast<FPieceId>(BasePieceId + Index);
    }
    return PieceOrder;
}

FSetupPlain BuildSetupFromPieceOrder(ESide Side, const std::array<FPieceId, 16>& PieceOrder, const char* Nonce)
{
    FSetupPlain Setup{};
    Setup.Side = Side;
    Setup.Nonce = Nonce;
    Setup.Placements.reserve(16);

    for (int32_t SlotIndex = 0; SlotIndex < 16; ++SlotIndex)
    {
        FBoardPos Pos = RedSetupSlots[SlotIndex].Pos;
        if (Side == ESide::Black)
        {
            Pos.Y = static_cast<int8_t>(9 - Pos.Y);
        }

        Setup.Placements.push_back(FSetupPlacement{
            PieceOrder[SlotIndex],
            Pos});
    }

    return Setup;
}

FSetupPlain BuildStandardSetup(ESide Side)
{
    const std::array<FPieceId, 16> PieceOrder = BuildDefaultPieceOrder(Side);
    return BuildSetupFromPieceOrder(Side, PieceOrder, Side == ESide::Red ? "RedNonce" : "BlackNonce");
}

void StartBattle(FMatchReferee& MatchReferee, const FSetupPlain& RedSetup, const FSetupPlain& BlackSetup)
{
    FCommandResult CommitRed = MatchReferee.ApplyCommit({ESide::Red, ""});
    FCommandResult CommitBlack = MatchReferee.ApplyCommit({ESide::Black, ""});
    ASSERT_TRUE(CommitRed.bAccepted);
    ASSERT_TRUE(CommitBlack.bAccepted);
    ASSERT_EQ(MatchReferee.GetState().Phase, EGamePhase::SetupReveal);

    FCommandResult RevealRed = MatchReferee.ApplyReveal(RedSetup);
    FCommandResult RevealBlack = MatchReferee.ApplyReveal(BlackSetup);
    ASSERT_TRUE(RevealRed.bAccepted);
    ASSERT_TRUE(RevealBlack.bAccepted);
    ASSERT_EQ(MatchReferee.GetState().Phase, EGamePhase::Battle);
}

void StartStandardBattle(FMatchReferee& MatchReferee)
{
    StartBattle(MatchReferee, BuildStandardSetup(ESide::Red), BuildStandardSetup(ESide::Black));
}

std::optional<FMoveAction> FindMove(const std::vector<FMoveAction>& Moves, FPieceId PieceId, const FBoardPos& To)
{
    const auto It = std::find_if(Moves.begin(), Moves.end(), [PieceId, To](const FMoveAction& Move) {
        return Move.PieceId == PieceId && Move.To == To;
    });
    if (It == Moves.end())
    {
        return std::nullopt;
    }
    return *It;
}

const FPieceState* FindPieceOrNull(const FGameState& State, FPieceId PieceId)
{
    const size_t Index = static_cast<size_t>(PieceId);
    if (Index >= State.Pieces.size())
    {
        return nullptr;
    }
    return &State.Pieces[Index];
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

TEST(CoreSmokeTests, ShouldRejectDuplicateCommitForSameSide)
{
    FMatchReferee MatchReferee;

    FCommandResult FirstCommit = MatchReferee.ApplyCommit({ESide::Red, ""});
    FCommandResult DuplicateCommit = MatchReferee.ApplyCommit({ESide::Red, ""});

    EXPECT_TRUE(FirstCommit.bAccepted);
    EXPECT_FALSE(DuplicateCommit.bAccepted);
    EXPECT_EQ(DuplicateCommit.ErrorCode, "ERR_DUPLICATE_COMMIT");
}

TEST(CoreSmokeTests, ShouldRejectRevealOnCommitMismatch)
{
    FMatchReferee MatchReferee;

    EXPECT_TRUE(MatchReferee.ApplyCommit({ESide::Red, "HashA"}).bAccepted);
    EXPECT_TRUE(MatchReferee.ApplyCommit({ESide::Black, "HashB"}).bAccepted);
    ASSERT_EQ(MatchReferee.GetState().Phase, EGamePhase::SetupReveal);

    FCommandResult RevealResult = MatchReferee.ApplyReveal(BuildStandardSetup(ESide::Red));
    EXPECT_FALSE(RevealResult.bAccepted);
    EXPECT_EQ(RevealResult.ErrorCode, "ERR_COMMIT_MISMATCH");
}

TEST(CoreSmokeTests, ShouldRejectRevealWhenPlacementPositionInvalid)
{
    FMatchReferee MatchReferee;

    EXPECT_TRUE(MatchReferee.ApplyCommit({ESide::Red, ""}).bAccepted);
    EXPECT_TRUE(MatchReferee.ApplyCommit({ESide::Black, ""}).bAccepted);
    ASSERT_EQ(MatchReferee.GetState().Phase, EGamePhase::SetupReveal);

    FSetupPlain InvalidRedSetup = BuildStandardSetup(ESide::Red);
    ASSERT_FALSE(InvalidRedSetup.Placements.empty());
    InvalidRedSetup.Placements[0].TargetPos = FBoardPos{4, 9};

    FCommandResult RevealResult = MatchReferee.ApplyReveal(InvalidRedSetup);
    EXPECT_FALSE(RevealResult.bAccepted);
    EXPECT_EQ(RevealResult.ErrorCode, "ERR_INVALID_REVEAL");
}

TEST(CoreSmokeTests, ShouldRejectRevealBeforeCommitPhaseFinished)
{
    FMatchReferee MatchReferee;

    FCommandResult RevealResult = MatchReferee.ApplyReveal(BuildStandardSetup(ESide::Red));
    EXPECT_FALSE(RevealResult.bAccepted);
    EXPECT_EQ(RevealResult.ErrorCode, "ERR_INVALID_PHASE");
}

TEST(CoreSmokeTests, ShouldRejectDuplicateRevealForSameSide)
{
    FMatchReferee MatchReferee;

    EXPECT_TRUE(MatchReferee.ApplyCommit({ESide::Red, ""}).bAccepted);
    EXPECT_TRUE(MatchReferee.ApplyCommit({ESide::Black, ""}).bAccepted);
    ASSERT_EQ(MatchReferee.GetState().Phase, EGamePhase::SetupReveal);

    const FSetupPlain RedSetup = BuildStandardSetup(ESide::Red);
    EXPECT_TRUE(MatchReferee.ApplyReveal(RedSetup).bAccepted);

    FCommandResult DuplicateReveal = MatchReferee.ApplyReveal(RedSetup);
    EXPECT_FALSE(DuplicateReveal.bAccepted);
    EXPECT_EQ(DuplicateReveal.ErrorCode, "ERR_DUPLICATE_REVEAL");
}

TEST(CoreSmokeTests, ShouldRejectRevealWhenTargetPositionDuplicated)
{
    FMatchReferee MatchReferee;

    EXPECT_TRUE(MatchReferee.ApplyCommit({ESide::Red, ""}).bAccepted);
    EXPECT_TRUE(MatchReferee.ApplyCommit({ESide::Black, ""}).bAccepted);
    ASSERT_EQ(MatchReferee.GetState().Phase, EGamePhase::SetupReveal);

    FSetupPlain InvalidRedSetup = BuildStandardSetup(ESide::Red);
    ASSERT_GE(InvalidRedSetup.Placements.size(), static_cast<size_t>(2));
    InvalidRedSetup.Placements[1].TargetPos = InvalidRedSetup.Placements[0].TargetPos;

    FCommandResult RevealResult = MatchReferee.ApplyReveal(InvalidRedSetup);
    EXPECT_FALSE(RevealResult.bAccepted);
    EXPECT_EQ(RevealResult.ErrorCode, "ERR_INVALID_REVEAL");
}

TEST(CoreSmokeTests, ShouldRejectPassWhenLegalMovesExist)
{
    FMatchReferee MatchReferee;
    StartStandardBattle(MatchReferee);

    const std::vector<FMoveAction> RedMoves = MatchReferee.GenerateLegalMoves(ESide::Red);
    ASSERT_FALSE(RedMoves.empty());

    FPlayerCommand PassCommand{};
    PassCommand.CommandType = ECommandType::Pass;
    PassCommand.Side = ESide::Red;

    FCommandResult PassResult = MatchReferee.ApplyCommand(PassCommand);
    EXPECT_FALSE(PassResult.bAccepted);
    EXPECT_EQ(PassResult.ErrorCode, "ERR_PASS_NOT_ALLOWED");
}

TEST(CoreSmokeTests, ShouldRejectPassAndIgnoreUnrelatedMoveWhenSideIsInCheck)
{
    FMatchReferee MatchReferee;

    std::array<FPieceId, 16> RedPieceOrder = BuildDefaultPieceOrder(ESide::Red);
    std::swap(RedPieceOrder[1], RedPieceOrder[4]); // Put red king (piece 4) at slot (1,0)

    StartBattle(
        MatchReferee,
        BuildSetupFromPieceOrder(ESide::Red, RedPieceOrder, "RedCheckNonce"),
        BuildStandardSetup(ESide::Black));

    FPlayerCommand PassCommand{};
    PassCommand.CommandType = ECommandType::Pass;
    PassCommand.Side = ESide::Red;
    FCommandResult PassResult = MatchReferee.ApplyCommand(PassCommand);
    EXPECT_FALSE(PassResult.bAccepted);
    EXPECT_EQ(PassResult.ErrorCode, "ERR_PASS_NOT_ALLOWED");

    // Pawn at slot (8,3) is unrelated to the check line on file x=1.
    FPlayerCommand UnrelatedMove{};
    UnrelatedMove.CommandType = ECommandType::Move;
    UnrelatedMove.Side = ESide::Red;
    UnrelatedMove.Move = FMoveAction{
        static_cast<FPieceId>(15),
        FBoardPos{8, 3},
        FBoardPos{8, 4},
        std::nullopt};

    FCommandResult UnrelatedMoveResult = MatchReferee.ApplyCommand(UnrelatedMove);
    EXPECT_FALSE(UnrelatedMoveResult.bAccepted);
    EXPECT_EQ(UnrelatedMoveResult.ErrorCode, "ERR_ILLEGAL_MOVE");
}

TEST(CoreSmokeTests, ShouldAdvanceTurnAfterLegalMove)
{
    FMatchReferee MatchReferee;
    StartStandardBattle(MatchReferee);

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

TEST(CoreSmokeTests, ShouldRejectMoveWhenNotYourTurn)
{
    FMatchReferee MatchReferee;
    StartStandardBattle(MatchReferee);

    FPlayerCommand BlackMoveOnRedTurn{};
    BlackMoveOnRedTurn.CommandType = ECommandType::Move;
    BlackMoveOnRedTurn.Side = ESide::Black;
    BlackMoveOnRedTurn.Move = FMoveAction{
        static_cast<FPieceId>(16),
        FBoardPos{0, 9},
        FBoardPos{0, 8},
        std::nullopt};

    FCommandResult MoveResult = MatchReferee.ApplyCommand(BlackMoveOnRedTurn);
    EXPECT_FALSE(MoveResult.bAccepted);
    EXPECT_EQ(MoveResult.ErrorCode, "ERR_NOT_YOUR_TURN");
}

TEST(CoreSmokeTests, ShouldRejectMoveWithoutPayload)
{
    FMatchReferee MatchReferee;
    StartStandardBattle(MatchReferee);

    FPlayerCommand MissingPayload{};
    MissingPayload.CommandType = ECommandType::Move;
    MissingPayload.Side = ESide::Red;

    FCommandResult MoveResult = MatchReferee.ApplyCommand(MissingPayload);
    EXPECT_FALSE(MoveResult.bAccepted);
    EXPECT_EQ(MoveResult.ErrorCode, "ERR_INVALID_PAYLOAD");
}

TEST(CoreSmokeTests, ShouldRejectMoveWhenFromDoesNotMatchPiecePosition)
{
    FMatchReferee MatchReferee;
    StartStandardBattle(MatchReferee);

    FPlayerCommand WrongFrom{};
    WrongFrom.CommandType = ECommandType::Move;
    WrongFrom.Side = ESide::Red;
    WrongFrom.Move = FMoveAction{
        static_cast<FPieceId>(0),
        FBoardPos{1, 1},
        FBoardPos{0, 1},
        std::nullopt};

    FCommandResult MoveResult = MatchReferee.ApplyCommand(WrongFrom);
    EXPECT_FALSE(MoveResult.bAccepted);
    EXPECT_EQ(MoveResult.ErrorCode, "ERR_INVALID_FROM");
}

TEST(CoreSmokeTests, ShouldRevealAndFreezePieceAfterFirstCaptureIfActualRoleIllegalAtTarget)
{
    FMatchReferee MatchReferee;

    std::array<FPieceId, 16> RedPieceOrder = BuildDefaultPieceOrder(ESide::Red);
    std::swap(RedPieceOrder[3], RedPieceOrder[9]); // Put red advisor (piece 3) at cannon slot (1,2)

    StartBattle(
        MatchReferee,
        BuildSetupFromPieceOrder(ESide::Red, RedPieceOrder, "RedFreezeNonce"),
        BuildStandardSetup(ESide::Black));

    const std::vector<FMoveAction> RedMoves = MatchReferee.GenerateLegalMoves(ESide::Red);
    const std::optional<FMoveAction> CaptureMove = FindMove(RedMoves, static_cast<FPieceId>(3), FBoardPos{1, 9});
    ASSERT_TRUE(CaptureMove.has_value());
    ASSERT_TRUE(CaptureMove->CapturedPieceId.has_value());

    FPlayerCommand MoveCommand{};
    MoveCommand.CommandType = ECommandType::Move;
    MoveCommand.Side = ESide::Red;
    MoveCommand.Move = CaptureMove;

    FCommandResult MoveResult = MatchReferee.ApplyCommand(MoveCommand);
    ASSERT_TRUE(MoveResult.bAccepted);

    const FGameState& StateAfterMove = MatchReferee.GetState();
    const FPieceState* Piece = FindPieceOrNull(StateAfterMove, static_cast<FPieceId>(3));
    ASSERT_NE(Piece, nullptr);
    EXPECT_EQ(Piece->PieceState, EPieceState::RevealedActual);
    EXPECT_TRUE(Piece->bHasCaptured);
    EXPECT_TRUE(Piece->bFrozen);
    EXPECT_EQ(Piece->Pos, (FBoardPos{1, 9}));

    const FPieceState* CapturedPiece = FindPieceOrNull(StateAfterMove, CaptureMove->CapturedPieceId.value());
    ASSERT_NE(CapturedPiece, nullptr);
    EXPECT_FALSE(CapturedPiece->bAlive);

    const std::vector<FMoveAction> RedMovesAfter = MatchReferee.GenerateLegalMoves(ESide::Red);
    const bool bFrozenPieceHasMove = std::any_of(RedMovesAfter.begin(), RedMovesAfter.end(), [](const FMoveAction& Move) {
        return Move.PieceId == static_cast<FPieceId>(3);
    });
    EXPECT_FALSE(bFrozenPieceHasMove);
}
