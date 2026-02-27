#include "StupidChessBattlePrototypeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Math/RandomStream.h"

namespace
{
UTextBlock* CreateNamedTextBlock(UWidgetTree* WidgetTree, const TCHAR* Name, const FString& Text, const FLinearColor& Color)
{
    UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
    TextBlock->SetText(FText::FromString(Text));
    TextBlock->SetColorAndOpacity(FSlateColor(Color));
    TextBlock->SetAutoWrapText(true);
    return TextBlock;
}

UButton* CreateNamedButtonWithLabel(UWidgetTree* WidgetTree, const TCHAR* ButtonName, const TCHAR* LabelName, const FString& LabelText)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(ButtonName));
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(LabelName));
    Label->SetText(FText::FromString(LabelText));
    Label->SetJustification(ETextJustify::Center);
    Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Button->SetContent(Label);
    return Button;
}

int32 GetActualRoleTypeFromPieceIdForPrototype(int32 PieceId)
{
    const int32 LocalIndex = PieceId % 16;
    switch (LocalIndex)
    {
    case 0:
    case 8:
        return 4; // Rook
    case 1:
    case 7:
        return 3; // Horse
    case 2:
    case 6:
        return 2; // Elephant
    case 3:
    case 5:
        return 1; // Advisor
    case 4:
        return 0; // King
    case 9:
    case 10:
        return 5; // Cannon
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    default:
        return 6; // Pawn
    }
}

int32 GetSurfaceRoleTypeForSetupSlotCoordinate(EStupidChessSide Side, int32 X, int32 Y)
{
    // Canonical setup-slot template matches UStupidChessLocalMatchSubsystem::StandardSetupSlots
    // for Red. Black uses Y-mirror in BuildStandardSetupPlacements().
    struct FCanonicalSetupSlot
    {
        int32 X;
        int32 Y;
    };
    static constexpr FCanonicalSetupSlot CanonicalRedSetupSlots[16] = {
        {0, 0}, {1, 0}, {2, 0}, {3, 0},
        {4, 0}, {5, 0}, {6, 0}, {7, 0},
        {8, 0}, {1, 2}, {7, 2}, {0, 3},
        {2, 3}, {4, 3}, {6, 3}, {8, 3}};

    const int32 CanonicalY = (Side == EStupidChessSide::Black) ? (9 - Y) : Y;
    for (int32 LocalIndex = 0; LocalIndex < 16; ++LocalIndex)
    {
        if (CanonicalRedSetupSlots[LocalIndex].X == X && CanonicalRedSetupSlots[LocalIndex].Y == CanonicalY)
        {
            return GetActualRoleTypeFromPieceIdForPrototype(LocalIndex);
        }
    }

    return -1;
}

FString RoleTypeToChineseLabel(int32 Side, int32 RoleType)
{
    const bool bRed = (Side == 0);
    switch (RoleType)
    {
    case 0: // King
        return bRed ? TEXT("帥") : TEXT("將");
    case 1: // Advisor
        return bRed ? TEXT("仕") : TEXT("士");
    case 2: // Elephant
        return bRed ? TEXT("相") : TEXT("象");
    case 3: // Horse
        return TEXT("馬");
    case 4: // Rook
        return TEXT("車");
    case 5: // Cannon
        return TEXT("炮");
    case 6: // Pawn
        return bRed ? TEXT("兵") : TEXT("卒");
    default:
        return TEXT("?");
    }
}
}

void UStupidChessBoardCellClickProxy::Initialize(UStupidChessBattlePrototypeWidget* InOwnerWidget, int32 InX, int32 InY)
{
    OwnerWidget = InOwnerWidget;
    CellX = InX;
    CellY = InY;
}

void UStupidChessBoardCellClickProxy::HandleClicked()
{
    if (OwnerWidget != nullptr)
    {
        OwnerWidget->HandleBoardCellClicked(CellX, CellY);
    }
}

void UStupidChessBattlePrototypeWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindUiButtonEvents();
    BindSubsystemDelegatesIfNeeded();
    RefreshAllUi();

    if (bAutoBootstrapOnConstruct)
    {
        BootstrapBattlePrototypeMatch();
    }
}

TSharedRef<SWidget> UStupidChessBattlePrototypeWidget::RebuildWidget()
{
    BuildRuntimeWidgetTreeIfNeeded();
    return Super::RebuildWidget();
}

void UStupidChessBattlePrototypeWidget::NativeDestruct()
{
    UnbindSubsystemDelegates();
    Super::NativeDestruct();
}

void UStupidChessBattlePrototypeWidget::BootstrapBattlePrototypeMatch()
{
    BootstrapBattleInternal(false);
}

void UStupidChessBattlePrototypeWidget::BootstrapScrambledBattlePrototypeMatch()
{
    BootstrapBattleInternal(true);
}

void UStupidChessBattlePrototypeWidget::BootstrapSetupPrototypeMatch()
{
    UStupidChessLocalMatchSubsystem* Subsystem = GetLocalSubsystem();
    if (Subsystem == nullptr)
    {
        SetPrototypeStatus(TEXT("[SetupBootstrap] Local subsystem unavailable"));
        return;
    }

    ResetPrototypeStateOnly();
    ResetSetupPrototypeState();
    Subsystem->ResetLocalServer();

    const bool bJoinRedOk = Subsystem->JoinLocalMatch(MatchId, RedPlayerId);
    const bool bJoinBlackOk = Subsystem->JoinLocalMatch(MatchId, BlackPlayerId);
    PullBothSides();

    SetupStandardPlacementsRed = Subsystem->BuildStandardSetupPlacements(EStupidChessSide::Red);
    SetupStandardPlacementsBlack = Subsystem->BuildStandardSetupPlacements(EStupidChessSide::Black);
    SetupPendingPlacementsRed.Reset();
    SetupPendingPlacementsBlack.Reset();
    SetupPendingPlacementsRed.Reserve(SetupStandardPlacementsRed.Num());
    SetupPendingPlacementsBlack.Reserve(SetupStandardPlacementsBlack.Num());
    SetupActiveSide = EStupidChessSide::Red;
    SetupNextPlacementIndexRed = 0;
    SetupNextPlacementIndexBlack = 0;
    bSetupPrototypeActive = bJoinRedOk && bJoinBlackOk && SetupStandardPlacementsRed.Num() == 16 && SetupStandardPlacementsBlack.Num() == 16;
    bSetupPrototypeReadyToSubmit = false;

    SetPrototypeStatus(FString::Printf(
        TEXT("[SetupBootstrap] Join(R/B)=%s/%s Slots(R/B)=%d/%d"),
        bJoinRedOk ? TEXT("OK") : TEXT("Fail"),
        bJoinBlackOk ? TEXT("OK") : TEXT("Fail"),
        SetupStandardPlacementsRed.Num(),
        SetupStandardPlacementsBlack.Num()));
    RefreshSetupSelectionStatus();
    RefreshBoardCells();
}

void UStupidChessBattlePrototypeWidget::SubmitSetupPrototypeReveal()
{
    if (!bSetupPrototypeActive)
    {
        SetPrototypeStatus(TEXT("[SetupSubmit] Setup mode inactive"));
        return;
    }

    if (!bSetupPrototypeReadyToSubmit)
    {
        SetPrototypeStatus(TEXT("[SetupSubmit] Placements incomplete"));
        RefreshSetupSelectionStatus();
        return;
    }

    UStupidChessLocalMatchSubsystem* Subsystem = GetLocalSubsystem();
    if (Subsystem == nullptr)
    {
        SetPrototypeStatus(TEXT("[SetupSubmit] Local subsystem unavailable"));
        return;
    }

    const bool bCommitRedOk = Subsystem->SubmitCommitSetup(MatchId, RedPlayerId, EStupidChessSide::Red, TEXT(""));
    const bool bCommitBlackOk = Subsystem->SubmitCommitSetup(MatchId, BlackPlayerId, EStupidChessSide::Black, TEXT(""));
    CacheSurfaceRolesFromPlacements(EStupidChessSide::Red, SetupPendingPlacementsRed);
    CacheSurfaceRolesFromPlacements(EStupidChessSide::Black, SetupPendingPlacementsBlack);
    const bool bRevealRedOk = Subsystem->SubmitRevealSetup(MatchId, RedPlayerId, EStupidChessSide::Red, TEXT("R"), SetupPendingPlacementsRed);
    const bool bRevealBlackOk = Subsystem->SubmitRevealSetup(MatchId, BlackPlayerId, EStupidChessSide::Black, TEXT("B"), SetupPendingPlacementsBlack);
    PullBothSides();

    if (bCommitRedOk && bCommitBlackOk && bRevealRedOk && bRevealBlackOk)
    {
        bSetupPrototypeActive = false;
        bSetupPrototypeReadyToSubmit = false;
    }

    SetPrototypeStatus(FString::Printf(
        TEXT("[SetupSubmit] Commit(R/B)=%s/%s Reveal(R/B)=%s/%s"),
        bCommitRedOk ? TEXT("OK") : TEXT("Fail"),
        bCommitBlackOk ? TEXT("OK") : TEXT("Fail"),
        bRevealRedOk ? TEXT("OK") : TEXT("Fail"),
        bRevealBlackOk ? TEXT("OK") : TEXT("Fail")));
    RefreshSetupSelectionStatus();
    RefreshBoardCells();
}

void UStupidChessBattlePrototypeWidget::UndoSetupPrototypePlacement()
{
    if (!bSetupPrototypeActive)
    {
        SetPrototypeStatus(TEXT("[SetupUndo] Setup mode inactive"));
        return;
    }

    if (SetupPlacementHistory.Num() <= 0)
    {
        SetPrototypeStatus(TEXT("[SetupUndo] No placement history"));
        return;
    }

    const FStupidChessSetupPlacementHistoryEntry LastEntry = SetupPlacementHistory.Last();
    if (!RemoveLastSetupPlacementForSide(LastEntry.Side, &LastEntry.Placement))
    {
        SetPrototypeStatus(TEXT("[SetupUndo] Failed to rollback placement"));
        return;
    }

    SetupPlacementHistory.Pop();
    bSetupPrototypeReadyToSubmit = false;
    SetupActiveSide = LastEntry.Side;

    SetPrototypeStatus(FString::Printf(
        TEXT("[SetupUndo] Remove %s P%d from (%d,%d) (%d/%d, %d/%d)"),
        *SideToDebugLabel(static_cast<int32>(LastEntry.Side)),
        LastEntry.Placement.PieceId,
        LastEntry.Placement.X,
        LastEntry.Placement.Y,
        SetupPendingPlacementsRed.Num(),
        SetupStandardPlacementsRed.Num(),
        SetupPendingPlacementsBlack.Num(),
        SetupStandardPlacementsBlack.Num()));
    RefreshSetupSelectionStatus();
    RefreshBoardCells();
}

bool UStupidChessBattlePrototypeWidget::BootstrapBattleInternal(bool bUseScrambledSetup)
{
    UStupidChessLocalMatchSubsystem* Subsystem = GetLocalSubsystem();
    if (Subsystem == nullptr)
    {
        SetPrototypeStatus(TEXT("[Prototype] Local subsystem unavailable"));
        return false;
    }

    ResetPrototypeStateOnly();
    Subsystem->ResetLocalServer();

    const bool bJoinRedOk = Subsystem->JoinLocalMatch(MatchId, RedPlayerId);
    const bool bJoinBlackOk = Subsystem->JoinLocalMatch(MatchId, BlackPlayerId);
    PullBothSides();

    SetupStandardPlacementsRed = Subsystem->BuildStandardSetupPlacements(EStupidChessSide::Red);
    SetupStandardPlacementsBlack = Subsystem->BuildStandardSetupPlacements(EStupidChessSide::Black);
    const TArray<FStupidChessSetupPlacement> RedPlacements = bUseScrambledSetup
        ? BuildScrambledSetupPlacements(EStupidChessSide::Red)
        : SetupStandardPlacementsRed;
    const TArray<FStupidChessSetupPlacement> BlackPlacements = bUseScrambledSetup
        ? BuildScrambledSetupPlacements(EStupidChessSide::Black)
        : SetupStandardPlacementsBlack;

    const bool bCommitRedOk = Subsystem->SubmitCommitSetup(MatchId, RedPlayerId, EStupidChessSide::Red, TEXT(""));
    const bool bCommitBlackOk = Subsystem->SubmitCommitSetup(MatchId, BlackPlayerId, EStupidChessSide::Black, TEXT(""));
    CacheSurfaceRolesFromPlacements(EStupidChessSide::Red, RedPlacements);
    CacheSurfaceRolesFromPlacements(EStupidChessSide::Black, BlackPlacements);
    const bool bRevealRedOk = Subsystem->SubmitRevealSetup(MatchId, RedPlayerId, EStupidChessSide::Red, TEXT("R"), RedPlacements);
    const bool bRevealBlackOk = Subsystem->SubmitRevealSetup(MatchId, BlackPlayerId, EStupidChessSide::Black, TEXT("B"), BlackPlacements);
    PullBothSides();

    SetPrototypeStatus(FString::Printf(
        TEXT("[Bootstrap%s] Join(R/B)=%s/%s Commit(R/B)=%s/%s Reveal(R/B)=%s/%s"),
        bUseScrambledSetup ? TEXT(":Scrambled") : TEXT(""),
        bJoinRedOk ? TEXT("OK") : TEXT("Fail"),
        bJoinBlackOk ? TEXT("OK") : TEXT("Fail"),
        bCommitRedOk ? TEXT("OK") : TEXT("Fail"),
        bCommitBlackOk ? TEXT("OK") : TEXT("Fail"),
        bRevealRedOk ? TEXT("OK") : TEXT("Fail"),
        bRevealBlackOk ? TEXT("OK") : TEXT("Fail")));
    return bJoinRedOk && bJoinBlackOk && bCommitRedOk && bCommitBlackOk && bRevealRedOk && bRevealBlackOk;
}

void UStupidChessBattlePrototypeWidget::PullBothSides()
{
    UStupidChessLocalMatchSubsystem* Subsystem = GetLocalSubsystem();
    if (Subsystem == nullptr)
    {
        SetPrototypeStatus(TEXT("[Pull] Local subsystem unavailable"));
        return;
    }

    const int32 RedParsed = Subsystem->PullParseAndDispatchOutboundMessagesIncremental(RedPlayerId);
    const int32 BlackParsed = Subsystem->PullParseAndDispatchOutboundMessagesIncremental(BlackPlayerId);
    ApplyDisplayedViewerSnapshotIfAvailable();
    SetPrototypeStatus(FString::Printf(TEXT("[Pull] Parsed Red=%d Black=%d"), RedParsed, BlackParsed));
}

void UStupidChessBattlePrototypeWidget::SubmitCurrentTurnPass()
{
    UStupidChessLocalMatchSubsystem* Subsystem = GetLocalSubsystem();
    if (Subsystem == nullptr)
    {
        SetPrototypeStatus(TEXT("[Pass] Local subsystem unavailable"));
        return;
    }

    EStupidChessSide TurnSide = EStupidChessSide::Red;
    if (!TryGetCurrentTurnSide(TurnSide))
    {
        SetPrototypeStatus(TEXT("[Pass] No live snapshot / unknown turn"));
        return;
    }

    const int64 PlayerId = GetPlayerIdForSide(TurnSide);
    const bool bSubmitOk = Subsystem->SubmitPass(MatchId, PlayerId, TurnSide);
    SetPrototypeStatus(FString::Printf(TEXT("[Pass] Side=%s Submit=%s"), *SideToDebugLabel(static_cast<int32>(TurnSide)), bSubmitOk ? TEXT("OK") : TEXT("Fail")));
    if (bSubmitOk)
    {
        PullBothSides();
    }
}

void UStupidChessBattlePrototypeWidget::SubmitBlackResign()
{
    UStupidChessLocalMatchSubsystem* Subsystem = GetLocalSubsystem();
    if (Subsystem == nullptr)
    {
        SetPrototypeStatus(TEXT("[Resign] Local subsystem unavailable"));
        return;
    }

    const bool bSubmitOk = Subsystem->SubmitResign(MatchId, BlackPlayerId, EStupidChessSide::Black);
    SetPrototypeStatus(FString::Printf(TEXT("[Resign] Black Submit=%s"), bSubmitOk ? TEXT("OK") : TEXT("Fail")));
    if (bSubmitOk)
    {
        PullBothSides();
    }
}

void UStupidChessBattlePrototypeWidget::ResetPrototypeStateOnly()
{
    ResetSetupPrototypeState();
    bHasLiveSnapshot = false;
    LiveSnapshot = FStupidChessSnapshotView{};
    bHasSnapshotForViewer[0] = false;
    bHasSnapshotForViewer[1] = false;
    SnapshotByViewer[0] = FStupidChessSnapshotView{};
    SnapshotByViewer[1] = FStupidChessSnapshotView{};
    AckStatusText = TEXT("[CommandAck] <none>");
    EventDeltaStatusText = TEXT("[EventDelta] <none>");
    GameOverStatusText = TEXT("[GameOver] <none>");
    PrototypeStatusText = TEXT("Idle");
    ClearSelection();
    RefreshAllUi();
}

void UStupidChessBattlePrototypeWidget::ShowRedPlayerView()
{
    DisplayViewerSide = EStupidChessSide::Red;
    ApplyDisplayedViewerSnapshotIfAvailable();
    SetPrototypeStatus(TEXT("[View] Display viewer = Red"));
}

void UStupidChessBattlePrototypeWidget::ShowBlackPlayerView()
{
    DisplayViewerSide = EStupidChessSide::Black;
    ApplyDisplayedViewerSnapshotIfAvailable();
    SetPrototypeStatus(TEXT("[View] Display viewer = Black"));
}

void UStupidChessBattlePrototypeWidget::ToggleStrictPlayerView()
{
    bStrictPlayerView = !bStrictPlayerView;
    RefreshAllUi();
    SetPrototypeStatus(FString::Printf(TEXT("[View] StrictPlayerView=%s"), bStrictPlayerView ? TEXT("true") : TEXT("false")));
}

void UStupidChessBattlePrototypeWidget::HandleBoardCellClicked(int32 X, int32 Y)
{
    if (bSetupPrototypeActive)
    {
        HandleSetupBoardCellClicked(X, Y);
        return;
    }

    if (!bHasLiveSnapshot)
    {
        SetPrototypeStatus(TEXT("[CellClick] No live snapshot yet"));
        return;
    }

    if (!bHasSelection)
    {
        const FStupidChessPieceSnapshot* SourcePiece = FindLivePieceAt(X, Y);
        if (SourcePiece == nullptr)
        {
            SetPrototypeStatus(FString::Printf(TEXT("[CellClick] Empty cell (%d,%d)"), X, Y));
            return;
        }

        EStupidChessSide TurnSide = EStupidChessSide::Red;
        if (!TryGetCurrentTurnSide(TurnSide))
        {
            SetPrototypeStatus(TEXT("[CellClick] Unknown turn"));
            return;
        }

        if (SourcePiece->Side != static_cast<int32>(TurnSide))
        {
            SetPrototypeStatus(FString::Printf(
                TEXT("[CellClick] Not current side piece at (%d,%d). PieceSide=%s Turn=%s"),
                X,
                Y,
                *SideToDebugLabel(SourcePiece->Side),
                *SideToDebugLabel(static_cast<int32>(TurnSide))));
            return;
        }

        bHasSelection = true;
        SelectedBoardCell = FIntPoint(X, Y);
        SetSelectionStatus(FString::Printf(TEXT("[Select] (%d,%d)"), X, Y));
        RefreshBoardCells();
        return;
    }

    if (SelectedBoardCell.X == X && SelectedBoardCell.Y == Y)
    {
        ClearSelection();
        RefreshBoardCells();
        return;
    }

    if (TrySubmitMoveFromSelection(X, Y))
    {
        ClearSelection();
        RefreshBoardCells();
    }
}

void UStupidChessBattlePrototypeWidget::HandleBootstrapButtonClicked()
{
    BootstrapBattlePrototypeMatch();
}

void UStupidChessBattlePrototypeWidget::HandlePullButtonClicked()
{
    PullBothSides();
}

void UStupidChessBattlePrototypeWidget::HandleBootstrapScrambledButtonClicked()
{
    BootstrapScrambledBattlePrototypeMatch();
}

void UStupidChessBattlePrototypeWidget::HandleBootstrapSetupButtonClicked()
{
    BootstrapSetupPrototypeMatch();
}

void UStupidChessBattlePrototypeWidget::HandleSubmitSetupButtonClicked()
{
    SubmitSetupPrototypeReveal();
}

void UStupidChessBattlePrototypeWidget::HandleUndoSetupButtonClicked()
{
    UndoSetupPrototypePlacement();
}

void UStupidChessBattlePrototypeWidget::HandleShowRedViewButtonClicked()
{
    ShowRedPlayerView();
}

void UStupidChessBattlePrototypeWidget::HandleShowBlackViewButtonClicked()
{
    ShowBlackPlayerView();
}

void UStupidChessBattlePrototypeWidget::HandleToggleStrictViewButtonClicked()
{
    ToggleStrictPlayerView();
}

void UStupidChessBattlePrototypeWidget::HandlePassButtonClicked()
{
    SubmitCurrentTurnPass();
}

void UStupidChessBattlePrototypeWidget::HandleBlackResignButtonClicked()
{
    SubmitBlackResign();
}

void UStupidChessBattlePrototypeWidget::HandleJoinAckParsed(const FStupidChessJoinAckView& JoinAck)
{
    SetPrototypeStatus(FString::Printf(
        TEXT("[JoinAck] Accepted=%s Side=%d ErrorCode=%s"),
        JoinAck.bAccepted ? TEXT("true") : TEXT("false"),
        JoinAck.AssignedSide,
        JoinAck.ErrorCode.IsEmpty() ? TEXT("<empty>") : *JoinAck.ErrorCode));
}

void UStupidChessBattlePrototypeWidget::HandleCommandAckParsed(const FStupidChessCommandAckView& CommandAck)
{
    AckStatusText = FString::Printf(
        TEXT("[CommandAck] Accepted=%s Code=%s Msg=%s"),
        CommandAck.bAccepted ? TEXT("true") : TEXT("false"),
        CommandAck.ErrorCode.IsEmpty() ? TEXT("<empty>") : *CommandAck.ErrorCode,
        CommandAck.ErrorMessage.IsEmpty() ? TEXT("<empty>") : *CommandAck.ErrorMessage);
    RefreshStatusTexts();
}

void UStupidChessBattlePrototypeWidget::HandleErrorParsed(const FStupidChessErrorView& Error)
{
    SetPrototypeStatus(FString::Printf(TEXT("[Error] %s"), Error.ErrorMessage.IsEmpty() ? TEXT("<empty>") : *Error.ErrorMessage));
}

void UStupidChessBattlePrototypeWidget::HandleSnapshotParsed(const FStupidChessSnapshotView& Snapshot)
{
    if (Snapshot.ViewerSide == 0 || Snapshot.ViewerSide == 1)
    {
        SnapshotByViewer[Snapshot.ViewerSide] = Snapshot;
        bHasSnapshotForViewer[Snapshot.ViewerSide] = true;
    }
    if (Snapshot.ViewerSide == static_cast<int32>(DisplayViewerSide) || !bHasLiveSnapshot)
    {
        LiveSnapshot = Snapshot;
        bHasLiveSnapshot = true;
    }
    RefreshAllUi();
}

void UStupidChessBattlePrototypeWidget::HandleEventDeltaParsed(const FStupidChessEventDeltaView& EventDelta)
{
    EventDeltaStatusText = FString::Printf(
        TEXT("[EventDelta] Events=%d RequestedAfter=%lld Latest=%lld"),
        EventDelta.Events.Num(),
        static_cast<long long>(EventDelta.RequestedAfterSequence),
        static_cast<long long>(EventDelta.LatestSequence));
    RefreshStatusTexts();
}

void UStupidChessBattlePrototypeWidget::HandleGameOverParsed(const FStupidChessGameOverView& GameOver)
{
    GameOverStatusText = FString::Printf(
        TEXT("[GameOver] Result=%d EndReason=%d Winner=%s Turn=%lld"),
        GameOver.Result,
        GameOver.EndReason,
        *SideToDebugLabel(GameOver.WinnerSide),
        static_cast<long long>(GameOver.TurnIndex));
    RefreshStatusTexts();
}

void UStupidChessBattlePrototypeWidget::BuildRuntimeWidgetTreeIfNeeded()
{
    if (bWidgetTreeBuilt)
    {
        return;
    }

    if (WidgetTree == nullptr)
    {
        WidgetTree = NewObject<UWidgetTree>(this, UWidgetTree::StaticClass(), TEXT("RuntimeWidgetTree"));
    }

    if (WidgetTree == nullptr)
    {
        return;
    }

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    BoardGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("BoardGrid"));
    SidePanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SidePanel"));
    if (BoardGrid != nullptr)
    {
        BoardGrid->SetMinDesiredSlotWidth(56.0f);
        BoardGrid->SetMinDesiredSlotHeight(56.0f);
        BoardGrid->SetSlotPadding(FMargin(1.0f));
    }

    if (RootCanvas != nullptr)
    {
        if (UCanvasPanelSlot* BoardSlot = RootCanvas->AddChildToCanvas(BoardGrid))
        {
            BoardSlot->SetPosition(FVector2D(24.0f, 24.0f));
            BoardSlot->SetSize(FVector2D(540.0f, 600.0f));
            BoardSlot->SetZOrder(1);
        }
        if (UCanvasPanelSlot* SideSlot = RootCanvas->AddChildToCanvas(SidePanel))
        {
            SideSlot->SetPosition(FVector2D(584.0f, 24.0f));
            SideSlot->SetSize(FVector2D(320.0f, 600.0f));
            SideSlot->SetZOrder(2);
        }
    }

    TxtStatus = CreateNamedTextBlock(WidgetTree, TEXT("TxtStatus"), TEXT("Status"), FLinearColor::White);
    TxtSelection = CreateNamedTextBlock(WidgetTree, TEXT("TxtSelection"), TEXT("Selection"), FLinearColor(0.85f, 0.85f, 0.2f));
    TxtSnapshot = CreateNamedTextBlock(WidgetTree, TEXT("TxtSnapshot"), TEXT("Snapshot"), FLinearColor(0.7f, 0.95f, 1.0f));
    TxtEventDelta = CreateNamedTextBlock(WidgetTree, TEXT("TxtEventDelta"), TEXT("EventDelta"), FLinearColor(0.8f, 0.8f, 0.8f));
    TxtAck = CreateNamedTextBlock(WidgetTree, TEXT("TxtAck"), TEXT("Ack"), FLinearColor(0.8f, 1.0f, 0.8f));
    TxtGameOver = CreateNamedTextBlock(WidgetTree, TEXT("TxtGameOver"), TEXT("GameOver"), FLinearColor(1.0f, 0.75f, 0.75f));

    BtnBootstrap = CreateNamedButtonWithLabel(WidgetTree, TEXT("BtnBootstrap"), TEXT("TxtBtnBootstrap"), TEXT("Bootstrap Battle"));
    BtnBootstrapScrambled = CreateNamedButtonWithLabel(
        WidgetTree,
        TEXT("BtnBootstrapScrambled"),
        TEXT("TxtBtnBootstrapScrambled"),
        TEXT("Bootstrap Scrambled"));
    BtnBootstrapSetup = CreateNamedButtonWithLabel(
        WidgetTree,
        TEXT("BtnBootstrapSetup"),
        TEXT("TxtBtnBootstrapSetup"),
        TEXT("Bootstrap Setup"));
    BtnSubmitSetup = CreateNamedButtonWithLabel(
        WidgetTree,
        TEXT("BtnSubmitSetup"),
        TEXT("TxtBtnSubmitSetup"),
        TEXT("Submit Setup"));
    BtnUndoSetup = CreateNamedButtonWithLabel(
        WidgetTree,
        TEXT("BtnUndoSetup"),
        TEXT("TxtBtnUndoSetup"),
        TEXT("Undo Setup"));
    BtnShowRedView = CreateNamedButtonWithLabel(
        WidgetTree,
        TEXT("BtnShowRedView"),
        TEXT("TxtBtnShowRedView"),
        TEXT("Show Red View"));
    BtnShowBlackView = CreateNamedButtonWithLabel(
        WidgetTree,
        TEXT("BtnShowBlackView"),
        TEXT("TxtBtnShowBlackView"),
        TEXT("Show Black View"));
    BtnToggleStrictView = CreateNamedButtonWithLabel(
        WidgetTree,
        TEXT("BtnToggleStrictView"),
        TEXT("TxtBtnToggleStrictView"),
        TEXT("Toggle Strict View"));
    BtnPull = CreateNamedButtonWithLabel(WidgetTree, TEXT("BtnPull"), TEXT("TxtBtnPull"), TEXT("Pull Both"));
    BtnPass = CreateNamedButtonWithLabel(WidgetTree, TEXT("BtnPass"), TEXT("TxtBtnPass"), TEXT("Pass Current"));
    BtnBlackResign = CreateNamedButtonWithLabel(WidgetTree, TEXT("BtnBlackResign"), TEXT("TxtBtnBlackResign"), TEXT("Black Resign"));

    if (SidePanel != nullptr)
    {
        auto AddToSide = [this](UWidget* ChildWidget, float PaddingBottom = 6.0f)
        {
            if (ChildWidget == nullptr || SidePanel == nullptr)
            {
                return;
            }
            if (UVerticalBoxSlot* Slot = SidePanel->AddChildToVerticalBox(ChildWidget))
            {
                Slot->SetPadding(FMargin(4.0f, 4.0f, 4.0f, PaddingBottom));
            }
        };

        AddToSide(BtnBootstrap);
        AddToSide(BtnBootstrapScrambled);
        AddToSide(BtnBootstrapSetup);
        AddToSide(BtnSubmitSetup);
        AddToSide(BtnUndoSetup);
        AddToSide(BtnShowRedView);
        AddToSide(BtnShowBlackView);
        AddToSide(BtnToggleStrictView);
        AddToSide(BtnPull, 10.0f);
        AddToSide(BtnPass);
        AddToSide(BtnBlackResign, 12.0f);
        AddToSide(TxtStatus);
        AddToSide(TxtSelection);
        AddToSide(TxtSnapshot);
        AddToSide(TxtEventDelta);
        AddToSide(TxtAck);
        AddToSide(TxtGameOver, 12.0f);
    }

    BuildBoardCells();
    bWidgetTreeBuilt = true;
}

void UStupidChessBattlePrototypeWidget::BuildBoardCells()
{
    if (WidgetTree == nullptr || BoardGrid == nullptr || BoardCellButtons.Num() > 0)
    {
        return;
    }

    BoardCellButtons.SetNum(BoardWidth * BoardHeight);
    BoardCellLabels.SetNum(BoardWidth * BoardHeight);
    BoardCellClickProxies.SetNum(BoardWidth * BoardHeight);

    for (int32 Y = 0; Y < BoardHeight; ++Y)
    {
        for (int32 X = 0; X < BoardWidth; ++X)
        {
            const int32 CellIndex = GetCellIndex(X, Y);
            const FName ButtonName(*FString::Printf(TEXT("BtnCell_%d_%d"), Y, X));
            const FName LabelName(*FString::Printf(TEXT("TxtCell_%d_%d"), Y, X));

            UButton* CellButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
            UTextBlock* CellLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
            CellLabel->SetText(FText::FromString(FString::Printf(TEXT("%d,%d"), X, Y)));
            CellLabel->SetJustification(ETextJustify::Center);
            CellLabel->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
            FSlateFontInfo CellFont = CellLabel->GetFont();
            CellFont.Size = 10;
            CellLabel->SetFont(CellFont);
            CellButton->SetContent(CellLabel);
            CellButton->SetBackgroundColor(FLinearColor(0.85f, 0.85f, 0.85f));

            UStupidChessBoardCellClickProxy* ClickProxy = NewObject<UStupidChessBoardCellClickProxy>(this);
            ClickProxy->Initialize(this, X, Y);
            CellButton->OnClicked.AddDynamic(ClickProxy, &UStupidChessBoardCellClickProxy::HandleClicked);

            if (UUniformGridSlot* GridSlot = BoardGrid->AddChildToUniformGrid(CellButton, Y, X))
            {
                GridSlot->SetHorizontalAlignment(HAlign_Fill);
                GridSlot->SetVerticalAlignment(VAlign_Fill);
            }

            BoardCellButtons[CellIndex] = CellButton;
            BoardCellLabels[CellIndex] = CellLabel;
            BoardCellClickProxies[CellIndex] = ClickProxy;
        }
    }
}

void UStupidChessBattlePrototypeWidget::BindUiButtonEvents()
{
    if (bUiButtonsBound)
    {
        return;
    }

    if (BtnBootstrap != nullptr)
    {
        BtnBootstrap->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleBootstrapButtonClicked);
    }
    if (BtnPull != nullptr)
    {
        BtnPull->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandlePullButtonClicked);
    }
    if (BtnBootstrapScrambled != nullptr)
    {
        BtnBootstrapScrambled->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleBootstrapScrambledButtonClicked);
    }
    if (BtnBootstrapSetup != nullptr)
    {
        BtnBootstrapSetup->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleBootstrapSetupButtonClicked);
    }
    if (BtnSubmitSetup != nullptr)
    {
        BtnSubmitSetup->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleSubmitSetupButtonClicked);
    }
    if (BtnUndoSetup != nullptr)
    {
        BtnUndoSetup->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleUndoSetupButtonClicked);
    }
    if (BtnShowRedView != nullptr)
    {
        BtnShowRedView->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleShowRedViewButtonClicked);
    }
    if (BtnShowBlackView != nullptr)
    {
        BtnShowBlackView->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleShowBlackViewButtonClicked);
    }
    if (BtnToggleStrictView != nullptr)
    {
        BtnToggleStrictView->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleToggleStrictViewButtonClicked);
    }
    if (BtnPass != nullptr)
    {
        BtnPass->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandlePassButtonClicked);
    }
    if (BtnBlackResign != nullptr)
    {
        BtnBlackResign->OnClicked.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleBlackResignButtonClicked);
    }

    bUiButtonsBound = true;
}

void UStupidChessBattlePrototypeWidget::BindSubsystemDelegatesIfNeeded()
{
    if (bSubsystemDelegatesBound)
    {
        return;
    }

    UStupidChessLocalMatchSubsystem* Subsystem = GetLocalSubsystem();
    if (Subsystem == nullptr)
    {
        return;
    }

    Subsystem->OnJoinAckParsed.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleJoinAckParsed);
    Subsystem->OnCommandAckParsed.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleCommandAckParsed);
    Subsystem->OnErrorParsed.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleErrorParsed);
    Subsystem->OnSnapshotParsed.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleSnapshotParsed);
    Subsystem->OnEventDeltaParsed.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleEventDeltaParsed);
    Subsystem->OnGameOverParsed.AddDynamic(this, &UStupidChessBattlePrototypeWidget::HandleGameOverParsed);
    bSubsystemDelegatesBound = true;
}

void UStupidChessBattlePrototypeWidget::UnbindSubsystemDelegates()
{
    if (!bSubsystemDelegatesBound)
    {
        return;
    }

    UStupidChessLocalMatchSubsystem* Subsystem = GetLocalSubsystem();
    if (Subsystem != nullptr)
    {
        Subsystem->OnJoinAckParsed.RemoveDynamic(this, &UStupidChessBattlePrototypeWidget::HandleJoinAckParsed);
        Subsystem->OnCommandAckParsed.RemoveDynamic(this, &UStupidChessBattlePrototypeWidget::HandleCommandAckParsed);
        Subsystem->OnErrorParsed.RemoveDynamic(this, &UStupidChessBattlePrototypeWidget::HandleErrorParsed);
        Subsystem->OnSnapshotParsed.RemoveDynamic(this, &UStupidChessBattlePrototypeWidget::HandleSnapshotParsed);
        Subsystem->OnEventDeltaParsed.RemoveDynamic(this, &UStupidChessBattlePrototypeWidget::HandleEventDeltaParsed);
        Subsystem->OnGameOverParsed.RemoveDynamic(this, &UStupidChessBattlePrototypeWidget::HandleGameOverParsed);
    }

    bSubsystemDelegatesBound = false;
}

void UStupidChessBattlePrototypeWidget::RefreshAllUi()
{
    RefreshBoardCells();
    RefreshStatusTexts();
}

void UStupidChessBattlePrototypeWidget::ApplyDisplayedViewerSnapshotIfAvailable()
{
    const int32 ViewerIndex = static_cast<int32>(DisplayViewerSide);
    if (ViewerIndex < 0 || ViewerIndex > 1)
    {
        return;
    }

    if (!bHasSnapshotForViewer[ViewerIndex])
    {
        return;
    }

    LiveSnapshot = SnapshotByViewer[ViewerIndex];
    bHasLiveSnapshot = true;
    RefreshAllUi();
}

void UStupidChessBattlePrototypeWidget::RefreshBoardCells()
{
    for (int32 Y = 0; Y < BoardHeight; ++Y)
    {
        for (int32 X = 0; X < BoardWidth; ++X)
        {
            const int32 CellIndex = GetCellIndex(X, Y);
            UButton* CellButton = BoardCellButtons.IsValidIndex(CellIndex) ? BoardCellButtons[CellIndex] : nullptr;
            UTextBlock* CellLabel = BoardCellLabels.IsValidIndex(CellIndex) ? BoardCellLabels[CellIndex] : nullptr;
            if (CellButton == nullptr || CellLabel == nullptr)
            {
                continue;
            }

            const bool bSelected = bHasSelection && SelectedBoardCell.X == X && SelectedBoardCell.Y == Y;
            const FStupidChessPieceSnapshot* Piece = (!bSetupPrototypeActive && bHasLiveSnapshot) ? FindLivePieceAt(X, Y) : nullptr;
            FStupidChessSetupPlacement SetupPlacement{};
            int32 SetupSide = -1;
            const bool bHasSetupPreviewPiece = bSetupPrototypeActive && TryFindSetupPreviewPieceAt(X, Y, SetupPlacement, SetupSide);
            if (bHasSetupPreviewPiece)
            {
                CellLabel->SetText(FText::FromString(MakeSetupCellLabelText(SetupPlacement, SetupSide, bSelected)));
            }
            else
            {
                CellLabel->SetText(FText::FromString(MakeCellLabelText(Piece, bSelected)));
            }

            FLinearColor CellColor = FLinearColor(0.84f, 0.84f, 0.84f);
            if ((X + Y) % 2 == 0)
            {
                CellColor = FLinearColor(0.92f, 0.92f, 0.92f);
            }
            if (bHasSetupPreviewPiece)
            {
                CellColor = (SetupSide == 0) ? FLinearColor(0.96f, 0.70f, 0.70f) : FLinearColor(0.72f, 0.78f, 0.96f);
            }
            else if (Piece != nullptr)
            {
                CellColor = Piece->Side == 0 ? FLinearColor(0.96f, 0.70f, 0.70f) : FLinearColor(0.72f, 0.78f, 0.96f);
                if (Piece->bFrozen)
                {
                    CellColor *= 0.7f;
                }
            }
            if (bSetupPrototypeActive && !bHasSetupPreviewPiece)
            {
                EStupidChessSide ActiveSetupSide = EStupidChessSide::Red;
                if (TryGetSetupActiveSide(ActiveSetupSide) && IsValidSetupSlot(ActiveSetupSide, X, Y))
                {
                    CellColor = (ActiveSetupSide == EStupidChessSide::Red)
                        ? FLinearColor(1.0f, 0.85f, 0.85f)
                        : FLinearColor(0.84f, 0.89f, 1.0f);
                }
            }
            if (bSelected)
            {
                CellColor = FLinearColor(1.0f, 0.92f, 0.35f);
            }
            CellButton->SetBackgroundColor(CellColor);
        }
    }
}

void UStupidChessBattlePrototypeWidget::RefreshStatusTexts()
{
    if (TxtStatus != nullptr)
    {
        TxtStatus->SetText(FText::FromString(FString::Printf(
            TEXT("%s\n[View] %s | Strict=%s"),
            *PrototypeStatusText,
            *SideToDebugLabel(static_cast<int32>(DisplayViewerSide)),
            bStrictPlayerView ? TEXT("On") : TEXT("Off"))));
    }
    if (TxtSelection != nullptr)
    {
        TxtSelection->SetText(FText::FromString(SelectionStatusText));
    }

    if (TxtSnapshot != nullptr)
    {
        if (!bHasLiveSnapshot)
        {
            TxtSnapshot->SetText(FText::FromString(TEXT("[Snapshot] <none>")));
        }
        else
        {
            TxtSnapshot->SetText(FText::FromString(FString::Printf(
                TEXT("[Snapshot] Phase=%s Turn=%s TurnIndex=%lld Pieces=%d LastSeq=%lld"),
                *PhaseToDebugLabel(LiveSnapshot.Phase),
                *SideToDebugLabel(LiveSnapshot.CurrentTurn),
                static_cast<long long>(LiveSnapshot.TurnIndex),
                LiveSnapshot.Pieces.Num(),
                static_cast<long long>(LiveSnapshot.LastEventSequence))));
        }
    }

    if (TxtEventDelta != nullptr)
    {
        TxtEventDelta->SetText(FText::FromString(EventDeltaStatusText));
    }
    if (TxtAck != nullptr)
    {
        TxtAck->SetText(FText::FromString(AckStatusText));
    }
    if (TxtGameOver != nullptr)
    {
        TxtGameOver->SetText(FText::FromString(GameOverStatusText));
    }

    if (BtnSubmitSetup != nullptr)
    {
        BtnSubmitSetup->SetIsEnabled(bSetupPrototypeActive && bSetupPrototypeReadyToSubmit);
    }
    if (BtnUndoSetup != nullptr)
    {
        BtnUndoSetup->SetIsEnabled(bSetupPrototypeActive && SetupPlacementHistory.Num() > 0);
    }
}

void UStupidChessBattlePrototypeWidget::ClearSelection()
{
    bHasSelection = false;
    SelectedBoardCell = FIntPoint(-1, -1);
    SetSelectionStatus(TEXT("[Select] <none>"));
}

void UStupidChessBattlePrototypeWidget::ResetSetupPrototypeState()
{
    bSetupPrototypeActive = false;
    bSetupPrototypeReadyToSubmit = false;
    SetupActiveSide = EStupidChessSide::Red;
    SetupNextPlacementIndexRed = 0;
    SetupNextPlacementIndexBlack = 0;
    SetupStandardPlacementsRed.Reset();
    SetupStandardPlacementsBlack.Reset();
    SetupPendingPlacementsRed.Reset();
    SetupPendingPlacementsBlack.Reset();
    SetupPlacementHistory.Reset();
    PieceSurfaceRoleTypeByPieceId.Reset();
}

void UStupidChessBattlePrototypeWidget::RefreshSetupSelectionStatus()
{
    if (!bSetupPrototypeActive)
    {
        return;
    }

    if (bSetupPrototypeReadyToSubmit)
    {
        SetSelectionStatus(TEXT("[Setup] Ready to submit (Commit/Reveal)"));
        return;
    }

    EStupidChessSide ActiveSide = EStupidChessSide::Red;
    if (!TryGetSetupActiveSide(ActiveSide))
    {
        SetSelectionStatus(TEXT("[Setup] Internal state invalid"));
        return;
    }

    const TArray<FStupidChessSetupPlacement>& StandardPlacements =
        (ActiveSide == EStupidChessSide::Red) ? SetupStandardPlacementsRed : SetupStandardPlacementsBlack;
    const int32 NextIndex = (ActiveSide == EStupidChessSide::Red) ? SetupNextPlacementIndexRed : SetupNextPlacementIndexBlack;
    if (!StandardPlacements.IsValidIndex(NextIndex))
    {
        SetSelectionStatus(TEXT("[Setup] Ready to submit (Commit/Reveal)"));
        return;
    }

    const int32 PieceId = StandardPlacements[NextIndex].PieceId;
    const int32 ActualRole = GetActualRoleTypeFromPieceIdForPrototype(PieceId);
    const FString SideLabel = SideToDebugLabel(static_cast<int32>(ActiveSide));
    const FString ActualRoleLabel = RoleTypeToChineseLabel(static_cast<int32>(ActiveSide), ActualRole);
    SetSelectionStatus(FString::Printf(
        TEXT("[Setup] %s #%d P%d %s -> click legal start slot"),
        *SideLabel,
        NextIndex + 1,
        PieceId,
        *ActualRoleLabel));
}

bool UStupidChessBattlePrototypeWidget::HandleSetupBoardCellClicked(int32 X, int32 Y)
{
    if (!bSetupPrototypeActive)
    {
        return false;
    }

    EStupidChessSide ActiveSide = EStupidChessSide::Red;
    if (!TryGetSetupActiveSide(ActiveSide))
    {
        SetPrototypeStatus(TEXT("[Setup] Invalid active side"));
        RefreshSetupSelectionStatus();
        return false;
    }

    const TArray<FStupidChessSetupPlacement>& StandardPlacements =
        (ActiveSide == EStupidChessSide::Red) ? SetupStandardPlacementsRed : SetupStandardPlacementsBlack;
    TArray<FStupidChessSetupPlacement>& PendingPlacements =
        (ActiveSide == EStupidChessSide::Red) ? SetupPendingPlacementsRed : SetupPendingPlacementsBlack;
    int32& NextIndex = (ActiveSide == EStupidChessSide::Red) ? SetupNextPlacementIndexRed : SetupNextPlacementIndexBlack;

    if (!StandardPlacements.IsValidIndex(NextIndex))
    {
        SetPrototypeStatus(TEXT("[Setup] Side complete, switch/submit"));
        RefreshSetupSelectionStatus();
        return false;
    }

    if (!IsValidSetupSlot(ActiveSide, X, Y))
    {
        SetPrototypeStatus(FString::Printf(
            TEXT("[Setup] %s invalid slot (%d,%d)"),
            *SideToDebugLabel(static_cast<int32>(ActiveSide)),
            X,
            Y));
        return false;
    }

    if (IsSetupCellOccupied(ActiveSide, X, Y))
    {
        SetPrototypeStatus(FString::Printf(
            TEXT("[Setup] %s slot occupied (%d,%d)"),
            *SideToDebugLabel(static_cast<int32>(ActiveSide)),
            X,
            Y));
        return false;
    }

    FStupidChessSetupPlacement Placement = StandardPlacements[NextIndex];
    Placement.X = X;
    Placement.Y = Y;
    PendingPlacements.Add(Placement);
    FStupidChessSetupPlacementHistoryEntry& HistoryEntry = SetupPlacementHistory.AddDefaulted_GetRef();
    HistoryEntry.Side = ActiveSide;
    HistoryEntry.Placement = Placement;
    ++NextIndex;

    if (SetupNextPlacementIndexRed >= SetupStandardPlacementsRed.Num() && SetupNextPlacementIndexBlack >= SetupStandardPlacementsBlack.Num())
    {
        bSetupPrototypeReadyToSubmit = true;
    }
    else if (ActiveSide == EStupidChessSide::Red && SetupNextPlacementIndexRed >= SetupStandardPlacementsRed.Num())
    {
        SetupActiveSide = EStupidChessSide::Black;
    }
    else if (ActiveSide == EStupidChessSide::Black && SetupNextPlacementIndexBlack >= SetupStandardPlacementsBlack.Num())
    {
        SetupActiveSide = EStupidChessSide::Red;
    }

    SetPrototypeStatus(FString::Printf(
        TEXT("[Setup] Place %s P%d -> (%d,%d) (%d/%d, %d/%d)"),
        *SideToDebugLabel(static_cast<int32>(ActiveSide)),
        Placement.PieceId,
        X,
        Y,
        SetupPendingPlacementsRed.Num(),
        SetupStandardPlacementsRed.Num(),
        SetupPendingPlacementsBlack.Num(),
        SetupStandardPlacementsBlack.Num()));
    RefreshSetupSelectionStatus();
    RefreshBoardCells();
    return true;
}

bool UStupidChessBattlePrototypeWidget::RemoveLastSetupPlacementForSide(
    EStupidChessSide Side,
    const FStupidChessSetupPlacement* ExpectedPlacement)
{
    TArray<FStupidChessSetupPlacement>& PendingPlacements =
        (Side == EStupidChessSide::Red) ? SetupPendingPlacementsRed : SetupPendingPlacementsBlack;
    int32& NextIndex = (Side == EStupidChessSide::Red) ? SetupNextPlacementIndexRed : SetupNextPlacementIndexBlack;

    if (PendingPlacements.Num() <= 0 || NextIndex <= 0)
    {
        return false;
    }

    const int32 LastIndex = PendingPlacements.Num() - 1;
    if (ExpectedPlacement != nullptr)
    {
        const FStupidChessSetupPlacement& CurrentLast = PendingPlacements[LastIndex];
        if (CurrentLast.PieceId != ExpectedPlacement->PieceId || CurrentLast.X != ExpectedPlacement->X || CurrentLast.Y != ExpectedPlacement->Y)
        {
            return false;
        }
    }

    PendingPlacements.RemoveAt(LastIndex);
    NextIndex = FMath::Max(0, NextIndex - 1);
    return true;
}

bool UStupidChessBattlePrototypeWidget::TryGetSetupActiveSide(EStupidChessSide& OutSide) const
{
    if (!bSetupPrototypeActive)
    {
        return false;
    }

    if (bSetupPrototypeReadyToSubmit)
    {
        OutSide = SetupActiveSide;
        return true;
    }

    if (SetupNextPlacementIndexRed < SetupStandardPlacementsRed.Num())
    {
        OutSide = EStupidChessSide::Red;
        if (SetupActiveSide == EStupidChessSide::Black && SetupNextPlacementIndexBlack < SetupStandardPlacementsBlack.Num())
        {
            OutSide = EStupidChessSide::Black;
        }
        return true;
    }
    if (SetupNextPlacementIndexBlack < SetupStandardPlacementsBlack.Num())
    {
        OutSide = EStupidChessSide::Black;
        return true;
    }
    return false;
}

bool UStupidChessBattlePrototypeWidget::TryFindSetupPreviewPieceAt(int32 X, int32 Y, FStupidChessSetupPlacement& OutPlacement, int32& OutSide) const
{
    for (const FStupidChessSetupPlacement& Placement : SetupPendingPlacementsRed)
    {
        if (Placement.X == X && Placement.Y == Y)
        {
            OutPlacement = Placement;
            OutSide = 0;
            return true;
        }
    }
    for (const FStupidChessSetupPlacement& Placement : SetupPendingPlacementsBlack)
    {
        if (Placement.X == X && Placement.Y == Y)
        {
            OutPlacement = Placement;
            OutSide = 1;
            return true;
        }
    }
    return false;
}

bool UStupidChessBattlePrototypeWidget::IsValidSetupSlot(EStupidChessSide Side, int32 X, int32 Y) const
{
    const TArray<FStupidChessSetupPlacement>& StandardPlacements =
        (Side == EStupidChessSide::Red) ? SetupStandardPlacementsRed : SetupStandardPlacementsBlack;
    for (const FStupidChessSetupPlacement& Placement : StandardPlacements)
    {
        if (Placement.X == X && Placement.Y == Y)
        {
            return true;
        }
    }
    return false;
}

bool UStupidChessBattlePrototypeWidget::IsSetupCellOccupied(EStupidChessSide Side, int32 X, int32 Y) const
{
    const TArray<FStupidChessSetupPlacement>& PendingPlacements =
        (Side == EStupidChessSide::Red) ? SetupPendingPlacementsRed : SetupPendingPlacementsBlack;
    for (const FStupidChessSetupPlacement& Placement : PendingPlacements)
    {
        if (Placement.X == X && Placement.Y == Y)
        {
            return true;
        }
    }
    return false;
}

int32 UStupidChessBattlePrototypeWidget::GetSetupSlotVisibleRole(EStupidChessSide Side, int32 X, int32 Y) const
{
    return GetSurfaceRoleTypeForSetupSlotCoordinate(Side, X, Y);
}

void UStupidChessBattlePrototypeWidget::CacheSurfaceRolesFromPlacements(
    EStupidChessSide Side,
    const TArray<FStupidChessSetupPlacement>& Placements)
{
    for (const FStupidChessSetupPlacement& Placement : Placements)
    {
        const int32 SurfaceRoleType = GetSetupSlotVisibleRole(Side, Placement.X, Placement.Y);
        if (SurfaceRoleType >= 0)
        {
            PieceSurfaceRoleTypeByPieceId.Add(Placement.PieceId, SurfaceRoleType);
        }
    }
}

int32 UStupidChessBattlePrototypeWidget::GetCachedSurfaceRoleTypeForPiece(int32 PieceId, int32 Side) const
{
    if (const int32* CachedRoleType = PieceSurfaceRoleTypeByPieceId.Find(PieceId))
    {
        return *CachedRoleType;
    }

    return GetActualRoleTypeFromPieceIdForPrototype(PieceId);
}

FString UStupidChessBattlePrototypeWidget::MakeSetupCellLabelText(const FStupidChessSetupPlacement& Placement, int32 Side, bool bSelected) const
{
    const FString SidePrefix = Side == 0 ? TEXT("R") : TEXT("B");
    const int32 VisibleRole = GetSetupSlotVisibleRole(static_cast<EStupidChessSide>(Side), Placement.X, Placement.Y);
    const int32 ActualRole = GetActualRoleTypeFromPieceIdForPrototype(Placement.PieceId);
    const FString VisibleLabel = RoleTypeToChineseLabel(Side, VisibleRole);
    const FString ActualLabel = RoleTypeToChineseLabel(Side, ActualRole);
    FString BaseText = FString::Printf(TEXT("%s%d\n%s/%s 设"), *SidePrefix, Placement.PieceId, *VisibleLabel, *ActualLabel);
    if (bSelected)
    {
        BaseText = FString::Printf(TEXT("[%s]"), *BaseText);
    }
    return BaseText;
}

bool UStupidChessBattlePrototypeWidget::TrySubmitMoveFromSelection(int32 ToX, int32 ToY)
{
    if (!bHasLiveSnapshot || !bHasSelection)
    {
        return false;
    }

    UStupidChessLocalMatchSubsystem* Subsystem = GetLocalSubsystem();
    if (Subsystem == nullptr)
    {
        SetPrototypeStatus(TEXT("[Move] Local subsystem unavailable"));
        return false;
    }

    EStupidChessSide TurnSide = EStupidChessSide::Red;
    if (!TryGetCurrentTurnSide(TurnSide))
    {
        SetPrototypeStatus(TEXT("[Move] Unknown current turn side"));
        return false;
    }

    const FStupidChessPieceSnapshot* SourcePiece = FindLivePieceAt(SelectedBoardCell.X, SelectedBoardCell.Y);
    if (SourcePiece == nullptr)
    {
        SetPrototypeStatus(TEXT("[Move] Source piece missing"));
        return false;
    }

    if (SourcePiece->Side != static_cast<int32>(TurnSide))
    {
        SetPrototypeStatus(TEXT("[Move] Selected piece no longer matches current turn"));
        return false;
    }

    FStupidChessMoveCommand Move{};
    Move.PieceId = SourcePiece->PieceId;
    Move.FromX = SelectedBoardCell.X;
    Move.FromY = SelectedBoardCell.Y;
    Move.ToX = ToX;
    Move.ToY = ToY;

    if (const FStupidChessPieceSnapshot* TargetPiece = FindLivePieceAt(ToX, ToY))
    {
        if (TargetPiece->Side == SourcePiece->Side)
        {
            SetPrototypeStatus(TEXT("[Move] Target has same-side piece"));
            return false;
        }

        Move.bHasCapturedPieceId = true;
        Move.CapturedPieceId = TargetPiece->PieceId;
    }

    const int64 PlayerId = GetPlayerIdForSide(TurnSide);
    const bool bSubmitOk = Subsystem->SubmitMove(MatchId, PlayerId, TurnSide, Move);
    SetPrototypeStatus(FString::Printf(
        TEXT("[Move] %s P%d (%d,%d)->(%d,%d) Submit=%s"),
        *SideToDebugLabel(static_cast<int32>(TurnSide)),
        Move.PieceId,
        Move.FromX,
        Move.FromY,
        Move.ToX,
        Move.ToY,
        bSubmitOk ? TEXT("OK") : TEXT("Fail")));

    if (bSubmitOk)
    {
        PullBothSides();
    }

    return bSubmitOk;
}

UStupidChessLocalMatchSubsystem* UStupidChessBattlePrototypeWidget::GetLocalSubsystem()
{
    if (CachedSubsystem != nullptr)
    {
        return CachedSubsystem;
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        CachedSubsystem = GameInstance->GetSubsystem<UStupidChessLocalMatchSubsystem>();
        return CachedSubsystem;
    }

    return nullptr;
}

const FStupidChessPieceSnapshot* UStupidChessBattlePrototypeWidget::FindLivePieceAt(int32 X, int32 Y) const
{
    if (!bHasLiveSnapshot)
    {
        return nullptr;
    }

    for (const FStupidChessPieceSnapshot& Piece : LiveSnapshot.Pieces)
    {
        if (Piece.bAlive && Piece.X == X && Piece.Y == Y)
        {
            return &Piece;
        }
    }

    return nullptr;
}

FString UStupidChessBattlePrototypeWidget::MakeCellLabelText(const FStupidChessPieceSnapshot* Piece, bool bSelected) const
{
    FString BaseText = TEXT(".");
    if (Piece != nullptr)
    {
        const FString SidePrefix = Piece->Side == 0 ? TEXT("R") : TEXT("B");
        const int32 SurfaceRoleType = GetCachedSurfaceRoleTypeForPiece(Piece->PieceId, Piece->Side);
        const FString SurfaceRoleLabel = RoleTypeToChineseLabel(Piece->Side, SurfaceRoleType);
        const int32 ActualRoleType = GetActualRoleTypeFromPieceIdForPrototype(Piece->PieceId);
        const FString ActualRoleLabel = RoleTypeToChineseLabel(Piece->Side, ActualRoleType);
        FString Flags;
        if (Piece->bRevealed)
        {
            Flags += TEXT("公");
        }
        if (Piece->bFrozen)
        {
            if (!Flags.IsEmpty())
            {
                Flags += TEXT("|");
            }
            Flags += TEXT("冻");
        }
        if (Flags.IsEmpty())
        {
            Flags = TEXT("-");
        }

        const bool bIsOwnPieceForDisplayedViewer = Piece->Side == static_cast<int32>(DisplayViewerSide);
        const bool bCanSeeActualRoleInStrictView = bIsOwnPieceForDisplayedViewer || Piece->bRevealed;
        const FString ActualRoleForDisplay = (!bStrictPlayerView || bCanSeeActualRoleInStrictView)
            ? ActualRoleLabel
            : TEXT("？");
        const FString RoleText = FString::Printf(TEXT("%s/%s"), *SurfaceRoleLabel, *ActualRoleForDisplay);

        BaseText = FString::Printf(
            TEXT("%s%d\n%s %s"),
            *SidePrefix,
            Piece->PieceId,
            *RoleText,
            *Flags);
    }

    if (bSelected)
    {
        return FString::Printf(TEXT("[%s]"), *BaseText);
    }

    return BaseText;
}

FString UStupidChessBattlePrototypeWidget::PhaseToDebugLabel(int32 Phase)
{
    switch (Phase)
    {
    case 0:
        return TEXT("SetupCommit");
    case 1:
        return TEXT("SetupReveal");
    case 2:
        return TEXT("Battle");
    case 3:
        return TEXT("GameOver");
    default:
        return FString::Printf(TEXT("Unknown(%d)"), Phase);
    }
}

FString UStupidChessBattlePrototypeWidget::SideToDebugLabel(int32 Side)
{
    if (Side == 0)
    {
        return TEXT("Red");
    }
    if (Side == 1)
    {
        return TEXT("Black");
    }
    return FString::Printf(TEXT("Unknown(%d)"), Side);
}

bool UStupidChessBattlePrototypeWidget::TryGetCurrentTurnSide(EStupidChessSide& OutSide) const
{
    if (!bHasLiveSnapshot)
    {
        return false;
    }

    if (LiveSnapshot.CurrentTurn == 0)
    {
        OutSide = EStupidChessSide::Red;
        return true;
    }
    if (LiveSnapshot.CurrentTurn == 1)
    {
        OutSide = EStupidChessSide::Black;
        return true;
    }
    return false;
}

int64 UStupidChessBattlePrototypeWidget::GetPlayerIdForSide(EStupidChessSide Side) const
{
    return Side == EStupidChessSide::Red ? RedPlayerId : BlackPlayerId;
}

void UStupidChessBattlePrototypeWidget::SetPrototypeStatus(const FString& InStatus)
{
    PrototypeStatusText = InStatus;
    RefreshStatusTexts();
}

void UStupidChessBattlePrototypeWidget::SetSelectionStatus(const FString& InStatus)
{
    SelectionStatusText = InStatus;
    RefreshStatusTexts();
}

TArray<FStupidChessSetupPlacement> UStupidChessBattlePrototypeWidget::BuildScrambledSetupPlacements(EStupidChessSide Side) const
{
    UStupidChessLocalMatchSubsystem* Subsystem = CachedSubsystem;
    if (Subsystem == nullptr && const_cast<UStupidChessBattlePrototypeWidget*>(this)->GetGameInstance() != nullptr)
    {
        Subsystem = const_cast<UStupidChessBattlePrototypeWidget*>(this)->GetGameInstance()->GetSubsystem<UStupidChessLocalMatchSubsystem>();
    }

    if (Subsystem == nullptr)
    {
        return {};
    }

    TArray<FStupidChessSetupPlacement> Placements = Subsystem->BuildStandardSetupPlacements(Side);
    if (Placements.Num() <= 1)
    {
        return Placements;
    }

    TArray<FIntPoint> Positions;
    Positions.Reserve(Placements.Num());
    for (const FStupidChessSetupPlacement& Placement : Placements)
    {
        Positions.Add(FIntPoint(Placement.X, Placement.Y));
    }

    const int32 SeedA = static_cast<int32>((MatchId & 0x7fffffff) ^ (Side == EStupidChessSide::Red ? 0x51A3 : 0xA315));
    const int32 SeedB = static_cast<int32>((Subsystem->GetNextClientSequence() & 0x7fffffff) ^ (Side == EStupidChessSide::Red ? 0x1234 : 0x4321));
    FRandomStream RandomStream(SeedA ^ SeedB);
    for (int32 Index = Positions.Num() - 1; Index > 0; --Index)
    {
        const int32 SwapIndex = RandomStream.RandRange(0, Index);
        Positions.Swap(Index, SwapIndex);
    }

    for (int32 Index = 0; Index < Placements.Num(); ++Index)
    {
        Placements[Index].X = Positions[Index].X;
        Placements[Index].Y = Positions[Index].Y;
    }

    return Placements;
}

int32 UStupidChessBattlePrototypeWidget::GetCellIndex(int32 X, int32 Y) const
{
    return Y * BoardWidth + X;
}

UStupidChessBattlePrototypeWidget* UStupidChessBattlePrototypeBlueprintLibrary::ShowBattlePrototypeWidget(
    UObject* WorldContextObject,
    int32 ZOrder)
{
    if (WorldContextObject == nullptr)
    {
        return nullptr;
    }

    UWorld* World = GEngine != nullptr
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
        : nullptr;
    if (World == nullptr)
    {
        return nullptr;
    }

    UStupidChessBattlePrototypeWidget* Widget =
        CreateWidget<UStupidChessBattlePrototypeWidget>(World, UStupidChessBattlePrototypeWidget::StaticClass());
    if (Widget == nullptr)
    {
        return nullptr;
    }

    Widget->AddToViewport(ZOrder);
    return Widget;
}
