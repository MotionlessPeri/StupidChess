#pragma once

#include "CoreRules/CoreTypes.h"

class FMatchReferee
{
public:
    explicit FMatchReferee(FRuleConfig InRuleConfig = {});

    void ResetNewMatch();
    const FGameState& GetState() const noexcept;

    FCommandResult ApplyCommit(const FSetupCommit& Commit);
    FCommandResult ApplyReveal(const FSetupPlain& SetupPlain);
    FCommandResult ApplyCommand(const FPlayerCommand& Command);

    std::vector<FMoveAction> GenerateLegalMoves(ESide Side) const;
    bool CanPass(ESide Side) const;

private:
    static int32_t ToCellIndex(const FBoardPos& Pos) noexcept;
    static ESide GetOppositeSide(ESide Side) noexcept;

    const FPieceState* FindPieceById(FPieceId PieceId) const noexcept;
    FPieceState* FindPieceById(FPieceId PieceId) noexcept;

    const FPieceState* GetPieceAt(const FBoardPos& Pos) const noexcept;
    FPieceState* GetPieceAt(const FBoardPos& Pos) noexcept;

    bool IsInsidePalace(ESide Side, const FBoardPos& Pos) const noexcept;
    bool IsAdvisorPoint(ESide Side, const FBoardPos& Pos) const noexcept;
    bool IsElephantPoint(ESide Side, const FBoardPos& Pos) const noexcept;
    bool IsCrossedRiver(ESide Side, const FBoardPos& Pos) const noexcept;
    ERoleType GetInitialSurfaceRoleForPos(ESide Side, const FBoardPos& Pos) const;
    bool IsSetupPositionAllowed(ESide Side, const FBoardPos& Pos) const;
    bool IsPieceIdOwnedBySide(FPieceId PieceId, ESide Side) const noexcept;
    ERoleType GetActualRoleForPieceId(FPieceId PieceId) const;

    bool IsRolePositionLegal(ERoleType Role, ESide Side, const FBoardPos& Pos) const noexcept;
    ERoleType GetActiveRole(const FPieceState& Piece) const noexcept;
    bool IsPathClearStraight(const FBoardPos& From, const FBoardPos& To) const noexcept;
    int32_t CountPiecesBetweenStraight(const FBoardPos& From, const FBoardPos& To) const noexcept;

    std::vector<FMoveAction> GeneratePseudoMovesForPiece(const FPieceState& Piece) const;
    bool IsSquareAttackedBySide(const FBoardPos& Target, ESide AttackerSide) const;
    bool CanPieceAttackSquare(const FPieceState& Piece, const FBoardPos& Target) const;
    bool AreKingsFacing() const;
    std::optional<FBoardPos> FindKingPos(ESide Side) const;
    bool IsSideInCheck(ESide Side) const;

    void ApplyMoveUnchecked(const FMoveAction& Move);
    bool IsMoveLegalForSide(const FMoveAction& Move, ESide Side) const;
    void EvaluateEndAfterMove(ESide MovedSide);

    std::string BuildRevealDigest(const FSetupPlain& SetupPlain) const;
    bool ValidateSetupPlain(const FSetupPlain& SetupPlain, std::string& OutError) const;
    FCommandResult ApplyRevealPlacement(const FSetupPlain& SetupPlain);
    void InitializePieceRoster();

    FRuleConfig RuleConfig;
    FGameState GameState;
    std::string RedCommitHash;
    std::string BlackCommitHash;
    bool bHasRedCommit = false;
    bool bHasBlackCommit = false;
};
