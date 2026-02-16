#pragma once

#include "Protocol/ProtocolTypes.h"
#include "Server/MatchService.h"

struct FProtocolSyncBundle
{
    FProtocolSnapshotPayload Snapshot{};
    FProtocolEventDeltaPayload EventDelta{};
};

class FProtocolMapper
{
public:
    static FProtocolJoinAckPayload BuildJoinAckPayload(const FMatchJoinResponse& JoinResponse);
    static FProtocolCommandAckPayload BuildCommandAckPayload(const FCommandResult& CommandResult);
    static FProtocolSnapshotPayload BuildSnapshotPayload(const FMatchPlayerView& View, uint64_t LastEventSequence);
    static FProtocolEventDeltaPayload BuildEventDeltaPayload(const FMatchSyncResponse& SyncResponse);
    static FProtocolSyncBundle BuildSyncBundle(const FMatchSyncResponse& SyncResponse);
};
