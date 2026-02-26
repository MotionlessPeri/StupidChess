#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "StupidChessLocalMatchSubsystem.h"
#include "StupidChessBattlePrototypeWidget.generated.h"

class UButton;
class UCanvasPanel;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;

class UStupidChessBattlePrototypeWidget;

struct FStupidChessSetupPlacementHistoryEntry
{
    EStupidChessSide Side = EStupidChessSide::Red;
    FStupidChessSetupPlacement Placement{};
};

UCLASS()
class STUPIDCHESSCOREBRIDGE_API UStupidChessBoardCellClickProxy : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UStupidChessBattlePrototypeWidget* InOwnerWidget, int32 InX, int32 InY);

    UFUNCTION()
    void HandleClicked();

private:
    UPROPERTY(Transient)
    TObjectPtr<UStupidChessBattlePrototypeWidget> OwnerWidget = nullptr;

    int32 CellX = 0;
    int32 CellY = 0;
};

UCLASS(BlueprintType, Blueprintable)
class STUPIDCHESSCOREBRIDGE_API UStupidChessBattlePrototypeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void BootstrapBattlePrototypeMatch();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void BootstrapScrambledBattlePrototypeMatch();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void BootstrapSetupPrototypeMatch();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void SubmitSetupPrototypeReveal();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void UndoSetupPrototypePlacement();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void PullBothSides();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void SubmitCurrentTurnPass();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void SubmitBlackResign();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void ResetPrototypeStateOnly();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void ShowRedPlayerView();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void ShowBlackPlayerView();

    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype")
    void ToggleStrictPlayerView();

    UFUNCTION(BlueprintPure, Category = "StupidChess|Prototype")
    bool HasLiveSnapshot() const
    {
        return bHasLiveSnapshot;
    }

    UFUNCTION(BlueprintPure, Category = "StupidChess|Prototype")
    FString GetPrototypeStatusText() const
    {
        return PrototypeStatusText;
    }

    UFUNCTION(BlueprintPure, Category = "StupidChess|Prototype")
    FString GetSelectionStatusText() const
    {
        return SelectionStatusText;
    }

    void HandleBoardCellClicked(int32 X, int32 Y);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StupidChess|Prototype")
    int64 MatchId = 900;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StupidChess|Prototype")
    int64 RedPlayerId = 10001;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StupidChess|Prototype")
    int64 BlackPlayerId = 10002;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StupidChess|Prototype")
    bool bAutoBootstrapOnConstruct = false;

private:
    UFUNCTION()
    void HandleBootstrapButtonClicked();

    UFUNCTION()
    void HandlePullButtonClicked();

    UFUNCTION()
    void HandleBootstrapScrambledButtonClicked();

    UFUNCTION()
    void HandleBootstrapSetupButtonClicked();

    UFUNCTION()
    void HandleSubmitSetupButtonClicked();

    UFUNCTION()
    void HandleUndoSetupButtonClicked();

    UFUNCTION()
    void HandleShowRedViewButtonClicked();

    UFUNCTION()
    void HandleShowBlackViewButtonClicked();

    UFUNCTION()
    void HandleToggleStrictViewButtonClicked();

    UFUNCTION()
    void HandlePassButtonClicked();

    UFUNCTION()
    void HandleBlackResignButtonClicked();

    UFUNCTION()
    void HandleJoinAckParsed(const FStupidChessJoinAckView& JoinAck);

    UFUNCTION()
    void HandleCommandAckParsed(const FStupidChessCommandAckView& CommandAck);

    UFUNCTION()
    void HandleErrorParsed(const FStupidChessErrorView& Error);

    UFUNCTION()
    void HandleSnapshotParsed(const FStupidChessSnapshotView& Snapshot);

    UFUNCTION()
    void HandleEventDeltaParsed(const FStupidChessEventDeltaView& EventDelta);

    UFUNCTION()
    void HandleGameOverParsed(const FStupidChessGameOverView& GameOver);

    void BuildRuntimeWidgetTreeIfNeeded();
    void BuildBoardCells();
    void BindUiButtonEvents();
    void BindSubsystemDelegatesIfNeeded();
    void UnbindSubsystemDelegates();
    void RefreshAllUi();
    void RefreshBoardCells();
    void RefreshStatusTexts();
    void ApplyDisplayedViewerSnapshotIfAvailable();
    void ClearSelection();
    void ResetSetupPrototypeState();
    void RefreshSetupSelectionStatus();
    bool HandleSetupBoardCellClicked(int32 X, int32 Y);
    bool RemoveLastSetupPlacementForSide(EStupidChessSide Side, const FStupidChessSetupPlacement* ExpectedPlacement = nullptr);
    bool TryGetSetupActiveSide(EStupidChessSide& OutSide) const;
    bool TryFindSetupPreviewPieceAt(int32 X, int32 Y, FStupidChessSetupPlacement& OutPlacement, int32& OutSide) const;
    bool IsValidSetupSlot(EStupidChessSide Side, int32 X, int32 Y) const;
    bool IsSetupCellOccupied(EStupidChessSide Side, int32 X, int32 Y) const;
    int32 GetSetupSlotVisibleRole(EStupidChessSide Side, int32 X, int32 Y) const;
    FString MakeSetupCellLabelText(const FStupidChessSetupPlacement& Placement, int32 Side, bool bSelected) const;
    void CacheSurfaceRolesFromPlacements(EStupidChessSide Side, const TArray<FStupidChessSetupPlacement>& Placements);
    int32 GetCachedSurfaceRoleTypeForPiece(int32 PieceId, int32 Side) const;
    bool TrySubmitMoveFromSelection(int32 ToX, int32 ToY);
    UStupidChessLocalMatchSubsystem* GetLocalSubsystem();
    const FStupidChessPieceSnapshot* FindLivePieceAt(int32 X, int32 Y) const;
    FString MakeCellLabelText(const FStupidChessPieceSnapshot* Piece, bool bSelected) const;
    static FString PhaseToDebugLabel(int32 Phase);
    static FString SideToDebugLabel(int32 Side);
    bool TryGetCurrentTurnSide(EStupidChessSide& OutSide) const;
    int64 GetPlayerIdForSide(EStupidChessSide Side) const;
    void SetPrototypeStatus(const FString& InStatus);
    void SetSelectionStatus(const FString& InStatus);
    bool BootstrapBattleInternal(bool bUseScrambledSetup);
    TArray<FStupidChessSetupPlacement> BuildScrambledSetupPlacements(EStupidChessSide Side) const;

    static constexpr int32 BoardWidth = 9;
    static constexpr int32 BoardHeight = 10;
    int32 GetCellIndex(int32 X, int32 Y) const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UUniformGridPanel> BoardGrid = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> SidePanel = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TxtStatus = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TxtSelection = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TxtSnapshot = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TxtEventDelta = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TxtAck = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TxtGameOver = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnBootstrap = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnBootstrapScrambled = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnPull = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnBootstrapSetup = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnSubmitSetup = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnUndoSetup = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnShowRedView = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnShowBlackView = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnToggleStrictView = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnPass = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BtnBlackResign = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UButton>> BoardCellButtons;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> BoardCellLabels;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UStupidChessBoardCellClickProxy>> BoardCellClickProxies;

    UPROPERTY(Transient)
    TObjectPtr<UStupidChessLocalMatchSubsystem> CachedSubsystem = nullptr;

    bool bWidgetTreeBuilt = false;
    bool bUiButtonsBound = false;
    bool bSubsystemDelegatesBound = false;
    bool bSetupPrototypeActive = false;
    bool bSetupPrototypeReadyToSubmit = false;
    bool bHasLiveSnapshot = false;
    bool bHasSnapshotForViewer[2] = {false, false};
    bool bStrictPlayerView = false;
    bool bHasSelection = false;
    FIntPoint SelectedBoardCell = FIntPoint(-1, -1);
    EStupidChessSide DisplayViewerSide = EStupidChessSide::Red;
    EStupidChessSide SetupActiveSide = EStupidChessSide::Red;
    int32 SetupNextPlacementIndexRed = 0;
    int32 SetupNextPlacementIndexBlack = 0;
    TArray<FStupidChessSetupPlacement> SetupStandardPlacementsRed;
    TArray<FStupidChessSetupPlacement> SetupStandardPlacementsBlack;
    TArray<FStupidChessSetupPlacement> SetupPendingPlacementsRed;
    TArray<FStupidChessSetupPlacement> SetupPendingPlacementsBlack;
    TArray<FStupidChessSetupPlacementHistoryEntry> SetupPlacementHistory;
    TMap<int32, int32> PieceSurfaceRoleTypeByPieceId;
    FStupidChessSnapshotView LiveSnapshot{};
    FStupidChessSnapshotView SnapshotByViewer[2]{};
    FString PrototypeStatusText = TEXT("Idle");
    FString SelectionStatusText = TEXT("No selection");
    FString EventDeltaStatusText = TEXT("[EventDelta] <none>");
    FString AckStatusText = TEXT("[CommandAck] <none>");
    FString GameOverStatusText = TEXT("[GameOver] <none>");
};

UCLASS()
class STUPIDCHESSCOREBRIDGE_API UStupidChessBattlePrototypeBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "StupidChess|Prototype", meta = (WorldContext = "WorldContextObject"))
    static UStupidChessBattlePrototypeWidget* ShowBattlePrototypeWidget(UObject* WorldContextObject, int32 ZOrder = 0);
};
