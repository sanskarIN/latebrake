# Rival passing and emergency traffic-braking reconciliation — source QA

**Status: integrated actual Unreal AI source, not a UE5.8 engine-compiled
or installed playable build.** The 3D car remains a simplified Chaos
rigid-body prototype without independently simulated wheel contacts.

## Problem confirmed in the source

PR #287 made the only genuine AI rival steer around a slower player,
but allowed passing to begin **900–1600 cm behind** the player even
when the measured closing speed was very high. Meanwhile the existing
independent `YieldRivalToPlayer` emergency stop uses true forward
closing velocity and a 380 cm safety gap. For example, a car **1050 cm
behind and closing at 1350 cm/s** has a predicted time to the safety
margin of about **0.50 seconds**; the real traffic policy correctly
brakes immediately, even though the car's passing controller may have
just ordered a sharp lane-change steer. The previous integration
retained that steer while braking. That is an unsafe control conflict.

## Actual new behaviour

The existing physical `ALatebrakeRivalCar::Tick` now:

- Allows new passing only on the **two longer 5600 cm** straight
  approaches of the original four-gate circuit, never the shorter
  3600 cm straights where road length is insufficient for this
  untested manoeuvre.
- Requires the slower player to be **1600–2400 cm ahead**, relative
  closing speed **300–900 cm/s**, time to the conservative contact
  margin of at least **1.8 seconds** and **at least 4200 cm** before
  the approaching corner. If these checks fail, the original traffic
  governor still slows/brakes the rival normally.
- Keeps the original physical passing-steer command within a bounded
  road corridor, with no new engine power or modified gate position.
- Runs the original independent real-world-velocity traffic safety
  policy **after** selecting a passing steer, followed by its
  existing **0.18-second** following-brake hold.
- Calls `ResolveRivalPassTraffic` with the original route command,
  active passing result and traffic-held command. **If the real
  traffic system brakes, the pass is cancelled and steering returns
  to the original route direction while the brake remains engaged.**
  The passing state resets so it cannot silently reappear mid-brake.
- Preserves existing rival yaw/spin moderation, verified checkpoint
  crossings, recovery placement, race times, reward and saved-profile
  authority, without touching the player's controls.

The correction does **not** disable emergency braking to make passes
easier. It avoids beginning a manoeuvre too close to impact, and
fails safe if live traffic still requires braking.

## Automated qualification

- `Tests/Standalone/RivalStraightPassAssistTests.cpp` now rejects
  the old unsafe short-distance and high-relative-speed starts;
  validates safe long-straight entries, shorter-straight refusal,
  both lane choices, lane-change abort and 20,000 sampled road states.
- `Tests/Standalone/RivalPassTrafficResolutionTests.cpp` checks
  physical same-lane emergency braking, actual separated-lane passing,
  normal prior braking, existing brake-hold hysteresis, malformed
  control inputs and 20,000 diverse traffic encounters.
- `Tests/Editor/UnrealRivalPassTrafficResolutionBindingTests.py`
  verifies the actual AI pawn calls the new reconciliation between
  real traffic/hold decisions and yaw/spin/physical motor output.
- All portable tests run strict C++20 on Windows, Linux and macOS;
  source-binding tests **do not compile Unreal**.

## Still required for playable racing qualification

1. Compile actual Unreal 5.8/UHT and cook the original map on an
   authorized Windows workstation. Install the real 3D candidate.
2. On both long straights place a slow player 16–24 m in front and
   record actual passing steering, brake demand, separation, speed,
   turning radius and verified gate progression.
3. Repeat with high closing speed, player sudden braking, player
   swerving into pass lane, overlapping cars and a traffic-brake
   hysteresis already active; confirm no aggressive passing steer
   persists under emergency brake.
4. Confirm no pass on the shorter straights or near corners; test
   road boundaries, vehicle collision shapes and 30/60/120 FPS.
5. Test restart/focus loss/recovery/invalid sample and compare
   winning/finishing only from actual directional gate crossings.
6. Improve after real physics telemetry—not based on synthetic
   portable tests alone.

Playable-release qualification remains tracked in
[issue #148](https://github.com/sanskarIN/latebrake/issues/148).
