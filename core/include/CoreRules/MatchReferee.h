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
    FRuleConfig RuleConfig;
    FGameState GameState;
};
