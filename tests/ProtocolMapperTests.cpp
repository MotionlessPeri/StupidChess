#include "Server/MatchService.h"
#include "Server/ProtocolMapper.h"

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

TEST(ProtocolMapperTests, ShouldMapJoinAndCommandAckPayloads)
{
    const FProtocolJoinAckPayload JoinAccepted = FProtocolMapper::BuildJoinAckPayload({true, ESide::Black, {}});
    EXPECT_TRUE(JoinAccepted.bAccepted);
    EXPECT_EQ(JoinAccepted.AssignedSide, static_cast<int32_t>(ESide::Black));
    EXPECT_EQ(JoinAccepted.ErrorCode, "");

    const FProtocolJoinAckPayload JoinRejected = FProtocolMapper::BuildJoinAckPayload({false, ESide::Red, "match full"});
    EXPECT_FALSE(JoinRejected.bAccepted);
    EXPECT_EQ(JoinRejected.ErrorCode, "ERR_JOIN_REJECTED");
    EXPECT_EQ(JoinRejected.ErrorMessage, "match full");

    const FProtocolCommandAckPayload CommandRejected = FProtocolMapper::BuildCommandAckPayload(
        {false, "ERR_NOT_YOUR_TURN", "not your turn"});
    EXPECT_FALSE(CommandRejected.bAccepted);
    EXPECT_EQ(CommandRejected.ErrorCode, "ERR_NOT_YOUR_TURN");
    EXPECT_EQ(CommandRejected.ErrorMessage, "not your turn");
}

TEST(ProtocolMapperTests, ShouldMapReconnectSyncBundleByAckCursor)
{
    FInMemoryMatchService Service;
    ASSERT_TRUE(Service.JoinMatch({200, 6001}).bAccepted);
    ASSERT_TRUE(Service.JoinMatch({200, 6002}).bAccepted);

    const FMatchSyncResponse InitialSync = Service.PullPlayerSync(6001);
    ASSERT_TRUE(InitialSync.bAccepted);
    ASSERT_EQ(InitialSync.Events.size(), static_cast<size_t>(2));

    const FProtocolSyncBundle InitialBundle = FProtocolMapper::BuildSyncBundle(InitialSync);
    EXPECT_EQ(InitialBundle.Snapshot.LastEventSequence, InitialSync.LatestSequence);
    EXPECT_EQ(InitialBundle.EventDelta.RequestedAfterSequence, static_cast<uint64_t>(0));
    ASSERT_EQ(InitialBundle.EventDelta.Events.size(), static_cast<size_t>(2));
    EXPECT_EQ(InitialBundle.Snapshot.Pieces.size(), InitialSync.View.Pieces.size());

    ASSERT_TRUE(Service.AckPlayerEvents(6001, InitialSync.LatestSequence));
    const FCommandResult RejectedMoveResult = Service.SubmitPlayerCommand(6001, BuildInvalidMoveCommand());
    EXPECT_FALSE(RejectedMoveResult.bAccepted);

    const FMatchSyncResponse DeltaSync = Service.PullPlayerSync(6001);
    ASSERT_TRUE(DeltaSync.bAccepted);
    ASSERT_EQ(DeltaSync.Events.size(), static_cast<size_t>(1));
    EXPECT_EQ(DeltaSync.RequestedAfterSequence, InitialSync.LatestSequence);

    const FProtocolSyncBundle DeltaBundle = FProtocolMapper::BuildSyncBundle(DeltaSync);
    ASSERT_EQ(DeltaBundle.EventDelta.Events.size(), static_cast<size_t>(1));
    EXPECT_EQ(DeltaBundle.EventDelta.Events[0].EventType, static_cast<int32_t>(EMatchEventType::CommandRejected));

    ASSERT_TRUE(Service.AckPlayerEvents(6001, DeltaSync.LatestSequence));
    const FMatchSyncResponse EmptySync = Service.PullPlayerSync(6001);
    ASSERT_TRUE(EmptySync.bAccepted);
    EXPECT_TRUE(EmptySync.Events.empty());

    const FProtocolSyncBundle EmptyBundle = FProtocolMapper::BuildSyncBundle(EmptySync);
    EXPECT_TRUE(EmptyBundle.EventDelta.Events.empty());

    const FMatchSyncResponse FullResync = Service.PullPlayerSync(6001, 0);
    ASSERT_TRUE(FullResync.bAccepted);
    EXPECT_GE(FullResync.Events.size(), static_cast<size_t>(3));
}
