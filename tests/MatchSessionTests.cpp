#include "Server/MatchSession.h"

#include <algorithm>

#include <gtest/gtest.h>

namespace
{
FSetupPlain BuildStandardSetup(ESide Side)
{
    static constexpr FBoardPos RedSlots[16] = {
        {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0},
        {8, 0}, {1, 2}, {7, 2}, {0, 3}, {2, 3}, {4, 3}, {6, 3}, {8, 3}};

    FSetupPlain Setup{};
    Setup.Side = Side;
    Setup.Nonce = Side == ESide::Red ? "R" : "B";
    Setup.Placements.reserve(16);

    const int32_t BasePieceId = Side == ESide::Red ? 0 : 16;
    for (int32_t Index = 0; Index < 16; ++Index)
    {
        FBoardPos Pos = RedSlots[Index];
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

FSetupPlain BuildBlackSwappedSetupForVisibilityTest()
{
    FSetupPlain Setup = BuildStandardSetup(ESide::Black);

    auto FindPlacement = [&Setup](FPieceId PieceId) -> FSetupPlacement*
    {
        auto It = std::find_if(Setup.Placements.begin(), Setup.Placements.end(), [PieceId](const FSetupPlacement& Placement) {
            return Placement.PieceId == PieceId;
        });

        return It == Setup.Placements.end() ? nullptr : &(*It);
    };

    FSetupPlacement* KingPlacement = FindPlacement(static_cast<FPieceId>(20));
    FSetupPlacement* HorsePlacement = FindPlacement(static_cast<FPieceId>(17));
    EXPECT_NE(KingPlacement, nullptr);
    EXPECT_NE(HorsePlacement, nullptr);
    if (KingPlacement == nullptr || HorsePlacement == nullptr)
    {
        return Setup;
    }

    std::swap(KingPlacement->TargetPos, HorsePlacement->TargetPos);
    return Setup;
}

void SetupBattlePhase(
    FInMemoryMatchSession& Session,
    FPlayerId RedPlayerId,
    FPlayerId BlackPlayerId,
    const FSetupPlain* RedSetupOverride = nullptr,
    const FSetupPlain* BlackSetupOverride = nullptr)
{
    FPlayerCommand RedCommit{};
    RedCommit.CommandType = ECommandType::CommitSetup;
    RedCommit.SetupCommit = FSetupCommit{ESide::Red, ""};
    ASSERT_TRUE(Session.SubmitCommand(RedPlayerId, RedCommit).bAccepted);

    FPlayerCommand BlackCommit{};
    BlackCommit.CommandType = ECommandType::CommitSetup;
    BlackCommit.SetupCommit = FSetupCommit{ESide::Black, ""};
    ASSERT_TRUE(Session.SubmitCommand(BlackPlayerId, BlackCommit).bAccepted);

    FPlayerCommand RedReveal{};
    RedReveal.CommandType = ECommandType::RevealSetup;
    RedReveal.SetupPlain = RedSetupOverride == nullptr ? BuildStandardSetup(ESide::Red) : *RedSetupOverride;
    ASSERT_TRUE(Session.SubmitCommand(RedPlayerId, RedReveal).bAccepted);

    FPlayerCommand BlackReveal{};
    BlackReveal.CommandType = ECommandType::RevealSetup;
    BlackReveal.SetupPlain = BlackSetupOverride == nullptr ? BuildStandardSetup(ESide::Black) : *BlackSetupOverride;
    ASSERT_TRUE(Session.SubmitCommand(BlackPlayerId, BlackReveal).bAccepted);
}
}

TEST(MatchSessionTests, ShouldAssignRedThenBlackOnJoin)
{
    FInMemoryMatchSession Session(7);

    const FMatchJoinResponse Red = Session.Join({7, 1001});
    const FMatchJoinResponse Black = Session.Join({7, 1002});
    const FMatchJoinResponse Third = Session.Join({7, 1003});

    EXPECT_TRUE(Red.bAccepted);
    EXPECT_EQ(Red.AssignedSide, ESide::Red);
    EXPECT_TRUE(Black.bAccepted);
    EXPECT_EQ(Black.AssignedSide, ESide::Black);
    EXPECT_FALSE(Third.bAccepted);
}

TEST(MatchSessionTests, ShouldHideOpponentActualRoleBeforeReveal)
{
    FInMemoryMatchSession Session(8);
    ASSERT_TRUE(Session.Join({8, 2001}).bAccepted);
    ASSERT_TRUE(Session.Join({8, 2002}).bAccepted);

    const FSetupPlain BlackSwappedSetup = BuildBlackSwappedSetupForVisibilityTest();
    SetupBattlePhase(Session, 2001, 2002, nullptr, &BlackSwappedSetup);

    const FMatchPlayerView RedView = Session.GetPlayerView(2001);
    const auto BlackKingInRedView = std::find_if(RedView.Pieces.begin(), RedView.Pieces.end(), [](const FPlayerPieceView& Piece) {
        return Piece.PieceId == static_cast<FPieceId>(20);
    });
    ASSERT_NE(BlackKingInRedView, RedView.Pieces.end());
    EXPECT_EQ(BlackKingInRedView->Pos, (FBoardPos{1, 9}));
    EXPECT_EQ(BlackKingInRedView->VisibleRole, ERoleType::Horse); // slot role, not actual role
    EXPECT_FALSE(BlackKingInRedView->bRevealed);

    const FMatchPlayerView BlackView = Session.GetPlayerView(2002);
    const auto BlackKingInBlackView = std::find_if(BlackView.Pieces.begin(), BlackView.Pieces.end(), [](const FPlayerPieceView& Piece) {
        return Piece.PieceId == static_cast<FPieceId>(20);
    });
    ASSERT_NE(BlackKingInBlackView, BlackView.Pieces.end());
    EXPECT_EQ(BlackKingInBlackView->VisibleRole, ERoleType::King);
}

TEST(MatchSessionTests, ShouldRecordEventsAndSupportSequencePull)
{
    FInMemoryMatchSession Session(9);
    ASSERT_TRUE(Session.Join({9, 3001}).bAccepted);
    ASSERT_TRUE(Session.Join({9, 3002}).bAccepted);

    const std::vector<FMatchEventRecord> JoinEvents = Session.PullEvents(3001, 0);
    ASSERT_EQ(JoinEvents.size(), static_cast<size_t>(2));
    EXPECT_EQ(JoinEvents[0].EventType, EMatchEventType::PlayerJoined);
    EXPECT_EQ(JoinEvents[1].EventType, EMatchEventType::PlayerJoined);

    SetupBattlePhase(Session, 3001, 3002);

    const std::vector<FMatchEventRecord> PostJoinEvents = Session.PullEvents(3001, 2);
    ASSERT_GE(PostJoinEvents.size(), static_cast<size_t>(4));
    EXPECT_EQ(PostJoinEvents.front().Sequence, static_cast<uint64_t>(3));

    FPlayerCommand InvalidMove{};
    InvalidMove.CommandType = ECommandType::Move;
    InvalidMove.Side = ESide::Red;
    InvalidMove.Move = FMoveAction{
        static_cast<FPieceId>(0),
        FBoardPos{1, 1},
        FBoardPos{1, 2},
        std::nullopt};

    const FCommandResult MoveResult = Session.SubmitCommand(3001, InvalidMove);
    EXPECT_FALSE(MoveResult.bAccepted);

    const std::vector<FMatchEventRecord> FinalEvents = Session.PullEvents(3001, 0);
    EXPECT_EQ(FinalEvents.back().EventType, EMatchEventType::CommandRejected);
}
