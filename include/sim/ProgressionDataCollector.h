//
// ProgressionDataCollector.h
// Collects out-of-combat progression data for training data generation
//

#ifndef STS_LIGHTSPEED_PROGRESSIONDATACOLLECTOR_H
#define STS_LIGHTSPEED_PROGRESSIONDATACOLLECTOR_H

#include <vector>
#include <optional>
#include <random>
#include <cstdint>

#include "game/Card.h"
#include "constants/Cards.h"
#include "constants/Relics.h"
#include "constants/Potions.h"
#include "constants/MonsterEncounters.h"
#include "constants/Events.h"
#include "constants/CharacterClasses.h"
#include "constants/Rooms.h"
#include "constants/Misc.h"
#include "game/GameContext.h"

namespace sts {
namespace progression {

// Shop contents at a shop node
struct ShopData {
    Card cards[7];
    int cardPrices[7];
    RelicId relics[3];
    int relicPrices[3];
    Potion potions[3];
    int potionPrices[3];
    int removeCost;

    ShopData() = default;
};

// Data collected at a single node
struct NodeRecord {
    int floorNum = 0;
    int x = 0;
    int y = 0;
    Room roomType = Room::INVALID;

    // For MONSTER/ELITE/BOSS nodes
    MonsterEncounter encounter = MonsterEncounter::INVALID;
    std::vector<std::vector<Card>> cardRewards;  // Can have multiple card reward sets

    // For SHOP nodes
    std::optional<ShopData> shop;

    // For EVENT nodes
    Event eventType = Event::INVALID;
    std::vector<Card> cardsOffered;
    std::vector<RelicId> relicsOffered;
    std::vector<Card> cursesGained;
    int optionChosen = -1;

    // For TREASURE nodes
    ChestSize chestSize = ChestSize::INVALID;
    std::vector<RelicId> chestRelics;

    // For BOSS nodes (boss relic rewards)
    RelicId bossRelics[3] = {RelicId::INVALID, RelicId::INVALID, RelicId::INVALID};

    NodeRecord() = default;
};

// Complete progression data for one run
struct RunProgression {
    std::uint64_t seed = 0;
    CharacterClass characterClass = CharacterClass::IRONCLAD;
    int ascension = 0;
    GameOutcome outcome = GameOutcome::UNDECIDED;
    int actReached = 0;
    int floorReached = 0;

    std::vector<NodeRecord> nodes;

    RunProgression() = default;
};

// Main collector class
class ProgressionDataCollector {
private:
    std::default_random_engine rng;

    // Helper: Choose random valid path at current map position
    int chooseRandomPath(const GameContext &gc);

    // Helper: Record data at current node based on screen state
    void recordNode(GameContext &gc, NodeRecord &node);

    // Helper: Record reward screen data
    void recordRewards(const GameContext &gc, NodeRecord &node);

    // Helper: Record shop contents
    void recordShop(const GameContext &gc, NodeRecord &node);

    // Helper: Record treasure room data
    void recordTreasure(const GameContext &gc, NodeRecord &node);

    // Helper: Record boss relics
    void recordBossRelics(const GameContext &gc, NodeRecord &node);

    // Helper: Handle event and record offerings
    void handleEvent(GameContext &gc, NodeRecord &node);

    // Helper: Select event option that prioritizes cards/relics
    int selectEventOption(const GameContext &gc);

public:
    ProgressionDataCollector();
    explicit ProgressionDataCollector(unsigned int seed);

    // Main API: Run a full game and collect progression data
    RunProgression collectRun(CharacterClass cc, std::uint64_t seed, int ascension);
};

} // namespace progression
} // namespace sts

#endif //STS_LIGHTSPEED_PROGRESSIONDATACOLLECTOR_H
