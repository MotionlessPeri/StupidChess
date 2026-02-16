#include "Server/TransportAdapter.h"

#include <iostream>

int main()
{
    std::cout << "StupidChessServer skeleton booting..." << std::endl;

    FInMemoryMatchService Service;
    FInMemoryServerMessageSink Sink;
    FServerTransportAdapter Adapter(&Service, &Sink);

    const bool bRedJoined = Adapter.HandleJoinRequest({1, 1001});
    const bool bBlackJoined = Adapter.HandleJoinRequest({1, 1002});
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
