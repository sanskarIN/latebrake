# Quick Race: Rookie / Standard / Expert rival difficulty

**Milestone:** Unreal Engine 5.8 source-only development. No verified Unreal
build, installed Windows executable, Android playtest, or published game.

## Player interaction

Open **Quick Race** in the existing in-game Canvas menu. Use **BROWSE**
(`N` on keyboard, `Gamepad D-pad Right`, existing Android menu BROWSE)
to cycle **STANDARD CIRCUIT -> EXPERT CIRCUIT -> SOLO TIME ATTACK ->
ROOKIE CIRCUIT -> STANDARD CIRCUIT**. The SOLO option uses the existing
single-lap Time Attack event and spawns no rival. No new track is invented.
See [Quick Race mode picker QA](QUICK_RACE_MODE_PICKER_QA.md). The selection is displayed
in the Quick Race menu. Choose **Select/Enter** to start a NEW race
with the selected tier. Closing the menu without starting a new race
does **not** retune an ongoing opponent.

- ROOKIE: the real rival's forward motor demand is capped at 0.55.
- STANDARD (default): identical to the earlier rival controller;
  no behavioral change.
- EXPERT: raises the planner's already-positive motor demand by at most
  0.08 on observed straights with modest speed, never near sharp steering,
  under braking, at rest, at high speed or after route completion.

All numbers are **controller throttle fractions**, not guaranteed lap
times, top speed or proof of an AI skill rating. Safety decisions for
verified gate traces, tire grip, off-track traction, collision damage,
engine heat, physical traffic, yaw rate and spin protection still apply.
The tuning cannot turn an earlier BRAKE into acceleration.

## Career fairness and existing saves

This is only a *session-local* Quick Race selection; no SaveGame fields
or ownership/economy changes are introduced. A newly started career
event ALWAYS snapshots STANDARD, regardless of the Quick Race selection.
Cycling options while a race is in progress changes the **pending**
selection only; the actual rival stays on its last latched difficulty
until a successful restart. Invalid/corrupt tier values fail closed
inside the motor governor.

## Tests and limits

Portable C++20 tests cover cycling, names, three distinct throttle
outputs, career mode restriction, planner brake/arrival preservation,
zero-demand/steering/high-speed exclusions, malformed physics input and
100k bounded predictions. Python source-binding tests verify
existing keyboard/controller/touch input parity, the menu, race-latching
and ordering before the already-tested AI safety governors.

Real licensed Unreal 5.8 editor compilation, physics behaviour, head-to-head
lap-time tuning on different hardware, cooked game/circuit, Windows setup
and Android lifecycle tests are still necessary. See release gate #148 and
Android #13. This is not a stable release and does not create a playable
executable.
