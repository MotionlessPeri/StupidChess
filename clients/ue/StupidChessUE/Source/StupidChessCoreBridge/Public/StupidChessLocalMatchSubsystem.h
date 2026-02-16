#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include <string>

#include "StupidChessLocalMatchSubsystem.generated.h"

UENUM(BlueprintType)
enum class EStupidChessProtocolMessageType : uint8
{
    Unknown = 0 UMETA(Hidden),
    C2S_Join = 100,
    C2S_Command = 101,
    C2S_Ping = 102,
    C2S_PullSync = 103,
    C2S_Ack = 104,
    S2C_JoinAck = 200,
    S2C_CommandAck = 201,
    S2C_Snapshot = 202,
    S2C_EventDelta = 203,
    S2C_GameOver = 204,
    S2C_Error = 205
};

UENUM(BlueprintType)
enum class EStupidChessSide : uint8
{
    Red = 0,
    Black = 1
};

USTRUCT(BlueprintType)
struct FStupidChessSetupPlacement
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "StupidChess|Server")
    int32 PieceId = 0;

    UPROPERTY(BlueprintReadWrite, Category = "StupidChess|Server")
    int32 X = -1;

    UPROPERTY(BlueprintReadWrite, Category = "StupidChess|Server")
    int32 Y = -1;
};

USTRUCT(BlueprintType)
struct FStupidChessMoveCommand
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "StupidChess|Server")
    int32 PieceId = 0;

    UPROPERTY(BlueprintReadWrite, Category = "StupidChess|Server")
    int32 FromX = -1;

    UPROPERTY(BlueprintReadWrite, Category = "StupidChess|Server")
    int32 FromY = -1;

    UPROPERTY(BlueprintReadWrite, Category = "StupidChess|Server")
    int32 ToX = -1;

    UPROPERTY(BlueprintReadWrite, Category = "StupidChess|Server")
    int32 ToY = -1;

    UPROPERTY(BlueprintReadWrite, Category = "StupidChess|Server")
    bool bHasCapturedPieceId = false;

    UPROPERTY(BlueprintReadWrite, Category = "StupidChess|Server")
    int32 CapturedPieceId = 0;
};

USTRUCT(BlueprintType)
struct FStupidChessOutboundMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 PlayerId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 ServerSequence = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 MatchId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    EStupidChessProtocolMessageType MessageType = EStupidChessProtocolMessageType::S2C_Error;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    FString PayloadJson;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct FStupidChessJoinAckView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    bool bAccepted = false;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 AssignedSide = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    FString ErrorCode;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct FStupidChessCommandAckView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    bool bAccepted = false;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    FString ErrorCode;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct FStupidChessErrorView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct FStupidChessPieceSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 PieceId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 Side = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 VisibleRole = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 X = -1;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 Y = -1;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    bool bAlive = false;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    bool bFrozen = false;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    bool bRevealed = false;
};

USTRUCT(BlueprintType)
struct FStupidChessSnapshotView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 ViewerSide = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 Phase = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 CurrentTurn = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 PassCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 Result = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 EndReason = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 TurnIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 LastEventSequence = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    TArray<FStupidChessPieceSnapshot> Pieces;
};

USTRUCT(BlueprintType)
struct FStupidChessEventRecordView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 Sequence = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 TurnIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 EventType = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 ActorPlayerId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    FString ErrorCode;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    FString Description;
};

USTRUCT(BlueprintType)
struct FStupidChessEventDeltaView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 RequestedAfterSequence = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 LatestSequence = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    TArray<FStupidChessEventRecordView> Events;
};

USTRUCT(BlueprintType)
struct FStupidChessGameOverView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 Result = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 EndReason = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int64 TurnIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    bool bIsDraw = false;

    UPROPERTY(BlueprintReadOnly, Category = "StupidChess|Server")
    int32 WinnerSide = -1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStupidChessJoinAckParsedDelegate, const FStupidChessJoinAckView&, JoinAck);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStupidChessCommandAckParsedDelegate, const FStupidChessCommandAckView&, CommandAck);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStupidChessErrorParsedDelegate, const FStupidChessErrorView&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStupidChessSnapshotParsedDelegate, const FStupidChessSnapshotView&, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStupidChessEventDeltaParsedDelegate, const FStupidChessEventDeltaView&, EventDelta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStupidChessGameOverParsedDelegate, const FStupidChessGameOverView&, GameOver);

struct FStupidChessServerRuntime;
struct FProtocolCommandPayload;

UCLASS()
class STUPIDCHESSCOREBRIDGE_API UStupidChessLocalMatchSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "StupidChess|Server|Events")
    FStupidChessJoinAckParsedDelegate OnJoinAckParsed;

    UPROPERTY(BlueprintAssignable, Category = "StupidChess|Server|Events")
    FStupidChessCommandAckParsedDelegate OnCommandAckParsed;

    UPROPERTY(BlueprintAssignable, Category = "StupidChess|Server|Events")
    FStupidChessErrorParsedDelegate OnErrorParsed;

    UPROPERTY(BlueprintAssignable, Category = "StupidChess|Server|Events")
    FStupidChessSnapshotParsedDelegate OnSnapshotParsed;

    UPROPERTY(BlueprintAssignable, Category = "StupidChess|Server|Events")
    FStupidChessEventDeltaParsedDelegate OnEventDeltaParsed;

    UPROPERTY(BlueprintAssignable, Category = "StupidChess|Server|Events")
    FStupidChessGameOverParsedDelegate OnGameOverParsed;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    void ResetLocalServer();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool JoinLocalMatch(int64 MatchId, int64 PlayerId);

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool PullLocalSync(int64 MatchId, int64 PlayerId, bool bHasAfterSequenceOverride = false, int64 AfterSequenceOverride = 0);

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool AckLocalEvents(int64 MatchId, int64 PlayerId, int64 Sequence);

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool SubmitCommitSetup(int64 MatchId, int64 PlayerId, EStupidChessSide Side, const FString& HashHex);

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool SubmitRevealSetup(
        int64 MatchId,
        int64 PlayerId,
        EStupidChessSide Side,
        const FString& Nonce,
        const TArray<FStupidChessSetupPlacement>& Placements);

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    TArray<FStupidChessSetupPlacement> BuildStandardSetupPlacements(EStupidChessSide Side) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool SubmitPass(int64 MatchId, int64 PlayerId, EStupidChessSide Side);

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool SubmitMove(int64 MatchId, int64 PlayerId, EStupidChessSide Side, const FStupidChessMoveCommand& Move);

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool SubmitResign(int64 MatchId, int64 PlayerId, EStupidChessSide Side);

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    TArray<FStupidChessOutboundMessage> PullOutboundMessages(int64 PlayerId, int64 AfterServerSequence = 0) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    void ClearOutboundMessages();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    void ResetParsedCache();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    int32 ParseOutboundMessagesToCache(const TArray<FStupidChessOutboundMessage>& Messages);

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    int32 PullParseAndDispatchOutboundMessages(int64 PlayerId, int64 AfterServerSequence = 0);

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    TArray<FStupidChessOutboundMessage> GetLastPulledMessages() const;

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    int32 GetLastPulledMessageCount() const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool DecodeJoinAckPayloadJson(const FString& PayloadJson, FStupidChessJoinAckView& OutJoinAck) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool DecodeCommandAckPayloadJson(const FString& PayloadJson, FStupidChessCommandAckView& OutCommandAck) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool DecodeErrorPayloadJson(const FString& PayloadJson, FStupidChessErrorView& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool DecodeSnapshotPayloadJson(const FString& PayloadJson, FStupidChessSnapshotView& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool DecodeEventDeltaPayloadJson(const FString& PayloadJson, FStupidChessEventDeltaView& OutEventDelta) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool DecodeGameOverPayloadJson(const FString& PayloadJson, FStupidChessGameOverView& OutGameOver) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool TryParseSnapshotMessage(const FStupidChessOutboundMessage& Message, FStupidChessSnapshotView& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool TryParseEventDeltaMessage(const FStupidChessOutboundMessage& Message, FStupidChessEventDeltaView& OutEventDelta) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool TryParseJoinAckMessage(const FStupidChessOutboundMessage& Message, FStupidChessJoinAckView& OutJoinAck) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool TryParseCommandAckMessage(const FStupidChessOutboundMessage& Message, FStupidChessCommandAckView& OutCommandAck) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool TryParseErrorMessage(const FStupidChessOutboundMessage& Message, FStupidChessErrorView& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Server")
    bool TryParseGameOverMessage(const FStupidChessOutboundMessage& Message, FStupidChessGameOverView& OutGameOver) const;

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    bool GetCachedJoinAck(FStupidChessJoinAckView& OutJoinAck) const;

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    bool GetCachedCommandAck(FStupidChessCommandAckView& OutCommandAck) const;

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    bool GetCachedError(FStupidChessErrorView& OutError) const;

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    bool GetCachedSnapshot(FStupidChessSnapshotView& OutSnapshot) const;

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    bool GetCachedEventDelta(FStupidChessEventDeltaView& OutEventDelta) const;

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    bool GetCachedGameOver(FStupidChessGameOverView& OutGameOver) const;

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    int64 GetNextClientSequence() const;

    UFUNCTION(BlueprintPure, Category = "StupidChess|Server")
    int64 GetNextServerSequence() const;

private:
    bool SubmitCommandPayload(int64 MatchId, const FProtocolCommandPayload& CommandPayload);
    static int32 ToProtocolSide(EStupidChessSide Side);
    bool ProcessEnvelope(EStupidChessProtocolMessageType MessageType, int64 MatchId, const std::string& PayloadJson);
    static int64 ParseMatchId(const std::string& MatchIdText);

private:
    uint64 NextClientSequence = 1;
    FStupidChessServerRuntime* ServerRuntime = nullptr;
    TArray<FStupidChessOutboundMessage> LastPulledMessages;
    bool bHasCachedJoinAck = false;
    bool bHasCachedCommandAck = false;
    bool bHasCachedError = false;
    bool bHasCachedSnapshot = false;
    bool bHasCachedEventDelta = false;
    bool bHasCachedGameOver = false;
    FStupidChessJoinAckView CachedJoinAck{};
    FStupidChessCommandAckView CachedCommandAck{};
    FStupidChessErrorView CachedError{};
    FStupidChessSnapshotView CachedSnapshot{};
    FStupidChessEventDeltaView CachedEventDelta{};
    FStupidChessGameOverView CachedGameOver{};
};
