#include "Protocol/ProtocolCodec.h"
#include "Server/ServerGateway.h"

#include <array>

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

std::vector<FProtocolSetupPlacementPayload> BuildStandardPlacements(ESide Side)
{
    static constexpr std::array<FBoardPos, 16> RedSlots = {{
        {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0},
        {8, 0}, {1, 2}, {7, 2}, {0, 3}, {2, 3}, {4, 3}, {6, 3}, {8, 3}}};

    std::vector<FProtocolSetupPlacementPayload> Placements;
    Placements.reserve(16);
    const int32_t BasePieceId = Side == ESide::Red ? 0 : 16;

    for (int32_t Index = 0; Index < 16; ++Index)
    {
        FBoardPos Pos = RedSlots[Index];
        if (Side == ESide::Black)
        {
            Pos.Y = static_cast<int8_t>(9 - Pos.Y);
        }

        Placements.push_back(FProtocolSetupPlacementPayload{
            static_cast<uint16_t>(BasePieceId + Index),
            static_cast<int32_t>(Pos.X),
            static_cast<int32_t>(Pos.Y)});
    }

    return Placements;
}

FProtocolCommandPayload BuildCommitCommandPayload(uint64_t PlayerId, ESide Side)
{
    FProtocolCommandPayload Payload{};
    Payload.PlayerId = PlayerId;
    Payload.CommandType = static_cast<int32_t>(ECommandType::CommitSetup);
    Payload.Side = static_cast<int32_t>(Side);
    Payload.bHasSetupCommit = true;
    Payload.SetupCommit.Side = static_cast<int32_t>(Side);
    Payload.SetupCommit.HashHex = "";
    return Payload;
}

FProtocolCommandPayload BuildRevealCommandPayload(uint64_t PlayerId, ESide Side, std::string Nonce)
{
    FProtocolCommandPayload Payload{};
    Payload.PlayerId = PlayerId;
    Payload.CommandType = static_cast<int32_t>(ECommandType::RevealSetup);
    Payload.Side = static_cast<int32_t>(Side);
    Payload.bHasSetupPlain = true;
    Payload.SetupPlain.Side = static_cast<int32_t>(Side);
    Payload.SetupPlain.Nonce = std::move(Nonce);
    Payload.SetupPlain.Placements = BuildStandardPlacements(Side);
    return Payload;
}

FProtocolCommandPayload BuildMoveCommandPayload(
    uint64_t PlayerId,
    ESide Side,
    uint16_t PieceId,
    int32_t FromX,
    int32_t FromY,
    int32_t ToX,
    int32_t ToY)
{
    FProtocolCommandPayload Payload{};
    Payload.PlayerId = PlayerId;
    Payload.CommandType = static_cast<int32_t>(ECommandType::Move);
    Payload.Side = static_cast<int32_t>(Side);
    Payload.bHasMove = true;
    Payload.Move.PieceId = PieceId;
    Payload.Move.FromX = FromX;
    Payload.Move.FromY = FromY;
    Payload.Move.ToX = ToX;
    Payload.Move.ToY = ToY;
    Payload.Move.bHasCapturedPieceId = false;
    Payload.Move.CapturedPieceId = 0;
    return Payload;
}

FProtocolCommandPayload BuildResignCommandPayload(uint64_t PlayerId, ESide Side)
{
    FProtocolCommandPayload Payload{};
    Payload.PlayerId = PlayerId;
    Payload.CommandType = static_cast<int32_t>(ECommandType::Resign);
    Payload.Side = static_cast<int32_t>(Side);
    return Payload;
}

bool SendCommandEnvelope(FServerGateway& Gateway, uint64_t Sequence, uint64_t MatchId, const FProtocolCommandPayload& CommandPayload)
{
    std::string CommandJson;
    if (!ProtocolCodec::EncodeCommandPayload(CommandPayload, CommandJson))
    {
        return false;
    }

    return Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Command, Sequence, MatchId, std::move(CommandJson)));
}

bool SetupBattlePhase(FServerGateway& Gateway, uint64_t MatchId, uint64_t RedPlayerId, uint64_t BlackPlayerId, uint64_t& InOutSequence)
{
    return SendCommandEnvelope(Gateway, InOutSequence++, MatchId, BuildCommitCommandPayload(RedPlayerId, ESide::Red)) &&
           SendCommandEnvelope(Gateway, InOutSequence++, MatchId, BuildCommitCommandPayload(BlackPlayerId, ESide::Black)) &&
           SendCommandEnvelope(Gateway, InOutSequence++, MatchId, BuildRevealCommandPayload(RedPlayerId, ESide::Red, "R")) &&
           SendCommandEnvelope(Gateway, InOutSequence++, MatchId, BuildRevealCommandPayload(BlackPlayerId, ESide::Black, "B"));
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

TEST(ServerGatewayTests, ShouldRouteMoveCommandAndBroadcastSyncForBothPlayers)
{
    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);
    FServerGateway Gateway(&Adapter);

    std::string RedJoinJson;
    ASSERT_TRUE(ProtocolCodec::EncodeJoinPayload({604, 8601}, RedJoinJson));
    ASSERT_TRUE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Join, 1, 604, RedJoinJson)));

    std::string BlackJoinJson;
    ASSERT_TRUE(ProtocolCodec::EncodeJoinPayload({604, 8602}, BlackJoinJson));
    ASSERT_TRUE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Join, 2, 604, BlackJoinJson)));

    uint64_t Sequence = 3;
    ASSERT_TRUE(SetupBattlePhase(Gateway, 604, 8601, 8602, Sequence));

    Sink.Clear();

    const FProtocolCommandPayload MovePayload = BuildMoveCommandPayload(
        8601,
        ESide::Red,
        static_cast<uint16_t>(11),
        0,
        3,
        0,
        4);

    EXPECT_TRUE(SendCommandEnvelope(Gateway, Sequence++, 604, MovePayload));

    const std::vector<FOutboundProtocolMessage> RedMessages = Sink.PullMessages(8601);
    const std::vector<FOutboundProtocolMessage> BlackMessages = Sink.PullMessages(8602);
    ASSERT_EQ(RedMessages.size(), static_cast<size_t>(3));
    ASSERT_TRUE(RedMessages[0].CommandAck.has_value());
    EXPECT_TRUE(RedMessages[0].CommandAck->bAccepted);
    ASSERT_TRUE(RedMessages[1].Snapshot.has_value());
    EXPECT_EQ(RedMessages[1].Snapshot->CurrentTurn, static_cast<int32_t>(ESide::Black));
    ASSERT_TRUE(RedMessages[2].EventDelta.has_value());

    ASSERT_EQ(BlackMessages.size(), static_cast<size_t>(2));
    ASSERT_TRUE(BlackMessages[0].Snapshot.has_value());
    EXPECT_EQ(BlackMessages[0].Snapshot->CurrentTurn, static_cast<int32_t>(ESide::Black));
    ASSERT_TRUE(BlackMessages[1].EventDelta.has_value());
}

TEST(ServerGatewayTests, ShouldRouteResignCommandAndEnterGameOver)
{
    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);
    FServerGateway Gateway(&Adapter);

    std::string RedJoinJson;
    ASSERT_TRUE(ProtocolCodec::EncodeJoinPayload({605, 8701}, RedJoinJson));
    ASSERT_TRUE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Join, 1, 605, RedJoinJson)));

    std::string BlackJoinJson;
    ASSERT_TRUE(ProtocolCodec::EncodeJoinPayload({605, 8702}, BlackJoinJson));
    ASSERT_TRUE(Gateway.ProcessEnvelope(BuildEnvelope(EProtocolMessageType::C2S_Join, 2, 605, BlackJoinJson)));

    uint64_t Sequence = 3;
    ASSERT_TRUE(SetupBattlePhase(Gateway, 605, 8701, 8702, Sequence));

    Sink.Clear();

    const FProtocolCommandPayload ResignPayload = BuildResignCommandPayload(8701, ESide::Red);
    EXPECT_TRUE(SendCommandEnvelope(Gateway, Sequence++, 605, ResignPayload));

    const std::vector<FOutboundProtocolMessage> RedMessages = Sink.PullMessages(8701);
    const std::vector<FOutboundProtocolMessage> BlackMessages = Sink.PullMessages(8702);
    ASSERT_EQ(RedMessages.size(), static_cast<size_t>(3));
    ASSERT_TRUE(RedMessages[0].CommandAck.has_value());
    EXPECT_TRUE(RedMessages[0].CommandAck->bAccepted);
    ASSERT_TRUE(RedMessages[1].Snapshot.has_value());
    EXPECT_EQ(RedMessages[1].Snapshot->Result, static_cast<int32_t>(EGameResult::BlackWin));
    EXPECT_EQ(RedMessages[1].Snapshot->EndReason, static_cast<int32_t>(EEndReason::Resign));
    EXPECT_EQ(RedMessages[1].Snapshot->Phase, static_cast<int32_t>(EGamePhase::GameOver));

    ASSERT_EQ(BlackMessages.size(), static_cast<size_t>(2));
    ASSERT_TRUE(BlackMessages[0].Snapshot.has_value());
    EXPECT_EQ(BlackMessages[0].Snapshot->Result, static_cast<int32_t>(EGameResult::BlackWin));
    EXPECT_EQ(BlackMessages[0].Snapshot->EndReason, static_cast<int32_t>(EEndReason::Resign));
    EXPECT_EQ(BlackMessages[0].Snapshot->Phase, static_cast<int32_t>(EGamePhase::GameOver));
}
