# Status Effects Architecture in sts_lightspeed

## Overview

The status effect (power) system in sts_lightspeed uses a hybrid storage approach combining **specific member variables**, **bitfields**, and **template metaprogramming** for type-safe, efficient handling.

## Monster Status Effects

### Storage Architecture

The `Monster` struct uses three complementary storage mechanisms:

#### 1. **Specific Member Variables** (Lines 51-66 in Monster.h)
Individual fields for most status effects:
```cpp
std::int8_t artifact = 0;
std::int8_t blockReturn = 0;
std::int8_t choked = 0;
std::int8_t corpseExplosion = 0;
std::int8_t lockOn = 0;
std::int16_t mark = 0;
std::int8_t metallicize = 0;
std::int8_t platedArmor = 0;
std::int8_t poison = 0;
std::int8_t regen = 0;
std::int8_t shackled = 0;
int strength = 0;
int vulnerable = 0;
int weak = 0;
```

#### 2. **Status Bitfield** (Line 50)
Tracks which statuses are currently active:
```cpp
std::uint64_t statusBits = 0;
```
- Each bit corresponds to a `MonsterStatus` enum value
- Bits 61-63 are used for "just applied" tracking (vulnerable, weak, ritual)
- Boolean powers (like ASLEEP, BARRICADE) only exist in this bitfield

#### 3. **Unique Power Slots** (Lines 68-69)
For mutually exclusive status effects:
```cpp
int uniquePower0 = 0;  // ANGRY through TIME_WARP
std::int16_t uniquePower1 = 0;  // INVINCIBLE, REACTIVE, SHARP_HIDE
```

### MonsterStatus Categories

The enum in `MonsterStatusEffects.h` organizes status effects into categories:

1. **Regular Statuses** (ARTIFACT through WEAK)
   - Have dedicated member variables
   - Tracked in statusBits
   - Can be stacked/have values

2. **Unique Powers Group 1** (ANGRY through TIME_WARP)
   - Only ONE can be active at a time per monster
   - Share `uniquePower0` for storage
   - Tracked in statusBits

3. **Unique Powers Group 2** (INVINCIBLE, REACTIVE, SHARP_HIDE)
   - Share `uniquePower1` for storage
   - Tracked in statusBits

4. **Boolean Powers** (ASLEEP through STASIS)
   - Only exist in statusBits (no separate storage)
   - Checked via `isBooleanPower()` helper

### Template Methods

All status manipulation uses compile-time dispatch:

```cpp
template <MonsterStatus s> bool hasStatus() const;
template <MonsterStatus s> int getStatus() const;
template <MonsterStatus s> void setHasStatus(bool value=true);
template <MonsterStatus s> void setStatus(int amount);
template <MonsterStatus s> void decrementStatus(int amount=1);
template <MonsterStatus s> void addDebuff(int amount, bool isSourceMonster=true);
template <MonsterStatus s> void removeStatus();
template <MonsterStatus s> void buff(int amount=1);
```

**Benefits:**
- Type-safe: compile-time checking of valid status effects
- Efficient: no runtime dispatch overhead
- Clear: intent is explicit at call site

**Example Usage:**
```cpp
if (monster.hasStatus<MS::VULNERABLE>()) {
    int vulnAmount = monster.getStatus<MS::VULNERABLE>();
    monster.decrementStatus<MS::VULNERABLE>();
}
monster.buff<MS::STRENGTH>(2);
```

### Runtime Methods

For dynamic dispatch (e.g., from Python or debugging):
```cpp
bool hasStatusInternal(MonsterStatus s) const;
int getStatusInternal(MonsterStatus s) const;
```

These use switch statements internally and are marked "only to be used by printLogs methods."

## Player Status Effects

### Storage Architecture

The `Player` struct (Player.h) uses a similar but distinct approach:

#### 1. **Special Member Variables**
Core stats stored directly:
```cpp
int artifact = 0;
int dexterity = 0;
int focus = 0;
int strength = 0;
```

#### 2. **Status Bitfields** (Lines 56-58)
Two bitfields to support >64 status effects:
```cpp
std::uint32_t justAppliedBits = 0;  // For debuffs that need JustApplied tracking
std::uint64_t statusBits0 = 0;      // First 64 status effects
std::uint32_t statusBits1 = 0;      // Additional status effects
```

#### 3. **Status Map**
For most status values:
```cpp
std::map<PlayerStatus, std::int16_t> statusMap;
```
- Only contains entries for active statuses (sparse storage)
- Removed when status reaches 0

### PlayerStatus Categories

From `PlayerStatusEffects.h`:

1. **JustApplied Statuses** (VULNERABLE, WEAK, FRAIL, etc.)
   - Track whether they were applied this turn
   - Affects when they decrement

2. **Debuffs** (BIAS, CONFUSED, CONSTRICTED, etc.)
   - Negative effects applied by enemies

3. **Boolean Powers** (BARRICADE, CORRUPTION, etc.)
   - True/false presence

4. **Counter Powers** (AMPLIFY, BLUR, BUFFER, etc.)
   - Decremented each use/turn

5. **Intensity Powers** (ACCURACY, DEMON_FORM, etc.)
   - Stack and apply multiple times

### Template Methods

Similar to Monster but uses statusMap:
```cpp
template <PlayerStatus s> bool hasStatus() const;
template <PlayerStatus s> int getStatus() const;
template <PlayerStatus s> void setHasStatus(bool value);
template <PlayerStatus s> void removeStatus();
template <PlayerStatus s> void decrementStatus(int amount=1);
template <PlayerStatus s> void buff(int amount=1);
template <PlayerStatus s> void debuff(int amount, bool isSourceMonster=true);
```

## Current Python Bindings

### What's Currently Exposed (slaythespire.cpp)

#### PlayerStatus Enum (Lines 845-865)
Only a **partial subset** is exposed:
```python
PlayerStatus.VULNERABLE
PlayerStatus.WEAK
PlayerStatus.FRAIL
PlayerStatus.INTANGIBLE
PlayerStatus.ARTIFACT
PlayerStatus.BARRICADE
PlayerStatus.METALLICIZE
PlayerStatus.PLATED_ARMOR
PlayerStatus.BLUR
PlayerStatus.DEMON_FORM
PlayerStatus.COMBUST
PlayerStatus.FLAME_BARRIER
PlayerStatus.FEEL_NO_PAIN
PlayerStatus.DARK_EMBRACE
PlayerStatus.EVOLVE
PlayerStatus.CORRUPTION
PlayerStatus.BRUTALITY
PlayerStatus.RUPTURE
```

#### BattleContext Methods (Lines 1000-1007)
Manual setters for specific monster statuses:
```python
bc.set_monster_strength(idx, value)
bc.set_monster_vulnerable(idx, value)
bc.set_monster_weak(idx, value)
bc.set_monster_poison(idx, value)
```

Manual setter for player statuses:
```python
bc.set_player_strength(value)
bc.set_player_dexterity(value)
bc.set_player_status(PlayerStatus, value)
```

### Problems with Current Approach

1. **Incomplete**: Only ~18/100+ PlayerStatus values exposed
2. **No MonsterStatus enum**: Can't reference monster statuses from Python
3. **Hardcoded**: Each status needs manual binding code
4. **Inconsistent**: mix of direct setters and generic setter
5. **No getters**: Can't read monster/player statuses from Python
6. **No type safety**: `set_player_status` takes any PlayerStatus but may not work for all

## Recommended Python API Design

### Option 1: Expose Runtime Methods (Recommended)

**Pros:**
- Simple to implement
- Pythonic interface
- Works for all statuses without manual bindings

**Implementation:**
```cpp
// In slaythespire.cpp

// Expose complete enums
pybind11::enum_<MonsterStatus>(m, "MonsterStatus")
    .value("ARTIFACT", MonsterStatus::ARTIFACT)
    .value("BLOCK_RETURN", MonsterStatus::BLOCK_RETURN)
    .value("CHOKED", MonsterStatus::CHOKED)
    // ... all values from MonsterStatusEffects.h
    .value("INVALID", MonsterStatus::INVALID);

pybind11::enum_<PlayerStatus>(m, "PlayerStatus")
    .value("INVALID", PlayerStatus::INVALID)
    .value("DOUBLE_DAMAGE", PlayerStatus::DOUBLE_DAMAGE)
    // ... all values from PlayerStatusEffects.h
    .export_values();

// Add to BattleContext binding
.def("get_monster_status", [](const BattleContext &bc, int idx, MonsterStatus status) {
    if (idx < 0 || idx >= bc.monsters.monsterCount) {
        throw pybind11::index_error("Monster index out of range");
    }
    return bc.monsters.arr[idx].getStatusInternal(status);
}, "Get monster status value")
.def("has_monster_status", [](const BattleContext &bc, int idx, MonsterStatus status) {
    if (idx < 0 || idx >= bc.monsters.monsterCount) {
        throw pybind11::index_error("Monster index out of range");
    }
    return bc.monsters.arr[idx].hasStatusInternal(status);
}, "Check if monster has status")
.def("set_monster_status", [](BattleContext &bc, int idx, MonsterStatus status, int amount) {
    if (idx < 0 || idx >= bc.monsters.monsterCount) {
        throw pybind11::index_error("Monster index out of range");
    }
    // Need to add runtime setStatus method to Monster
    // For now, could use a switch statement like getStatusInternal
    // ... implementation needed
}, "Set monster status value")

.def("get_player_status", [](const BattleContext &bc, PlayerStatus status) {
    return bc.player.getStatusRuntime(status);
}, "Get player status value")
.def("has_player_status", [](const BattleContext &bc, PlayerStatus status) {
    return bc.player.hasStatusRuntime(status);
}, "Check if player has status")
.def("set_player_status", [](BattleContext &bc, PlayerStatus status, int amount) {
    // Implementation needed - runtime version of setStatus
    // ... 
}, "Set player status value");
```

**Python Usage:**
```python
# Check monster poison
if bc.has_monster_status(0, MonsterStatus.POISON):
    poison_amt = bc.get_monster_status(0, MonsterStatus.POISON)
    
# Set player vulnerable
bc.set_player_status(PlayerStatus.VULNERABLE, 2)

# Check player has barricade
if bc.has_player_status(PlayerStatus.BARRICADE):
    # ...
```

### Option 2: Expose Monster/Player Structs Directly

**Pros:**
- Most flexible
- Can use properties
- Direct access to all fields

**Cons:**
- Exposes internals
- More complex bindings
- Template methods hard to expose

**Implementation:**
```cpp
pybind11::class_<Monster>(m, "Monster")
    .def_property("hp",
        [](const Monster &m) { return m.curHp; },
        [](Monster &m, int hp) { m.curHp = hp; })
    .def_property("strength",
        [](const Monster &m) { return m.strength; },
        [](Monster &m, int s) { m.strength = s; })
    .def("get_status", &Monster::getStatusInternal)
    .def("has_status", &Monster::hasStatusInternal);
    // etc...

// Then in BattleContext:
.def("get_monster", [](BattleContext &bc, int idx) -> Monster& {
    if (idx < 0 || idx >= bc.monsters.monsterCount) {
        throw pybind11::index_error("Monster index out of range");
    }
    return bc.monsters.arr[idx];
}, pybind11::return_value_policy::reference_internal);
```

**Python Usage:**
```python
monster = bc.get_monster(0)
monster.hp = 50
monster.strength = 2
if monster.has_status(MonsterStatus.VULNERABLE):
    vuln = monster.get_status(MonsterStatus.VULNERABLE)
```

### Option 3: Hybrid Approach (Best Balance)

Combine both approaches:
- Direct access via BattleContext methods (simple, safe)
- Optional Monster/Player struct access (for advanced use)

## Implementation Roadmap

1. **Add runtime setter methods** to Monster and Player classes
   - `Monster::setStatusInternal(MonsterStatus s, int amount)`
   - Already have getters

2. **Complete the enum bindings**
   - Expose all MonsterStatus values
   - Expose all PlayerStatus values

3. **Add generic status methods to BattleContext binding**
   - `get_monster_status(idx, status)`
   - `has_monster_status(idx, status)`
   - `set_monster_status(idx, status, amount)`
   - `get_player_status(status)`
   - `has_player_status(status)`
   - `set_player_status(status, amount)`

4. **Remove manual setters** (or keep for backward compatibility)
   - `set_monster_strength`, etc. → use generic method

5. **Document the status effect system** for Python users
   - Which statuses do what
   - Value ranges
   - Boolean vs counted vs intensity

## Design Rationale

### Why Templates in C++?

1. **Performance**: No virtual function calls or runtime dispatch
2. **Type Safety**: Impossible to use invalid status at compile time
3. **Optimization**: Compiler can inline and optimize each specific case
4. **Code Generation**: Single template generates all needed variants

### Why Runtime Methods for Python?

1. **Python is dynamic**: Can't use C++ templates from Python
2. **Simplicity**: One method handles all statuses
3. **Maintainability**: No need to update bindings when adding statuses
4. **Performance acceptable**: Python overhead dominates anyway

### Why Hybrid Storage?

1. **Memory Efficiency**: 
   - Most monsters don't have most statuses
   - Bitfield is 8 bytes for presence tracking
   - Individual fields only for common/likely statuses

2. **Access Speed**:
   - Direct member access is fastest
   - Bitfield check is one AND operation
   - uniquePower0/1 saves space for exclusive powers

3. **Game Semantics**:
   - Some monsters can only have one "unique" power
   - Boolean powers don't need values
   - Common statuses benefit from direct access

## Key Differences: Monster vs Player

| Aspect | Monster | Player |
|--------|---------|--------|
| **Status Count** | ~44 statuses | ~100+ statuses |
| **Value Storage** | Direct members | statusMap |
| **Bitfield** | 1x uint64 | 2x (uint64 + uint32) |
| **Unique Powers** | 2 slots (uniquePower0/1) | None |
| **Runtime Access** | hasStatusInternal, getStatusInternal | hasStatusRuntime, getStatusRuntime |

## Conclusion

The sts_lightspeed status system is well-designed for C++ performance:
- Template methods provide type-safe, zero-overhead access
- Hybrid storage balances memory and speed
- Bitfields efficiently track presence

For Python bindings, we should:
- Use runtime dispatch methods (already exist for reading)
- Add runtime setters to complete the API
- Expose complete enums for all statuses
- Provide simple, consistent interface through BattleContext
- Optionally expose Monster/Player directly for advanced users
