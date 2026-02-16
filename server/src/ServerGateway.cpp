#include "Server/ServerGateway.h"

#include "Protocol/ProtocolCodec.h"

#include <optional>
#include <utility>

FServerGateway::FServerGateway(FServerTransportAdapter* InTransportAdapter)
    : TransportAdapter(InTransportAdapter)
{
}

bool FServerGateway::ProcessEnvelope(const FProtocolEnvelope& Envelope)
{
    if (TransportAdapter == nullptr)
    {
        return false;
    }

    switch (Envelope.MessageType)
    {
    case EProtocolMessageType::C2S_Join:
    {
        FProtocolJoinPayload Payload{};
        if (!ProtocolCodec::DecodeJoinPayload(Envelope.PayloadJson, Payload))
        {
            return false;
        }

        return TransportAdapter->HandleJoinRequest(Payload);
    }
    case EProtocolMessageType::C2S_Command:
    {
        FProtocolCommandPayload Payload{};
        if (!ProtocolCodec::DecodeCommandPayload(Envelope.PayloadJson, Payload))
        {
            return false;
        }

        FPlayerCommand Command{};
        if (!BuildPlayerCommand(Payload, Command))
        {
            return false;
        }

        return TransportAdapter->HandlePlayerCommand(Payload.PlayerId, Command);
    }
    case EProtocolMessageType::C2S_PullSync:
    {
        FProtocolPullSyncPayload Payload{};
        if (!ProtocolCodec::DecodePullSyncPayload(Envelope.PayloadJson, Payload))
        {
            return false;
        }

        const std::optional<uint64_t> SequenceOverride =
            Payload.bHasAfterSequenceOverride ? std::optional<uint64_t>(Payload.AfterSequenceOverride) : std::nullopt;
        return TransportAdapter->HandlePullSync(Payload.PlayerId, SequenceOverride);
    }
    case EProtocolMessageType::C2S_Ack:
    {
        FProtocolAckPayload Payload{};
        if (!ProtocolCodec::DecodeAckPayload(Envelope.PayloadJson, Payload))
        {
            return false;
        }

        return TransportAdapter->HandleAck(Payload.PlayerId, Payload.Sequence);
    }
    case EProtocolMessageType::C2S_Ping:
        return true;
    default:
        return false;
    }
}

bool FServerGateway::ProcessEnvelopeJson(const std::string& EnvelopeJson)
{
    FProtocolEnvelope Envelope{};
    if (!ProtocolCodec::DecodeEnvelope(EnvelopeJson, Envelope))
    {
        return false;
    }

    return ProcessEnvelope(Envelope);
}

bool FServerGateway::BuildPlayerCommand(const FProtocolCommandPayload& Payload, FPlayerCommand& OutCommand) const
{
    const ECommandType CommandType = static_cast<ECommandType>(Payload.CommandType);
    switch (CommandType)
    {
    case ECommandType::CommitSetup:
        if (!Payload.bHasSetupCommit)
        {
            return false;
        }
        break;
    case ECommandType::RevealSetup:
        if (!Payload.bHasSetupPlain)
        {
            return false;
        }
        break;
    case ECommandType::Move:
        if (!Payload.bHasMove)
        {
            return false;
        }
        break;
    case ECommandType::Pass:
    case ECommandType::Resign:
        break;
    default:
        return false;
    }

    OutCommand.CommandType = CommandType;
    OutCommand.Side = static_cast<ESide>(Payload.Side);

    if (Payload.bHasMove)
    {
        const std::optional<FPieceId> CapturedPieceId = Payload.Move.bHasCapturedPieceId
                                                            ? std::optional<FPieceId>(Payload.Move.CapturedPieceId)
                                                            : std::nullopt;
        OutCommand.Move = FMoveAction{
            Payload.Move.PieceId,
            FBoardPos{static_cast<int8_t>(Payload.Move.FromX), static_cast<int8_t>(Payload.Move.FromY)},
            FBoardPos{static_cast<int8_t>(Payload.Move.ToX), static_cast<int8_t>(Payload.Move.ToY)},
            CapturedPieceId};
    }

    if (Payload.bHasSetupCommit)
    {
        OutCommand.SetupCommit = FSetupCommit{
            static_cast<ESide>(Payload.SetupCommit.Side),
            Payload.SetupCommit.HashHex};
    }

    if (Payload.bHasSetupPlain)
    {
        FSetupPlain SetupPlain{};
        SetupPlain.Side = static_cast<ESide>(Payload.SetupPlain.Side);
        SetupPlain.Nonce = Payload.SetupPlain.Nonce;
        SetupPlain.Placements.reserve(Payload.SetupPlain.Placements.size());
        for (const FProtocolSetupPlacementPayload& Placement : Payload.SetupPlain.Placements)
        {
            SetupPlain.Placements.push_back(FSetupPlacement{
                Placement.PieceId,
                FBoardPos{
                    static_cast<int8_t>(Placement.X),
                    static_cast<int8_t>(Placement.Y)}});
        }
        OutCommand.SetupPlain = std::move(SetupPlain);
    }

    return true;
}
