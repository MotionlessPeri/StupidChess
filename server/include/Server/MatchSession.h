#pragma once

#include "CoreRules/MatchReferee.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using FMatchId = uint64_t;
using FPlayerId = uint64_t;

struct FMatchJoinRequest
{
    FMatchId MatchId = 0;
    FPlayerId PlayerId = 0;
};

struct FMatchJoinResponse
{
    bool bAccepted = false;
    ESide AssignedSide = ESide::Red;
    std::string ErrorMessage;
};

class FInMemoryMatchSession
{
public:
    explicit FInMemoryMatchSession(FMatchId InMatchId);

    FMatchJoinResponse Join(const FMatchJoinRequest& Request);
    FCommandResult SubmitCommand(FPlayerId PlayerId, const FPlayerCommand& Command);

    const FGameState& GetState() const noexcept;

private:
    std::optional<ESide> GetPlayerSide(FPlayerId PlayerId) const;

private:
    FMatchId MatchId = 0;
    FMatchReferee MatchReferee;
    std::unordered_map<FPlayerId, ESide> PlayerSides;
};
