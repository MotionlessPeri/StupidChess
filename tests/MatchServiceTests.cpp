#include "Server/MatchService.h"

#include <gtest/gtest.h>

namespace
{
FPlayerCommand BuildInvalidMoveCommand()
{
    FPlayerCommand Command{};
    Command.CommandType = ECommandType::Move;
    Command.Move = FMoveAction{
        static_cast<FPieceId>(0),
        FBoardPos{0, 0},
        FBoardPos{0, 1},
        std::nullopt};
    return Command;
}
}

TEST(MatchServiceTests, ShouldCreateSingleSessionAndBindPlayers)
{
    FInMemoryMatchService Service;

    const FMatchJoinResponse RedJoin = Service.JoinMatch({100, 1001});
    const FMatchJoinResponse BlackJoin = Service.JoinMatch({100, 1002});
    const FMatchJoinResponse RedRejoin = Service.JoinMatch({100, 1001});

    ASSERT_TRUE(RedJoin.bAccepted);
    ASSERT_TRUE(BlackJoin.bAccepted);
    ASSERT_TRUE(RedRejoin.bAccepted);
    EXPECT_EQ(RedJoin.AssignedSide, ESide::Red);
    EXPECT_EQ(BlackJoin.AssignedSide, ESide::Black);
    EXPECT_EQ(RedRejoin.AssignedSide, ESide::Red);
    EXPECT_EQ(Service.GetActiveMatchCount(), static_cast<size_t>(1));
}

TEST(MatchServiceTests, ShouldRejectJoinWhenPlayerAlreadyBoundToAnotherMatch)
{
    FInMemoryMatchService Service;
    ASSERT_TRUE(Service.JoinMatch({101, 2001}).bAccepted);

    const FMatchJoinResponse JoinAnotherMatch = Service.JoinMatch({102, 2001});
    EXPECT_FALSE(JoinAnotherMatch.bAccepted);
}

TEST(MatchServiceTests, ShouldPullDeltaByStoredAckCursor)
{
    FInMemoryMatchService Service;
    ASSERT_TRUE(Service.JoinMatch({103, 3001}).bAccepted);
    ASSERT_TRUE(Service.JoinMatch({103, 3002}).bAccepted);

    const FMatchSyncResponse InitialSync = Service.PullPlayerSync(3001);
    ASSERT_TRUE(InitialSync.bAccepted);
    EXPECT_EQ(InitialSync.RequestedAfterSequence, static_cast<uint64_t>(0));
    ASSERT_EQ(InitialSync.Events.size(), static_cast<size_t>(2));
    EXPECT_EQ(InitialSync.LatestSequence, static_cast<uint64_t>(2));

    ASSERT_TRUE(Service.AckPlayerEvents(3001, 2));
    const FMatchSyncResponse EmptyDeltaSync = Service.PullPlayerSync(3001);
    ASSERT_TRUE(EmptyDeltaSync.bAccepted);
    EXPECT_EQ(EmptyDeltaSync.RequestedAfterSequence, static_cast<uint64_t>(2));
    EXPECT_TRUE(EmptyDeltaSync.Events.empty());

    const FCommandResult InvalidMoveResult = Service.SubmitPlayerCommand(3001, BuildInvalidMoveCommand());
    EXPECT_FALSE(InvalidMoveResult.bAccepted);

    const FMatchSyncResponse RejectedDeltaSync = Service.PullPlayerSync(3001);
    ASSERT_TRUE(RejectedDeltaSync.bAccepted);
    ASSERT_EQ(RejectedDeltaSync.Events.size(), static_cast<size_t>(1));
    EXPECT_EQ(RejectedDeltaSync.Events[0].EventType, EMatchEventType::CommandRejected);
}

TEST(MatchServiceTests, ShouldAllowOverrideCursorForFullResync)
{
    FInMemoryMatchService Service;
    ASSERT_TRUE(Service.JoinMatch({104, 4001}).bAccepted);
    ASSERT_TRUE(Service.JoinMatch({104, 4002}).bAccepted);
    ASSERT_TRUE(Service.AckPlayerEvents(4001, 2));

    const FCommandResult InvalidMoveResult = Service.SubmitPlayerCommand(4001, BuildInvalidMoveCommand());
    EXPECT_FALSE(InvalidMoveResult.bAccepted);

    const FMatchSyncResponse FullResync = Service.PullPlayerSync(4001, 0);
    ASSERT_TRUE(FullResync.bAccepted);
    EXPECT_EQ(FullResync.RequestedAfterSequence, static_cast<uint64_t>(0));
    ASSERT_GE(FullResync.Events.size(), static_cast<size_t>(3));
}

TEST(MatchServiceTests, ShouldRejectAckBeyondLatestSequence)
{
    FInMemoryMatchService Service;
    ASSERT_TRUE(Service.JoinMatch({105, 5001}).bAccepted);
    ASSERT_TRUE(Service.JoinMatch({105, 5002}).bAccepted);

    EXPECT_FALSE(Service.AckPlayerEvents(5001, 3));
    EXPECT_TRUE(Service.AckPlayerEvents(5001, 2));
    EXPECT_FALSE(Service.AckPlayerEvents(5001, 1));
}
