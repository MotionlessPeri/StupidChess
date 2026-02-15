#include "CoreRules/MatchReferee.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
struct FSetupSlot
{
    FBoardPos Pos{};
    ERoleType SurfaceRole = ERoleType::Pawn;
};

constexpr std::array<FSetupSlot, 16> RedSetupSlots = {{
    {{0, 0}, ERoleType::Rook},
    {{1, 0}, ERoleType::Horse},
    {{2, 0}, ERoleType::Elephant},
    {{3, 0}, ERoleType::Advisor},
    {{4, 0}, ERoleType::King},
    {{5, 0}, ERoleType::Advisor},
    {{6, 0}, ERoleType::Elephant},
    {{7, 0}, ERoleType::Horse},
    {{8, 0}, ERoleType::Rook},
    {{1, 2}, ERoleType::Cannon},
    {{7, 2}, ERoleType::Cannon},
    {{0, 3}, ERoleType::Pawn},
    {{2, 3}, ERoleType::Pawn},
    {{4, 3}, ERoleType::Pawn},
    {{6, 3}, ERoleType::Pawn},
    {{8, 3}, ERoleType::Pawn},
}};

constexpr uint64_t FnvOffset = 1469598103934665603ull;
constexpr uint64_t FnvPrime = 1099511628211ull;

FCommandResult BuildAcceptedResult()
{
    return FCommandResult{true, {}, {}};
}

FCommandResult BuildRejectedResult(std::string ErrorCode, std::string ErrorMessage)
{
    return FCommandResult{false, std::move(ErrorCode), std::move(ErrorMessage)};
}
}

int32_t FMatchReferee::ToCellIndex(const FBoardPos& Pos) noexcept
{
    return static_cast<int32_t>(Pos.Y) * 9 + static_cast<int32_t>(Pos.X);
}

ESide FMatchReferee::GetOppositeSide(ESide Side) noexcept
{
    return Side == ESide::Red ? ESide::Black : ESide::Red;
}

FMatchReferee::FMatchReferee(FRuleConfig InRuleConfig)
    : RuleConfig(InRuleConfig)
{
    ResetNewMatch();
}

void FMatchReferee::InitializePieceRoster()
{
    GameState.Pieces.clear();
    GameState.Pieces.resize(32);

    for (int32_t PieceIdValue = 0; PieceIdValue < static_cast<int32_t>(GameState.Pieces.size()); ++PieceIdValue)
    {
        const FPieceId PieceId = static_cast<FPieceId>(PieceIdValue);
        FPieceState& Piece = GameState.Pieces[PieceIdValue];
        Piece.PieceId = PieceId;
        Piece.Side = PieceIdValue < 16 ? ESide::Red : ESide::Black;
        Piece.ActualRole = GetActualRoleForPieceId(PieceId);
        Piece.SurfaceRole = Piece.ActualRole;
        Piece.PieceState = EPieceState::HiddenSurface;
        Piece.Pos = FBoardPos{};
        Piece.bAlive = false;
        Piece.bFrozen = false;
        Piece.bHasCaptured = false;
    }
}

void FMatchReferee::ResetNewMatch()
{
    GameState = FGameState{};
    GameState.Phase = EGamePhase::SetupCommit;
    GameState.CurrentTurn = ESide::Red;
    GameState.Result = EGameResult::Ongoing;
    GameState.EndReason = EEndReason::None;
    GameState.PassCount = 0;
    GameState.TurnIndex = 0;
    GameState.BoardCells.fill(std::nullopt);

    RedCommitHash.clear();
    BlackCommitHash.clear();
    bHasRedCommit = false;
    bHasBlackCommit = false;

    InitializePieceRoster();
}

const FGameState& FMatchReferee::GetState() const noexcept
{
    return GameState;
}

const FPieceState* FMatchReferee::FindPieceById(FPieceId PieceId) const noexcept
{
    const int32_t Index = static_cast<int32_t>(PieceId);
    if (Index < 0 || Index >= static_cast<int32_t>(GameState.Pieces.size()))
    {
        return nullptr;
    }
    return &GameState.Pieces[Index];
}

FPieceState* FMatchReferee::FindPieceById(FPieceId PieceId) noexcept
{
    const int32_t Index = static_cast<int32_t>(PieceId);
    if (Index < 0 || Index >= static_cast<int32_t>(GameState.Pieces.size()))
    {
        return nullptr;
    }
    return &GameState.Pieces[Index];
}

const FPieceState* FMatchReferee::GetPieceAt(const FBoardPos& Pos) const noexcept
{
    if (!Pos.IsValid())
    {
        return nullptr;
    }

    const std::optional<FPieceId>& Cell = GameState.BoardCells[ToCellIndex(Pos)];
    if (!Cell.has_value())
    {
        return nullptr;
    }

    return FindPieceById(Cell.value());
}

FPieceState* FMatchReferee::GetPieceAt(const FBoardPos& Pos) noexcept
{
    if (!Pos.IsValid())
    {
        return nullptr;
    }

    const std::optional<FPieceId>& Cell = GameState.BoardCells[ToCellIndex(Pos)];
    if (!Cell.has_value())
    {
        return nullptr;
    }

    return FindPieceById(Cell.value());
}

bool FMatchReferee::IsInsidePalace(ESide Side, const FBoardPos& Pos) const noexcept
{
    if (!Pos.IsValid() || Pos.X < 3 || Pos.X > 5)
    {
        return false;
    }

    if (Side == ESide::Red)
    {
        return Pos.Y >= 0 && Pos.Y <= 2;
    }

    return Pos.Y >= 7 && Pos.Y <= 9;
}

bool FMatchReferee::IsAdvisorPoint(ESide Side, const FBoardPos& Pos) const noexcept
{
    if (!IsInsidePalace(Side, Pos))
    {
        return false;
    }

    static constexpr std::array<FBoardPos, 5> RedAdvisorPoints = {{{3, 0}, {5, 0}, {4, 1}, {3, 2}, {5, 2}}};

    for (const FBoardPos& RedPoint : RedAdvisorPoints)
    {
        const FBoardPos Candidate = Side == ESide::Red ? RedPoint : FBoardPos{RedPoint.X, static_cast<int8_t>(9 - RedPoint.Y)};
        if (Candidate == Pos)
        {
            return true;
        }
    }

    return false;
}

bool FMatchReferee::IsElephantPoint(ESide Side, const FBoardPos& Pos) const noexcept
{
    if (!Pos.IsValid())
    {
        return false;
    }

    static constexpr std::array<FBoardPos, 7> RedElephantPoints = {{{2, 0}, {6, 0}, {0, 2}, {4, 2}, {8, 2}, {2, 4}, {6, 4}}};

    for (const FBoardPos& RedPoint : RedElephantPoints)
    {
        const FBoardPos Candidate = Side == ESide::Red ? RedPoint : FBoardPos{RedPoint.X, static_cast<int8_t>(9 - RedPoint.Y)};
        if (Candidate == Pos)
        {
            return true;
        }
    }

    return false;
}

bool FMatchReferee::IsCrossedRiver(ESide Side, const FBoardPos& Pos) const noexcept
{
    if (Side == ESide::Red)
    {
        return Pos.Y >= 5;
    }
    return Pos.Y <= 4;
}

ERoleType FMatchReferee::GetInitialSurfaceRoleForPos(ESide Side, const FBoardPos& Pos) const
{
    for (const FSetupSlot& RedSlot : RedSetupSlots)
    {
        const FBoardPos CandidatePos = Side == ESide::Red ? RedSlot.Pos : FBoardPos{RedSlot.Pos.X, static_cast<int8_t>(9 - RedSlot.Pos.Y)};
        if (CandidatePos == Pos)
        {
            return RedSlot.SurfaceRole;
        }
    }

    return ERoleType::Pawn;
}

bool FMatchReferee::IsSetupPositionAllowed(ESide Side, const FBoardPos& Pos) const
{
    for (const FSetupSlot& RedSlot : RedSetupSlots)
    {
        const FBoardPos CandidatePos = Side == ESide::Red ? RedSlot.Pos : FBoardPos{RedSlot.Pos.X, static_cast<int8_t>(9 - RedSlot.Pos.Y)};
        if (CandidatePos == Pos)
        {
            return true;
        }
    }
    return false;
}

bool FMatchReferee::IsPieceIdOwnedBySide(FPieceId PieceId, ESide Side) const noexcept
{
    if (Side == ESide::Red)
    {
        return PieceId < 16;
    }
    return PieceId >= 16 && PieceId < 32;
}

ERoleType FMatchReferee::GetActualRoleForPieceId(FPieceId PieceId) const
{
    const int32_t LocalIndex = static_cast<int32_t>(PieceId % 16);
    switch (LocalIndex)
    {
    case 0:
    case 8:
        return ERoleType::Rook;
    case 1:
    case 7:
        return ERoleType::Horse;
    case 2:
    case 6:
        return ERoleType::Elephant;
    case 3:
    case 5:
        return ERoleType::Advisor;
    case 4:
        return ERoleType::King;
    case 9:
    case 10:
        return ERoleType::Cannon;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        return ERoleType::Pawn;
    default:
        return ERoleType::Pawn;
    }
}

bool FMatchReferee::IsRolePositionLegal(ERoleType Role, ESide Side, const FBoardPos& Pos) const noexcept
{
    if (!Pos.IsValid())
    {
        return false;
    }

    switch (Role)
    {
    case ERoleType::King:
        return IsInsidePalace(Side, Pos);
    case ERoleType::Advisor:
        return IsAdvisorPoint(Side, Pos);
    case ERoleType::Elephant:
        return IsElephantPoint(Side, Pos);
    default:
        return true;
    }
}

ERoleType FMatchReferee::GetActiveRole(const FPieceState& Piece) const noexcept
{
    return Piece.PieceState == EPieceState::HiddenSurface ? Piece.SurfaceRole : Piece.ActualRole;
}

bool FMatchReferee::IsPathClearStraight(const FBoardPos& From, const FBoardPos& To) const noexcept
{
    return CountPiecesBetweenStraight(From, To) == 0;
}

int32_t FMatchReferee::CountPiecesBetweenStraight(const FBoardPos& From, const FBoardPos& To) const noexcept
{
    if (From.X != To.X && From.Y != To.Y)
    {
        return -1;
    }

    const int32_t StepX = To.X == From.X ? 0 : (To.X > From.X ? 1 : -1);
    const int32_t StepY = To.Y == From.Y ? 0 : (To.Y > From.Y ? 1 : -1);
    int32_t CursorX = static_cast<int32_t>(From.X) + StepX;
    int32_t CursorY = static_cast<int32_t>(From.Y) + StepY;
    int32_t Count = 0;

    while (CursorX != To.X || CursorY != To.Y)
    {
        const FBoardPos CursorPos{static_cast<int8_t>(CursorX), static_cast<int8_t>(CursorY)};
        if (GetPieceAt(CursorPos) != nullptr)
        {
            ++Count;
        }
        CursorX += StepX;
        CursorY += StepY;
    }

    return Count;
}

std::vector<FMoveAction> FMatchReferee::GeneratePseudoMovesForPiece(const FPieceState& Piece) const
{
    std::vector<FMoveAction> Moves;
    if (!Piece.bAlive || Piece.bFrozen || !Piece.Pos.IsValid())
    {
        return Moves;
    }

    auto TryAddMove = [this, &Piece, &Moves](const FBoardPos& To, bool bCaptureOnly, bool bMoveOnly) {
        if (!To.IsValid())
        {
            return;
        }

        const FPieceState* Occupant = GetPieceAt(To);
        if (Occupant == nullptr)
        {
            if (!bCaptureOnly)
            {
                Moves.push_back(FMoveAction{Piece.PieceId, Piece.Pos, To, std::nullopt});
            }
            return;
        }

        if (Occupant->Side == Piece.Side || bMoveOnly)
        {
            return;
        }

        Moves.push_back(FMoveAction{Piece.PieceId, Piece.Pos, To, Occupant->PieceId});
    };

    const ERoleType ActiveRole = GetActiveRole(Piece);
    switch (ActiveRole)
    {
    case ERoleType::King:
    {
        static constexpr std::array<FBoardPos, 4> Delta = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
        for (const FBoardPos& D : Delta)
        {
            const FBoardPos To{static_cast<int8_t>(Piece.Pos.X + D.X), static_cast<int8_t>(Piece.Pos.Y + D.Y)};
            if (IsInsidePalace(Piece.Side, To))
            {
                TryAddMove(To, false, false);
            }
        }
        break;
    }
    case ERoleType::Advisor:
    {
        static constexpr std::array<FBoardPos, 4> Delta = {{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};
        for (const FBoardPos& D : Delta)
        {
            const FBoardPos To{static_cast<int8_t>(Piece.Pos.X + D.X), static_cast<int8_t>(Piece.Pos.Y + D.Y)};
            if (IsAdvisorPoint(Piece.Side, To))
            {
                TryAddMove(To, false, false);
            }
        }
        break;
    }
    case ERoleType::Elephant:
    {
        static constexpr std::array<FBoardPos, 4> Delta = {{{2, 2}, {2, -2}, {-2, 2}, {-2, -2}}};
        for (const FBoardPos& D : Delta)
        {
            const FBoardPos Eye{static_cast<int8_t>(Piece.Pos.X + D.X / 2), static_cast<int8_t>(Piece.Pos.Y + D.Y / 2)};
            const FBoardPos To{static_cast<int8_t>(Piece.Pos.X + D.X), static_cast<int8_t>(Piece.Pos.Y + D.Y)};
            if (!To.IsValid())
            {
                continue;
            }
            if (Piece.Side == ESide::Red && To.Y > 4)
            {
                continue;
            }
            if (Piece.Side == ESide::Black && To.Y < 5)
            {
                continue;
            }
            if (GetPieceAt(Eye) != nullptr)
            {
                continue;
            }
            TryAddMove(To, false, false);
        }
        break;
    }
    case ERoleType::Horse:
    {
        struct FHorsePattern
        {
            FBoardPos Leg;
            FBoardPos To;
        };
        static constexpr std::array<FHorsePattern, 8> Patterns = {{
            {{1, 0}, {2, 1}},
            {{1, 0}, {2, -1}},
            {{-1, 0}, {-2, 1}},
            {{-1, 0}, {-2, -1}},
            {{0, 1}, {1, 2}},
            {{0, 1}, {-1, 2}},
            {{0, -1}, {1, -2}},
            {{0, -1}, {-1, -2}},
        }};

        for (const FHorsePattern& Pattern : Patterns)
        {
            const FBoardPos LegPos{static_cast<int8_t>(Piece.Pos.X + Pattern.Leg.X), static_cast<int8_t>(Piece.Pos.Y + Pattern.Leg.Y)};
            if (!LegPos.IsValid() || GetPieceAt(LegPos) != nullptr)
            {
                continue;
            }
            const FBoardPos To{static_cast<int8_t>(Piece.Pos.X + Pattern.To.X), static_cast<int8_t>(Piece.Pos.Y + Pattern.To.Y)};
            TryAddMove(To, false, false);
        }
        break;
    }
    case ERoleType::Rook:
    {
        static constexpr std::array<FBoardPos, 4> Delta = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
        for (const FBoardPos& D : Delta)
        {
            int32_t CursorX = Piece.Pos.X + D.X;
            int32_t CursorY = Piece.Pos.Y + D.Y;
            while (CursorX >= 0 && CursorX < 9 && CursorY >= 0 && CursorY < 10)
            {
                const FBoardPos To{static_cast<int8_t>(CursorX), static_cast<int8_t>(CursorY)};
                const FPieceState* Occupant = GetPieceAt(To);
                if (Occupant == nullptr)
                {
                    Moves.push_back(FMoveAction{Piece.PieceId, Piece.Pos, To, std::nullopt});
                }
                else
                {
                    if (Occupant->Side != Piece.Side)
                    {
                        Moves.push_back(FMoveAction{Piece.PieceId, Piece.Pos, To, Occupant->PieceId});
                    }
                    break;
                }
                CursorX += D.X;
                CursorY += D.Y;
            }
        }
        break;
    }
    case ERoleType::Cannon:
    {
        static constexpr std::array<FBoardPos, 4> Delta = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
        for (const FBoardPos& D : Delta)
        {
            int32_t CursorX = Piece.Pos.X + D.X;
            int32_t CursorY = Piece.Pos.Y + D.Y;
            bool bScreenFound = false;
            while (CursorX >= 0 && CursorX < 9 && CursorY >= 0 && CursorY < 10)
            {
                const FBoardPos To{static_cast<int8_t>(CursorX), static_cast<int8_t>(CursorY)};
                const FPieceState* Occupant = GetPieceAt(To);

                if (!bScreenFound)
                {
                    if (Occupant == nullptr)
                    {
                        Moves.push_back(FMoveAction{Piece.PieceId, Piece.Pos, To, std::nullopt});
                    }
                    else
                    {
                        bScreenFound = true;
                    }
                }
                else if (Occupant != nullptr)
                {
                    if (Occupant->Side != Piece.Side)
                    {
                        Moves.push_back(FMoveAction{Piece.PieceId, Piece.Pos, To, Occupant->PieceId});
                    }
                    break;
                }

                CursorX += D.X;
                CursorY += D.Y;
            }
        }
        break;
    }
    case ERoleType::Pawn:
    {
        const int8_t ForwardY = Piece.Side == ESide::Red ? 1 : -1;
        const FBoardPos Forward{Piece.Pos.X, static_cast<int8_t>(Piece.Pos.Y + ForwardY)};
        TryAddMove(Forward, false, false);

        if (IsCrossedRiver(Piece.Side, Piece.Pos))
        {
            const FBoardPos Left{static_cast<int8_t>(Piece.Pos.X - 1), Piece.Pos.Y};
            const FBoardPos Right{static_cast<int8_t>(Piece.Pos.X + 1), Piece.Pos.Y};
            TryAddMove(Left, false, false);
            TryAddMove(Right, false, false);
        }
        break;
    }
    }

    return Moves;
}

bool FMatchReferee::CanPieceAttackSquare(const FPieceState& Piece, const FBoardPos& Target) const
{
    if (!Piece.bAlive || Piece.bFrozen || !Target.IsValid())
    {
        return false;
    }

    const FPieceState* TargetPiece = GetPieceAt(Target);
    if (TargetPiece == nullptr || TargetPiece->Side == Piece.Side)
    {
        return false;
    }

    const std::vector<FMoveAction> PseudoMoves = GeneratePseudoMovesForPiece(Piece);
    for (const FMoveAction& Move : PseudoMoves)
    {
        if (Move.To == Target && Move.CapturedPieceId.has_value())
        {
            return true;
        }
    }

    return false;
}

bool FMatchReferee::IsSquareAttackedBySide(const FBoardPos& Target, ESide AttackerSide) const
{
    for (const FPieceState& Piece : GameState.Pieces)
    {
        if (!Piece.bAlive || Piece.Side != AttackerSide)
        {
            continue;
        }

        if (CanPieceAttackSquare(Piece, Target))
        {
            return true;
        }
    }

    return false;
}

std::optional<FBoardPos> FMatchReferee::FindKingPos(ESide Side) const
{
    for (const FPieceState& Piece : GameState.Pieces)
    {
        if (Piece.bAlive && Piece.Side == Side && Piece.ActualRole == ERoleType::King)
        {
            return Piece.Pos;
        }
    }
    return std::nullopt;
}

bool FMatchReferee::AreKingsFacing() const
{
    const std::optional<FBoardPos> RedKingPos = FindKingPos(ESide::Red);
    const std::optional<FBoardPos> BlackKingPos = FindKingPos(ESide::Black);
    if (!RedKingPos.has_value() || !BlackKingPos.has_value())
    {
        return false;
    }

    if (RedKingPos->X != BlackKingPos->X)
    {
        return false;
    }

    return CountPiecesBetweenStraight(RedKingPos.value(), BlackKingPos.value()) == 0;
}

bool FMatchReferee::IsSideInCheck(ESide Side) const
{
    const std::optional<FBoardPos> KingPos = FindKingPos(Side);
    if (!KingPos.has_value())
    {
        return true;
    }

    if (AreKingsFacing())
    {
        return true;
    }

    return IsSquareAttackedBySide(KingPos.value(), GetOppositeSide(Side));
}

void FMatchReferee::ApplyMoveUnchecked(const FMoveAction& Move)
{
    FPieceState* MovingPiece = FindPieceById(Move.PieceId);
    if (MovingPiece == nullptr)
    {
        return;
    }

    std::optional<FPieceId>& FromCell = GameState.BoardCells[ToCellIndex(Move.From)];
    std::optional<FPieceId>& ToCell = GameState.BoardCells[ToCellIndex(Move.To)];

    if (ToCell.has_value())
    {
        FPieceState* CapturedPiece = FindPieceById(ToCell.value());
        if (CapturedPiece != nullptr)
        {
            CapturedPiece->bAlive = false;
            CapturedPiece->Pos = FBoardPos{};
            CapturedPiece->bFrozen = false;
        }
    }

    ToCell = Move.PieceId;
    FromCell = std::nullopt;
    MovingPiece->Pos = Move.To;
}

bool FMatchReferee::IsMoveLegalForSide(const FMoveAction& Move, ESide Side) const
{
    FMatchReferee Simulation = *this;
    const FPieceState* Piece = Simulation.FindPieceById(Move.PieceId);
    if (Piece == nullptr || !Piece->bAlive || Piece->Side != Side)
    {
        return false;
    }

    Simulation.ApplyMoveUnchecked(Move);
    return !Simulation.IsSideInCheck(Side);
}

std::string FMatchReferee::BuildRevealDigest(const FSetupPlain& SetupPlain) const
{
    std::vector<FSetupPlacement> SortedPlacements = SetupPlain.Placements;
    std::sort(SortedPlacements.begin(), SortedPlacements.end(), [](const FSetupPlacement& Lhs, const FSetupPlacement& Rhs) {
        return Lhs.PieceId < Rhs.PieceId;
    });

    uint64_t Hash = FnvOffset;
    auto MixString = [&Hash](const std::string& Value) {
        for (unsigned char C : Value)
        {
            Hash ^= static_cast<uint64_t>(C);
            Hash *= FnvPrime;
        }
    };
    auto MixInt = [&MixString](int32_t Value) {
        MixString(std::to_string(Value));
        MixString("|");
    };

    MixInt(static_cast<int32_t>(SetupPlain.Side));
    MixString(SetupPlain.Nonce);
    MixString("|");
    for (const FSetupPlacement& Placement : SortedPlacements)
    {
        MixInt(static_cast<int32_t>(Placement.PieceId));
        MixInt(Placement.TargetPos.X);
        MixInt(Placement.TargetPos.Y);
    }

    std::ostringstream Stream;
    Stream << std::hex << std::setfill('0') << std::setw(16) << Hash;
    return Stream.str();
}

bool FMatchReferee::ValidateSetupPlain(const FSetupPlain& SetupPlain, std::string& OutError) const
{
    if (SetupPlain.Placements.size() != 16)
    {
        OutError = "Reveal must include exactly 16 placements.";
        return false;
    }

    std::array<bool, 32> SeenPieceIds{};
    std::array<bool, 90> SeenPositions{};
    SeenPieceIds.fill(false);
    SeenPositions.fill(false);

    for (const FSetupPlacement& Placement : SetupPlain.Placements)
    {
        if (!IsPieceIdOwnedBySide(Placement.PieceId, SetupPlain.Side))
        {
            OutError = "Placement contains piece id that does not belong to side.";
            return false;
        }
        if (!Placement.TargetPos.IsValid() || !IsSetupPositionAllowed(SetupPlain.Side, Placement.TargetPos))
        {
            OutError = "Placement target position is not allowed.";
            return false;
        }

        const int32_t PieceIdIndex = static_cast<int32_t>(Placement.PieceId);
        if (SeenPieceIds[PieceIdIndex])
        {
            OutError = "Placement contains duplicated piece id.";
            return false;
        }
        SeenPieceIds[PieceIdIndex] = true;

        const int32_t PosIndex = ToCellIndex(Placement.TargetPos);
        if (SeenPositions[PosIndex])
        {
            OutError = "Placement contains duplicated target position.";
            return false;
        }
        SeenPositions[PosIndex] = true;
    }

    const int32_t StartId = SetupPlain.Side == ESide::Red ? 0 : 16;
    const int32_t EndId = SetupPlain.Side == ESide::Red ? 16 : 32;
    for (int32_t PieceIdValue = StartId; PieceIdValue < EndId; ++PieceIdValue)
    {
        if (!SeenPieceIds[PieceIdValue])
        {
            OutError = "Reveal is missing required piece id.";
            return false;
        }
    }

    return true;
}

FCommandResult FMatchReferee::ApplyRevealPlacement(const FSetupPlain& SetupPlain)
{
    for (FPieceState& Piece : GameState.Pieces)
    {
        if (Piece.Side != SetupPlain.Side)
        {
            continue;
        }

        if (Piece.Pos.IsValid())
        {
            GameState.BoardCells[ToCellIndex(Piece.Pos)] = std::nullopt;
        }

        Piece.PieceState = EPieceState::HiddenSurface;
        Piece.SurfaceRole = Piece.ActualRole;
        Piece.Pos = FBoardPos{};
        Piece.bAlive = false;
        Piece.bFrozen = false;
        Piece.bHasCaptured = false;
    }

    for (const FSetupPlacement& Placement : SetupPlain.Placements)
    {
        FPieceState* Piece = FindPieceById(Placement.PieceId);
        if (Piece == nullptr)
        {
            return BuildRejectedResult("ERR_INVALID_PIECE_ID", "Piece id not found.");
        }

        Piece->Pos = Placement.TargetPos;
        Piece->SurfaceRole = GetInitialSurfaceRoleForPos(SetupPlain.Side, Placement.TargetPos);
        Piece->PieceState = EPieceState::HiddenSurface;
        Piece->bAlive = true;
        Piece->bFrozen = false;
        Piece->bHasCaptured = false;

        std::optional<FPieceId>& Cell = GameState.BoardCells[ToCellIndex(Placement.TargetPos)];
        if (Cell.has_value())
        {
            return BuildRejectedResult("ERR_POSITION_CONFLICT", "Placement position conflicts with existing piece.");
        }
        Cell = Piece->PieceId;
    }

    return BuildAcceptedResult();
}

FCommandResult FMatchReferee::ApplyCommit(const FSetupCommit& Commit)
{
    if (GameState.Phase != EGamePhase::SetupCommit)
    {
        return BuildRejectedResult("ERR_INVALID_PHASE", "Commit is only allowed in SetupCommit phase.");
    }

    if (Commit.Side == ESide::Red)
    {
        if (GameState.bRedCommitted)
        {
            return BuildRejectedResult("ERR_DUPLICATE_COMMIT", "Red side already committed.");
        }
        GameState.bRedCommitted = true;
        bHasRedCommit = true;
        RedCommitHash = Commit.HashHex;
    }
    else
    {
        if (GameState.bBlackCommitted)
        {
            return BuildRejectedResult("ERR_DUPLICATE_COMMIT", "Black side already committed.");
        }
        GameState.bBlackCommitted = true;
        bHasBlackCommit = true;
        BlackCommitHash = Commit.HashHex;
    }

    if (GameState.bRedCommitted && GameState.bBlackCommitted)
    {
        GameState.Phase = EGamePhase::SetupReveal;
    }

    return BuildAcceptedResult();
}

FCommandResult FMatchReferee::ApplyReveal(const FSetupPlain& SetupPlain)
{
    if (GameState.Phase != EGamePhase::SetupReveal)
    {
        return BuildRejectedResult("ERR_INVALID_PHASE", "Reveal is only allowed in SetupReveal phase.");
    }

    const bool bIsRed = SetupPlain.Side == ESide::Red;
    if (bIsRed ? GameState.bRedRevealed : GameState.bBlackRevealed)
    {
        return BuildRejectedResult("ERR_DUPLICATE_REVEAL", "Side already revealed.");
    }

    if (bIsRed ? !bHasRedCommit : !bHasBlackCommit)
    {
        return BuildRejectedResult("ERR_MISSING_COMMIT", "Reveal requires prior commit.");
    }

    const std::string StoredHash = bIsRed ? RedCommitHash : BlackCommitHash;
    if (!StoredHash.empty())
    {
        const std::string ComputedHash = BuildRevealDigest(SetupPlain);
        if (ComputedHash != StoredHash)
        {
            return BuildRejectedResult("ERR_COMMIT_MISMATCH", "Reveal payload does not match commit hash.");
        }
    }

    std::string ValidationError;
    if (!ValidateSetupPlain(SetupPlain, ValidationError))
    {
        return BuildRejectedResult("ERR_INVALID_REVEAL", ValidationError);
    }

    const FCommandResult PlacementResult = ApplyRevealPlacement(SetupPlain);
    if (!PlacementResult.bAccepted)
    {
        return PlacementResult;
    }

    if (bIsRed)
    {
        GameState.bRedRevealed = true;
    }
    else
    {
        GameState.bBlackRevealed = true;
    }

    if (GameState.bRedRevealed && GameState.bBlackRevealed)
    {
        GameState.Phase = EGamePhase::Battle;
        GameState.CurrentTurn = ESide::Red;
    }

    return BuildAcceptedResult();
}

std::vector<FMoveAction> FMatchReferee::GenerateLegalMoves(ESide Side) const
{
    if (GameState.Phase != EGamePhase::Battle || GameState.Result != EGameResult::Ongoing)
    {
        return {};
    }

    std::vector<FMoveAction> LegalMoves;
    for (const FPieceState& Piece : GameState.Pieces)
    {
        if (!Piece.bAlive || Piece.Side != Side || Piece.bFrozen)
        {
            continue;
        }

        const std::vector<FMoveAction> CandidateMoves = GeneratePseudoMovesForPiece(Piece);
        for (const FMoveAction& Candidate : CandidateMoves)
        {
            if (IsMoveLegalForSide(Candidate, Side))
            {
                LegalMoves.push_back(Candidate);
            }
        }
    }

    return LegalMoves;
}

bool FMatchReferee::CanPass(ESide Side) const
{
    if (!RuleConfig.bAllowPassWhenNoLegalMove)
    {
        return false;
    }

    if (GameState.Phase != EGamePhase::Battle || GameState.Result != EGameResult::Ongoing)
    {
        return false;
    }

    if (Side != GameState.CurrentTurn)
    {
        return false;
    }

    if (IsSideInCheck(Side))
    {
        return false;
    }

    return GenerateLegalMoves(Side).empty();
}

void FMatchReferee::EvaluateEndAfterMove(ESide MovedSide)
{
    const ESide DefenderSide = GetOppositeSide(MovedSide);
    if (!FindKingPos(DefenderSide).has_value())
    {
        GameState.Result = MovedSide == ESide::Red ? EGameResult::RedWin : EGameResult::BlackWin;
        GameState.EndReason = EEndReason::Checkmate;
        GameState.Phase = EGamePhase::GameOver;
        return;
    }

    if (IsSideInCheck(DefenderSide))
    {
        const std::vector<FMoveAction> DefenderMoves = GenerateLegalMoves(DefenderSide);
        if (DefenderMoves.empty())
        {
            GameState.Result = MovedSide == ESide::Red ? EGameResult::RedWin : EGameResult::BlackWin;
            GameState.EndReason = EEndReason::Checkmate;
            GameState.Phase = EGamePhase::GameOver;
        }
    }
}

FCommandResult FMatchReferee::ApplyCommand(const FPlayerCommand& Command)
{
    if (GameState.Phase != EGamePhase::Battle)
    {
        return BuildRejectedResult("ERR_INVALID_PHASE", "Battle command is not allowed in current phase.");
    }

    if (GameState.Result != EGameResult::Ongoing)
    {
        return BuildRejectedResult("ERR_GAME_OVER", "Game already ended.");
    }

    if (Command.Side != GameState.CurrentTurn)
    {
        return BuildRejectedResult("ERR_NOT_YOUR_TURN", "It is not the player's turn.");
    }

    switch (Command.CommandType)
    {
    case ECommandType::Pass:
    {
        if (!CanPass(Command.Side))
        {
            return BuildRejectedResult("ERR_PASS_NOT_ALLOWED", "Pass is not allowed now.");
        }

        ++GameState.PassCount;
        ++GameState.TurnIndex;

        if (RuleConfig.bDoublePassIsDraw && GameState.PassCount >= 2)
        {
            GameState.Result = EGameResult::Draw;
            GameState.EndReason = EEndReason::DoublePassDraw;
            GameState.Phase = EGamePhase::GameOver;
        }
        else
        {
            GameState.CurrentTurn = GetOppositeSide(GameState.CurrentTurn);
        }
        return BuildAcceptedResult();
    }
    case ECommandType::Resign:
    {
        GameState.Result = Command.Side == ESide::Red ? EGameResult::BlackWin : EGameResult::RedWin;
        GameState.EndReason = EEndReason::Resign;
        GameState.Phase = EGamePhase::GameOver;
        ++GameState.TurnIndex;
        return BuildAcceptedResult();
    }
    case ECommandType::Move:
    {
        if (!Command.Move.has_value())
        {
            return BuildRejectedResult("ERR_INVALID_PAYLOAD", "Move command is missing move payload.");
        }

        const FMoveAction InputMove = Command.Move.value();
        const FPieceState* Piece = FindPieceById(InputMove.PieceId);
        if (Piece == nullptr || !Piece->bAlive)
        {
            return BuildRejectedResult("ERR_INVALID_PIECE", "Move piece does not exist.");
        }
        if (Piece->Side != Command.Side)
        {
            return BuildRejectedResult("ERR_INVALID_PIECE_SIDE", "Move piece is not owned by side.");
        }
        if (!(Piece->Pos == InputMove.From))
        {
            return BuildRejectedResult("ERR_INVALID_FROM", "Move from position does not match piece position.");
        }

        const std::vector<FMoveAction> LegalMoves = GenerateLegalMoves(Command.Side);
        auto It = std::find_if(LegalMoves.begin(), LegalMoves.end(), [&InputMove](const FMoveAction& Candidate) {
            return Candidate.PieceId == InputMove.PieceId && Candidate.From == InputMove.From && Candidate.To == InputMove.To;
        });
        if (It == LegalMoves.end())
        {
            return BuildRejectedResult("ERR_ILLEGAL_MOVE", "Move is not legal.");
        }

        const FMoveAction Move = *It;
        ApplyMoveUnchecked(Move);

        FPieceState* MovedPiece = FindPieceById(Move.PieceId);
        if (MovedPiece == nullptr)
        {
            return BuildRejectedResult("ERR_INTERNAL", "Moved piece cannot be found after move.");
        }

        if (Move.CapturedPieceId.has_value())
        {
            if (MovedPiece->PieceState == EPieceState::HiddenSurface && RuleConfig.bRevealOnFirstCapture)
            {
                MovedPiece->PieceState = EPieceState::RevealedActual;
                if (RuleConfig.bFreezeIfIllegalAfterReveal &&
                    !IsRolePositionLegal(MovedPiece->ActualRole, MovedPiece->Side, MovedPiece->Pos))
                {
                    MovedPiece->bFrozen = true;
                }
            }
            MovedPiece->bHasCaptured = true;
        }

        GameState.PassCount = 0;
        ++GameState.TurnIndex;

        EvaluateEndAfterMove(Command.Side);
        if (GameState.Phase != EGamePhase::GameOver)
        {
            GameState.CurrentTurn = GetOppositeSide(GameState.CurrentTurn);
        }

        return BuildAcceptedResult();
    }
    default:
        return BuildRejectedResult("ERR_UNSUPPORTED_COMMAND", "Command is not implemented.");
    }
}
