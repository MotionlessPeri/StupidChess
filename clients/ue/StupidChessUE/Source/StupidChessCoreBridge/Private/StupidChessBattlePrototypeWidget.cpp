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

    const TArray<FStupidChessSetupPlacement> RedPlacements = bUseScrambledSetup
        ? BuildScrambledSetupPlacements(EStupidChessSide::Red)
        : Subsystem->BuildStandardSetupPlacements(EStupidChessSide::Red);
    const TArray<FStupidChessSetupPlacement> BlackPlacements = bUseScrambledSetup
        ? BuildScrambledSetupPlacements(EStupidChessSide::Black)
        : Subsystem->BuildStandardSetupPlacements(EStupidChessSide::Black);

    const bool bCommitRedOk = Subsystem->SubmitCommitSetup(MatchId, RedPlayerId, EStupidChessSide::Red, TEXT(""));
    const bool bCommitBlackOk = Subsystem->SubmitCommitSetup(MatchId, BlackPlayerId, EStupidChessSide::Black, TEXT(""));
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
    bHasLiveSnapshot = false;
    LiveSnapshot = FStupidChessSnapshotView{};
    AckStatusText = TEXT("[CommandAck] <none>");
    EventDeltaStatusText = TEXT("[EventDelta] <none>");
    GameOverStatusText = TEXT("[GameOver] <none>");
    PrototypeStatusText = TEXT("Idle");
    ClearSelection();
    RefreshAllUi();
}

void UStupidChessBattlePrototypeWidget::HandleBoardCellClicked(int32 X, int32 Y)
{
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
    LiveSnapshot = Snapshot;
    bHasLiveSnapshot = true;
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

        AddToSide(TxtStatus);
        AddToSide(TxtSelection);
        AddToSide(TxtSnapshot);
        AddToSide(TxtEventDelta);
        AddToSide(TxtAck);
        AddToSide(TxtGameOver, 12.0f);
        AddToSide(BtnBootstrap);
        AddToSide(BtnBootstrapScrambled);
        AddToSide(BtnPull);
        AddToSide(BtnPass);
        AddToSide(BtnBlackResign);
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
            const FStupidChessPieceSnapshot* Piece = bHasLiveSnapshot ? FindLivePieceAt(X, Y) : nullptr;
            CellLabel->SetText(FText::FromString(MakeCellLabelText(Piece, bSelected)));

            FLinearColor CellColor = FLinearColor(0.84f, 0.84f, 0.84f);
            if ((X + Y) % 2 == 0)
            {
                CellColor = FLinearColor(0.92f, 0.92f, 0.92f);
            }
            if (Piece != nullptr)
            {
                CellColor = Piece->Side == 0 ? FLinearColor(0.96f, 0.70f, 0.70f) : FLinearColor(0.72f, 0.78f, 0.96f);
                if (Piece->bFrozen)
                {
                    CellColor *= 0.7f;
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
        TxtStatus->SetText(FText::FromString(PrototypeStatusText));
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
}

void UStupidChessBattlePrototypeWidget::ClearSelection()
{
    bHasSelection = false;
    SelectedBoardCell = FIntPoint(-1, -1);
    SetSelectionStatus(TEXT("[Select] <none>"));
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

FString UStupidChessBattlePrototypeWidget::MakeCellLabelText(const FStupidChessPieceSnapshot* Piece, bool bSelected)
{
    FString BaseText = TEXT(".");
    if (Piece != nullptr)
    {
        const FString SidePrefix = Piece->Side == 0 ? TEXT("R") : TEXT("B");
        const FString VisibleRoleLabel = RoleTypeToChineseLabel(Piece->Side, Piece->VisibleRole);
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

        const FString RoleText = FString::Printf(TEXT("%s/%s"), *VisibleRoleLabel, *ActualRoleLabel);

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
