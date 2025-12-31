#!/usr/bin/env python3
"""
Collect progression data and export to JSON
"""

import sys
sys.path.insert(0, 'build')

import slaythespire as sts
import json
from typing import List, Dict, Any

def card_to_dict(card) -> Dict[str, Any]:
    """Convert Card to dictionary"""
    return {
        'id': str(card.id),
        'upgraded': card.upgraded,
        'rarity': str(card.rarity)
    }

def shop_to_dict(shop) -> Dict[str, Any]:
    """Convert ShopData to dictionary"""
    return {
        'cards': [
            {
                'id': str(card.id),
                'upgraded': card.upgraded,
                'price': price
            }
            for card, price in zip(shop.cards, shop.card_prices)
        ],
        'relics': [
            {
                'id': str(relic),
                'price': price
            }
            for relic, price in zip(shop.relics, shop.relic_prices)
        ],
        'potions': [
            {
                'id': str(potion),
                'price': price
            }
            for potion, price in zip(shop.potions, shop.potion_prices)
        ],
        'remove_cost': shop.remove_cost
    }

def node_to_dict(node) -> Dict[str, Any]:
    """Convert NodeRecord to dictionary"""
    data = {
        'floor_num': node.floor_num,
        'x': node.x,
        'y': node.y,
        'room_type': str(node.room_type)
    }

    # Add encounter if present
    if node.encounter != sts.MonsterEncounter.INVALID:
        data['encounter'] = str(node.encounter)

    # Add card rewards
    if node.card_rewards:
        data['card_rewards'] = [
            [card_to_dict(card) for card in reward_set]
            for reward_set in node.card_rewards
        ]

    # Add shop data
    if node.shop is not None:
        data['shop'] = shop_to_dict(node.shop)

    # Add event data
    if node.event_type != sts.Event.INVALID:
        data['event'] = {
            'type': str(node.event_type),
            'option_chosen': node.option_chosen
        }

        if node.cards_offered:
            data['event']['cards_offered'] = [card_to_dict(c) for c in node.cards_offered]

        if node.relics_offered:
            data['event']['relics_offered'] = [str(r) for r in node.relics_offered]

        if node.curses_gained:
            data['event']['curses_gained'] = [card_to_dict(c) for c in node.curses_gained]

    # Add treasure data - check against INVALID, not SMALL (which is a valid size)
    if node.chest_size != sts.ChestSize.INVALID:
        data['chest'] = {
            'size': str(node.chest_size),
            'relics': [str(r) for r in node.chest_relics]
        }

    # Add boss relics
    if any(r != sts.RelicId.INVALID for r in node.boss_relics):
        data['boss_relics'] = [str(r) for r in node.boss_relics if r != sts.RelicId.INVALID]

    # Add rest site upgradeable cards
    if node.upgradeable_cards:
        data['upgradeable_cards'] = [card_to_dict(c) for c in node.upgradeable_cards]

    return data

def run_to_dict(run) -> Dict[str, Any]:
    """Convert RunProgression to dictionary"""
    return {
        'seed': run.seed,
        'character': str(run.character_class),
        'ascension': run.ascension,
        'outcome': str(run.outcome),
        'act_reached': run.act_reached,
        'floor_reached': run.floor_reached,
        'nodes': [node_to_dict(node) for node in run.nodes]
    }

def collect_runs(
    character: sts.CharacterClass,
    num_runs: int,
    ascension: int = 0,
    start_seed: int = 1000000
) -> List[Dict[str, Any]]:
    """Collect multiple runs and convert to dictionaries"""

    collector = sts.ProgressionDataCollector()
    runs = []

    print(f"Collecting {num_runs} runs for {character} at A{ascension}...")

    seed = start_seed
    collected = 0
    attempts = 0
    max_attempts = num_runs * 3

    while collected < num_runs and attempts < max_attempts:
        attempts += 1

        try:
            run = collector.collect_run(character, seed=seed, ascension=ascension)

            # Only keep successful or substantially progressed runs
            if run.outcome != sts.GameOutcome.UNDECIDED or run.floor_reached > 20:
                runs.append(run_to_dict(run))
                collected += 1

                if collected % 100 == 0:
                    print(f"  Collected {collected}/{num_runs} runs (tried {attempts} seeds)")

        except Exception as e:
            print(f"  Warning: Run with seed {seed} failed: {e}")

        seed += 1

    print(f"Collected {collected} runs in {attempts} attempts")
    return runs

def main():
    """Main data collection script"""

    # Configuration
    config = {
        'num_runs': 10,  # Number of runs to collect
        'ascension': 0,
        'characters': [
            sts.CharacterClass.IRONCLAD,
            # Add more characters as needed:
            # sts.CharacterClass.SILENT,
            # sts.CharacterClass.DEFECT,
            # sts.CharacterClass.WATCHER,
        ],
        'output_file': 'progression_data.json'
    }

    all_runs = []

    for character in config['characters']:
        runs = collect_runs(
            character=character,
            num_runs=config['num_runs'],
            ascension=config['ascension']
        )
        all_runs.extend(runs)

    # Save to JSON
    output_data = {
        'version': '1.0',
        'total_runs': len(all_runs),
        'config': {
            'ascension': config['ascension'],
            'characters': [str(c) for c in config['characters']]
        },
        'runs': all_runs
    }

    print(f"\nSaving {len(all_runs)} runs to {config['output_file']}...")
    with open(config['output_file'], 'w') as f:
        json.dump(output_data, f, indent=2)

    print(f"Done! Saved to {config['output_file']}")

    # Print summary statistics
    total_nodes = sum(len(run['nodes']) for run in all_runs)
    total_card_rewards = sum(
        len(node.get('card_rewards', []))
        for run in all_runs
        for node in run['nodes']
    )
    total_shops = sum(
        1 for run in all_runs
        for node in run['nodes']
        if 'shop' in node
    )

    print(f"\n=== Summary ===")
    print(f"Total runs: {len(all_runs)}")
    print(f"Total nodes: {total_nodes}")
    print(f"Total card reward sets: {total_card_rewards}")
    print(f"Total shops: {total_shops}")
    print(f"Average nodes per run: {total_nodes / len(all_runs):.1f}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n[FAILED] {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
