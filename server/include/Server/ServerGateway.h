#pragma once

#include "Protocol/ProtocolTypes.h"
#include "Server/TransportAdapter.h"

#include <string>

class FServerGateway
{
public:
    explicit FServerGateway(FServerTransportAdapter* InTransportAdapter);

    bool ProcessEnvelope(const FProtocolEnvelope& Envelope);
    bool ProcessEnvelopeJson(const std::string& EnvelopeJson);

private:
    bool BuildPlayerCommand(const FProtocolCommandPayload& Payload, FPlayerCommand& OutCommand) const;

private:
    FServerTransportAdapter* TransportAdapter = nullptr;
};
