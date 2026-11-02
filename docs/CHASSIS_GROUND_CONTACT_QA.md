# Actual chassis ground-contact traction — source and installed-game QA

**SOURCE-ONLY Unreal Engine 5.8 C++ integration. NOT a playable,
qualified UE5.8 build.** The existing vehicle is a pitch/roll-locked
single simulated Chaos cube chassis, **not** a genuine per-wheel
spring/damper, articulated suspension or Chaos wheeled vehicle.

## Why this changes actual vehicle gameplay

Previously the physical player/rival pawn used only planar chassis
velocity and a rough circuit XY road mask to apply engine, braking,
lateral tire-like force and yaw torque. The source could therefore
produce strong in-air turning, acceleration and tire grip despite
having no actual supporting physical road beneath the vehicle.

This change samples four **real Unreal line traces** from the current
physical chassis transform (approximately front-left, front-right,
rear-left and rear-right) **down onto blocking WorldStatic** surfaces.
The car's own actor is ignored. Only well-formed trace hits below
the chassis within **115 cm** and with upward-facing surface normal
**Z ≥ 0.68** count as ground support.

The same source-driven traction scale is used by both human and
autonomous physical pawns:

| Qualified supporting probes | Contact-based drive/grip scale |
| --- | ---: |
| 0 or 1 | 0.00 — airborne/insufficient support |
| 2 | 0.45 |
| 3 | 0.75 |
| 4 | 1.00 |

This scales physical baseline/nitro/drafting propulsion, opposite
throttle braking, service/handbrake resistance, lateral tire-like
force and yaw torque **without overwriting actual Chaos velocity,
applying artificial suspension springs or modifying gravity**.
Raw free-flight motion, air resistance, chassis gravity/collisions
and race-clock authority stay managed by their existing systems.

Airborne frames do not earn extra *synthetic tire wear*, new brake-pad
heat from unsupported braking or consume a fresh nitro burst.
The existing pit-service eligibility also requires contact.

A four-point trace is NOT a wheel contact manifold. No force is
applied at wheel location, pitch/roll remain locked, normal load is
not integrated, and the current visual cylinder wheels are still
non-colliding decorations. A future real suspension effort needs
separate UE wheel components, contact tuning and real builds.

## Cross-platform source qualification

- `ChassisGroundContactTractionTests.cpp` verifies all 0–4 support
  counts, near-ground full driving, airborne contact removal, steep
  wall rejection, two-probe road-edge response, invalid/paused
  conditions and **80,000 varied measured road-contact frames**.
- `UnrealChassisGroundContactBindingTests.py` checks the real
  C++ pawn calls four Unreal WorldStatic line traces, rejects its
  own chassis, verifies hit normals/depths and gates all actual force
  paths before the motor/turn controller.

These are deterministic portable C++ and source-text tests. None
of them uses licensed Unreal SDK compilation.

## Real UE 5.8 hardware acceptance tests — mandatory

1. Compile the real project on an authorized UE5.8 Windows workstation.
   Cook the original project map; verify four asphalt slabs/ground
   are actually queryable `ECC_WorldStatic` and physics-blocking.
2. At GO and after each legitimate recovery, confirm gravity settles
   the chassis near asphalt, all four traces qualify, and the
   human/rival can still start, steer, brake and finish valid gates.
3. Drive slowly across the shoulder and road edge; verify partial
   support reduces *actual* force without stalling a car that remains
   reliably supported. Check inside a pit stop and restart.
4. Deliberately leave a raised surface using a safe test fixture;
   verify no midair motor/nitro/steering impulses and recovery of
   physical grip after landing. Check 30, 60, 120 FPS and physics
   substepping. An unbuilt source test cannot prove this.
5. Verify wall/side collisions do not become false grounding contacts,
   visually elevated cars are not wrongly accelerated, and slopes
   behave sensibly. Tune query ranges/normal thresholds only
   against real traces and actual chassis collision dimensions.
6. Test all six car visuals and both handling modes with true rendered
   material/collision geometry, plus brake heat, damage, AI corner
   braking and lane-overtake interaction.
7. Confirm no save, gate, reward or race session authority changed.

The authentic installed-game release gate is still
[issue #148](https://github.com/sanskarIN/latebrake/issues/148).
