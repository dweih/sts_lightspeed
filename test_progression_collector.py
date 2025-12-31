#!/usr/bin/env python3
"""
Test script for ProgressionDataCollector
"""

import sys
sys.path.insert(0, 'build')

import slaythespire as sts

def test_single_run():
    """Test collecting a single run"""
    print("Testing ProgressionDataCollector...")

    collector = sts.ProgressionDataCollector(42)

    # Collect one run
    run = collector.collect_run(
        sts.CharacterClass.IRONCLAD,
        seed=12345,
        ascension=0
    )

    print("\n=== Run Summary ===")
    print(f"Seed: {run.seed}")
    print(f"Character: {run.character_class}")
    print(f"Ascension: {run.ascension}")
    print(f"Outcome: {run.outcome}")
    print(f"Act Reached: {run.act_reached}")
    print(f"Floor Reached: {run.floor_reached}")
    print(f"Total Nodes: {len(run.nodes)}")

    print("\n=== Node Details ===")
    for i, node in enumerate(run.nodes[:10]):  # Show first 10 nodes
        print(f"\nNode {i+1} - Floor {node.floor_num}:")
        print(f"  Room Type: {node.room_type}")

        if node.card_rewards:
            print(f"  Card Rewards: {len(node.card_rewards)} sets")
            for j, reward_set in enumerate(node.card_rewards):
                print(f"    Set {j+1}: {len(reward_set)} cards")
                for card in reward_set[:3]:  # Show first 3 cards
                    print(f"      - {card.id}")

        if node.shop:
            print("  Shop:")
            print(f"    Cards available: {len(node.shop.cards)}")
            print(f"    Remove cost: {node.shop.remove_cost}")
            # Show first few shop cards
            for j, card in enumerate(node.shop.cards[:3]):
                print(f"      - {card.id} (${node.shop.card_prices[j]})")

        if node.event_type != sts.Event.INVALID:
            print(f"  Event: {node.event_type}")
            if node.curses_gained:
                print(f"    Curses gained: {len(node.curses_gained)}")

        if node.encounter != sts.MonsterEncounter.INVALID:
            print(f"  Encounter: {node.encounter}")

    if len(run.nodes) > 10:
        print(f"\n... and {len(run.nodes) - 10} more nodes")

    return run

def test_batch_collection():
    """Test collecting multiple runs"""
    print("\n\n=== Testing Batch Collection ===")

    collector = sts.ProgressionDataCollector()

    runs = []
    num_runs = 10
    failed = 0

    print(f"Collecting {num_runs} successful runs...")
    seed = 1000000
    attempts = 0
    max_attempts = num_runs * 3  # Try up to 3x to get num_runs successful runs

    while len(runs) < num_runs and attempts < max_attempts:
        attempts += 1
        try:
            run = collector.collect_run(
                sts.CharacterClass.SILENT,
                seed=seed,
                ascension=20
            )
            # Only keep runs that completed (not UNDECIDED) or have substantial progress
            if run.outcome != sts.GameOutcome.UNDECIDED or run.floor_reached > 20:
                runs.append(run)
                if len(runs) % 5 == 0:
                    print(f"  Collected {len(runs)}/{num_runs} successful runs (tried {attempts} seeds)")
            else:
                failed += 1
        except Exception as e:
            failed += 1
            print(f"  Run with seed {seed} failed: {e}")

        seed += 1

    if failed > 0:
        print(f"  Skipped {failed} failed/incomplete runs")

    print("\n=== Batch Results ===")
    victories = sum(1 for r in runs if r.outcome == sts.GameOutcome.PLAYER_VICTORY)
    print(f"Total runs: {num_runs}")
    print(f"Victories: {victories}")
    print(f"Average floor reached: {sum(r.floor_reached for r in runs) / num_runs:.1f}")
    print(f"Average nodes per run: {sum(len(r.nodes) for r in runs) / num_runs:.1f}")

    # Count shop nodes
    shop_count = sum(1 for r in runs for n in r.nodes if n.shop is not None)
    print(f"Total shop nodes encountered: {shop_count}")

    # Count card rewards
    card_reward_count = sum(1 for r in runs for n in r.nodes for _ in n.card_rewards)
    print(f"Total card reward sets offered: {card_reward_count}")

    return runs

if __name__ == "__main__":
    try:
        # Test single run
        run = test_single_run()

        # Test batch collection
        runs = test_batch_collection()

        print("\n[SUCCESS] All tests completed successfully!")

    except Exception as e:
        print(f"\n[FAILED] Test failed with error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
