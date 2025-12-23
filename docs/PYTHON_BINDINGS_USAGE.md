# Python Bindings Usage Guide

This guide explains how to build and use the Python bindings for the Slay the Spire Lightspeed simulator.

## Table of Contents
- [Building the Python Module](#building-the-python-module)
- [Importing the Module](#importing-the-module)
- [Available Classes and Functions](#available-classes-and-functions)
- [Usage Examples](#usage-examples)
- [Limitations](#limitations)

---

## Building the Python Module

### Prerequisites
- Python 3.7+
- CMake 4.0+
- C++ compiler with C++17 support
- pybind11 (included as submodule)

### Build Steps

```bash
# 1. Create build directory
mkdir build
cd build

# 2. Configure with CMake
cmake ..

# 3. Build the Python module
cmake --build . --target slaythespire

# Alternative: use make directly
make slaythespire
```

This creates a Python module file: `slaythespire.cp3XX-platform.pyd` (Windows) or `slaythespire.cp3XX-platform.so` (Linux/Mac)

**Location:** The compiled module will be in the `build/` directory

**Implementation:** `CMakeLists.txt:16` - `pybind11_add_module(slaythespire ...)`

---

## Importing the Module

### Method 1: Add build directory to Python path

```python
import sys
sys.path.insert(0, './build')  # Adjust path as needed
import slaythespire as sts
```

### Method 2: Install in development mode

```bash
# From the build directory
pip install -e .
```

Then:
```python
import slaythespire as sts
```

---

## Available Classes and Functions

### Core Classes

#### `GameContext`
**Purpose:** Full campaign game state

**Constructor:**
```python
gc = sts.GameContext(character_class, seed, ascension_level)
```

**Parameters:**
- `character_class`: `sts.CharacterClass` enum (IRONCLAD, SILENT, DEFECT, WATCHER)
- `seed`: int (uint64_t seed for RNG)
- `ascension_level`: int (0-20)

**Properties (Read/Write):**
- `outcome`: `sts.GameOutcome` (UNDECIDED, PLAYER_VICTORY, PLAYER_LOSS)
- `screen_state`: `sts.ScreenState` (BATTLE, MAP_SCREEN, REWARDS, etc.)
- `cur_hp`, `max_hp`: Current and maximum health
- `gold`: Current gold
- `act`: Current act (1-4)
- `floor_num`: Current floor number
- `seed`: Game seed
- `blue_key`, `green_key`, `red_key`: Key possession flags

**Properties (Read-Only):**
- `deck`: List of `Card` objects in deck
- `relics`: List of `Relic` objects
- `encounter`: Current `MonsterEncounter`

**Methods:**
- `obtain_card(card)`: Add a card to the deck
- `remove_card(idx)`: Remove card at index from deck
- `pick_reward_card(card)`: Select a card reward
- `skip_reward_cards()`: Skip card reward (triggers Singing Bowl if owned)
- `get_card_reward()`: Get current card reward options

**Bindings Location:** `bindings/slaythespire.cpp:44-106`

---

#### `Agent` (ScumSearchAgent2)
**Purpose:** MCTS agent for full campaign runs

**Constructor:**
```python
agent = sts.Agent()
```

**Properties (Read/Write):**
- `simulation_count_base`: int - Base number of simulations per battle (default: 50000)
- `boss_simulation_multiplier`: float - Multiplier for boss fights (default: 3.0)
- `pause_on_card_reward`: bool - Pause for manual card selection
- `print_logs`: bool - Enable debug output

**Methods:**
- `playout(game_context)`: Run complete campaign with MCTS

**Bindings Location:** `bindings/slaythespire.cpp:36-42`

---

#### `Card`
**Purpose:** Represents a card

**Constructor:**
```python
card = sts.Card(sts.CardId.STRIKE_RED)
```

**Properties (Read-Only):**
- `id`: `sts.CardId` enum
- `upgraded`: bool - Is upgraded
- `upgrade_count`: int - Number of upgrades (for Searing Blow)
- `innate`: bool
- `transformable`: bool
- `upgradable`: bool
- `is_strikeCard`: bool
- `is_starter_strike_or_defend`: bool
- `rarity`: `sts.CardRarity` enum
- `type`: `sts.CardType` enum

**Properties (Read/Write):**
- `misc`: int - Internal simulator value (e.g., Ritual Dagger damage)

**Methods:**
- `upgrade()`: Upgrade the card

**Bindings Location:** `bindings/slaythespire.cpp:121-146`

---

#### `Relic`
**Purpose:** Represents a relic

**Properties:**
- `id`: `sts.RelicId` enum
- `data`: int - Relic-specific data (counters, etc.)

**Bindings Location:** `bindings/slaythespire.cpp:108-110`

---

#### `SpireMap`
**Purpose:** The spire map layout

**Constructor:**
```python
spire_map = sts.SpireMap(seed, act, ascension_level, is_endless)
```

**Methods:**
- `get_room_type(x, y)`: Get room type at coordinates
- `has_edge(x, y, x2)`: Check if edge exists between rooms
- `get_nn_rep()`: Get neural network representation

**Bindings Location:** `bindings/slaythespire.cpp:112-119`

---

#### `NNInterface`
**Purpose:** Neural network observation interface

**Usage:**
```python
nn_interface = sts.getNNInterface()
observation = nn_interface.getObservation(game_context)
maximums = nn_interface.getObservationMaximums()
obs_size = nn_interface.observation_space_size  # 412
```

**Methods:**
- `getObservation(game_context)`: Returns 412-element observation array
- `getObservationMaximums()`: Returns maximum values for each observation dimension

**Bindings Location:** `bindings/slaythespire.cpp:31-34`

---

### Enumerations

All enums are available in the `sts` module:

**Character Classes:**
```python
sts.CharacterClass.IRONCLAD
sts.CharacterClass.SILENT
sts.CharacterClass.DEFECT
sts.CharacterClass.WATCHER
```

**Game Outcomes:**
```python
sts.GameOutcome.UNDECIDED
sts.GameOutcome.PLAYER_VICTORY
sts.GameOutcome.PLAYER_LOSS
```

**Screen States:**
```python
sts.ScreenState.BATTLE
sts.ScreenState.MAP_SCREEN
sts.ScreenState.REWARDS
sts.ScreenState.CARD_SELECT
sts.ScreenState.TREASURE_ROOM
sts.ScreenState.REST_ROOM
sts.ScreenState.SHOP_ROOM
# ... etc
```

**Cards:** All 580+ cards available as `sts.CardId.CARD_NAME`

**Relics:** All relics available as `sts.RelicId.RELIC_NAME`

**Monster Encounters:** All encounters available as `sts.MonsterEncounter.ENCOUNTER_NAME`

**Rooms:**
```python
sts.Room.MONSTER
sts.Room.ELITE
sts.Room.REST
sts.Room.SHOP
sts.Room.TREASURE
sts.Room.BOSS
# ... etc
```

---

## Usage Examples

### Example 1: Run MCTS Agent for Full Campaign

```python
import sys
sys.path.insert(0, './build')
import slaythespire as sts

# Create game context
seed = 123456789
gc = sts.GameContext(
    sts.CharacterClass.IRONCLAD,
    seed,
    ascension_level=0
)

# Create and configure agent
agent = sts.Agent()
agent.simulation_count_base = 50000  # 50k simulations per battle
agent.boss_simulation_multiplier = 3.0  # 150k for bosses
agent.print_logs = True  # Show debug output

# Run full campaign
agent.playout(gc)

# Check result
if gc.outcome == sts.GameOutcome.PLAYER_VICTORY:
    print(f"Victory! Final HP: {gc.cur_hp}/{gc.max_hp}")
    print(f"Act: {gc.act}, Floor: {gc.floor_num}")
    print(f"Gold: {gc.gold}")
    print(f"Deck size: {len(gc.deck)}")
    print(f"Relics: {len(gc.relics)}")
else:
    print(f"Defeat on Act {gc.act}, Floor {gc.floor_num}")
```

### Example 2: Inspect Game State

```python
import sys
sys.path.insert(0, './build')
import slaythespire as sts

# Create game
gc = sts.GameContext(sts.CharacterClass.SILENT, 42, 0)

# Inspect initial state
print(f"Character: Silent")
print(f"HP: {gc.cur_hp}/{gc.max_hp}")
print(f"Gold: {gc.gold}")
print(f"Act: {gc.act}, Floor: {gc.floor_num}")
print(f"Screen: {gc.screen_state}")

# View deck
print("\nStarting Deck:")
for i, card in enumerate(gc.deck):
    print(f"  {i}: {card}")

# View relics
print("\nStarting Relics:")
for relic in gc.relics:
    print(f"  {relic.id}")
```

### Example 3: Manual Card Reward Selection

```python
import sys
sys.path.insert(0, './build')
import slaythespire as sts

# Create game
gc = sts.GameContext(sts.CharacterClass.DEFECT, 999, 0)

# Configure agent to pause on card rewards
agent = sts.Agent()
agent.pause_on_card_reward = True
agent.print_logs = False

# Start playout (will pause at first card reward)
# Note: This requires interactive input handling
agent.playout(gc)

# Alternatively, handle rewards manually:
if gc.screen_state == sts.ScreenState.REWARDS:
    rewards = gc.get_card_reward()
    print("Card reward options:")
    for i, card in enumerate(rewards):
        print(f"  {i}: {card}")

    # Pick a card
    chosen_card = rewards[0]
    gc.pick_reward_card(chosen_card)

    # Or skip the reward
    # gc.skip_reward_cards()
```

### Example 4: Neural Network Observations

```python
import sys
sys.path.insert(0, './build')
import slaythespire as sts
import numpy as np

# Create game
gc = sts.GameContext(sts.CharacterClass.WATCHER, 7777, 15)

# Get NN interface
nn_interface = sts.getNNInterface()

# Get observation
observation = nn_interface.getObservation(gc)
maximums = nn_interface.getObservationMaximums()

# Convert to numpy array and normalize
obs_array = np.array(observation, dtype=np.float32)
max_array = np.array(maximums, dtype=np.float32)
normalized_obs = obs_array / (max_array + 1e-8)

print(f"Observation space size: {nn_interface.observation_space_size}")
print(f"Observation shape: {obs_array.shape}")
print(f"Observation range: [{obs_array.min()}, {obs_array.max()}]")
print(f"Normalized range: [{normalized_obs.min()}, {normalized_obs.max()}]")
```

### Example 5: Run Multiple Seeds

```python
import sys
sys.path.insert(0, './build')
import slaythespire as sts

def run_seed(seed, character, ascension):
    """Run a single seed and return results"""
    gc = sts.GameContext(character, seed, ascension)

    agent = sts.Agent()
    agent.simulation_count_base = 30000  # Faster for batch runs
    agent.print_logs = False

    agent.playout(gc)

    return {
        'seed': seed,
        'outcome': gc.outcome,
        'act': gc.act,
        'floor': gc.floor_num,
        'hp': gc.cur_hp if gc.outcome == sts.GameOutcome.PLAYER_VICTORY else 0,
        'max_hp': gc.max_hp,
        'gold': gc.gold
    }

# Run multiple seeds
seeds = [123, 456, 789, 1011, 1213]
character = sts.CharacterClass.IRONCLAD
ascension = 5

results = []
for seed in seeds:
    print(f"Running seed {seed}...")
    result = run_seed(seed, character, ascension)
    results.append(result)

    outcome_str = "Victory" if result['outcome'] == sts.GameOutcome.PLAYER_VICTORY else "Defeat"
    print(f"  {outcome_str} - Act {result['act']}, Floor {result['floor']}")

# Calculate win rate
victories = sum(1 for r in results if r['outcome'] == sts.GameOutcome.PLAYER_VICTORY)
print(f"\nWin rate: {victories}/{len(results)} ({100*victories/len(results):.1f}%)")
```

### Example 6: Deck Building Analysis

```python
import sys
sys.path.insert(0, './build')
import slaythespire as sts
from collections import Counter

# Run a game
gc = sts.GameContext(sts.CharacterClass.SILENT, 42424, 10)

agent = sts.Agent()
agent.print_logs = False
agent.playout(gc)

# Analyze final deck
print("Final Deck Analysis:")
print(f"Total cards: {len(gc.deck)}")

# Count by rarity
rarity_counts = Counter(card.rarity for card in gc.deck)
print("\nBy Rarity:")
for rarity, count in rarity_counts.items():
    print(f"  {rarity}: {count}")

# Count by type
type_counts = Counter(card.type for card in gc.deck)
print("\nBy Type:")
for card_type, count in type_counts.items():
    print(f"  {card_type}: {count}")

# Count upgraded cards
upgraded = sum(1 for card in gc.deck if card.upgraded)
print(f"\nUpgraded cards: {upgraded}/{len(gc.deck)}")

# List all cards
print("\nFull Deck:")
for i, card in enumerate(gc.deck, 1):
    upgrade_str = "+" if card.upgraded else ""
    print(f"  {i}. {card.id}{upgrade_str}")

# List relics
print(f"\nRelics ({len(gc.relics)}):")
for relic in gc.relics:
    print(f"  - {relic.id}")
```

---

## Limitations

### Currently NOT Available in Python

The following C++ classes are **not** exposed to Python:

1. **`BattleContext`** - Combat state representation
   - Cannot access battle-level state during combat
   - Cannot inspect hand, draw pile, enemy HP, etc. during battle

2. **`Action`** - Combat action enumeration/execution
   - Cannot enumerate legal combat actions
   - Cannot execute individual combat actions
   - Cannot implement custom combat policies

3. **`GameAction`** - Campaign action enumeration/execution
   - Cannot enumerate legal campaign actions
   - Cannot execute individual campaign actions
   - Cannot implement step-by-step campaign strategies

4. **`BattleScumSearcher2`** - MCTS tree implementation
   - Cannot access MCTS tree structure
   - Cannot inspect node statistics (visit counts, values)
   - Cannot customize MCTS parameters beyond agent settings

### What This Means

**You CAN:**
- Run full campaign simulations using the MCTS agent
- Access high-level game state (deck, HP, gold, relics)
- Configure agent parameters (simulation count, logging)
- Get neural network observations
- Run batch experiments across multiple seeds

**You CANNOT:**
- Implement custom MCTS agents in Python
- Step through battles action-by-action
- Access intermediate battle states
- Implement custom combat or campaign policies
- Train RL agents with step-by-step interaction

### Workaround

For custom MCTS or RL training, you need to:
1. **Extend the Python bindings** - Add bindings for `BattleContext`, `Action`, `GameAction` in `bindings/slaythespire.cpp`
2. **Work in C++** - Implement custom agents directly in C++ (see `include/sim/search/`)

---

## Utility Functions

### Seed Conversion

```python
# Convert seed string to integer
seed_int = sts.get_seed_str("ABCD1234")

# Convert integer seed to string
seed_str = sts.get_seed_long(123456789)
```

### Play Console Mode

```python
# Launch interactive console game
sts.play()
```

**Bindings Location:** `bindings/slaythespire.cpp:26-28`

---

## Troubleshooting

### Import Error: Module not found

**Problem:** `ModuleNotFoundError: No module named 'slaythespire'`

**Solution:**
1. Ensure the module is built: Check for `.pyd` or `.so` file in `build/`
2. Add build directory to path: `sys.path.insert(0, './build')`
3. Check Python version matches: Module is built for specific Python version (e.g., cp313 = Python 3.13)

### Import Error: Symbol not found / DLL load failed

**Problem:** `DLL load failed while importing slaythespire: The specified module could not be found.` (Windows)

**Solution:**

**Windows with MinGW:**
The compiled module needs MinGW runtime DLLs. Add MinGW's bin directory to your PATH:

```bash
# Temporarily (for current session)
set PATH=%PATH%;C:\mingw64\bin

# Or in PowerShell
$env:PATH += ";C:\mingw64\bin"

# Permanently (System Settings)
# Add to System Environment Variables in Control Panel
```

Common MinGW locations:
- `C:\mingw64\bin`
- `C:\msys64\mingw64\bin`
- `C:\Program Files\mingw-w64\...\mingw64\bin`

**Alternative Solutions:**
1. Use Microsoft Visual C++ compiler instead of MinGW (requires reconfiguring CMake)
2. Copy required DLLs to the build directory:
   - `libgcc_s_seh-1.dll`
   - `libstdc++-6.dll`
   - `libwinpthread-1.dll`
3. Run Python from MSYS2/MinGW terminal where PATH is already configured

**Linux/Mac:**
1. Ensure C++ runtime libraries are available (`libstdc++`)
2. Check `ldd build/slaythespire.*.so` to see missing dependencies
3. Install missing libraries via package manager

### Agent crashes or hangs

**Problem:** `agent.playout()` crashes or never returns

**Solution:**
1. Reduce `simulation_count_base` for testing
2. Enable `print_logs = True` to see progress
3. Check seed is valid (non-zero, reasonable value)
4. Ensure sufficient memory for MCTS tree

---

## Further Reading

- **MCTS API Reference:** `docs/MCTS_API_REFERENCE.md` - C++ interfaces for custom MCTS
- **Source Code:** `bindings/slaythespire.cpp` - Python bindings implementation
- **C++ Headers:** `include/` - Full C++ API documentation
