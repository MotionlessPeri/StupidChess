#include "Protocol/ProtocolCodec.h"
#include "Server/ServerGateway.h"
#include "Server/TransportAdapter.h"

#include <iostream>

int main()
{
    std::cout << "StupidChessServer skeleton booting..." << std::endl;

    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);
    FServerGateway Gateway(&Adapter);

    std::string RedJoinJson;
    ProtocolCodec::EncodeJoinPayload({1, 1001}, RedJoinJson);
    const bool bRedJoined = Gateway.ProcessEnvelope(FProtocolEnvelope{
        EProtocolMessageType::C2S_Join,
        1,
        "1",
        RedJoinJson});

    std::string BlackJoinJson;
    ProtocolCodec::EncodeJoinPayload({1, 1002}, BlackJoinJson);
    const bool bBlackJoined = Gateway.ProcessEnvelope(FProtocolEnvelope{
        EProtocolMessageType::C2S_Join,
        2,
        "1",
        BlackJoinJson});
    if (!bRedJoined || !bBlackJoined)
    {
        std::cerr << "Failed to build a two-player session." << std::endl;
        return 1;
    }

    const std::vector<FOutboundProtocolMessage> RedMessages = Sink.PullMessages(1001);
    std::cout << "Session initialized with two players. Outbound messages to red="
              << RedMessages.size() << std::endl;

    return 0;
}
