/**
 * Test random playout logic without Python bindings.
 * Compile and run this to verify the C++ logic works.
 */

#include <iostream>
#include "game/GameContext.h"
#include "combat/BattleContext.h"
#include "sim/search/Action.h"

using namespace sts;

int main() {
    std::cout << "Testing random playout logic..." << std::endl;

    // Create a game context
    GameContext gc(CharacterClass::IRONCLAD, 12345, 0);
    std::cout << "GameContext created: HP=" << gc.curHp << "/" << gc.maxHp << std::endl;

    // Create and initialize battle context
    BattleContext bc;
    bc.init(gc, MonsterEncounter::JAW_WORM);
    std::cout << "BattleContext initialized for JAW_WORM encounter" << std::endl;
    std::cout << "  Player HP: " << bc.player.curHp << "/" << bc.player.maxHp << std::endl;
    std::cout << "  Outcome: " << battleOutcomeStrings[static_cast<int>(bc.outcome)] << std::endl;

    // Run random playout
    std::cout << "\nStarting random playout..." << std::endl;
    int turn_count = 0;
    while (bc.outcome == Outcome::UNDECIDED) {
        // Get legal actions
        auto actions = search::Action::enumerateCardSelectActions(bc);

        if (actions.empty()) {
            std::cout << "No legal actions available!" << std::endl;
            break;
        }

        std::cout << "Turn " << turn_count << ": " << actions.size() << " legal actions" << std::endl;

        // Pick random action
        int idx = bc.aiRng.random(static_cast<int>(actions.size()) - 1);
        std::cout << "  Executing action " << idx << std::endl;

        actions[idx].execute(bc);

        turn_count++;

        // Safety check to prevent infinite loops
        if (turn_count > 1000) {
            std::cout << "ERROR: Exceeded 1000 turns, aborting" << std::endl;
            return 1;
        }
    }

    std::cout << "\nPlayout complete!" << std::endl;
    std::cout << "  Turns: " << turn_count << std::endl;
    std::cout << "  Outcome: " << battleOutcomeStrings[static_cast<int>(bc.outcome)] << std::endl;
    std::cout << "  Player HP: " << bc.player.curHp << "/" << bc.player.maxHp << std::endl;

    if (bc.outcome == Outcome::PLAYER_VICTORY) {
        std::cout << "SUCCESS: Player won!" << std::endl;
        return 0;
    } else if (bc.outcome == Outcome::PLAYER_LOSS) {
        std::cout << "Player lost (but test succeeded - reached terminal state)" << std::endl;
        return 0;
    } else {
        std::cout << "ERROR: Battle did not reach terminal state" << std::endl;
        return 1;
    }
}
