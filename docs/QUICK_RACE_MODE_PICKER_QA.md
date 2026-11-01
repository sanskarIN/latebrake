# Quick Race — choose opponent circuit or solo Time Attack

**Unreal Engine 5.8 SOURCE ONLY.** Existing C++ gameplay and UI wiring
is not yet validated in a licensed Unreal 5.8 editor/game package.

## What players can do

Open the main menu and navigate to **Quick Race**. Use the existing
**BROWSE** action: `N` on keyboard, gamepad D-pad Right or the Android
menu Browse touch pad (actual device unverified). The choices cycle:

1. **STANDARD CIRCUIT** (default): 3 laps, one prototype physical AI rival.
2. **EXPERT CIRCUIT**: same real 3-lap event with a slightly more
   aggressive straight-line physical AI throttle policy.
3. **SOLO TIME ATTACK**: the existing genuine **one-lap** event,
   **no AI rival**, using the real time-attack personal-best record.
4. **ROOKIE CIRCUIT**: 3 laps, one prototype rival with lower
   allowed forward throttle demand; then cycles to Standard.

Press **Select/Enter** (or the existing on-screen menu button) to
start the *selected* event. It uses the ordinary authoritative event
configuration: real checkpoint count, race reset, full start-light
countdown and the correct spawned/destroyed rival lifecycle.
A second `RestartRace()` is not issued if event selection already
restarted the race.

Changing the highlighted choice while the menu is open changes
**only the pending menu selection**. It cannot change the running
event, cause racing rewards or retune the existing rival until the
player explicitly starts a NEW session. Previous keyboard shortcuts
`3` for Circuit and `4` for Time Attack remain valid.

## Fairness and limits

- Existing career/championship/challenges remain separate. Career always
  runs Standard AI when it requests an opponent.
- No new circuit map, opponents, physics objects, SaveGame fields,
  rewards or purchases are created by this change.
- Malformed option data falls back to the original Standard circuit.
- The AI throttle difficulty is not an official competition ranking,
  live place/position, measured performance benchmark or guarantee.
- `QuickRaceModePicker` C++20 tests exercise all four cyclic choices,
  real one-lap / three-lap event policies, invalid inputs and repeated
  browse stress. Python source checks verify engine binding, menu
  visibility and existing keyboard/gamepad/mobile input paths.

Still required: licensed Unreal 5.8/UHT compile, real menu interaction,
installed Windows/Android playtest, original cooked track and physical
rival/graphics QA (see GitHub issue #148).
