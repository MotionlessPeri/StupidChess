#include "Server/MatchSession.h"

#include <iostream>

int main()
{
    std::cout << "StupidChessServer skeleton booting..." << std::endl;

    FInMemoryMatchSession Session(1);

    const FMatchJoinResponse RedJoin = Session.Join({1, 1001});
    const FMatchJoinResponse BlackJoin = Session.Join({1, 1002});

    if (!RedJoin.bAccepted || !BlackJoin.bAccepted)
    {
        std::cerr << "Failed to build a two-player session." << std::endl;
        return 1;
    }

    std::cout << "Session initialized with two players. Current phase="
              << static_cast<int>(Session.GetState().Phase) << std::endl;

    return 0;
}
