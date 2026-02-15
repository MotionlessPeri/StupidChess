#include "Server/MatchSession.h"

FInMemoryMatchSession::FInMemoryMatchSession(FMatchId InMatchId)
    : MatchId(InMatchId)
{
}

FMatchJoinResponse FInMemoryMatchSession::Join(const FMatchJoinRequest& Request)
{
    const auto ExistingIt = PlayerSides.find(Request.PlayerId);
    if (ExistingIt != PlayerSides.end())
    {
        return {true, ExistingIt->second, {}};
    }

    if (PlayerSides.size() >= 2)
    {
        return {false, ESide::Red, "Match already has two players."};
    }

    const ESide AssignedSide = PlayerSides.empty() ? ESide::Red : ESide::Black;
    PlayerSides.emplace(Request.PlayerId, AssignedSide);

    return {true, AssignedSide, {}};
}

FCommandResult FInMemoryMatchSession::SubmitCommand(FPlayerId PlayerId, const FPlayerCommand& Command)
{
    const std::optional<ESide> PlayerSide = GetPlayerSide(PlayerId);
    if (!PlayerSide.has_value())
    {
        return {false, "ERR_PLAYER_NOT_IN_MATCH", "Player has not joined this match."};
    }

    FPlayerCommand NormalizedCommand = Command;
    NormalizedCommand.Side = PlayerSide.value();

    switch (NormalizedCommand.CommandType)
    {
    case ECommandType::CommitSetup:
        if (NormalizedCommand.SetupCommit.has_value())
        {
            NormalizedCommand.SetupCommit->Side = PlayerSide.value();
            return MatchReferee.ApplyCommit(NormalizedCommand.SetupCommit.value());
        }
        return {false, "ERR_INVALID_PAYLOAD", "Commit command missing setup commit payload."};

    case ECommandType::RevealSetup:
        if (NormalizedCommand.SetupPlain.has_value())
        {
            NormalizedCommand.SetupPlain->Side = PlayerSide.value();
            return MatchReferee.ApplyReveal(NormalizedCommand.SetupPlain.value());
        }
        return {false, "ERR_INVALID_PAYLOAD", "Reveal command missing setup plain payload."};

    default:
        return MatchReferee.ApplyCommand(NormalizedCommand);
    }
}

const FGameState& FInMemoryMatchSession::GetState() const noexcept
{
    return MatchReferee.GetState();
}

std::optional<ESide> FInMemoryMatchSession::GetPlayerSide(FPlayerId PlayerId) const
{
    const auto It = PlayerSides.find(PlayerId);
    if (It == PlayerSides.end())
    {
        return std::nullopt;
    }

    return It->second;
}
