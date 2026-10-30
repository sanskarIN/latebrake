# Arcade brake-heat source and installed-game QA

**Status: post-v0.5.84 source-development candidate. NOT a published
or playable pre-release.** The C++20 policy and source callsites require
licensed Unreal Engine 5.8/UHT, cooked-circuit and installed-game tests.

## Intended arcade response

- The real shared player/rival rigid-body pawn samples signed forward
  chassis velocity and its ordinary service-brake request after race,
  focus, pause, track and Chaos-readiness checks.
- At high speed, repeated ordinary braking raises synthetic heat from
  an ambient floor of 20%. Sustained heating beyond 62% gradually
  reduces the **existing** forward service-brake resistance, to no less
  than 74%. It cannot amplify braking above the existing baseline.
- Releasing the ordinary brake gradually cools the state, including
  while the chassis continues moving. Reversing retains the same finite
  speed-magnitude checks.
- The handbrake is independent: handbrake-only operation does not heat
  the service brake. With both brakes held, the pre-existing parking-
  brake resistance floor of 4.2 remains available when the ordinary
  brake's force fades.
- Frame duration must be finite and between zero and 0.10 seconds;
  signed forward speed must be finite and at most 10,000 cm/s in
  magnitude. Invalid readings suspend authority and apply the neutral
  (100%) multiplier for that frame; heat history does not mutate.
- Resetting driving stats for a fresh event resets heat. Pause, focus
  loss, teleport and invalid physics suspend sampling; normal driving
  resumes from retained bounded history. No save schema is changed.
- Expanded desktop HUD only shows **BRAKE HEAT** when measured heat
  reaches 60% and the available vertical layout can accommodate it.
  Compact mobile timing/touch controls retain priority.

This is a **prototype arcade response**, not actual rotor temperature,
brake fluid, wheel-lock, ABS or a certified tire/suspension simulation.
No change is made to checkpoint validation, career rewards, lap
records, player profile writes, or official timing.

## Portable validation

From the repository root:

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --parallel
ctest --test-dir Tests/Standalone/build -R ArcadeBrakeHeat --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

The C++20 regression checks initial heat, bounded sustained fade,
cooling, handbrake independence, reset, invalid/NaN/infinite values,
reverse movement and 250,000 deterministic frames. Editor Python
checks actual pawn/HUD source bindings only; it is **not** UHT,
UE compilation or gameplay verification.

## Mandatory Unreal and installed-build acceptance

1. Compile Editor and Game targets with licensed Unreal Engine 5.8
   and verify the cookable, original circuit is present.
2. Test 5–20 seconds of on-road braking at high and moderate speed
   with no handbrake; measure slower deceleration only after the
   displayed heat crosses the fade threshold. Verify cooling.
3. Test keyboard, gamepad, Android touch BRAKE and independent
   handbrake, including simultaneous service and parking brake.
4. Repeat the same physical readouts on an autonomous rival during
   measured in-race braking. Ensure no NaNs, one-frame reverse
   impulses, or restart-state leakage.
5. Repeat around pause/resume, OS focus interruption, failed and
   successful recovery, race restart, off-pavement crossings and
   frame hitches. Verify timing, lap records and rewards remain
   strictly controlled by existing race authority.
6. Collect engine build SHA, source commit, cooked-map SHA, launch
   logs and playtester results for Windows and hardware Android.

Until those observations exist, this work remains **SOURCE ONLY**.
