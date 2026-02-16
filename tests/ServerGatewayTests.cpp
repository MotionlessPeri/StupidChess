#include "Protocol/ProtocolCodec.h"
#include "Server/ServerGateway.h"

#include <gtest/gtest.h>

namespace
{
FProtocolEnvelope BuildEnvelope(EProtocolMessageType MessageType, uint64_t Sequence, uint64_t MatchId, std::string PayloadJson)
{
    return FProtocolEnvelope{
        MessageType,
        Sequence,
        std::to_string(MatchId),
        std::move(PayloadJson)};
}
}

TEST(ServerGatewayTests, ShouldProcessJoinEnvelopeAndEmitInitialSync)
{
    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);
    FServerGateway Gateway(&Adapter);

    std::string JoinPayloadJson;
    ASSERT_TRUE(ProtocolCodec::EncodeJoinPayload({600, 8201}, JoinPayloadJson));

    EXPECT_TRUE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Join, 1, 600, JoinPayloadJson)));

    const std::vector<FOutboundProtocolMessage> Messages = Sink.PullMessages(8201);
    ASSERT_EQ(Messages.size(), static_cast<size_t>(3));
    EXPECT_EQ(Messages[0].Envelope.MessageType, EProtocolMessageType::S2C_JoinAck);
    EXPECT_EQ(Messages[1].Envelope.MessageType, EProtocolMessageType::S2C_Snapshot);
    EXPECT_EQ(Messages[2].Envelope.MessageType, EProtocolMessageType::S2C_EventDelta);
}

TEST(ServerGatewayTests, ShouldRouteCommandEnvelopeToAdapter)
{
    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);
    FServerGateway Gateway(&Adapter);

    std::string RedJoinJson;
    ASSERT_TRUE(ProtocolCodec::EncodeJoinPayload({601, 8301}, RedJoinJson));
    ASSERT_TRUE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Join, 1, 601, RedJoinJson)));

    std::string BlackJoinJson;
    ASSERT_TRUE(ProtocolCodec::EncodeJoinPayload({601, 8302}, BlackJoinJson));
    ASSERT_TRUE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Join, 2, 601, BlackJoinJson)));

    Sink.Clear();

    FProtocolCommandPayload CommandPayload{};
    CommandPayload.PlayerId = 8301;
    CommandPayload.CommandType = static_cast<int32_t>(ECommandType::CommitSetup);
    CommandPayload.Side = static_cast<int32_t>(ESide::Red);
    CommandPayload.bHasSetupCommit = true;
    CommandPayload.SetupCommit.Side = static_cast<int32_t>(ESide::Red);
    CommandPayload.SetupCommit.HashHex = "";

    std::string CommandJson;
    ASSERT_TRUE(ProtocolCodec::EncodeCommandPayload(CommandPayload, CommandJson));
    EXPECT_TRUE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Command, 3, 601, CommandJson)));

    const std::vector<FOutboundProtocolMessage> RedMessages = Sink.PullMessages(8301);
    const std::vector<FOutboundProtocolMessage> BlackMessages = Sink.PullMessages(8302);
    ASSERT_EQ(RedMessages.size(), static_cast<size_t>(3));
    EXPECT_EQ(RedMessages[0].Envelope.MessageType, EProtocolMessageType::S2C_CommandAck);
    ASSERT_EQ(BlackMessages.size(), static_cast<size_t>(2));
    EXPECT_EQ(BlackMessages[0].Envelope.MessageType, EProtocolMessageType::S2C_Snapshot);
    EXPECT_EQ(BlackMessages[1].Envelope.MessageType, EProtocolMessageType::S2C_EventDelta);
}

TEST(ServerGatewayTests, ShouldProcessAckAndRejectMalformedEnvelopeJson)
{
    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);
    FServerGateway Gateway(&Adapter);

    std::string JoinPayloadJson;
    ASSERT_TRUE(ProtocolCodec::EncodeJoinPayload({602, 8401}, JoinPayloadJson));
    ASSERT_TRUE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Join, 1, 602, JoinPayloadJson)));
    Sink.Clear();

    std::string AckJson;
    ASSERT_TRUE(ProtocolCodec::EncodeAckPayload({8401, 99}, AckJson));
    EXPECT_FALSE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Ack, 2, 602, AckJson)));
    ASSERT_EQ(Sink.PullMessages(8401).size(), static_cast<size_t>(1));
    EXPECT_EQ(Sink.PullMessages(8401)[0].Envelope.MessageType, EProtocolMessageType::S2C_Error);

    EXPECT_FALSE(Gateway.ProcessEnvelopeJson("{invalid json"));
}

TEST(ServerGatewayTests, ShouldRejectInvalidCommandTypePayload)
{
    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);
    FServerGateway Gateway(&Adapter);

    std::string JoinPayloadJson;
    ASSERT_TRUE(ProtocolCodec::EncodeJoinPayload({603, 8501}, JoinPayloadJson));
    ASSERT_TRUE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Join, 1, 603, JoinPayloadJson)));
    Sink.Clear();

    FProtocolCommandPayload CommandPayload{};
    CommandPayload.PlayerId = 8501;
    CommandPayload.CommandType = 777;
    CommandPayload.Side = static_cast<int32_t>(ESide::Red);

    std::string CommandJson;
    ASSERT_TRUE(ProtocolCodec::EncodeCommandPayload(CommandPayload, CommandJson));
    EXPECT_FALSE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Command, 2, 603, CommandJson)));
    EXPECT_TRUE(Sink.PullMessages(8501).empty());
}
