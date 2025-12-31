//
// ProgressionDataCollector.cpp
// Implementation of progression data collection
//

#include "sim/ProgressionDataCollector.h"
#include "game/GameContext.h"
#include "game/Shop.h"
#include "sim/search/GameAction.h"
#include <algorithm>
#include <set>

namespace sts {
namespace progression {

ProgressionDataCollector::ProgressionDataCollector()
    : rng(std::random_device{}()) {
}

ProgressionDataCollector::ProgressionDataCollector(unsigned int seed)
    : rng(seed) {
}

RunProgression ProgressionDataCollector::collectRun(CharacterClass cc, std::uint64_t seed, int ascension) {
    RunProgression progression;
    progression.seed = seed;
    progression.characterClass = cc;
    progression.ascension = ascension;

    // Create game context with skipBattles enabled
    GameContext gc(cc, seed, ascension);
    gc.skipBattles = true;

    // Main game loop with safety limit
    int maxIterations = 10000;  // Increased limit
    int iteration = 0;
    int lastFloor = -1;
    ScreenState lastRecordedScreen = ScreenState::INVALID;

    while (gc.outcome == GameOutcome::UNDECIDED && iteration < maxIterations) {
        iteration++;

        // Debug: print when floor changes
        if (gc.floorNum != lastFloor) {
            lastFloor = gc.floorNum;
            // Will be visible if running from console
        }

        // Advance game state based on screen
        switch (gc.screenState) {
            case ScreenState::MAP_SCREEN: {
                // Choose random path
                int nextX = chooseRandomPath(gc);
                gc.transitionToMapNode(nextX);
                lastRecordedScreen = ScreenState::INVALID; // Reset after map transition
                break;
            }

            case ScreenState::REWARDS: {
                // Only record if we haven't recorded this screen type recently
                if (lastRecordedScreen != ScreenState::REWARDS) {
                    NodeRecord node;
                    node.floorNum = gc.floorNum;
                    node.x = gc.curMapNodeX;
                    node.y = gc.curMapNodeY;
                    node.roomType = gc.curRoom;
                    recordNode(gc, node);
                    progression.nodes.push_back(node);
                    lastRecordedScreen = ScreenState::REWARDS;
                }

                // Skip all rewards
                search::GameAction skipAction(search::GameAction::RewardsActionType::SKIP);
                skipAction.execute(gc);
                break;
            }

            case ScreenState::SHOP_ROOM: {
                // Only record if we haven't recorded this screen recently
                if (lastRecordedScreen != ScreenState::SHOP_ROOM) {
                    NodeRecord node;
                    node.floorNum = gc.floorNum;
                    node.x = gc.curMapNodeX;
                    node.y = gc.curMapNodeY;
                    node.roomType = gc.curRoom;
                    recordNode(gc, node);
                    progression.nodes.push_back(node);
                    lastRecordedScreen = ScreenState::SHOP_ROOM;
                }

                // Don't buy anything, just leave
                gc.regainControl();
                break;
            }

            case ScreenState::REST_ROOM: {
                // Record rest site with upgradeable cards
                if (lastRecordedScreen != ScreenState::REST_ROOM) {
                    NodeRecord node;
                    node.floorNum = gc.floorNum;
                    node.x = gc.curMapNodeX;
                    node.y = gc.curMapNodeY;
                    node.roomType = gc.curRoom;
                    recordNode(gc, node);
                    progression.nodes.push_back(node);
                    lastRecordedScreen = ScreenState::REST_ROOM;
                }

                // Don't rest or smith, just leave
                gc.regainControl();
                break;
            }

            case ScreenState::TREASURE_ROOM: {
                // Don't record here - the relic isn't generated until we open the chest
                // We'll record this at the REWARDS screen that follows
                gc.chooseTreasureRoomOption(true);
                break;
            }

            case ScreenState::EVENT_SCREEN: {
                // Only record if we haven't recorded this screen recently
                if (lastRecordedScreen != ScreenState::EVENT_SCREEN) {
                    NodeRecord node;
                    node.floorNum = gc.floorNum;
                    node.x = gc.curMapNodeX;
                    node.y = gc.curMapNodeY;
                    node.roomType = gc.curRoom;
                    handleEvent(gc, node);
                    progression.nodes.push_back(node);
                    lastRecordedScreen = ScreenState::EVENT_SCREEN;
                }
                break;
            }

            case ScreenState::BOSS_RELIC_REWARDS: {
                // Only record if we haven't recorded this screen recently
                if (lastRecordedScreen != ScreenState::BOSS_RELIC_REWARDS) {
                    NodeRecord node;
                    node.floorNum = gc.floorNum;
                    node.x = gc.curMapNodeX;
                    node.y = gc.curMapNodeY;
                    node.roomType = gc.curRoom;
                    recordNode(gc, node);
                    progression.nodes.push_back(node);
                    lastRecordedScreen = ScreenState::BOSS_RELIC_REWARDS;
                }

                // Choose first boss relic
                if (gc.info.bossRelics[0] != RelicId::INVALID) {
                    gc.chooseBossRelic(0);
                } else {
                    gc.regainControl();
                }
                break;
            }

            case ScreenState::CARD_SELECT: {
                // Choose first card if available, otherwise skip
                if (gc.info.toSelectCards.size() > 0) {
                    gc.chooseSelectCardScreenOption(0);
                } else {
                    gc.regainControl();
                }
                break;
            }

            case ScreenState::BATTLE: {
                // Should not happen with skipBattles=true, but handle it anyway
                gc.regainControl();
                break;
            }

            default:
                // For any other state, try to regain control
                gc.regainControl();
                break;
        }
    }

    // Store final outcome
    progression.outcome = gc.outcome;
    progression.actReached = gc.act;
    progression.floorReached = gc.floorNum;

    return progression;
}

int ProgressionDataCollector::chooseRandomPath(const GameContext &gc) {
    // Safety check for valid map position
    if (gc.curMapNodeY < 0 || gc.curMapNodeY >= 15 || gc.curMapNodeX < 0 || gc.curMapNodeX >= 7) {
        return 3; // Default to middle column
    }

    // Collect all valid next positions from current node's edges
    const auto &currentNode = gc.map->getNode(gc.curMapNodeX, gc.curMapNodeY);
    std::vector<int> validPositions;

    for (int i = 0; i < currentNode.edgeCount; ++i) {
        validPositions.push_back(currentNode.edges[i]);
    }

    // If no valid paths, return middle column as default
    if (validPositions.empty()) {
        return 3;
    }

    // Choose random valid position
    std::uniform_int_distribution<int> dist(0, validPositions.size() - 1);
    return validPositions[dist(rng)];
}

void ProgressionDataCollector::recordNode(GameContext &gc, NodeRecord &node) {
    // Record based on screen state
    switch (gc.screenState) {
        case ScreenState::REWARDS:
            recordRewards(gc, node);
            break;

        case ScreenState::SHOP_ROOM:
            recordShop(gc, node);
            break;

        case ScreenState::REST_ROOM:
            recordRest(gc, node);
            break;

        case ScreenState::TREASURE_ROOM:
            recordTreasure(gc, node);
            break;

        case ScreenState::BOSS_RELIC_REWARDS:
            recordBossRelics(gc, node);
            break;

        default:
            // For other states, just record basic room info
            break;
    }
}

void ProgressionDataCollector::recordRewards(const GameContext &gc, NodeRecord &node) {
    const auto &rewards = gc.info.rewardsContainer;

    // Record encounter type for combat nodes
    if (gc.curRoom == Room::MONSTER || gc.curRoom == Room::ELITE || gc.curRoom == Room::BOSS) {
        node.encounter = gc.info.encounter;
    }

    // Record treasure room data
    if (gc.curRoom == Room::TREASURE) {
        node.chestSize = gc.info.chestSize;
        // Record relics from rewards
        for (int i = 0; i < rewards.relicCount; ++i) {
            node.chestRelics.push_back(rewards.relics[i]);
        }
    }

    // Record all card rewards
    for (int i = 0; i < rewards.cardRewardCount; ++i) {
        std::vector<Card> cardReward;
        const auto &rewardSet = rewards.cardRewards[i];

        for (int j = 0; j < rewardSet.size(); ++j) {
            cardReward.push_back(rewardSet[j]);
        }

        if (!cardReward.empty()) {
            node.cardRewards.push_back(cardReward);
        }
    }
}

void ProgressionDataCollector::recordShop(const GameContext &gc, NodeRecord &node) {
    ShopData shopData;
    const auto &shop = gc.info.shop;

    // Copy cards and prices
    for (int i = 0; i < 7; ++i) {
        shopData.cards[i] = shop.cards[i];
        shopData.cardPrices[i] = shop.cardPrice(i);
    }

    // Copy relics and prices
    for (int i = 0; i < 3; ++i) {
        shopData.relics[i] = shop.relics[i];
        shopData.relicPrices[i] = shop.relicPrice(i);
    }

    // Copy potions and prices
    for (int i = 0; i < 3; ++i) {
        shopData.potions[i] = shop.potions[i];
        shopData.potionPrices[i] = shop.potionPrice(i);
    }

    shopData.removeCost = shop.removeCost;

    node.shop = shopData;
}

void ProgressionDataCollector::recordTreasure(const GameContext &gc, NodeRecord &node) {
    node.chestSize = gc.info.chestSize;

    // The relic would be in the rewards container after opening
    // For now, we'll record it when we see the rewards screen
}

void ProgressionDataCollector::recordRest(const GameContext &gc, NodeRecord &node) {
    // Record unique cards that can be upgraded (only one entry per card type)
    // We use a set to track which cards we've already added
    std::set<std::pair<CardId, bool>> seenCards;

    for (int i = 0; i < gc.deck.size(); ++i) {
        const auto &card = gc.deck.cards[i];
        if (card.canUpgrade()) {
            auto key = std::make_pair(card.getId(), card.getUpgraded());
            if (seenCards.find(key) == seenCards.end()) {
                seenCards.insert(key);
                node.upgradeableCards.push_back(card);
            }
        }
    }
}

void ProgressionDataCollector::recordBossRelics(const GameContext &gc, NodeRecord &node) {
    // Record all 3 boss relics offered
    for (int i = 0; i < 3; ++i) {
        node.bossRelics[i] = gc.info.bossRelics[i];
    }
}

void ProgressionDataCollector::handleEvent(GameContext &gc, NodeRecord &node) {
    node.eventType = gc.curEvent;

    // Select event option
    int optionIdx = selectEventOption(gc);
    node.optionChosen = optionIdx;

    // Record deck before event
    std::vector<CardId> deckBefore;
    for (int i = 0; i < gc.deck.size(); ++i) {
        deckBefore.push_back(gc.deck.cards[i].getId());
    }

    // Execute event option
    if (optionIdx >= 0) {
        gc.chooseEventOption(optionIdx);
    } else {
        // Unknown event - skip it
        gc.regainControl();
        return;
    }

    // Check for curses added
    for (int i = 0; i < gc.deck.size(); ++i) {
        CardId cardId = gc.deck.cards[i].getId();

        // Check if this card is new (not in deckBefore)
        bool isNew = true;
        for (CardId beforeId : deckBefore) {
            if (beforeId == cardId) {
                isNew = false;
                // Remove this instance from deckBefore to handle duplicates
                deckBefore.erase(std::find(deckBefore.begin(), deckBefore.end(), cardId));
                break;
            }
        }

        if (isNew) {
            Card newCard(cardId);
            if (newCard.getType() == CardType::CURSE) {
                node.cursesGained.push_back(newCard);
            }
        }
    }
}

int ProgressionDataCollector::selectEventOption(const GameContext &gc) {
    // Simple policy: only handle known safe events
    // For unknown events, skip them to avoid crashes

    switch (gc.curEvent) {
        case Event::NEOW:
            // Neow has multiple options, pick first safe one
            return 0;

        case Event::DESIGNER_IN_SPIRE:
            // Option 0 gives card choices
            return 0;

        case Event::BONFIRE_SPIRITS:
            // Option 0 is offer, option 1 is remove
            return 0;

        case Event::OMINOUS_FORGE:
            // Option 0 gives relic
            return 0;

        case Event::WHEEL_OF_CHANGE:
            // Transforms cards
            return 0;

        default:
            // For unknown events, don't choose - skip to avoid crashes
            // Return -1 to indicate we should skip this event
            return -1;
    }
}

} // namespace progression
} // namespace sts
