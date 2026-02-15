#include "CoreRules/MatchReferee.h"

#include <cassert>

int main()
{
    FMatchReferee MatchReferee;

    const FGameState& InitialState = MatchReferee.GetState();
    assert(InitialState.Phase == EGamePhase::SetupCommit);
    assert(InitialState.CurrentTurn == ESide::Red);
    assert(InitialState.Result == EGameResult::Ongoing);

    FCommandResult CommitRed = MatchReferee.ApplyCommit({ESide::Red, "RedHash"});
    FCommandResult CommitBlack = MatchReferee.ApplyCommit({ESide::Black, "BlackHash"});
    assert(CommitRed.bAccepted);
    assert(CommitBlack.bAccepted);
    assert(MatchReferee.GetState().Phase == EGamePhase::SetupReveal);

    FSetupPlain RedReveal{};
    RedReveal.Side = ESide::Red;
    RedReveal.Nonce = "RedNonce";

    FSetupPlain BlackReveal{};
    BlackReveal.Side = ESide::Black;
    BlackReveal.Nonce = "BlackNonce";

    FCommandResult RevealRed = MatchReferee.ApplyReveal(RedReveal);
    FCommandResult RevealBlack = MatchReferee.ApplyReveal(BlackReveal);
    assert(RevealRed.bAccepted);
    assert(RevealBlack.bAccepted);
    assert(MatchReferee.GetState().Phase == EGamePhase::Battle);

    FPlayerCommand PassCommand{};
    PassCommand.CommandType = ECommandType::Pass;
    PassCommand.Side = ESide::Red;

    FCommandResult PassRed = MatchReferee.ApplyCommand(PassCommand);
    assert(PassRed.bAccepted);

    PassCommand.Side = ESide::Black;
    FCommandResult PassBlack = MatchReferee.ApplyCommand(PassCommand);
    assert(PassBlack.bAccepted);

    const FGameState& FinalState = MatchReferee.GetState();
    assert(FinalState.Result == EGameResult::Draw);
    assert(FinalState.EndReason == EEndReason::DoublePassDraw);

    return 0;
}
