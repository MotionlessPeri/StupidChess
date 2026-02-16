#include "Protocol/ProtocolCodec.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
enum class EJsonType : uint8_t
{
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

struct FProtocolJsonValue
{
    EJsonType Type = EJsonType::Null;
    bool BoolValue = false;
    int64_t NumberValue = 0;
    std::string StringValue;
    std::vector<FProtocolJsonValue> ArrayValue;
    std::unordered_map<std::string, FProtocolJsonValue> ObjectValue;
};

class FJsonParser
{
public:
    explicit FJsonParser(std::string_view InText)
        : Text(InText)
    {
    }

    bool Parse(FProtocolJsonValue& OutValue)
    {
        SkipWhitespace();
        if (!ParseValue(OutValue))
        {
            return false;
        }

        SkipWhitespace();
        return Index == Text.size();
    }

private:
    bool ParseValue(FProtocolJsonValue& OutValue)
    {
        if (Index >= Text.size())
        {
            return false;
        }

        const char Ch = Text[Index];
        if (Ch == '{')
        {
            return ParseObject(OutValue);
        }
        if (Ch == '[')
        {
            return ParseArray(OutValue);
        }
        if (Ch == '"')
        {
            OutValue.Type = EJsonType::String;
            return ParseString(OutValue.StringValue);
        }
        if (Ch == '-' || std::isdigit(static_cast<unsigned char>(Ch)))
        {
            OutValue.Type = EJsonType::Number;
            return ParseNumber(OutValue.NumberValue);
        }
        if (TryConsumeKeyword("true"))
        {
            OutValue.Type = EJsonType::Bool;
            OutValue.BoolValue = true;
            return true;
        }
        if (TryConsumeKeyword("false"))
        {
            OutValue.Type = EJsonType::Bool;
            OutValue.BoolValue = false;
            return true;
        }
        if (TryConsumeKeyword("null"))
        {
            OutValue.Type = EJsonType::Null;
            return true;
        }

        return false;
    }

    bool ParseObject(FProtocolJsonValue& OutValue)
    {
        if (!Consume('{'))
        {
            return false;
        }

        OutValue.Type = EJsonType::Object;
        OutValue.ObjectValue.clear();

        SkipWhitespace();
        if (Consume('}'))
        {
            return true;
        }

        while (true)
        {
            std::string Key;
            if (!ParseString(Key))
            {
                return false;
            }

            SkipWhitespace();
            if (!Consume(':'))
            {
                return false;
            }

            FProtocolJsonValue Value;
            SkipWhitespace();
            if (!ParseValue(Value))
            {
                return false;
            }

            OutValue.ObjectValue.emplace(std::move(Key), std::move(Value));

            SkipWhitespace();
            if (Consume('}'))
            {
                return true;
            }
            if (!Consume(','))
            {
                return false;
            }
            SkipWhitespace();
        }
    }

    bool ParseArray(FProtocolJsonValue& OutValue)
    {
        if (!Consume('['))
        {
            return false;
        }

        OutValue.Type = EJsonType::Array;
        OutValue.ArrayValue.clear();

        SkipWhitespace();
        if (Consume(']'))
        {
            return true;
        }

        while (true)
        {
            FProtocolJsonValue Element;
            if (!ParseValue(Element))
            {
                return false;
            }
            OutValue.ArrayValue.push_back(std::move(Element));

            SkipWhitespace();
            if (Consume(']'))
            {
                return true;
            }
            if (!Consume(','))
            {
                return false;
            }
            SkipWhitespace();
        }
    }

    bool ParseString(std::string& OutString)
    {
        if (!Consume('"'))
        {
            return false;
        }

        OutString.clear();
        while (Index < Text.size())
        {
            const char Ch = Text[Index++];
            if (Ch == '"')
            {
                return true;
            }
            if (Ch == '\\')
            {
                if (Index >= Text.size())
                {
                    return false;
                }

                const char Escaped = Text[Index++];
                switch (Escaped)
                {
                case '"':
                case '\\':
                case '/':
                    OutString.push_back(Escaped);
                    break;
                case 'b':
                    OutString.push_back('\b');
                    break;
                case 'f':
                    OutString.push_back('\f');
                    break;
                case 'n':
                    OutString.push_back('\n');
                    break;
                case 'r':
                    OutString.push_back('\r');
                    break;
                case 't':
                    OutString.push_back('\t');
                    break;
                default:
                    return false;
                }
            }
            else
            {
                OutString.push_back(Ch);
            }
        }

        return false;
    }

    bool ParseNumber(int64_t& OutNumber)
    {
        const size_t Start = Index;
        if (Text[Index] == '-')
        {
            ++Index;
        }

        if (Index >= Text.size() || !std::isdigit(static_cast<unsigned char>(Text[Index])))
        {
            return false;
        }

        while (Index < Text.size() && std::isdigit(static_cast<unsigned char>(Text[Index])))
        {
            ++Index;
        }

        const std::string NumberText(Text.substr(Start, Index - Start));
        char* EndPtr = nullptr;
        const long long Parsed = std::strtoll(NumberText.c_str(), &EndPtr, 10);
        if (EndPtr == nullptr || *EndPtr != '\0')
        {
            return false;
        }

        OutNumber = static_cast<int64_t>(Parsed);
        return true;
    }

    bool TryConsumeKeyword(std::string_view Keyword)
    {
        if (Text.substr(Index, Keyword.size()) == Keyword)
        {
            Index += Keyword.size();
            return true;
        }
        return false;
    }

    bool Consume(char Ch)
    {
        if (Index < Text.size() && Text[Index] == Ch)
        {
            ++Index;
            return true;
        }
        return false;
    }

    void SkipWhitespace()
    {
        while (Index < Text.size() && std::isspace(static_cast<unsigned char>(Text[Index])))
        {
            ++Index;
        }
    }

private:
    std::string_view Text;
    size_t Index = 0;
};

std::string EscapeJson(std::string_view Text)
{
    std::string Result;
    Result.reserve(Text.size() + 8);
    for (const char Ch : Text)
    {
        switch (Ch)
        {
        case '"':
        case '\\':
            Result.push_back('\\');
            Result.push_back(Ch);
            break;
        case '\n':
            Result += "\\n";
            break;
        case '\r':
            Result += "\\r";
            break;
        case '\t':
            Result += "\\t";
            break;
        default:
            Result.push_back(Ch);
            break;
        }
    }
    return Result;
}

void AppendQuoted(std::ostringstream& Stream, std::string_view Text)
{
    Stream << '"' << EscapeJson(Text) << '"';
}

bool ParseJsonRootObject(const std::string& Json, FProtocolJsonValue& OutRoot)
{
    FJsonParser Parser(Json);
    if (!Parser.Parse(OutRoot))
    {
        return false;
    }

    return OutRoot.Type == EJsonType::Object;
}

const FProtocolJsonValue* FindField(const FProtocolJsonValue& ObjectValue, const char* FieldName)
{
    if (ObjectValue.Type != EJsonType::Object)
    {
        return nullptr;
    }

    const auto It = ObjectValue.ObjectValue.find(FieldName);
    if (It == ObjectValue.ObjectValue.end())
    {
        return nullptr;
    }

    return &It->second;
}

bool ReadInt(const FProtocolJsonValue& ObjectValue, const char* FieldName, int64_t& OutValue)
{
    const FProtocolJsonValue* FieldValue = FindField(ObjectValue, FieldName);
    if (FieldValue == nullptr || FieldValue->Type != EJsonType::Number)
    {
        return false;
    }

    OutValue = FieldValue->NumberValue;
    return true;
}

bool ReadBool(const FProtocolJsonValue& ObjectValue, const char* FieldName, bool& OutValue)
{
    const FProtocolJsonValue* FieldValue = FindField(ObjectValue, FieldName);
    if (FieldValue == nullptr || FieldValue->Type != EJsonType::Bool)
    {
        return false;
    }

    OutValue = FieldValue->BoolValue;
    return true;
}

bool ReadString(const FProtocolJsonValue& ObjectValue, const char* FieldName, std::string& OutValue)
{
    const FProtocolJsonValue* FieldValue = FindField(ObjectValue, FieldName);
    if (FieldValue == nullptr || FieldValue->Type != EJsonType::String)
    {
        return false;
    }

    OutValue = FieldValue->StringValue;
    return true;
}

bool ReadObject(const FProtocolJsonValue& ObjectValue, const char* FieldName, const FProtocolJsonValue*& OutObject)
{
    const FProtocolJsonValue* FieldValue = FindField(ObjectValue, FieldName);
    if (FieldValue == nullptr || FieldValue->Type != EJsonType::Object)
    {
        return false;
    }

    OutObject = FieldValue;
    return true;
}

bool ReadArray(const FProtocolJsonValue& ObjectValue, const char* FieldName, const FProtocolJsonValue*& OutArray)
{
    const FProtocolJsonValue* FieldValue = FindField(ObjectValue, FieldName);
    if (FieldValue == nullptr || FieldValue->Type != EJsonType::Array)
    {
        return false;
    }

    OutArray = FieldValue;
    return true;
}

}

namespace ProtocolCodec
{
bool EncodeEnvelope(const FProtocolEnvelope& Envelope, std::string& OutJson)
{
    std::ostringstream Stream;
    Stream << "{\"messageType\":" << static_cast<int32_t>(Envelope.MessageType)
           << ",\"sequence\":" << Envelope.Sequence
           << ",\"matchId\":";
    AppendQuoted(Stream, Envelope.MatchId);
    Stream << ",\"payloadJson\":";
    AppendQuoted(Stream, Envelope.PayloadJson);
    Stream << '}';
    OutJson = Stream.str();
    return true;
}

bool DecodeEnvelope(const std::string& Json, FProtocolEnvelope& OutEnvelope)
{
    FProtocolJsonValue Root;
    if (!ParseJsonRootObject(Json, Root))
    {
        return false;
    }

    int64_t MessageTypeValue = 0;
    int64_t SequenceValue = 0;
    if (!ReadInt(Root, "messageType", MessageTypeValue) || !ReadInt(Root, "sequence", SequenceValue))
    {
        return false;
    }
    if (!ReadString(Root, "matchId", OutEnvelope.MatchId) || !ReadString(Root, "payloadJson", OutEnvelope.PayloadJson))
    {
        return false;
    }

    OutEnvelope.MessageType = static_cast<EProtocolMessageType>(MessageTypeValue);
    OutEnvelope.Sequence = static_cast<uint64_t>(SequenceValue);
    return true;
}

bool EncodeJoinPayload(const FProtocolJoinPayload& Payload, std::string& OutJson)
{
    std::ostringstream Stream;
    Stream << "{\"matchId\":" << Payload.MatchId
           << ",\"playerId\":" << Payload.PlayerId
           << '}';
    OutJson = Stream.str();
    return true;
}

bool DecodeJoinPayload(const std::string& Json, FProtocolJoinPayload& OutPayload)
{
    FProtocolJsonValue Root;
    if (!ParseJsonRootObject(Json, Root))
    {
        return false;
    }

    int64_t MatchIdValue = 0;
    int64_t PlayerIdValue = 0;
    if (!ReadInt(Root, "matchId", MatchIdValue) || !ReadInt(Root, "playerId", PlayerIdValue))
    {
        return false;
    }

    OutPayload.MatchId = static_cast<uint64_t>(MatchIdValue);
    OutPayload.PlayerId = static_cast<uint64_t>(PlayerIdValue);
    return true;
}

bool EncodeCommandPayload(const FProtocolCommandPayload& Payload, std::string& OutJson)
{
    std::ostringstream Stream;
    Stream << "{\"playerId\":" << Payload.PlayerId
           << ",\"commandType\":" << Payload.CommandType
           << ",\"side\":" << Payload.Side
           << ",\"hasMove\":" << (Payload.bHasMove ? "true" : "false")
           << ",\"hasSetupCommit\":" << (Payload.bHasSetupCommit ? "true" : "false")
           << ",\"hasSetupPlain\":" << (Payload.bHasSetupPlain ? "true" : "false");

    if (Payload.bHasMove)
    {
        Stream << ",\"move\":{\"pieceId\":" << Payload.Move.PieceId
               << ",\"fromX\":" << Payload.Move.FromX
               << ",\"fromY\":" << Payload.Move.FromY
               << ",\"toX\":" << Payload.Move.ToX
               << ",\"toY\":" << Payload.Move.ToY
               << ",\"hasCapturedPieceId\":" << (Payload.Move.bHasCapturedPieceId ? "true" : "false")
               << ",\"capturedPieceId\":" << Payload.Move.CapturedPieceId
               << '}';
    }

    if (Payload.bHasSetupCommit)
    {
        Stream << ",\"setupCommit\":{\"side\":" << Payload.SetupCommit.Side
               << ",\"hashHex\":";
        AppendQuoted(Stream, Payload.SetupCommit.HashHex);
        Stream << '}';
    }

    if (Payload.bHasSetupPlain)
    {
        Stream << ",\"setupPlain\":{\"side\":" << Payload.SetupPlain.Side
               << ",\"nonce\":";
        AppendQuoted(Stream, Payload.SetupPlain.Nonce);
        Stream << ",\"placements\":[";
        for (size_t Index = 0; Index < Payload.SetupPlain.Placements.size(); ++Index)
        {
            if (Index > 0)
            {
                Stream << ',';
            }
            const FProtocolSetupPlacementPayload& Placement = Payload.SetupPlain.Placements[Index];
            Stream << "{\"pieceId\":" << Placement.PieceId
                   << ",\"x\":" << Placement.X
                   << ",\"y\":" << Placement.Y
                   << '}';
        }
        Stream << "]}";
    }

    Stream << '}';
    OutJson = Stream.str();
    return true;
}

bool DecodeCommandPayload(const std::string& Json, FProtocolCommandPayload& OutPayload)
{
    FProtocolJsonValue Root;
    if (!ParseJsonRootObject(Json, Root))
    {
        return false;
    }

    int64_t PlayerIdValue = 0;
    int64_t CommandTypeValue = 0;
    int64_t SideValue = 0;
    if (!ReadInt(Root, "playerId", PlayerIdValue) ||
        !ReadInt(Root, "commandType", CommandTypeValue) ||
        !ReadInt(Root, "side", SideValue))
    {
        return false;
    }

    OutPayload.PlayerId = static_cast<uint64_t>(PlayerIdValue);
    OutPayload.CommandType = static_cast<int32_t>(CommandTypeValue);
    OutPayload.Side = static_cast<int32_t>(SideValue);

    if (!ReadBool(Root, "hasMove", OutPayload.bHasMove) ||
        !ReadBool(Root, "hasSetupCommit", OutPayload.bHasSetupCommit) ||
        !ReadBool(Root, "hasSetupPlain", OutPayload.bHasSetupPlain))
    {
        return false;
    }

    if (OutPayload.bHasMove)
    {
        const FProtocolJsonValue* MoveObject = nullptr;
        if (!ReadObject(Root, "move", MoveObject))
        {
            return false;
        }

        int64_t PieceIdValue = 0;
        int64_t FromX = 0;
        int64_t FromY = 0;
        int64_t ToX = 0;
        int64_t ToY = 0;
        int64_t CapturedPieceIdValue = 0;
        if (!ReadInt(*MoveObject, "pieceId", PieceIdValue) ||
            !ReadInt(*MoveObject, "fromX", FromX) ||
            !ReadInt(*MoveObject, "fromY", FromY) ||
            !ReadInt(*MoveObject, "toX", ToX) ||
            !ReadInt(*MoveObject, "toY", ToY) ||
            !ReadBool(*MoveObject, "hasCapturedPieceId", OutPayload.Move.bHasCapturedPieceId) ||
            !ReadInt(*MoveObject, "capturedPieceId", CapturedPieceIdValue))
        {
            return false;
        }

        OutPayload.Move.PieceId = static_cast<uint16_t>(PieceIdValue);
        OutPayload.Move.FromX = static_cast<int32_t>(FromX);
        OutPayload.Move.FromY = static_cast<int32_t>(FromY);
        OutPayload.Move.ToX = static_cast<int32_t>(ToX);
        OutPayload.Move.ToY = static_cast<int32_t>(ToY);
        OutPayload.Move.CapturedPieceId = static_cast<uint16_t>(CapturedPieceIdValue);
    }

    if (OutPayload.bHasSetupCommit)
    {
        const FProtocolJsonValue* SetupCommitObject = nullptr;
        if (!ReadObject(Root, "setupCommit", SetupCommitObject))
        {
            return false;
        }

        int64_t CommitSide = 0;
        if (!ReadInt(*SetupCommitObject, "side", CommitSide) ||
            !ReadString(*SetupCommitObject, "hashHex", OutPayload.SetupCommit.HashHex))
        {
            return false;
        }

        OutPayload.SetupCommit.Side = static_cast<int32_t>(CommitSide);
    }

    if (OutPayload.bHasSetupPlain)
    {
        const FProtocolJsonValue* SetupPlainObject = nullptr;
        if (!ReadObject(Root, "setupPlain", SetupPlainObject))
        {
            return false;
        }

        int64_t SetupPlainSide = 0;
        const FProtocolJsonValue* PlacementsArray = nullptr;
        if (!ReadInt(*SetupPlainObject, "side", SetupPlainSide) ||
            !ReadString(*SetupPlainObject, "nonce", OutPayload.SetupPlain.Nonce) ||
            !ReadArray(*SetupPlainObject, "placements", PlacementsArray))
        {
            return false;
        }

        OutPayload.SetupPlain.Side = static_cast<int32_t>(SetupPlainSide);
        OutPayload.SetupPlain.Placements.clear();
        OutPayload.SetupPlain.Placements.reserve(PlacementsArray->ArrayValue.size());
        for (const FProtocolJsonValue& PlacementValue : PlacementsArray->ArrayValue)
        {
            int64_t PieceIdValue = 0;
            int64_t X = 0;
            int64_t Y = 0;
            if (!ReadInt(PlacementValue, "pieceId", PieceIdValue) ||
                !ReadInt(PlacementValue, "x", X) ||
                !ReadInt(PlacementValue, "y", Y))
            {
                return false;
            }

            OutPayload.SetupPlain.Placements.push_back(FProtocolSetupPlacementPayload{
                static_cast<uint16_t>(PieceIdValue),
                static_cast<int32_t>(X),
                static_cast<int32_t>(Y)});
        }
    }

    return true;
}

bool EncodePullSyncPayload(const FProtocolPullSyncPayload& Payload, std::string& OutJson)
{
    std::ostringstream Stream;
    Stream << "{\"playerId\":" << Payload.PlayerId
           << ",\"hasAfterSequenceOverride\":" << (Payload.bHasAfterSequenceOverride ? "true" : "false")
           << ",\"afterSequenceOverride\":" << Payload.AfterSequenceOverride
           << '}';
    OutJson = Stream.str();
    return true;
}

bool DecodePullSyncPayload(const std::string& Json, FProtocolPullSyncPayload& OutPayload)
{
    FProtocolJsonValue Root;
    if (!ParseJsonRootObject(Json, Root))
    {
        return false;
    }

    int64_t PlayerIdValue = 0;
    int64_t SequenceOverrideValue = 0;
    if (!ReadInt(Root, "playerId", PlayerIdValue) ||
        !ReadBool(Root, "hasAfterSequenceOverride", OutPayload.bHasAfterSequenceOverride) ||
        !ReadInt(Root, "afterSequenceOverride", SequenceOverrideValue))
    {
        return false;
    }

    OutPayload.PlayerId = static_cast<uint64_t>(PlayerIdValue);
    OutPayload.AfterSequenceOverride = static_cast<uint64_t>(SequenceOverrideValue);
    return true;
}

bool EncodeAckPayload(const FProtocolAckPayload& Payload, std::string& OutJson)
{
    std::ostringstream Stream;
    Stream << "{\"playerId\":" << Payload.PlayerId
           << ",\"sequence\":" << Payload.Sequence
           << '}';
    OutJson = Stream.str();
    return true;
}

bool DecodeAckPayload(const std::string& Json, FProtocolAckPayload& OutPayload)
{
    FProtocolJsonValue Root;
    if (!ParseJsonRootObject(Json, Root))
    {
        return false;
    }

    int64_t PlayerIdValue = 0;
    int64_t SequenceValue = 0;
    if (!ReadInt(Root, "playerId", PlayerIdValue) || !ReadInt(Root, "sequence", SequenceValue))
    {
        return false;
    }

    OutPayload.PlayerId = static_cast<uint64_t>(PlayerIdValue);
    OutPayload.Sequence = static_cast<uint64_t>(SequenceValue);
    return true;
}

bool EncodeJoinAckPayload(const FProtocolJoinAckPayload& Payload, std::string& OutJson)
{
    std::ostringstream Stream;
    Stream << "{\"accepted\":" << (Payload.bAccepted ? "true" : "false")
           << ",\"assignedSide\":" << Payload.AssignedSide
           << ",\"errorCode\":";
    AppendQuoted(Stream, Payload.ErrorCode);
    Stream << ",\"errorMessage\":";
    AppendQuoted(Stream, Payload.ErrorMessage);
    Stream << '}';
    OutJson = Stream.str();
    return true;
}

bool DecodeJoinAckPayload(const std::string& Json, FProtocolJoinAckPayload& OutPayload)
{
    FProtocolJsonValue Root;
    if (!ParseJsonRootObject(Json, Root))
    {
        return false;
    }

    int64_t AssignedSide = 0;
    if (!ReadBool(Root, "accepted", OutPayload.bAccepted) ||
        !ReadInt(Root, "assignedSide", AssignedSide) ||
        !ReadString(Root, "errorCode", OutPayload.ErrorCode) ||
        !ReadString(Root, "errorMessage", OutPayload.ErrorMessage))
    {
        return false;
    }

    OutPayload.AssignedSide = static_cast<int32_t>(AssignedSide);
    return true;
}

bool EncodeCommandAckPayload(const FProtocolCommandAckPayload& Payload, std::string& OutJson)
{
    std::ostringstream Stream;
    Stream << "{\"accepted\":" << (Payload.bAccepted ? "true" : "false")
           << ",\"errorCode\":";
    AppendQuoted(Stream, Payload.ErrorCode);
    Stream << ",\"errorMessage\":";
    AppendQuoted(Stream, Payload.ErrorMessage);
    Stream << '}';
    OutJson = Stream.str();
    return true;
}

bool DecodeCommandAckPayload(const std::string& Json, FProtocolCommandAckPayload& OutPayload)
{
    FProtocolJsonValue Root;
    if (!ParseJsonRootObject(Json, Root))
    {
        return false;
    }

    return ReadBool(Root, "accepted", OutPayload.bAccepted) &&
           ReadString(Root, "errorCode", OutPayload.ErrorCode) &&
           ReadString(Root, "errorMessage", OutPayload.ErrorMessage);
}

bool EncodeSnapshotPayload(const FProtocolSnapshotPayload& Payload, std::string& OutJson)
{
    std::ostringstream Stream;
    Stream << "{\"viewerSide\":" << Payload.ViewerSide
           << ",\"phase\":" << Payload.Phase
           << ",\"currentTurn\":" << Payload.CurrentTurn
           << ",\"passCount\":" << Payload.PassCount
           << ",\"result\":" << Payload.Result
           << ",\"endReason\":" << Payload.EndReason
           << ",\"turnIndex\":" << Payload.TurnIndex
           << ",\"lastEventSequence\":" << Payload.LastEventSequence
           << ",\"pieces\":[";
    for (size_t Index = 0; Index < Payload.Pieces.size(); ++Index)
    {
        if (Index > 0)
        {
            Stream << ',';
        }
        const FProtocolPieceSnapshot& Piece = Payload.Pieces[Index];
        Stream << "{\"pieceId\":" << Piece.PieceId
               << ",\"side\":" << Piece.Side
               << ",\"visibleRole\":" << Piece.VisibleRole
               << ",\"x\":" << Piece.X
               << ",\"y\":" << Piece.Y
               << ",\"alive\":" << (Piece.bAlive ? "true" : "false")
               << ",\"frozen\":" << (Piece.bFrozen ? "true" : "false")
               << ",\"revealed\":" << (Piece.bRevealed ? "true" : "false")
               << '}';
    }
    Stream << "]}";
    OutJson = Stream.str();
    return true;
}

bool DecodeSnapshotPayload(const std::string& Json, FProtocolSnapshotPayload& OutPayload)
{
    FProtocolJsonValue Root;
    if (!ParseJsonRootObject(Json, Root))
    {
        return false;
    }

    int64_t ViewerSide = 0;
    int64_t Phase = 0;
    int64_t CurrentTurn = 0;
    int64_t PassCount = 0;
    int64_t Result = 0;
    int64_t EndReason = 0;
    int64_t TurnIndex = 0;
    int64_t LastEventSequence = 0;
    const FProtocolJsonValue* PiecesArray = nullptr;
    if (!ReadInt(Root, "viewerSide", ViewerSide) ||
        !ReadInt(Root, "phase", Phase) ||
        !ReadInt(Root, "currentTurn", CurrentTurn) ||
        !ReadInt(Root, "passCount", PassCount) ||
        !ReadInt(Root, "result", Result) ||
        !ReadInt(Root, "endReason", EndReason) ||
        !ReadInt(Root, "turnIndex", TurnIndex) ||
        !ReadInt(Root, "lastEventSequence", LastEventSequence) ||
        !ReadArray(Root, "pieces", PiecesArray))
    {
        return false;
    }

    OutPayload.ViewerSide = static_cast<int32_t>(ViewerSide);
    OutPayload.Phase = static_cast<int32_t>(Phase);
    OutPayload.CurrentTurn = static_cast<int32_t>(CurrentTurn);
    OutPayload.PassCount = static_cast<int32_t>(PassCount);
    OutPayload.Result = static_cast<int32_t>(Result);
    OutPayload.EndReason = static_cast<int32_t>(EndReason);
    OutPayload.TurnIndex = static_cast<uint64_t>(TurnIndex);
    OutPayload.LastEventSequence = static_cast<uint64_t>(LastEventSequence);
    OutPayload.Pieces.clear();
    OutPayload.Pieces.reserve(PiecesArray->ArrayValue.size());
    for (const FProtocolJsonValue& PieceValue : PiecesArray->ArrayValue)
    {
        int64_t PieceId = 0;
        int64_t Side = 0;
        int64_t VisibleRole = 0;
        int64_t X = 0;
        int64_t Y = 0;
        bool bAlive = false;
        bool bFrozen = false;
        bool bRevealed = false;
        if (!ReadInt(PieceValue, "pieceId", PieceId) ||
            !ReadInt(PieceValue, "side", Side) ||
            !ReadInt(PieceValue, "visibleRole", VisibleRole) ||
            !ReadInt(PieceValue, "x", X) ||
            !ReadInt(PieceValue, "y", Y) ||
            !ReadBool(PieceValue, "alive", bAlive) ||
            !ReadBool(PieceValue, "frozen", bFrozen) ||
            !ReadBool(PieceValue, "revealed", bRevealed))
        {
            return false;
        }

        OutPayload.Pieces.push_back(FProtocolPieceSnapshot{
            static_cast<uint16_t>(PieceId),
            static_cast<int32_t>(Side),
            static_cast<int32_t>(VisibleRole),
            static_cast<int32_t>(X),
            static_cast<int32_t>(Y),
            bAlive,
            bFrozen,
            bRevealed});
    }

    return true;
}

bool EncodeEventDeltaPayload(const FProtocolEventDeltaPayload& Payload, std::string& OutJson)
{
    std::ostringstream Stream;
    Stream << "{\"requestedAfterSequence\":" << Payload.RequestedAfterSequence
           << ",\"latestSequence\":" << Payload.LatestSequence
           << ",\"events\":[";
    for (size_t Index = 0; Index < Payload.Events.size(); ++Index)
    {
        if (Index > 0)
        {
            Stream << ',';
        }
        const FProtocolEventRecordPayload& Event = Payload.Events[Index];
        Stream << "{\"sequence\":" << Event.Sequence
               << ",\"turnIndex\":" << Event.TurnIndex
               << ",\"eventType\":" << Event.EventType
               << ",\"actorPlayerId\":" << Event.ActorPlayerId
               << ",\"errorCode\":";
        AppendQuoted(Stream, Event.ErrorCode);
        Stream << ",\"description\":";
        AppendQuoted(Stream, Event.Description);
        Stream << '}';
    }
    Stream << "]}";
    OutJson = Stream.str();
    return true;
}

bool DecodeEventDeltaPayload(const std::string& Json, FProtocolEventDeltaPayload& OutPayload)
{
    FProtocolJsonValue Root;
    if (!ParseJsonRootObject(Json, Root))
    {
        return false;
    }

    int64_t RequestedAfterSequence = 0;
    int64_t LatestSequence = 0;
    const FProtocolJsonValue* EventsArray = nullptr;
    if (!ReadInt(Root, "requestedAfterSequence", RequestedAfterSequence) ||
        !ReadInt(Root, "latestSequence", LatestSequence) ||
        !ReadArray(Root, "events", EventsArray))
    {
        return false;
    }

    OutPayload.RequestedAfterSequence = static_cast<uint64_t>(RequestedAfterSequence);
    OutPayload.LatestSequence = static_cast<uint64_t>(LatestSequence);
    OutPayload.Events.clear();
    OutPayload.Events.reserve(EventsArray->ArrayValue.size());
    for (const FProtocolJsonValue& EventValue : EventsArray->ArrayValue)
    {
        int64_t Sequence = 0;
        int64_t TurnIndex = 0;
        int64_t EventType = 0;
        int64_t ActorPlayerId = 0;
        std::string ErrorCode;
        std::string Description;
        if (!ReadInt(EventValue, "sequence", Sequence) ||
            !ReadInt(EventValue, "turnIndex", TurnIndex) ||
            !ReadInt(EventValue, "eventType", EventType) ||
            !ReadInt(EventValue, "actorPlayerId", ActorPlayerId) ||
            !ReadString(EventValue, "errorCode", ErrorCode) ||
            !ReadString(EventValue, "description", Description))
        {
            return false;
        }

        OutPayload.Events.push_back(FProtocolEventRecordPayload{
            static_cast<uint64_t>(Sequence),
            static_cast<uint64_t>(TurnIndex),
            static_cast<int32_t>(EventType),
            static_cast<uint64_t>(ActorPlayerId),
            std::move(ErrorCode),
            std::move(Description)});
    }

    return true;
}

bool EncodeErrorPayload(const FProtocolErrorPayload& Payload, std::string& OutJson)
{
    std::ostringstream Stream;
    Stream << "{\"errorMessage\":";
    AppendQuoted(Stream, Payload.ErrorMessage);
    Stream << '}';
    OutJson = Stream.str();
    return true;
}

bool DecodeErrorPayload(const std::string& Json, FProtocolErrorPayload& OutPayload)
{
    FProtocolJsonValue Root;
    if (!ParseJsonRootObject(Json, Root))
    {
        return false;
    }

    return ReadString(Root, "errorMessage", OutPayload.ErrorMessage);
}
}

