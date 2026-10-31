# Arcade engine heat and bounded power fade — source QA

**Status: v0.5.86 development candidate, source-only.** This is not
evidence of Unreal Engine 5.8/UHT compilation, a cooked circuit, a
Win64 executable, Android APK, physical handling or playable status.

## Scope

The shared player/rival physical pawn samples live chassis speed and
the effective (already moderated) propulsion demand after readiness
checks. A synthetic heat fraction increases with sustained driving and
qualified human nitro. Coasting cools the system with additional airflow
at speed. A warm engine gradually reduces normal, nitro and drafting
motor forces **together** above 67% heat, never below 82% of the previous
propulsive force. Service braking does not add engine heat; the thermal
multiplier does not alter braking, steering or opposing-throttle damping.
Pauses, focus loss, invalid numbers and frame hitches suspend updates.
Race-local reset restores the ambient fraction. The optional expanded
HUD reports engine heat and available motor percentage above 64%.

This is an arcade gameplay model, **not** a real engine oil/coolant,
battery or thermal sensor simulation. No official lap, checkpoint,
career reward, credits, save data or record logic changes.

## Portable verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^ArcadeEngineHeat$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

The C++20 regression includes long full-throttle warmup, cooldown,
additional nitro load, forward/reverse velocity, malformed frames,
suspend/restart and 200,000 deterministic frames with strict limits.
The editor Python source test checks that all three motor paths share
the same heat multiplier and only expanded HUD shows the advisory.
Neither test proves real Unreal Engine gameplay.

## Mandatory installed-game checks

1. Build genuine `LatebrakeEditor` and game targets in licensed Unreal
   Engine 5.8; record engine/source and cooked map identity.
2. Test in an installed Win64 candidate, both handling modes: run
   sustained throttle, coast, brake, nitro, slipstream, reverse, impacts,
   recovery, and restart. Record actual acceleration and heat behavior.
3. Validate input interruption, paused frames, extreme frame hitches
   and low-height/compact HUD. Ensure no second boost path evades fade.
4. Qualify real Android packages on physical devices and collect logs,
   before making any mobile-playable claim. Use issue #148 for the full
   release gate.

Source-only CI success must never be presented as a playable release.
