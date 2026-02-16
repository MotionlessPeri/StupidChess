#pragma once

#include "Server/MatchSession.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

struct FMatchSyncResponse
{
    bool bAccepted = false;
    std::string ErrorCode;
    std::string ErrorMessage;
    FMatchId MatchId = 0;
    uint64_t RequestedAfterSequence = 0;
    uint64_t LatestSequence = 0;
    FMatchPlayerView View{};
    std::vector<FMatchEventRecord> Events;
};

class FInMemoryMatchService
{
public:
    FMatchJoinResponse JoinMatch(const FMatchJoinRequest& Request);
    FCommandResult SubmitPlayerCommand(FPlayerId PlayerId, const FPlayerCommand& Command);
    FMatchSyncResponse PullPlayerSync(FPlayerId PlayerId, std::optional<uint64_t> AfterSequenceOverride = std::nullopt) const;
    bool AckPlayerEvents(FPlayerId PlayerId, uint64_t Sequence);

    std::optional<FMatchId> FindPlayerMatch(FPlayerId PlayerId) const;
    std::optional<uint64_t> GetPlayerAckSequence(FPlayerId PlayerId) const;
    std::vector<FPlayerId> GetPlayersInMatch(FMatchId MatchId) const;
    size_t GetActiveMatchCount() const noexcept;

private:
    struct FPlayerBinding
    {
        FMatchId MatchId = 0;
        ESide Side = ESide::Red;
        uint64_t LastAckedSequence = 0;
    };

private:
    FInMemoryMatchSession* FindMutableSession(FMatchId MatchId);
    const FInMemoryMatchSession* FindSession(FMatchId MatchId) const;

private:
    std::unordered_map<FMatchId, std::unique_ptr<FInMemoryMatchSession>> Sessions;
    std::unordered_map<FPlayerId, FPlayerBinding> PlayerBindings;
};
