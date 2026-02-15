#pragma once

#include <cstdint>
#include <string>

enum class EProtocolMessageType : uint16_t
{
    C2S_Join = 100,
    C2S_Command = 101,
    C2S_Ping = 102,

    S2C_JoinAck = 200,
    S2C_CommandAck = 201,
    S2C_Snapshot = 202,
    S2C_EventDelta = 203,
    S2C_GameOver = 204,
    S2C_Error = 205
};

struct FProtocolEnvelope
{
    EProtocolMessageType MessageType = EProtocolMessageType::C2S_Ping;
    uint64_t Sequence = 0;
    std::string MatchId;
    std::string PayloadJson;
};
