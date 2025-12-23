#!/usr/bin/env python3
"""
Simple test to check if the slaythespire module can be imported.
"""

import sys
import os

# Add build directory to path
build_dir = os.path.join(os.path.dirname(__file__), 'build')
sys.path.insert(0, build_dir)

print(f"Python version: {sys.version}")
print(f"Looking for module in: {build_dir}")
print(f"Module files in build/:")
for f in os.listdir(build_dir):
    if f.endswith(('.pyd', '.so')):
        print(f"  - {f}")

print("\nAttempting to import slaythespire...")

try:
    import slaythespire as sts
    print("✓ Successfully imported slaythespire module!")

    print("\nAvailable classes and functions:")
    for name in dir(sts):
        if not name.startswith('_'):
            obj = getattr(sts, name)
            print(f"  - {name}: {type(obj).__name__}")

    # Test basic functionality
    print("\nTesting basic functionality:")

    # Test enum access
    print(f"  CharacterClass.IRONCLAD = {sts.CharacterClass.IRONCLAD}")
    print(f"  CardId.STRIKE_RED = {sts.CardId.STRIKE_RED}")

    # Try creating a GameContext
    print("\n  Creating GameContext...")
    gc = sts.GameContext(sts.CharacterClass.IRONCLAD, 12345, 0)
    print(f"  ✓ GameContext created successfully")
    print(f"    HP: {gc.cur_hp}/{gc.max_hp}")
    print(f"    Deck size: {len(gc.deck)}")

    # Try creating an Agent
    print("\n  Creating Agent...")
    agent = sts.Agent()
    print(f"  ✓ Agent created successfully")
    print(f"    Default simulations: {agent.simulation_count_base}")

    print("\n✓ All basic tests passed!")

except ImportError as e:
    print(f"✗ Failed to import: {e}")
    print("\nPossible solutions:")
    print("1. Ensure the module is built:")
    print("   cd build && cmake --build . --target slaythespire")
    print("\n2. On Windows, MinGW DLLs must be in PATH:")
    print("   Add MinGW bin directory to PATH, e.g.:")
    print("   set PATH=%PATH%;C:\\mingw64\\bin")
    print("\n3. Rebuild with different compiler if issue persists")
    sys.exit(1)

except Exception as e:
    print(f"✗ Error during testing: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)