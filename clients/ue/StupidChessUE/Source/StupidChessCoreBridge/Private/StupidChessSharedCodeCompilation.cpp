// Compile shared StupidChess modules inside UE bridge module.
// This keeps core rules/platform logic in one place while enabling UE usage.
#include "../../../../../../core/src/MatchReferee.cpp"
#include "../../../../../../protocol/src/ProtocolTypes.cpp"
#include "../../../../../../protocol/src/ProtocolCodec.cpp"
#include "../../../../../../server/src/MatchSession.cpp"
#include "../../../../../../server/src/MatchService.cpp"
#include "../../../../../../server/src/ProtocolMapper.cpp"
#include "../../../../../../server/src/TransportAdapter.cpp"
#include "../../../../../../server/src/ServerGateway.cpp"
