#pragma once

#include "Protocol/ProtocolTypes.h"

#include <optional>
#include <string>

namespace ProtocolCodec
{
bool EncodeEnvelope(const FProtocolEnvelope& Envelope, std::string& OutJson);
bool DecodeEnvelope(const std::string& Json, FProtocolEnvelope& OutEnvelope);

bool EncodeJoinPayload(const FProtocolJoinPayload& Payload, std::string& OutJson);
bool DecodeJoinPayload(const std::string& Json, FProtocolJoinPayload& OutPayload);

bool EncodeCommandPayload(const FProtocolCommandPayload& Payload, std::string& OutJson);
bool DecodeCommandPayload(const std::string& Json, FProtocolCommandPayload& OutPayload);

bool EncodePullSyncPayload(const FProtocolPullSyncPayload& Payload, std::string& OutJson);
bool DecodePullSyncPayload(const std::string& Json, FProtocolPullSyncPayload& OutPayload);

bool EncodeAckPayload(const FProtocolAckPayload& Payload, std::string& OutJson);
bool DecodeAckPayload(const std::string& Json, FProtocolAckPayload& OutPayload);

bool EncodeJoinAckPayload(const FProtocolJoinAckPayload& Payload, std::string& OutJson);
bool DecodeJoinAckPayload(const std::string& Json, FProtocolJoinAckPayload& OutPayload);

bool EncodeCommandAckPayload(const FProtocolCommandAckPayload& Payload, std::string& OutJson);
bool DecodeCommandAckPayload(const std::string& Json, FProtocolCommandAckPayload& OutPayload);

bool EncodeSnapshotPayload(const FProtocolSnapshotPayload& Payload, std::string& OutJson);
bool DecodeSnapshotPayload(const std::string& Json, FProtocolSnapshotPayload& OutPayload);

bool EncodeEventDeltaPayload(const FProtocolEventDeltaPayload& Payload, std::string& OutJson);
bool DecodeEventDeltaPayload(const std::string& Json, FProtocolEventDeltaPayload& OutPayload);

bool EncodeGameOverPayload(const FProtocolGameOverPayload& Payload, std::string& OutJson);
bool DecodeGameOverPayload(const std::string& Json, FProtocolGameOverPayload& OutPayload);

bool EncodeErrorPayload(const FProtocolErrorPayload& Payload, std::string& OutJson);
bool DecodeErrorPayload(const std::string& Json, FProtocolErrorPayload& OutPayload);
}
