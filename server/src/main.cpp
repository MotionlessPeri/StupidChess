#include "Server/MatchService.h"
#include "Server/ProtocolMapper.h"

#include <iostream>

int main()
{
    std::cout << "StupidChessServer skeleton booting..." << std::endl;

    FInMemoryMatchService Service;

    const FMatchJoinResponse RedJoin = Service.JoinMatch({1, 1001});
    const FMatchJoinResponse BlackJoin = Service.JoinMatch({1, 1002});
    if (!RedJoin.bAccepted || !BlackJoin.bAccepted)
    {
        std::cerr << "Failed to build a two-player session." << std::endl;
        return 1;
    }

    const FMatchSyncResponse RedSync = Service.PullPlayerSync(1001);
    const FProtocolSyncBundle Bundle = FProtocolMapper::BuildSyncBundle(RedSync);
    std::cout << "Session initialized with two players. Current phase="
              << Bundle.Snapshot.Phase
              << " events=" << Bundle.EventDelta.Events.size() << std::endl;

    return 0;
}
