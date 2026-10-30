# Arcade collision damage — source policy and installed-game QA

**Status: development candidate after the published v0.5.84 source-only release.**
The C++20 source and Python binding tests do not establish a UE 5.8 compile,
a cooked race circuit, or a playable Windows/Android game.

## Implemented prototype behavior

The shared player/rival pawn registers the *actual rigid-body chassis*
`UPrimitiveComponent::OnComponentHit` callback with rigid-body collision
notifications enabled. Decorative wheels, lights and camera components
cannot submit impacts. Eligible real Chaos collision events carry normal
impulse and the contact normal; the policy also uses the chassis mass and
latest qualified real planar speed (or observed current chassis velocity
when greater). A collision may only count after a validated, active
post-GO physics tick on an authorized race circuit.

A collision is ignored unless **all** of the following are true:

- The actual GameMode, player, map and restart-placement race gates are
  eligible, GO is green, and the world/menu/focus/manual-pause gates permit
  driving. The pawn's live Chaos rigid body is valid.
- The hit comes from the root chassis, has another valid actor and is
  neither a self-contact nor the same chassis component.
- Normal impulse is finite and within [0, 1e9] kg·cm/s; positive mass is
  between 50 kg and 10,000 kg; observed planar speed is finite, between
  **1100 and 10000 cm/s**.
- Impact normal is finite and predominantly horizontal
  (`abs(Normal.Z) <= 0.55`), excluding ordinary road-floor landings.
- Impulse divided by mass is at least **360 cm/s**, the last physics
  frame was eligible, the **0.40-second contact cooldown** has expired,
  and maximum damage has not been reached.

Each accepted hit adds **3–13%** race-local damage, with a strict
**60% damage cap**. Existing forward drive, nitro and qualified
slipstream additions all receive one shared multiplier
`1 - 0.35 * damage` (minimum **79%**); existing bounded arcade
lateral grip receives `1 - 0.24 * damage` (minimum **85.6%**).
Damage does **not** create motor power, add extra steering torque,
change legitimate lap timing, grant penalties, write rewards or
persist any data. The prototype includes no deformable car mesh,
repair cost, pit stop, actual structural forces or vehicle safety model.

Focus interruption, engine/world pause, inactive menu, invalid
physics, long frame and recovery/teleport clear event eligibility and
cached pre-impact speed, retaining already accrued race damage.
**Restart resets damage and collision count** through the actual
player and rival `ResetDrivingStats()` path. An optional expanded
desktop-only HUD line shows observed damage plus current motor/grip
multipliers; compact Android timing/controls remain untouched.

## Automated, reproducible source tests

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --parallel
ctest --test-dir Tests/Standalone/build --output-on-failure -R ArcadeCollisionDamage
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

The portable C++ suite covers clean initial state, real-impact
acceptance, ground-contact exclusion, low-speed/low-impulse rejection,
event-callback bursts, cooldown, NaN/infinite/malformed data,
pause/recovery suspension, bounded motor/grip, full race reset and
250,000 deterministic frames. Source contracts check the actual
component-hit binding, the after-GO authority checks, all **three**
motor-force paths, lateral grip, lifecycle and optional HUD.

## Mandatory Unreal 5.8 + installed-build acceptance

1. Compile the licensed 5.8 Editor and Game targets with UHT, and cook
   the original circuit; check that binding `OnComponentHit` succeeds
   and notifications actually deliver nonzero Chaos impulse.
2. Drive player and rival against owned wall meshes and each other at
   different speeds; inspect the impulse, mass, normal, count and HUD
   on a real Windows session. The damage threshold is intentionally a
   prototype and needs tuning from actual Chaos traces.
3. Drive over flat pavement and ordinary kerb edges; confirm that
   road-floor collisions do not accumulate damage.
4. Repeat contact at 30, 60 and 120 fps, including many same-manifold
   callbacks, frame hitches, pause, alt-tab/focus loss and manual
   recovery. Confirm no post-pause or post-teleport stale-speed damage.
5. Test damaged power on normal throttle, nitro and drafting as well as
   corner slip, autonomous rival behavior and race restart; verify
   damage persists until restart but does not leak into the next race.
6. Verify race clock, gate ordering, 3-lap finish, awards, records,
   saved profile, compact Android touch controls and Android hardware
   collisions remain unchanged.
7. Record the real Unreal build identifier, source commit, map SHA256,
   Windows EXE/Android package provenance, runtime logs and manual
   observer QA.

**No installed runtime results are asserted by this source-only change.**
