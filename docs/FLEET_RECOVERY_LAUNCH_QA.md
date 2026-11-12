# Experimental six-car recovery and launch gates — QA

**Developer-only DRAFT; no verified UE5.8 build, owned cooked map, or
installed Win64 race is claimed.**

This work is stacked on the opt-in physical fleet proposed in PR #331,
which itself depends on PR #330 (independent actor identity) and PR #329
(authored start geometry). It fixes two high-impact failure paths **only**
for a non-shipping, explicitly requested `-LatebrakeExperimentalSixCarGrid`
session. Normal one-rival gameplay does not enable the new fleet checks.

## 1. No paid player recovery into physical traffic

The legacy player `RecoverPlayer` path uses a synchronous transactional
`TryChargePlayerRecoveryAfterVerifiedTeleport`: it stages the five-second
penalty and counter on an isolated race snapshot and commits them only
after `Car->TryRecoverTo` has verified the actual Unreal placement.

The original circuit uses some of the **same** checkpoint recovery
coordinates for the human and primary AI car. Before running this
transaction in opt-in experimental Circuit sessions, the new code:

- Collects five **real** physics-ready opponents: the existing primary
  rival at slot 1 and four experimental opponents at slots 2–5.
- Rejects an incomplete/partial grid, destroyed actor, absent physical
  chassis, stale grid identity, duplicate token, invalid coordinate,
  heading or velocity, or incorrect slot ordering.
- Checks the intended player's entire padded chassis against one actual
  asphalt slab and original collision barrier geometry.
- Uses the existing **0.4-second continuous swept OBB** traffic
  intersection to deny a landing occupied now or projected to be crossed
  by any of the five observed rivals.
- On a rejected preflight, returns **before the paid recovery call**,
  without invoking any `TryRecoverTo`, adding time penalties, incrementing
  the recovery count or resetting the ghost/checkpoint sample.
- If the preflight is clear, still requires the original successful
  `Car->TryRecoverTo` engine verification before committing a charge.

The preflight is a conservative instantaneous constant-velocity
prediction. It does **not** guarantee collision freedom once bodies
turn or contact occurs; only real Chaos testing can establish behavior.

## 2. No experimental green light before a verified physical fleet

During the race countdown, the new explicit opt-in path defers the
additional four rival spawns until:

- the real local player is possessed and physics-qualified;
- the original map is ready and not paused, menu-blocked or focus-lost;
- the primary opponent has a matching registered actor identity and
  a valid physics chassis; and
- the authored grid and 20 next-gate recovery footprints are safe.

A newly spawned test rival is no longer accepted simply because Unreal
returned an actor pointer. The GameMode requires:

1. `DontSpawnIfColliding` at its distinct source-authored grid slot.
2. Successful **real** `TryRecoverTo` post-move transform/physics check
   at the exact slot before the actor's identifier is registered.
3. A complete four-extra-rival roster with each expected distinct grid
   identity and physics-ready chassis.

If an experimental grid is **requested**, `StartLights.Advance` never
receives a qualified start frame until all those conditions hold. The
normal one-rival clock/start remains unchanged when not opted in.
A failed actor spawn or placement destroys partial extras and disables
experimental mode for that run; the failure must be visible in UE logs
and must not be reported as six-car success.

The fleet traffic collector also refuses an absent/unregistered primary
rival, rather than silently omitting it from an extra rival's obstruction
checks.

## Exact-head portable/source verification

    cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
    cmake --build Tests/Standalone/build --parallel 4
    ctest --test-dir Tests/Standalone/build --output-on-failure
    python3 -m unittest discover -s Tests/Editor -p '*Tests.py'

`PlayerFleetRecoveryClearanceTests.cpp` covers all original player
recovery locations, five distinct actor slots, missing/stale/duplicate
opponents, real traffic overlap, high-speed traffic crossing the landing
between frames, off-road landing, and the **uncharged** staged recovery
transaction on denied placement.

`PlayerFleetRecoveryBindingTests.py` ensures the *real Unreal*
`RecoverPlayer` branch blocks prior to any charge/move. 
`ExperimentalFleetLaunchBindingTests.py` asserts the physical-fleet
start-light gate, real-engine postspawn verifier, and missing-primary
rival veto.

These are **not** substitutes for a real Unreal 5.8 Editor/UHT build.

## Installed UE5.8 and Win64 acceptance (still mandatory)

1. On a trusted licensed Windows workstation, compile Editor and
   Development Game at the **exact PR commit**; record build engine
   provenance, original map hash and compiler logs.
2. Run the authored Circuit with the developer flag. Confirm that no
   countdown tick is admitted while the human pawn or an AI body is
   missing, broken or physically not ready. Confirm all six cars actually
   start on their intended unique pavement.
3. Disable the flag, open career or Time Attack, and verify unchanged
   one-rival/solo launch behavior and no extra AI actor.
4. In experimental Circuit, park any one of the five rivals on the
   player's gate-specific recovery landing. Press RECOVER, verify no
   teleport, **no five-second charge**, no recovery counter increase, no
   fabricated checkpoint progress, and a clear refusal log. Repeat with
   an AI moving across the landing at various speeds.
5. Remove all five rivals from the immediate vicinity and repeat a
   paid recovery, verifying its Unreal post-placement decision and the
   exact fee only if the move truly succeeds.
6. Destroy or invalidate a primary/extra AI, test pause/focus loss and
   restart. A missing actor must **not** authorize a speculative paid
   teleport, traffic acceleration or a new start.
7. Verify each of the four extra cars fails closed when UE refuses its
   initial collision placement. Record real 30/60/120 FPS hitch and
   collision telemetry; do not infer safety from strict C++ geometry.

Keep [playable release gate #148](https://github.com/sanskarIN/latebrake/issues/148)
open. No v0.5.98 release, Win64 installer or stable v0.6.0 publication
is authorized by passing source-only tests.
