# MCTS API Reference

This document provides a high-level index of the key interfaces for implementing Monte Carlo Tree Search (MCTS) agents with the Slay the Spire simulator.

## Table of Contents
- [State Representation](#state-representation)
- [Action Enumeration](#action-enumeration)
- [State Transitions](#state-transitions)
- [Existing MCTS Implementation](#existing-mcts-implementation)
- [Python Bindings](#python-bindings)
- [Quick Reference](#quick-reference)

---

## State Representation

### BattleContext
**Purpose:** Core in-battle game state for combat simulation

**Location:** `include/combat/BattleContext.h`

**Key Members:**
- `Outcome outcome` - Battle result (UNDECIDED, PLAYER_VICTORY, PLAYER_LOSS)
- `InputState inputState` - Current action state
- `Player player` - Player state (HP, energy, block, statuses)
- `MonsterGroup monsters` - Enemy array (max 5 monsters)
- `CardManager cards` - Hand, draw pile, discard pile, exhaust pile
- `int turn` - Current battle turn
- Multiple RNG objects (aiRng, cardRandomRng, miscRng, etc.)

**Key Methods:**
- Copy constructor for state branching
- `init()` - Initialize from GameContext
- `executeActions()` - Process action queue
- `exitBattle()` - Update GameContext with battle results

### GameContext
**Purpose:** Full campaign game state including map, deck, relics, and progression

**Location:** `include/game/GameContext.h`

**Key Members:**
- `GameOutcome outcome` - Campaign result
- `ScreenState screenState` - Current screen (BATTLE, MAP_SCREEN, REWARDS, etc.)
- `CharacterClass cc` - Character (IRONCLAD, SILENT, DEFECT, WATCHER)
- `Deck deck` - Current deck
- `RelicContainer relics` - Collected relics
- `int act`, `int floorNum` - Progression state
- `int curHp`, `int maxHp`, `int gold`
- Multiple RNG objects

**Key Methods:**
- Constructor: `GameContext(CharacterClass, seed, ascensionLevel)`

### Player
**Purpose:** Player state within combat

**Location:** `include/combat/Player.h`

**Key Members:**
- `int curHp`, `int maxHp`
- `int energy`, `int8_t energyPerTurn`
- `int block`, `int strength`, `int dexterity`
- `std::map<PlayerStatus, int16_t> statusMap` - Buffs/debuffs
- `Stance stance` - Watcher-specific

### Monster
**Purpose:** Individual enemy state

**Location:** `include/combat/Monster.h`

**Key Members:**
- `MonsterId id`
- `int curHp`, `int maxHp`, `int block`
- `uint64_t statusBits` - Status effects
- `MMID moveHistory[2]` - Recent moves

### CardManager
**Purpose:** Manages all card piles during combat

**Location:** `include/combat/CardManager.h`

**Key Members:**
- `int cardsInHand` - Count of cards in hand
- `std::array<CardInstance, MAX_HAND_SIZE> hand`
- `std::vector<CardInstance> drawPile`
- `std::vector<CardInstance> discardPile`
- `std::vector<CardInstance> exhaustPile`

---

## Action Enumeration

### Action (Combat Actions)
**Purpose:** Represents and enumerates legal combat actions (play cards, use potions, end turn, card selections)

**Location:**
- Header: `include/sim/search/Action.h`
- Implementation: `src/sim/search/Action.cpp`

**Key Static Methods:**
- `enumerateCardSelectActions(const BattleContext &bc)` → `std::vector<Action>`
  - **Line:** `src/sim/search/Action.cpp:477`
  - Returns all legal actions for current battle state

**Key Instance Methods:**
- `bool isValidAction(const BattleContext &bc) const`
- `void execute(BattleContext &bc) const`
- `ActionType getActionType()` - Returns: CARD, POTION, SINGLE_CARD_SELECT, MULTI_CARD_SELECT, END_TURN
- `int getSourceIdx()` - Card/potion index
- `int getTargetIdx()` - Monster target index

**Encoding:** 32-bit compact binary encoding for efficient storage

### GameAction (Campaign Actions)
**Purpose:** Represents and enumerates legal campaign actions (rewards, events, map navigation, shops, rest sites)

**Location:**
- Header: `include/sim/search/GameAction.h`
- Implementation: `src/sim/search/GameAction.cpp`

**Key Static Methods:**
- `getAllActionsInState(const GameContext &gc)` → `std::vector<GameAction>`
  - **Line:** `src/sim/search/GameAction.cpp:618`
  - Returns all legal actions for current game state across all screen types

**Key Instance Methods:**
- `bool isValidAction(const GameContext &gc) const`
- `void execute(GameContext &gc) const`
- `RewardsActionType getRewardsActionType()` - Returns: CARD, GOLD, KEY, POTION, RELIC, CARD_REMOVE, SKIP
- `int getIdx1()`, `int getIdx2()`, `int getIdx3()` - Action-specific indices

**Encoding:** 32-bit compact binary encoding for efficient storage

---

## State Transitions

### Applying Actions

**Combat State Transitions:**
```cpp
// File: src/sim/search/Action.cpp
void Action::execute(BattleContext &bc) const;
```
- Adds action to card queue OR uses potion OR selects card OR ends turn
- Sets `bc.inputState = InputState::EXECUTING_ACTIONS`
- Calls `bc.executeActions()` to process queue
- Draw phase and monster turns execute automatically

**Campaign State Transitions:**
```cpp
// File: src/sim/search/GameAction.cpp
void GameAction::execute(GameContext &gc) const;
```
- Applies reward/event choice
- Updates GameContext (deck, gold, relics, etc.)
- Transitions to next screen state

### State Copying for Simulation

**BattleContext:** Supports default copy constructor for branching
```cpp
BattleContext bc_copy = bc;  // Creates independent copy
```

**Connecting Battle and Campaign:**
```cpp
// Initialize battle from campaign
bc.init(const GameContext &gc, MonsterEncounter encounterToInit);

// Exit battle and update campaign
bc.exitBattle(GameContext &gc) const;
```

---

## Existing MCTS Implementation

### BattleScumSearcher2
**Purpose:** Full MCTS implementation for combat optimization

**Location:**
- Header: `include/sim/search/BattleScumSearcher2.h`
- Implementation: `src/sim/search/BattleScumSearcher2.cpp`

**Tree Structure:**
```cpp
struct Node {
    int64_t simulationCount = 0;
    double evaluationSum = 0;
    std::vector<Edge> edges;
};

struct Edge {
    Action action;
    Node node;
};
```

**Key Methods:**
- `void search(int64_t simulations)` - Main MCTS loop
- `void step()` - Single MCTS iteration (select, expand, simulate, backprop)
- `void enumerateActionsForNode(Node &node, const BattleContext &bc)` - Lazy action enumeration
- `int selectBestEdgeToSearch(const Node &cur)` - UCB1 selection
- `void playoutRandom(BattleContext &state, std::vector<Action> &actionStack)` - Random rollout policy
- `static double evaluateEndState(const BattleContext &bc)` - Terminal state evaluation

**Key Properties:**
- `double explorationParameter = 3*sqrt(2)` - UCB exploration coefficient
- `EvalFnc evalFnc` - Customizable evaluation function
- `std::vector<Action> bestActionSequence` - Best solution found
- `int outcomePlayerHp` - HP of best solution

### ScumSearchAgent2
**Purpose:** MCTS agent for full campaign runs (combines battle MCTS with campaign policy)

**Location:**
- Header: `include/sim/search/ScumSearchAgent2.h`
- Implementation: `src/sim/search/ScumSearchAgent2.cpp`

**Key Methods:**
- `void playout(GameContext &gc)` - Runs complete campaign
- `void playoutBattle(BattleContext &bc)` - Runs MCTS for single battle

**Key Properties:**
- `int simulationCountBase = 50000` - Base simulations per battle
- `double bossSimulationMultiplier = 3` - Extra simulations for boss fights
- `bool pauseOnCardReward` - Pause for manual card selection
- `bool printLogs` - Debug output

### SimpleAgent
**Purpose:** Policy-based agent for comparison/baseline

**Location:** `include/sim/search/SimpleAgent.h`

**Key Methods:**
- `void playout(GameContext &gc)` - Runs full campaign with hand-crafted policies

---

## Python Bindings

### Currently Exposed to Python ✓

**File:** `bindings/slaythespire.cpp`

**GameContext** (lines 44-106)
- Constructor: `GameContext(CharacterClass, seed, ascension)`
- Properties: `outcome`, `cur_hp`, `max_hp`, `gold`, `deck`, `relics`, `floor_num`, `act`, `screen_state`
- Methods: `obtain_card()`, `remove_card()`, `pick_reward_card()`, `skip_reward_cards()`

**ScumSearchAgent2** (lines 36-42)
- Constructor
- `playout(GameContext)` - Runs full campaign
- Properties: `simulation_count_base`, `boss_simulation_multiplier`, `pause_on_card_reward`, `print_logs`

**Game Objects:**
- `Card` - All 580+ CardId enums (lines 121-580)
- `Relic` - All RelicId enums (lines 648-829)
- `Map` - Spire map navigation (lines 112-119)
- `MonsterEncounter` - All encounter types (lines 582-646)

**NNInterface** (lines 31-34)
- `getObservation(GameContext)` - 412-dim observation array
- `getObservationMaximums()` - Max values for normalization
- `observation_space_size` constant

**Enums:**
- `GameOutcome` (lines 148-151)
- `ScreenState` (lines 153-163)
- `CharacterClass` (lines 165-170)
- `Room` (lines 172-182)
- `CardRarity`, `CardColor`, `CardType` (lines 184-207)

### NOT Currently Exposed ❌

**Critical for Custom MCTS Implementation:**
- `BattleContext` - Combat state
- `Action` - Combat action enumeration/execution
- `GameAction` - Campaign action enumeration/execution
- `BattleScumSearcher2` - MCTS tree structure/statistics

**To Add Python Bindings:** Edit `bindings/slaythespire.cpp` and add pybind11 class definitions for the above classes.

---

## Quick Reference

### Typical MCTS Loop (C++)

```cpp
// Initialize campaign
GameContext gc(CharacterClass::IRONCLAD, seed, 0);

// Main game loop
while (gc.outcome == GameOutcome::UNDECIDED) {
    if (gc.screenState == ScreenState::BATTLE) {
        // Initialize battle
        BattleContext bc;
        bc.init(gc);

        // Run MCTS
        BattleScumSearcher2 searcher(bc);
        searcher.search(50000);

        // Execute best action sequence
        for (auto &action : searcher.bestActionSequence) {
            action.execute(bc);
        }

        // Update campaign
        bc.exitBattle(gc);
    } else {
        // Handle non-battle decisions
        auto actions = GameAction::getAllActionsInState(gc);
        // Select action via policy/search
        actions[chosen_idx].execute(gc);
    }
}
```

### Key Entry Points

| Operation | Function | File:Line |
|-----------|----------|-----------|
| Enumerate combat actions | `Action::enumerateCardSelectActions(bc)` | `src/sim/search/Action.cpp:477` |
| Enumerate campaign actions | `GameAction::getAllActionsInState(gc)` | `src/sim/search/GameAction.cpp:618` |
| Execute combat action | `action.execute(bc)` | `src/sim/search/Action.cpp` |
| Execute campaign action | `gameAction.execute(gc)` | `src/sim/search/GameAction.cpp` |
| Check terminal state | `bc.outcome`, `gc.outcome` | State objects |
| Existing MCTS implementation | `BattleScumSearcher2::search()` | `src/sim/search/BattleScumSearcher2.cpp` |
| Full campaign agent | `ScumSearchAgent2::playout()` | `src/sim/search/ScumSearchAgent2.cpp` |

### File Structure Summary

| Component | Header | Implementation |
|-----------|--------|----------------|
| Battle State | `include/combat/BattleContext.h` | `src/combat/BattleContext.cpp` |
| Campaign State | `include/game/GameContext.h` | `src/game/GameContext.cpp` |
| Combat Actions | `include/sim/search/Action.h` | `src/sim/search/Action.cpp` |
| Campaign Actions | `include/sim/search/GameAction.h` | `src/sim/search/GameAction.cpp` |
| MCTS Searcher | `include/sim/search/BattleScumSearcher2.h` | `src/sim/search/BattleScumSearcher2.cpp` |
| Campaign Agent | `include/sim/search/ScumSearchAgent2.h` | `src/sim/search/ScumSearchAgent2.cpp` |
| Python Bindings | `bindings/slaythespire.h` | `bindings/slaythespire.cpp` |
| NN Interface | `bindings/slaythespire.h` | (Implementation in cpp) |
