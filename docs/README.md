# STS Lightspeed Documentation

Documentation for the Slay the Spire Lightspeed simulator and MCTS implementation.

## Available Documentation

### [MCTS API Reference](MCTS_API_REFERENCE.md)
**For:** Developers implementing custom MCTS agents in C++

High-level index of key interfaces for Monte Carlo Tree Search implementation:
- State representation (BattleContext, GameContext)
- Action enumeration (Action, GameAction)
- State transitions
- Existing MCTS implementation reference
- Quick reference with code locations

### [Python Bindings Usage Guide](PYTHON_BINDINGS_USAGE.md)
**For:** Developers using the Python bindings

Complete guide to using the Python interface:
- Building the Python module
- Available classes and functions
- Usage examples
- Limitations and troubleshooting

## Quick Start

### C++ MCTS Development
See [MCTS_API_REFERENCE.md](MCTS_API_REFERENCE.md) for:
- Key entry points: `Action::enumerateCardSelectActions()`, `GameAction::getAllActionsInState()`
- Existing implementation: `BattleScumSearcher2`
- File locations and line numbers

### Python Usage
See [PYTHON_BINDINGS_USAGE.md](PYTHON_BINDINGS_USAGE.md) for:
- How to build: `cmake --build . --target slaythespire`
- How to import: `import slaythespire as sts`
- Example scripts: `test_mcts_agent.py`, `test_import.py`

## Test Scripts

Located in project root:

### `test_import.py`
Simple test to verify the Python module can be imported and basic functionality works.

```bash
python test_import.py
```

### `test_mcts_agent.py`
Full example running the MCTS agent for a complete campaign.

```bash
python test_mcts_agent.py
```

## Common Issues

### Windows: DLL Load Failed
If you get `DLL load failed while importing slaythespire`:

```bash
# Add MinGW bin directory to PATH
set PATH=%PATH%;C:\mingw64\bin

# Then run Python
python test_import.py
```

See [Python Bindings Troubleshooting](PYTHON_BINDINGS_USAGE.md#troubleshooting) for details.

### Missing Python Bindings for MCTS
The current Python bindings only expose high-level campaign simulation. For custom MCTS:
- **Option 1:** Work in C++ (see MCTS_API_REFERENCE.md)
- **Option 2:** Extend Python bindings by adding `BattleContext`, `Action`, `GameAction` to `bindings/slaythespire.cpp`

## Source Code Locations

| Component | Location |
|-----------|----------|
| Python bindings | `bindings/slaythespire.cpp` |
| MCTS implementation | `src/sim/search/BattleScumSearcher2.cpp` |
| Action enumeration | `src/sim/search/Action.cpp` |
| Game state | `include/game/GameContext.h` |
| Battle state | `include/combat/BattleContext.h` |
| Build configuration | `CMakeLists.txt` |