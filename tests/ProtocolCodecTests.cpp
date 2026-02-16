#include "Protocol/ProtocolCodec.h"
#include "Server/MatchService.h"
#include "Server/ProtocolMapper.h"

#include <gtest/gtest.h>

TEST(ProtocolCodecTests, ShouldRoundTripEnvelopeAndJoinPayload)
{
    const FProtocolJoinPayload JoinPayload{401, 8001};
    std::string JoinPayloadJson;
    ASSERT_TRUE(ProtocolCodec::EncodeJoinPayload(JoinPayload, JoinPayloadJson));

    FProtocolJoinPayload DecodedJoinPayload{};
    ASSERT_TRUE(ProtocolCodec::DecodeJoinPayload(JoinPayloadJson, DecodedJoinPayload));
    EXPECT_EQ(DecodedJoinPayload.MatchId, JoinPayload.MatchId);
    EXPECT_EQ(DecodedJoinPayload.PlayerId, JoinPayload.PlayerId);

    const FProtocolEnvelope Envelope{
        EProtocolMessageType::C2S_Join,
        12,
        "401",
        JoinPayloadJson};
    std::string EnvelopeJson;
    ASSERT_TRUE(ProtocolCodec::EncodeEnvelope(Envelope, EnvelopeJson));

    FProtocolEnvelope DecodedEnvelope{};
    ASSERT_TRUE(ProtocolCodec::DecodeEnvelope(EnvelopeJson, DecodedEnvelope));
    EXPECT_EQ(DecodedEnvelope.MessageType, Envelope.MessageType);
    EXPECT_EQ(DecodedEnvelope.Sequence, Envelope.Sequence);
    EXPECT_EQ(DecodedEnvelope.MatchId, Envelope.MatchId);
    EXPECT_EQ(DecodedEnvelope.PayloadJson, Envelope.PayloadJson);
}

TEST(ProtocolCodecTests, ShouldRoundTripCommandPayloadWithSetupPlain)
{
    FProtocolCommandPayload Payload{};
    Payload.PlayerId = 9001;
    Payload.CommandType = static_cast<int32_t>(ECommandType::RevealSetup);
    Payload.Side = static_cast<int32_t>(ESide::Red);
    Payload.bHasSetupPlain = true;
    Payload.SetupPlain.Side = static_cast<int32_t>(ESide::Red);
    Payload.SetupPlain.Nonce = "nonce";
    Payload.SetupPlain.Placements = {
        FProtocolSetupPlacementPayload{0, 0, 0},
        FProtocolSetupPlacementPayload{1, 1, 0}};

    std::string Json;
    ASSERT_TRUE(ProtocolCodec::EncodeCommandPayload(Payload, Json));

    FProtocolCommandPayload Decoded{};
    ASSERT_TRUE(ProtocolCodec::DecodeCommandPayload(Json, Decoded));
    EXPECT_EQ(Decoded.PlayerId, Payload.PlayerId);
    EXPECT_EQ(Decoded.CommandType, Payload.CommandType);
    EXPECT_EQ(Decoded.Side, Payload.Side);
    EXPECT_TRUE(Decoded.bHasSetupPlain);
    ASSERT_EQ(Decoded.SetupPlain.Placements.size(), static_cast<size_t>(2));
    EXPECT_EQ(Decoded.SetupPlain.Placements[0].PieceId, static_cast<uint16_t>(0));
    EXPECT_EQ(Decoded.SetupPlain.Placements[1].X, 1);
}

TEST(ProtocolCodecTests, ShouldRoundTripSnapshotAndEventDeltaPayload)
{
    FInMemoryMatchService Service;
    ASSERT_TRUE(Service.JoinMatch({500, 8101}).bAccepted);
    ASSERT_TRUE(Service.JoinMatch({500, 8102}).bAccepted);

    const FMatchSyncResponse Sync = Service.PullPlayerSync(8101);
    ASSERT_TRUE(Sync.bAccepted);
    const FProtocolSyncBundle Bundle = FProtocolMapper::BuildSyncBundle(Sync);

    std::string SnapshotJson;
    ASSERT_TRUE(ProtocolCodec::EncodeSnapshotPayload(Bundle.Snapshot, SnapshotJson));
    FProtocolSnapshotPayload DecodedSnapshot{};
    ASSERT_TRUE(ProtocolCodec::DecodeSnapshotPayload(SnapshotJson, DecodedSnapshot));
    EXPECT_EQ(DecodedSnapshot.LastEventSequence, Bundle.Snapshot.LastEventSequence);
    EXPECT_EQ(DecodedSnapshot.Pieces.size(), Bundle.Snapshot.Pieces.size());

    std::string DeltaJson;
    ASSERT_TRUE(ProtocolCodec::EncodeEventDeltaPayload(Bundle.EventDelta, DeltaJson));
    FProtocolEventDeltaPayload DecodedDelta{};
    ASSERT_TRUE(ProtocolCodec::DecodeEventDeltaPayload(DeltaJson, DecodedDelta));
    EXPECT_EQ(DecodedDelta.RequestedAfterSequence, Bundle.EventDelta.RequestedAfterSequence);
    EXPECT_EQ(DecodedDelta.Events.size(), Bundle.EventDelta.Events.size());
}
