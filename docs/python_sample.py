import sys

STS_BUILD_PATH = r"C:\Users\dave_\dev\sts_lightspeed\build"

sys.path.insert(0, STS_BUILD_PATH)
import slaythespire as sts

# Create game and agent
gc = sts.GameContext(sts.CharacterClass.IRONCLAD, 12345, 0)
agent = sts.Agent()
agent.simulation_count_base = 10000  # simulations per battle

# Run campaign
agent.playout(gc)

# Check results
if gc.outcome == sts.GameOutcome.PLAYER_VICTORY:
    print(f"Victory! HP: {gc.cur_hp}/{gc.max_hp}")