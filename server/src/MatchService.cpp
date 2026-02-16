#include "Server/MatchService.h"

#include <algorithm>

FMatchJoinResponse FInMemoryMatchService::JoinMatch(const FMatchJoinRequest& Request)
{
    if (Request.MatchId == 0 || Request.PlayerId == 0)
    {
        return {false, ESide::Red, "MatchId and PlayerId must be non-zero."};
    }

    const auto ExistingBinding = PlayerBindings.find(Request.PlayerId);
    if (ExistingBinding != PlayerBindings.end())
    {
        if (ExistingBinding->second.MatchId != Request.MatchId)
        {
            return {false, ESide::Red, "Player is already bound to another match."};
        }

        return {true, ExistingBinding->second.Side, {}};
    }

    FInMemoryMatchSession* Session = FindMutableSession(Request.MatchId);
    if (Session == nullptr)
    {
        auto NewSession = std::make_unique<FInMemoryMatchSession>(Request.MatchId);
        Session = NewSession.get();
        Sessions.emplace(Request.MatchId, std::move(NewSession));
    }

    const FMatchJoinResponse JoinResult = Session->Join(Request);
    if (JoinResult.bAccepted)
    {
        PlayerBindings.emplace(Request.PlayerId, FPlayerBinding{
                                                  Request.MatchId,
                                                  JoinResult.AssignedSide,
                                                  0});
    }

    return JoinResult;
}

FCommandResult FInMemoryMatchService::SubmitPlayerCommand(FPlayerId PlayerId, const FPlayerCommand& Command)
{
    const auto BindingIt = PlayerBindings.find(PlayerId);
    if (BindingIt == PlayerBindings.end())
    {
        return {false, "ERR_PLAYER_NOT_BOUND", "Player is not bound to any match."};
    }

    FInMemoryMatchSession* Session = FindMutableSession(BindingIt->second.MatchId);
    if (Session == nullptr)
    {
        return {false, "ERR_MATCH_NOT_FOUND", "Bound match does not exist."};
    }

    return Session->SubmitCommand(PlayerId, Command);
}

FMatchSyncResponse FInMemoryMatchService::PullPlayerSync(FPlayerId PlayerId, std::optional<uint64_t> AfterSequenceOverride) const
{
    const auto BindingIt = PlayerBindings.find(PlayerId);
    if (BindingIt == PlayerBindings.end())
    {
        return {false, "ERR_PLAYER_NOT_BOUND", "Player is not bound to any match."};
    }

    const FPlayerBinding& Binding = BindingIt->second;
    const FInMemoryMatchSession* Session = FindSession(Binding.MatchId);
    if (Session == nullptr)
    {
        return {false, "ERR_MATCH_NOT_FOUND", "Bound match does not exist."};
    }

    const uint64_t RequestedAfterSequence = AfterSequenceOverride.value_or(Binding.LastAckedSequence);
    FMatchSyncResponse Response{};
    Response.bAccepted = true;
    Response.MatchId = Binding.MatchId;
    Response.RequestedAfterSequence = RequestedAfterSequence;
    Response.LatestSequence = Session->GetLatestEventSequence();
    Response.View = Session->GetPlayerView(PlayerId);
    Response.Events = Session->PullEvents(PlayerId, RequestedAfterSequence);
    return Response;
}

bool FInMemoryMatchService::AckPlayerEvents(FPlayerId PlayerId, uint64_t Sequence)
{
    auto BindingIt = PlayerBindings.find(PlayerId);
    if (BindingIt == PlayerBindings.end())
    {
        return false;
    }

    FPlayerBinding& Binding = BindingIt->second;
    if (Sequence < Binding.LastAckedSequence)
    {
        return false;
    }

    const FInMemoryMatchSession* Session = FindSession(Binding.MatchId);
    if (Session == nullptr)
    {
        return false;
    }

    if (Sequence > Session->GetLatestEventSequence())
    {
        return false;
    }

    Binding.LastAckedSequence = Sequence;
    return true;
}

std::optional<FMatchId> FInMemoryMatchService::FindPlayerMatch(FPlayerId PlayerId) const
{
    const auto It = PlayerBindings.find(PlayerId);
    if (It == PlayerBindings.end())
    {
        return std::nullopt;
    }

    return It->second.MatchId;
}

std::optional<uint64_t> FInMemoryMatchService::GetPlayerAckSequence(FPlayerId PlayerId) const
{
    const auto It = PlayerBindings.find(PlayerId);
    if (It == PlayerBindings.end())
    {
        return std::nullopt;
    }

    return It->second.LastAckedSequence;
}

std::vector<FPlayerId> FInMemoryMatchService::GetPlayersInMatch(FMatchId MatchId) const
{
    std::vector<FPlayerId> Players;
    for (const auto& Entry : PlayerBindings)
    {
        if (Entry.second.MatchId == MatchId)
        {
            Players.push_back(Entry.first);
        }
    }

    std::sort(Players.begin(), Players.end());
    return Players;
}

size_t FInMemoryMatchService::GetActiveMatchCount() const noexcept
{
    return Sessions.size();
}

FInMemoryMatchSession* FInMemoryMatchService::FindMutableSession(FMatchId MatchId)
{
    const auto It = Sessions.find(MatchId);
    return It == Sessions.end() ? nullptr : It->second.get();
}

const FInMemoryMatchSession* FInMemoryMatchService::FindSession(FMatchId MatchId) const
{
    const auto It = Sessions.find(MatchId);
    return It == Sessions.end() ? nullptr : It->second.get();
}
