#include "Server/TransportAdapter.h"

#include <gtest/gtest.h>

namespace
{
FPlayerCommand BuildRedCommitCommand()
{
    FPlayerCommand Command{};
    Command.CommandType = ECommandType::CommitSetup;
    Command.SetupCommit = FSetupCommit{ESide::Red, {}};
    return Command;
}
}

TEST(TransportAdapterTests, ShouldSendJoinAckAndInitialSyncMessages)
{
    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);

    ASSERT_TRUE(Adapter.HandleJoinRequest({300, 7001}));

    const std::vector<FOutboundProtocolMessage> Messages = Sink.PullMessages(7001);
    ASSERT_EQ(Messages.size(), static_cast<size_t>(3));
    EXPECT_EQ(Messages[0].Envelope.MessageType, EProtocolMessageType::S2C_JoinAck);
    ASSERT_TRUE(Messages[0].JoinAck.has_value());
    EXPECT_TRUE(Messages[0].JoinAck->bAccepted);
    EXPECT_EQ(Messages[1].Envelope.MessageType, EProtocolMessageType::S2C_Snapshot);
    ASSERT_TRUE(Messages[1].Snapshot.has_value());
    EXPECT_EQ(Messages[2].Envelope.MessageType, EProtocolMessageType::S2C_EventDelta);
    ASSERT_TRUE(Messages[2].EventDelta.has_value());
    EXPECT_EQ(Messages[2].EventDelta->Events.size(), static_cast<size_t>(1));
}

TEST(TransportAdapterTests, ShouldBroadcastSnapshotAndDeltaAfterAcceptedCommand)
{
    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);

    ASSERT_TRUE(Adapter.HandleJoinRequest({301, 7101}));
    ASSERT_TRUE(Adapter.HandleJoinRequest({301, 7102}));
    Sink.Clear();

    ASSERT_TRUE(Adapter.HandlePlayerCommand(7101, BuildRedCommitCommand()));

    const std::vector<FOutboundProtocolMessage> RedMessages = Sink.PullMessages(7101);
    const std::vector<FOutboundProtocolMessage> BlackMessages = Sink.PullMessages(7102);
    ASSERT_EQ(RedMessages.size(), static_cast<size_t>(3));
    EXPECT_EQ(RedMessages[0].Envelope.MessageType, EProtocolMessageType::S2C_CommandAck);
    EXPECT_TRUE(RedMessages[0].CommandAck.has_value());
    EXPECT_TRUE(RedMessages[0].CommandAck->bAccepted);
    EXPECT_EQ(RedMessages[1].Envelope.MessageType, EProtocolMessageType::S2C_Snapshot);
    EXPECT_EQ(RedMessages[2].Envelope.MessageType, EProtocolMessageType::S2C_EventDelta);

    ASSERT_EQ(BlackMessages.size(), static_cast<size_t>(2));
    EXPECT_EQ(BlackMessages[0].Envelope.MessageType, EProtocolMessageType::S2C_Snapshot);
    EXPECT_EQ(BlackMessages[1].Envelope.MessageType, EProtocolMessageType::S2C_EventDelta);
}

TEST(TransportAdapterTests, ShouldSendErrorWhenAckIsInvalid)
{
    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);

    ASSERT_TRUE(Adapter.HandleJoinRequest({302, 7201}));
    Sink.Clear();

    EXPECT_FALSE(Adapter.HandleAck(7201, 10));

    const std::vector<FOutboundProtocolMessage> Messages = Sink.PullMessages(7201);
    ASSERT_EQ(Messages.size(), static_cast<size_t>(1));
    EXPECT_EQ(Messages[0].Envelope.MessageType, EProtocolMessageType::S2C_Error);
    EXPECT_EQ(Messages[0].ErrorMessage, "Ack sequence is invalid.");
}

TEST(TransportAdapterTests, ShouldSendErrorForUnknownPlayerSyncPull)
{
    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);

    EXPECT_FALSE(Adapter.HandlePullSync(9999));

    const std::vector<FOutboundProtocolMessage> Messages = Sink.PullMessages(9999);
    ASSERT_EQ(Messages.size(), static_cast<size_t>(1));
    EXPECT_EQ(Messages[0].Envelope.MessageType, EProtocolMessageType::S2C_Error);
}
