# Player stability assist display — source QA

**Development only.** This is a race-local visual indicator for the
prototype Arcade car, not a playable release, independent driving
qualification or a real tire/ABS/ESC sensor. No authorized Unreal Engine
5.8 compilation, genuine map cooking or installed Win64/Android driving
test has validated this change.

## Driver-facing change

The existing [human spin traction](HUMAN_SPIN_TRACTION_QA.md) policy
limits **positive motor demand** only when real Chaos forward motion,
lateral slip and angular yaw jointly indicate a severe high-speed spin.
Previously that intervention was invisible to the driver.

The expanded desktop HUD now conditionally shows:

```text
SPIN ASSIST  45% POSITIVE THROTTLE TRIM
```

This means the measured, existing spin guard reduced the already
corner-moderated positive throttle by 45% **on the most recently
accepted live frame**. It does not mean total engine power was reduced
45% from a normal straight-line driver request: corner assist, damaged
chassis, surface motor scaling and gear-aware power may also affect the
actual force. This is not a tire adhesion or braking measurement.

The row appears **only** while live intervention is at least 5% and
when the race HUD is expanded with enough vertical telemetry room.
Compact mobile/touch layouts, stopped/paused/recovering races, no
eligible human driver, intentional held handbrake slides, service
braking, Simulation Inspired and AI rivals cannot show the hint.

The read-only `HumanSpinAssistMeter` validates elapsed frame time,
positive before/after throttle, and the established maximum 70% spin
guard reduction. Nonfinite, implausible or interrupted samples clear
the **current** HUD indicator, never creating gameplay permission.

## Run-local lifecycle and limits

The meter records a bounded session peak and saturating intervention
activation count for source QA. Neither is persisted, added to race
results nor used by the physics controller. Pausing, focus loss,
touch interruption, invalid physics and recovery immediately suspend
live display. A real race restart resets its peak and count. Display
feedback never influences acceleration, score, lap, checkpoint,
garage, reward, SaveGame, or controller input.

## Portable regression

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R HumanSpinAssistMeter --output-on-failure
python3 -m unittest discover -s Tests/Editor -p HumanSpinAssistHudBindingTests.py -v
```

The portable suite runs 200,000 deterministic sample frames, including
activation transitions, no duplicate event counting during held
intervention, threshold behavior, full race restart, numeric
outliers, frame hitches, human eligibility and saturating counts.
Python source contracts ensure the **actual** final spin-adjusted
throttle is observed after measured Chaos yaw and before any nitro,
drafting or base AddForce call. The full HUD line still uses the
existing bounded vertical layout mechanism.

## Required Unreal Engine 5.8 gameplay QA

1. Build and cook the actual project through authorized Unreal
   Editor/UHT/UAT on a Windows workstation with the exact source and
   map hashes retained; do not substitute portable CMake executables.
2. On a real installed game, reproduce a sideways high-speed spin
   while recording signed forward speed, lateral speed, physical yaw
   and actual before/after motor throttle values. Verify the displayed
   percentage matches the recorded values without false indicators.
3. Compare normal high-speed turning, straight acceleration, intentional
   handbrake drifting, service braking, reverse and Simulation Inspired.
   Ineligible states should display no spin assist hint.
4. Verify the hint disappears immediately on pause/focus loss,
   restart, recovery, menu interruption, invalid frame and car swap.
   Confirm no stale reading after a race restart.
5. Verify HUD overflow on low-height Windows resolutions and Android
   compact/touch mode; authoritative lap, time and controls take
   precedence over this optional row.
6. Confirm race records, progression, rewards and physical handling
   remain unchanged by this *observation-only* addition.

Keep the [playable release qualification issue #148](
https://github.com/sanskarIN/latebrake/issues/148) open until actual
compiled game, owned circuit, installed gameplay and QA evidence exist.
