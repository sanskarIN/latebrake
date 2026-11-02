# Automatic four-speed gearbox — Unreal chassis source QA

**Status:** actual Unreal Engine 5.8 player/rival rigid-body source has
a new automatic shift policy; this is **not** a genuine modeled geartrain,
working real engine tachometer, certified shifting benchmark or installed
playable Windows game. Physical tuning still requires UE/UHT compilation,
cooking and hands-on hardware validation.

## What changes in the game source

LateBrake's existing physics car originally had one motor force and a
continuous forward/reverse speed limiter. The newly integrated
`AutomaticTransmission` adds a *per-vehicle, session-local* 4-forward-
gear and reverse control:

- Uses the **measured signed longitudinal chassis velocity** and
  configured handling-mode/garage forward-speed cap.
- Automatically upshifts and downshifts at bounded ratios of that cap,
  with separate hysteresis to avoid rapid gear hunting.
- Shifts one ratio at a time and reduces ordinary positive motor
  propulsion briefly (0.18 seconds) while shifting. The **same**
  conservative multiplier applies to base motor, nitro and slipstream,
  while pre-existing speed/power limits remain in force.
- Rejects immediate forward/reverse engagement at high opposite
  chassis speeds, preserving the existing opposite-pedal engine-
  braking behavior without instant gear-state reversal.
- Provides **estimated** idle-to-redline RPM as a function of measured
  chassis movement and effective gear ratio (900–7200). This is NOT a
  physical engine RPM sensor, true torque curve or engine sound model.
- Adds a desktop-only, space-aware HUD readout:
  `AUTO GEAR 3 | RPM EST 4900` and `| SHIFT` during a transition.
  Compact and mobile displays retain authoritative clock/gate rows.

The gearbox does **not** create a legitimate wheel/engine drivetrain:
the car remains one Unreal Chaos physics cube with procedural wheel
decorations and artificial motor force. It must not alter checkpoint
gates, career medals, SaveGame, recoveries, event selection or rival
race timing. Existing other motor force budgets and hard speed caps
still apply.

## Regression tests

- `Tests/Standalone/AutomaticTransmissionTests.cpp`: all four gears,
  up/down hysteresis, bounded shift interruption, engine-brake vs
  reverse selection, reverse-to-forward gating, RPM bounds,
  invalid physics inputs, frame hitches, reset isolation and 3,000
  rapidly changing physical samples.
- `Tests/Editor/UnrealAutomaticTransmissionBindingTests.py`:
  checks the actual shared player/rival Unreal pawn owns gearbox
  state, gates it behind valid driving physics, composes it into
  existing motor/boost force and exposes an honest estimated HUD.

These compile/run only pure portable C++20 and Python source checks.
They do not run Unreal, package a game or prove a good driving feel.

## Real hardware qualification needed

1. Compile in genuine UE 5.8/UHT and package Win64 using the existing
   `Build-Windows-Playtest.cmd` workflow.
2. Drive all six selected vehicles in Arcade and Simulation Inspired.
   Verify four upshifts and correct downshifts on a real track.
3. Test hard acceleration, liftoff/coasting, braking from 100km/h,
   rapidly changing throttle, reversing, power-on slides, nitro during
   shift and rival complete laps.
4. Confirm gear/RPM labels have no rapid flicker, no "R" at high forward
   speed, no false claim of real engine sensor readings and no
   hidden HUD overlap on compact Windows and mobile screens.
5. Compare frame-rate stability at 30/60/120FPS; tune shift times and
   approximate ratios in physical Chaos gameplay with the previously
   merged drag/yaw models. Check that shifts do not stop the car
   prematurely before the required lap speed.
6. Verify pause/restart/focus loss, no accidental continuing motor
   torque, re-entry at first gear, correct AI response and no saved
   race/garage metadata corruption.

Real vehicle suspension, wheel friction, engine sounds, gearbox and
tire forces, multiple tracks, better vehicle models and actual device
testing remain essential. See
[the playable release gate](https://github.com/sanskarIN/latebrake/issues/148).
