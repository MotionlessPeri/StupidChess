# StupidChess 接口文档（C++20）

## 1. 目标与边界

1. 核心规则层使用纯 C++20，不依赖 UE。
2. UE 与微信小程序共享同一规则语义与协议。
3. 联网 PvP 先行，AI（含强化学习）接口从首版预留。
4. 命名遵循 UE 风格（类型前缀 + 大驼峰，布尔 `b` 前缀）。

## 2. 构建与标准

1. 统一标准：`C++20`。
2. 核心编译选项建议：
   - MSVC: `/std:c++20 /permissive-`
   - Clang/GCC: `-std=c++20`
3. 跨端约束：避免依赖不稳定 WASM 特性的库组件（如复杂线程模型、`<stacktrace>`、重度本地化格式化）。

## 3. 目录建议

```text
Source/
  CoreRules/
    Public/
      Types/
      Rules/
      Engine/
      AI/
    Private/
  MatchServer/
    Public/
    Private/
  UEAdapter/
    Public/
    Private/
  Protocol/
    Public/
```

## 4. 核心枚举与基础类型（CoreRules）

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class ESide : uint8_t
{
    Red,
    Black
};

enum class ERoleType : uint8_t
{
    King,       // 将/帅
    Advisor,    // 士/仕
    Elephant,   // 象/相
    Horse,      // 马
    Rook,       // 车
    Cannon,     // 炮
    Pawn        // 兵/卒
};

enum class EPieceState : uint8_t
{
    HiddenSurface,   // 未切换，按 SurfaceRole 走
    RevealedActual   // 已切换，按 ActualRole 走
};

enum class EGamePhase : uint8_t
{
    SetupCommit,
    SetupReveal,
    Battle,
    GameOver
};

enum class EGameResult : uint8_t
{
    Ongoing,
    RedWin,
    BlackWin,
    Draw
};

enum class EEndReason : uint8_t
{
    None,
    Checkmate,
    Resign,
    Timeout,
    DoublePassDraw,
    RuleViolation
};

enum class ECommandType : uint8_t
{
    CommitSetup,
    RevealSetup,
    Move,
    Pass,
    Resign
};

struct FBoardPos
{
    int8_t X = -1; // [0,8]
    int8_t Y = -1; // [0,9]

    bool IsValid() const noexcept;
    bool operator==(const FBoardPos& Other) const noexcept = default;
};

using FPieceId = uint16_t;
```

## 5. 规则配置与棋子/棋盘状态

```cpp
struct FRuleConfig
{
    bool bRevealOnFirstCapture = true;
    bool bRevealCapturedRole = true;
    bool bFreezeIfIllegalAfterReveal = true;
    bool bAllowPassWhenNoLegalMove = true;
    bool bDoublePassIsDraw = true;
};

struct FPieceState
{
    FPieceId PieceId = 0;
    ESide Side = ESide::Red;

    ERoleType ActualRole = ERoleType::Pawn;
    ERoleType SurfaceRole = ERoleType::Pawn;

    EPieceState PieceState = EPieceState::HiddenSurface;

    FBoardPos Pos{};
    bool bAlive = true;
    bool bFrozen = false;
    bool bHasCaptured = false;
};

struct FMoveAction
{
    FPieceId PieceId = 0;
    FBoardPos From{};
    FBoardPos To{};
    std::optional<FPieceId> CapturedPieceId;
};

struct FMoveUndo
{
    FMoveAction AppliedMove{};
    std::optional<FPieceState> CapturedPieceBefore;
    FPieceState MovedPieceBefore;
    int32_t PassCountBefore = 0;
};

struct FGameState
{
    EGamePhase Phase = EGamePhase::SetupCommit;
    ESide CurrentTurn = ESide::Red;

    std::array<std::optional<FPieceId>, 90> BoardCells{};
    std::vector<FPieceState> Pieces;

    bool bRedCommitted = false;
    bool bBlackCommitted = false;
    bool bRedRevealed = false;
    bool bBlackRevealed = false;

    int32_t PassCount = 0;
    EGameResult Result = EGameResult::Ongoing;
    EEndReason EndReason = EEndReason::None;

    uint64_t TurnIndex = 0;
};
```

## 6. 摆子提交（盲摆 Commit-Reveal）接口

```cpp
struct FSetupPlacement
{
    FPieceId PieceId = 0;
    FBoardPos TargetPos{};
};

struct FSetupPlain
{
    ESide Side = ESide::Red;
    std::vector<FSetupPlacement> Placements;
    std::string Nonce;
};

struct FSetupCommit
{
    ESide Side = ESide::Red;
    std::string HashHex; // SHA-256(SerializedSetupPlain)
};

class ISetupHasher
{
public:
    virtual ~ISetupHasher() = default;
    virtual std::string BuildCommitHash(const FSetupPlain& SetupPlain) const = 0;
    virtual bool VerifyCommit(const FSetupCommit& Commit, const FSetupPlain& SetupPlain) const = 0;
};
```

## 7. 命令与事件接口

```cpp
struct FPlayerCommand
{
    ECommandType CommandType = ECommandType::Move;
    ESide Side = ESide::Red;

    // Move
    std::optional<FMoveAction> Move;

    // Commit/Reveal
    std::optional<FSetupCommit> SetupCommit;
    std::optional<FSetupPlain> SetupPlain;
};

struct FCommandResult
{
    bool bAccepted = false;
    std::string ErrorCode;     // e.g. "ERR_NOT_YOUR_TURN"
    std::string ErrorMessage;
};

enum class EGameEventType : uint8_t
{
    SetupCommitted,
    SetupRevealed,
    MoveApplied,
    PieceCaptured,
    PieceRevealed,
    PieceFrozen,
    PassApplied,
    Check,
    GameEnded
};

struct FGameEvent
{
    EGameEventType EventType;
    uint64_t TurnIndex = 0;
    std::string PayloadJson; // 首版可用 JSON；后续可改二进制
};
```

## 8. 核心规则引擎接口（最关键）

```cpp
class IRoleRuleSet
{
public:
    virtual ~IRoleRuleSet() = default;

    virtual std::vector<FBoardPos> GeneratePseudoMoves(
        const FGameState& GameState,
        const FPieceState& Piece,
        ERoleType ActiveRole) const = 0;

    virtual bool IsRolePositionLegal(
        ERoleType Role,
        ESide Side,
        const FBoardPos& Pos) const = 0;
};

class ICheckEvaluator
{
public:
    virtual ~ICheckEvaluator() = default;

    virtual bool IsInCheck(const FGameState& GameState, ESide DefenderSide) const = 0;
    virtual bool IsCheckmate(const FGameState& GameState, ESide DefenderSide) const = 0;
};

class FMatchReferee
{
public:
    explicit FMatchReferee(FRuleConfig InRuleConfig);

    const FGameState& GetState() const noexcept;
    void ResetNewMatch();

    // Setup Phase
    FCommandResult ApplyCommit(const FSetupCommit& Commit);
    FCommandResult ApplyReveal(const FSetupPlain& SetupPlain);

    // Battle Phase
    FCommandResult ApplyCommand(const FPlayerCommand& Command);

    // Query
    std::vector<FMoveAction> GenerateLegalMoves(ESide Side) const;
    std::vector<FMoveAction> GenerateLegalMovesForPiece(FPieceId PieceId) const;
    bool CanPass(ESide Side) const;

    // Determinism/Replay
    std::string ExportStateJson() const;
    bool ImportStateJson(const std::string& StateJson);

private:
    FRuleConfig RuleConfig;
    FGameState GameState;

    std::vector<FGameEvent> PendingEvents;

    FCommandResult ValidateAndApplyMove(const FMoveAction& Move, ESide Side);
    void ApplyRevealIfNeeded(FPieceState& MovedPiece);
    void ApplyFreezeCheckIfNeeded(FPieceState& Piece);
    void EvaluateEndConditionsAfterAction(ESide ActionSide, bool bWasPass);
};
```

## 9. 可见性视图接口（防信息泄露）

```cpp
struct FPiecePublicView
{
    FPieceId PieceId = 0;
    ESide Side = ESide::Red;

    // 对观察方可见的职业
    ERoleType VisibleRole = ERoleType::Pawn;

    FBoardPos Pos{};
    bool bAlive = true;
    bool bFrozen = false;
    bool bRevealed = false;
};

struct FPlayerViewState
{
    ESide ViewerSide = ESide::Red;
    EGamePhase Phase = EGamePhase::Battle;
    ESide CurrentTurn = ESide::Red;

    std::vector<FPiecePublicView> Pieces;
    int32_t PassCount = 0;
    EGameResult Result = EGameResult::Ongoing;
};

class IViewProjector
{
public:
    virtual ~IViewProjector() = default;
    virtual FPlayerViewState BuildPlayerView(const FGameState& FullState, ESide ViewerSide) const = 0;
};
```

## 10. Server 对战会话接口（MatchServer）

```cpp
using FMatchId = uint64_t;
using FPlayerId = uint64_t;

struct FMatchJoinRequest
{
    FMatchId MatchId = 0;
    FPlayerId PlayerId = 0;
    ESide PreferredSide = ESide::Red;
};

struct FMatchJoinResponse
{
    bool bAccepted = false;
    ESide AssignedSide = ESide::Red;
    std::string ErrorMessage;
};

class IMatchSession
{
public:
    virtual ~IMatchSession() = default;

    virtual FMatchJoinResponse Join(const FMatchJoinRequest& Request) = 0;
    virtual FCommandResult SubmitCommand(FPlayerId PlayerId, const FPlayerCommand& Command) = 0;

    virtual FPlayerViewState GetPlayerView(FPlayerId PlayerId) const = 0;
    virtual std::vector<FGameEvent> PullEvents(FPlayerId PlayerId, uint64_t AfterTurnIndex) const = 0;
};

class IReplayStore
{
public:
    virtual ~IReplayStore() = default;

    virtual void AppendEvent(FMatchId MatchId, const FGameEvent& Event) = 0;
    virtual std::vector<FGameEvent> LoadAll(FMatchId MatchId) const = 0;
};
```

## 11. 协议 DTO（Protocol）

```cpp
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
    EProtocolMessageType MessageType;
    uint64_t Sequence = 0;
    std::string MatchId;
    std::string PayloadJson;
};
```

消息约定：

1. `S2C_Snapshot` 仅下发观察方可见信息。
2. `S2C_EventDelta` 带 `TurnIndex`，客户端可断线续拉。
3. 客户端永不上传“规则结论”，只上传命令意图。

## 12. UE 适配层接口（UEAdapter）

```cpp
// 示例：对接蓝图/UI
class IGamePresenter
{
public:
    virtual ~IGamePresenter() = default;

    virtual void OnSnapshot(const FPlayerViewState& ViewState) = 0;
    virtual void OnEvent(const FGameEvent& Event) = 0;
    virtual void OnCommandRejected(const FCommandResult& Result) = 0;
};

class FUEGameClientFacade
{
public:
    explicit FUEGameClientFacade(IGamePresenter* InPresenter);

    void ConnectAndJoin(const FMatchJoinRequest& Request);
    void SendMove(const FMoveAction& Move);
    void SendPass();
    void SendResign();

    void TickNetwork(float DeltaSeconds);
};
```

## 13. AI/RL 预留接口（CoreRules::AI）

```cpp
struct FObservation
{
    ESide PerspectiveSide = ESide::Red;
    std::string TensorLikeJson; // 首版先文本，训练端可替换为张量结构
};

struct FActionMask
{
    std::vector<FMoveAction> LegalMoves;
    bool bCanPass = false;
    bool bCanResign = true;
};

class IAgentPolicy
{
public:
    virtual ~IAgentPolicy() = default;
    virtual FPlayerCommand ChooseAction(const FObservation& Observation, const FActionMask& ActionMask) = 0;
};

class ITrainingEnv
{
public:
    virtual ~ITrainingEnv() = default;

    virtual void Reset(uint64_t Seed) = 0;
    virtual FObservation GetObservation(ESide Side) const = 0;
    virtual FActionMask GetActionMask(ESide Side) const = 0;
    virtual FCommandResult Step(const FPlayerCommand& Command) = 0;
    virtual bool IsTerminal() const = 0;
    virtual float GetReward(ESide Side) const = 0;
};
```

## 14. 关键规则口径（实现必须一致）

1. 双方盲摆：采用 `Commit -> Reveal`，服务端验哈希。
2. 表面职业由初始格模板决定，实际职业来自棋子固有身份。
3. 首次吃子后永久切换到实际职业并公开。
4. 切换瞬间若位置不满足该实际职业位置合法性，则 `bFrozen=true`。
5. 冻结棋子不可移动，不产生攻击威胁，但占格可被吃。
6. 仅当“未被将军且无合法着法”可 `Pass`。
7. 双方连续各一次 `Pass` 判和。
8. 将死判负（沿用象棋将军/将死语义）。

## 15. 测试接口与最小用例集

```cpp
class IRuleTestHarness
{
public:
    virtual ~IRuleTestHarness() = default;

    virtual FGameState BuildStateFromFenLike(const std::string& FenLike) const = 0;
    virtual std::vector<FMoveAction> QueryLegalMoves(const FGameState& State, ESide Side) const = 0;
    virtual FCommandResult RunCommand(FGameState& State, const FPlayerCommand& Command) const = 0;
};
```

建议最小回归组：

1. 摆子合法性（16 格限定、无禁摆）。
2. 首次吃子触发切换与公开。
3. 切换后冻结（士/象非法位置）。
4. 冻结棋子不产生攻击威胁。
5. 将军状态下禁止 `Pass`。
6. 双 `Pass` 和棋。
7. 同局面跨平台一致合法步集合。

## 16. 版本化与兼容建议

1. 协议增加 `RuleVersion` 与 `ConfigHash` 字段，客户端入局先校验。
2. 回放文件记录 `EngineVersion + RuleConfig + RandomSeed`。
3. 核心层保证确定性：禁用依赖未定义遍历顺序的数据结构作为判定基础。

---

以上接口文档可以直接作为 `CoreRules` 首批头文件设计蓝本。
