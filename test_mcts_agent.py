#!/usr/bin/env python3
"""
Example script to test the MCTS agent with the Slay the Spire simulator.

This script demonstrates:
1. Importing the compiled slaythespire module
2. Creating a game context
3. Running the MCTS agent
4. Inspecting results
"""

import sys
import os
import random

# Add build directory to path
build_dir = os.path.join(os.path.dirname(__file__), 'build')
sys.path.insert(0, build_dir)

try:
    import slaythespire as sts
except ImportError as e:
    print(f"Error importing slaythespire module: {e}")
    print(f"\nMake sure the module is built:")
    print("  1. cd build")
    print("  2. cmake --build . --target slaythespire")
    print(f"\nLooking for module in: {build_dir}")
    sys.exit(1)


def main():
    """Run a simple MCTS agent test"""

    print("=" * 60)
    print("Slay the Spire MCTS Agent Test")
    print("=" * 60)

    # Configuration
    seed = random.randint(0, 2**32 - 1)
    character = sts.CharacterClass.IRONCLAD
    ascension_level = 0
    simulations_per_battle = 10000  # Reduced for faster testing

    print(f"\nConfiguration:")
    print(f"  Character: {character}")
    print(f"  Seed: {seed}")
    print(f"  Ascension: {ascension_level}")
    print(f"  Simulations per battle: {simulations_per_battle}")

    # Create game context
    print("\nCreating game context...")
    gc = sts.GameContext(character, seed, ascension_level)

    # Inspect initial state
    print(f"\nInitial State:")
    print(f"  HP: {gc.cur_hp}/{gc.max_hp}")
    print(f"  Gold: {gc.gold}")
    print(f"  Act: {gc.act}, Floor: {gc.floor_num}")
    print(f"  Deck size: {len(gc.deck)}")
    print(f"  Relics: {len(gc.relics)}")

    # Create and configure agent
    print("\nConfiguring MCTS agent...")
    agent = sts.Agent()
    agent.simulation_count_base = simulations_per_battle
    agent.boss_simulation_multiplier = 2.0  # 2x simulations for bosses
    agent.print_logs = True  # Show progress
    agent.pause_on_card_reward = False

    # Run the agent
    print("\n" + "=" * 60)
    print("Running MCTS agent (this may take a while)...")
    print("=" * 60 + "\n")

    try:
        agent.playout(gc)
    except KeyboardInterrupt:
        print("\n\nInterrupted by user!")
    except Exception as e:
        print(f"\n\nError during playout: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

    # Display results
    print("\n" + "=" * 60)
    print("Results")
    print("=" * 60)

    if gc.outcome == sts.GameOutcome.PLAYER_VICTORY:
        print(f"✓ VICTORY!")
        print(f"\nFinal Stats:")
        print(f"  HP: {gc.cur_hp}/{gc.max_hp}")
        print(f"  Act: {gc.act}, Floor: {gc.floor_num}")
    else:
        print(f"✗ Defeat")
        print(f"\nDied on:")
        print(f"  Act: {gc.act}, Floor: {gc.floor_num}")

    print(f"\nFinal Resources:")
    print(f"  Gold: {gc.gold}")
    print(f"  Deck size: {len(gc.deck)}")
    print(f"  Relics: {len(gc.relics)}")

    # Show final deck
    print(f"\nFinal Deck ({len(gc.deck)} cards):")
    from collections import Counter
    card_names = [str(card) for card in gc.deck]
    card_counts = Counter(card_names)
    for card_name, count in sorted(card_counts.items()):
        if count > 1:
            print(f"  {count}x {card_name}")
        else:
            print(f"  {card_name}")

    # Show relics
    print(f"\nRelics ({len(gc.relics)}):")
    for relic in gc.relics:
        print(f"  - {relic.id}")

    print("\n" + "=" * 60)


if __name__ == "__main__":
    main()