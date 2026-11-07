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
propulsive force. Service braking and *actual opposite-pedal deceleration* do not add
propulsion heat, even when input is held throughout a long direction change.
Both still permit heat to cool while the car moves. The thermal multiplier
does not alter braking, steering or opposing-throttle damping.
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
It also composes the **production opposing-throttle classifier** with
the heat model over 40-second forward and reverse direction-change
sequences at **30/60/120 FPS**: opposite-pedal braking must cause no
artificial propulsion heat, whereas ordinary forward/reverse propulsion
must still heat and derate as before. A high-speed sideways slide with
less than 100 cm/s signed travel exercises the distinct near-stop
direction-change allowance, where real motor demand is again eligible.
The editor Python source test binds the actual Unreal pawn's heat
admission to the **same** opposite-pedal classifier used to suppress
motor force and verifies evaluation order. The original heat
multiplier remains shared by base, nitro and drafting motor paths.
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

## Nitro heat settlement after real force delivery

Engine derating is estimated **before** final gearbox and contact/
friction-circle propulsion, but an eligible nitro press can still
deliver zero additional motor thrust. `ArcadeEngineHeat::Observe`
now immediately commits **only ordinary throttle/airflow heat** and
holds incremental candidate nitro heat in a separate pending receipt.
The same-frame output heat fraction, motor multiplier and HUD-facing
getter therefore cannot temporarily report bonus heat that the car
never delivered. The real shared Chaos pawn calls `SettleNitroDelivery`
with the **actual positive nitro AddForce result**: only a true verdict
commits the held extra, once, for later frames. A rejected, interrupted
or forgotten candidate is simply discarded, with no state to refund.
Optional downstream validation failures reject the receipt before
`ClearDriverInputs` suspends the source state.

The same physical motor decision now commits nitro charge depletion
and 95% meter-heat cutoff (fresh key and 2.5s cooldown). A terminal
candidate blocked by gear/traction can no longer burn the last charge
or trigger a cooldown without actual boost. Portable C++ covers
400 denied engine-nitro samples against an otherwise identical
ordinary-throttle controller, 120 rejected overheat-limit samples,
and a separately reached low-charge terminal case. These are
*source-level* tests, not genuine Unreal Engine 5.8 gameplay.

## Unanswered receipt and pre-dispatch motor-power isolation

Draft PR #295 now rolls back an **unsettled** incremental nitro-heat
receipt on the next `Observe` or `Suspend`. It does not erase the
ordinary throttle/airflow heat from the same frame. More importantly,
`Observe` returns the **baseline** motor multiplier for the motor
force currently being dispatched; proposed nitro-specific heat cannot
derate a motor before the corresponding bonus `AddForce` is actually
confirmed. A genuinely delivered bonus commits its additional heat for
subsequent frames. The player and rival still share the same engine
power limit and the physical motor path remains authoritative.

Portable C++ regressions compare 600 unanswered nitro candidates with
an equivalent ordinary-throttle heat history, including repeated
suspend/resume, and check power equality even after genuine ordinary
engine derating begins. This is **source-only regression coverage**;
runtime frame ordering, displayed HUD state, and actual acceleration
still require a compiled Unreal Engine 5.8 and installed Win64 QA.

## Commit-only heat observation regression (November 7)

The draft physics branch also covers 400 deliberately denied boost samples,
600 unanswered candidate samples across engine derating and interruptions,
and a standalone same-frame comparison of unconfirmed nitro against ordinary
throttle. The returned `EngineHeatOutput.HeatFraction`, the live getter and
the motor multiplier now agree before the actual bonus `AddForce`; an
accepted bonus alone increments heat **after** dispatch. Subsequent
pause/suspend cannot undo a successfully committed boost, and no orphan
receipt can leak across frames. These are portable source contracts only,
not measured heat or real UE5.8 Win64 performance.
