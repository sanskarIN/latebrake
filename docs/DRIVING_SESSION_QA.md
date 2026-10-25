# Driving session statistics — v0.5.60 source development

These are **run-local, advisory driving metrics**, not gameplay records or
rewards. They are compiled from measured Unreal rigid-body speed and driver
BRAKE state but have **not** been validated in a real UE 5.8 game.

## New code

- `Source/Latebrake/Public/Racing/DrivingSessionStats.h`
- `Source/Latebrake/Private/Racing/DrivingSessionStats.cpp`
- `Tests/Standalone/DrivingSessionStatsTests.cpp`

The player's `ALatebrakeCarPawn::Tick` samples valid measured planar velocity,
lateral velocity and effective braking after the existing finite physics
guards. It captures approximate driven kilometers, maximum speed in km/h,
brake application count, braking seconds, and high-side-slip seconds.
Desktop HUD reports those observed metrics. Distance is integrated from
speed by frame time, **not GPS**, and may differ from real distance due to
drift or sampling. Speed is capped to the policy's safe window and bad
nonfinite/negative/oversized frames are rejected.

Counter state resets on a successful accepted `RestartRace` flow (before
the new teleport attempt); after an interruption, `Suspend()` breaks the
held-brake sequence without fabricating another interval. The same pawn
class is used by the AI, but these numbers are shown only for the player.
No values are persisted or used for coins/medals/records.

## Verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^DrivingSessionStats$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'DrivingSessionIntegrationTests.py' -v
```

Test reset, restart, 30,000 sample stress, 0.25s hitch guard, NaN/inf,
true brake transitions, low-speed drift suppression, correct units and
bounded counters. This does **not** verify actual Unreal time steps.

## Real UE / installed-device QA gate

1. Compile real UE 5.8 game targets and confirm no UHT/build failures.
2. Validate speed/distance readings against actual logged Chaos velocity.
3. Verify braking only counts transitions, not held frames, and telemetry
   freezes across menu/OS suspension, failed RESTART and unqualified physics.
4. Verify explicit RESTART resets stats, while a paid RECOVER does not
   masquerade as additional driven distance or speed.
5. Check desktop HUD readability, low-end Android frame time and no
   overlap with steering/pedal controls. Preserve evidence in issue #13.
