#include "StupidChessLocalMatchSubsystem.h"

#include "CoreRules/CoreTypes.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
TArray<FStupidChessSetupPlacement> BuildStandardSetup(EStupidChessSide Side)
{
    static const FIntPoint RedSlots[16] = {
        FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(2, 0), FIntPoint(3, 0),
        FIntPoint(4, 0), FIntPoint(5, 0), FIntPoint(6, 0), FIntPoint(7, 0),
        FIntPoint(8, 0), FIntPoint(1, 2), FIntPoint(7, 2), FIntPoint(0, 3),
        FIntPoint(2, 3), FIntPoint(4, 3), FIntPoint(6, 3), FIntPoint(8, 3)};

    TArray<FStupidChessSetupPlacement> Placements;
    Placements.Reserve(16);

    const int32 BasePieceId = Side == EStupidChessSide::Red ? 0 : 16;
    for (int32 Index = 0; Index < 16; ++Index)
    {
        int32 Y = RedSlots[Index].Y;
        if (Side == EStupidChessSide::Black)
        {
            Y = 9 - Y;
        }

        FStupidChessSetupPlacement Placement{};
        Placement.PieceId = BasePieceId + Index;
        Placement.X = RedSlots[Index].X;
        Placement.Y = Y;
        Placements.Add(Placement);
    }

    return Placements;
}

const FStupidChessOutboundMessage* FindFirstMessageByType(
    const TArray<FStupidChessOutboundMessage>& Messages,
    EStupidChessProtocolMessageType MessageType)
{
    for (const FStupidChessOutboundMessage& Message : Messages)
    {
        if (Message.MessageType == MessageType)
        {
            return &Message;
        }
    }

    return nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStupidChessLocalMatchSubsystemFlowTest,
    "StupidChess.UE.CoreBridge.LocalFlow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStupidChessLocalMatchSubsystemFlowTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    constexpr int64 MatchId = 900;
    constexpr int64 RedPlayerId = 10001;
    constexpr int64 BlackPlayerId = 10002;

    UGameInstance* GameInstance = NewObject<UGameInstance>();
    TestNotNull(TEXT("GameInstance should be created for subsystem outer."), GameInstance);
    if (GameInstance == nullptr)
    {
        return false;
    }

    UStupidChessLocalMatchSubsystem* Subsystem = NewObject<UStupidChessLocalMatchSubsystem>(GameInstance);
    TestNotNull(TEXT("Subsystem should be created."), Subsystem);
    if (Subsystem == nullptr)
    {
        return false;
    }

    Subsystem->ResetLocalServer();

    TestTrue(TEXT("Red join should be accepted."), Subsystem->JoinLocalMatch(MatchId, RedPlayerId));
    const TArray<FStupidChessOutboundMessage> RedJoinMessages = Subsystem->PullOutboundMessages(RedPlayerId);
    const FStupidChessOutboundMessage* RedJoinAckMessage = FindFirstMessageByType(
        RedJoinMessages,
        EStupidChessProtocolMessageType::S2C_JoinAck);
    TestNotNull(TEXT("Red join should contain join ack."), RedJoinAckMessage);
    if (RedJoinAckMessage != nullptr)
    {
        FStupidChessJoinAckView RedJoinAck{};
        TestTrue(TEXT("Red join ack should parse."), Subsystem->TryParseJoinAckMessage(*RedJoinAckMessage, RedJoinAck));
        TestTrue(TEXT("Red join ack should be accepted."), RedJoinAck.bAccepted);
        TestEqual(TEXT("Red should be assigned Red side."), RedJoinAck.AssignedSide, static_cast<int32>(ESide::Red));
    }

    Subsystem->ClearOutboundMessages();

    TestTrue(TEXT("Black join should be accepted."), Subsystem->JoinLocalMatch(MatchId, BlackPlayerId));
    const TArray<FStupidChessOutboundMessage> BlackJoinMessages = Subsystem->PullOutboundMessages(BlackPlayerId);
    const FStupidChessOutboundMessage* BlackJoinAckMessage = FindFirstMessageByType(
        BlackJoinMessages,
        EStupidChessProtocolMessageType::S2C_JoinAck);
    TestNotNull(TEXT("Black join should contain join ack."), BlackJoinAckMessage);
    if (BlackJoinAckMessage != nullptr)
    {
        FStupidChessJoinAckView BlackJoinAck{};
        TestTrue(TEXT("Black join ack should parse."), Subsystem->TryParseJoinAckMessage(*BlackJoinAckMessage, BlackJoinAck));
        TestTrue(TEXT("Black join ack should be accepted."), BlackJoinAck.bAccepted);
        TestEqual(TEXT("Black should be assigned Black side."), BlackJoinAck.AssignedSide, static_cast<int32>(ESide::Black));
    }

    Subsystem->ClearOutboundMessages();

    TestTrue(TEXT("Red commit should be accepted."),
             Subsystem->SubmitCommitSetup(MatchId, RedPlayerId, EStupidChessSide::Red, TEXT("")));
    TestTrue(TEXT("Black commit should be accepted."),
             Subsystem->SubmitCommitSetup(MatchId, BlackPlayerId, EStupidChessSide::Black, TEXT("")));
    TestTrue(TEXT("Red reveal should be accepted."),
             Subsystem->SubmitRevealSetup(MatchId, RedPlayerId, EStupidChessSide::Red, TEXT("R"), BuildStandardSetup(EStupidChessSide::Red)));
    TestTrue(TEXT("Black reveal should be accepted."),
             Subsystem->SubmitRevealSetup(MatchId, BlackPlayerId, EStupidChessSide::Black, TEXT("B"), BuildStandardSetup(EStupidChessSide::Black)));

    Subsystem->ClearOutboundMessages();

    FStupidChessMoveCommand Move{};
    Move.PieceId = 11;
    Move.FromX = 0;
    Move.FromY = 3;
    Move.ToX = 0;
    Move.ToY = 4;
    TestTrue(TEXT("Red move should be accepted."),
             Subsystem->SubmitMove(MatchId, RedPlayerId, EStupidChessSide::Red, Move));

    const TArray<FStupidChessOutboundMessage> RedMoveMessages = Subsystem->PullOutboundMessages(RedPlayerId);
    const FStupidChessOutboundMessage* RedCommandAckMessage = FindFirstMessageByType(
        RedMoveMessages,
        EStupidChessProtocolMessageType::S2C_CommandAck);
    const FStupidChessOutboundMessage* RedSnapshotMessage = FindFirstMessageByType(
        RedMoveMessages,
        EStupidChessProtocolMessageType::S2C_Snapshot);
    const FStupidChessOutboundMessage* RedDeltaMessage = FindFirstMessageByType(
        RedMoveMessages,
        EStupidChessProtocolMessageType::S2C_EventDelta);

    TestNotNull(TEXT("Move should emit command ack for actor."), RedCommandAckMessage);
    TestNotNull(TEXT("Move should emit snapshot for actor."), RedSnapshotMessage);
    TestNotNull(TEXT("Move should emit event delta for actor."), RedDeltaMessage);

    if (RedCommandAckMessage != nullptr)
    {
        FStupidChessCommandAckView CommandAck{};
        TestTrue(TEXT("Command ack should parse."), Subsystem->TryParseCommandAckMessage(*RedCommandAckMessage, CommandAck));
        TestTrue(TEXT("Move command ack should be accepted."), CommandAck.bAccepted);
    }

    if (RedSnapshotMessage != nullptr)
    {
        FStupidChessSnapshotView Snapshot{};
        TestTrue(TEXT("Snapshot should parse after move."), Subsystem->TryParseSnapshotMessage(*RedSnapshotMessage, Snapshot));
        TestEqual(TEXT("Turn should switch to black after red move."), Snapshot.CurrentTurn, static_cast<int32>(ESide::Black));
    }

    if (RedDeltaMessage != nullptr)
    {
        FStupidChessEventDeltaView Delta{};
        TestTrue(TEXT("Event delta should parse after move."), Subsystem->TryParseEventDeltaMessage(*RedDeltaMessage, Delta));
        TestTrue(TEXT("Move should append at least one event."), Delta.Events.Num() > 0);
    }

    Subsystem->ClearOutboundMessages();

    TestTrue(TEXT("Black resign should be accepted."),
             Subsystem->SubmitResign(MatchId, BlackPlayerId, EStupidChessSide::Black));
    const TArray<FStupidChessOutboundMessage> RedResignMessages = Subsystem->PullOutboundMessages(RedPlayerId);
    const FStupidChessOutboundMessage* RedResignSnapshotMessage = FindFirstMessageByType(
        RedResignMessages,
        EStupidChessProtocolMessageType::S2C_Snapshot);
    TestNotNull(TEXT("Resign should emit snapshot to red."), RedResignSnapshotMessage);

    if (RedResignSnapshotMessage != nullptr)
    {
        FStupidChessSnapshotView Snapshot{};
        TestTrue(TEXT("Resign snapshot should parse."), Subsystem->TryParseSnapshotMessage(*RedResignSnapshotMessage, Snapshot));
        TestEqual(TEXT("Result should be red win after black resign."), Snapshot.Result, static_cast<int32>(EGameResult::RedWin));
        TestEqual(TEXT("End reason should be resign."), Snapshot.EndReason, static_cast<int32>(EEndReason::Resign));
        TestEqual(TEXT("Phase should be game over."), Snapshot.Phase, static_cast<int32>(EGamePhase::GameOver));
    }

    Subsystem->ClearOutboundMessages();

    TestFalse(TEXT("Invalid ack should be rejected."), Subsystem->AckLocalEvents(MatchId, RedPlayerId, 99999));
    const TArray<FStupidChessOutboundMessage> ErrorMessages = Subsystem->PullOutboundMessages(RedPlayerId);
    const FStupidChessOutboundMessage* ErrorMessage = FindFirstMessageByType(
        ErrorMessages,
        EStupidChessProtocolMessageType::S2C_Error);
    TestNotNull(TEXT("Invalid ack should emit error message."), ErrorMessage);

    if (ErrorMessage != nullptr)
    {
        FStupidChessErrorView ErrorPayload{};
        TestTrue(TEXT("Error payload should parse."), Subsystem->TryParseErrorMessage(*ErrorMessage, ErrorPayload));
        TestFalse(TEXT("Error message text should not be empty."), ErrorPayload.ErrorMessage.IsEmpty());
    }

    Subsystem->Deinitialize();
    return true;
}

#endif
