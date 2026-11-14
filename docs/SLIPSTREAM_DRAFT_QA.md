# Physical-rival slipstream drafting — source-only racing feature

**SOURCE-ONLY C++20 and Unreal source integration; not playable.**
No Unreal Engine 5.8/UHT compilation, real cooked original circuit or
installed Windows/Android driving qualification is claimed. This is
a bounded **arcade drafting effect**, not aerodynamic simulation,
multiplayer positioning or a source of official race results.

## Design and source behavior

- Only the **locally controlled human** in a green-light, eligible race
  can benefit. The opponent must be the real existing and physics-ready
  moving rival; time attack (without rival), a missing/finished AI car,
  pause, menu, invalid placement, race finish, focus loss and invalid
  rigid-body physics never create phantom draft bonuses.
- Both the actual human and the actual rival need **at least three
  independently qualified WorldStatic chassis-corner contacts** on
  a recent accepted physics frame. Two-corner precarious support,
  airborne travel and absent ground samples cannot grant the optional
  arcade drafting bonus, even when the two cars' last sampled
  positions, headings and speeds appear aligned. The same pure
  qualifier is called by the player Tick and the rival sampling
  GameMode method. Normal two-corner driving traction and four-corner
  stationary PIT repair remain separate.
- The player and rival must travel forward at least 12 m/s and 10 m/s
  respectively. The relative geometry must be within **7–38 m ahead**,
  less than **3.8 m lateral** and **2.3 m vertical** separation, with
  heading alignment above 0.94 and forward throttle at least 30%.
  Both world-planar heading vectors are **normalized before** alignment,
  physical lane/gap measurement and relative-motion prediction. This
  prevents the accepted 5% squared-length tolerance from making
  oblique rivals look aligned or falsely inflating near-boundary gaps.
- Real measured relative speeds/positions/directions are supplied to
  an engine-free production C++20 policy. If the player closes more
  than 10 m/s faster than the rival, or either brakes/reverses/moves
  improperly, boost immediately clears.
- The existing actual-chassis velocity vectors also constrain both
  cars' **lateral sliding**: each measured right-axis travel component
  must remain at or below **25% of that car's own forward speed** and
  at or below **5 m/s**, whichever is smaller. Identically facing but
  broadside-sliding cars no longer qualify as an aerodynamic pair.
  Both measurements are supplied directly from the human's Chaos body
  and the rival's real world velocity, not inferred from heading
  or a frozen HUD reading. Reverse slip sign has no effect on the cap.
- A **solid barrier or blocking static circuit object between the actual
  player and rival chassis** denies drafting, even if headings, velocities,
  and an otherwise legal gap agree. GameMode performs an Unreal
  `LineTraceSingleByChannel` using `ECC_Visibility` between both live
  chassis-root positions, ignoring only those two endpoint actors. A
  blocking hit immediately fails source eligibility and clears previous
  credit; the pure C++20 boost policy separately defaults the physical
  path to **uncleared**, so missed runtime wiring cannot grant a boost.
  Decorative non-colliding track paint is not a barrier. This trace is
  limited to rivals within 50m to avoid distant scene queries. The existing
  original BlockAll track barriers are configured to block visibility;
  custom future meshes must have proper collision responses. Only an
  authentic UE5.8 in-editor obstacle test can validate that configuration.
- The source sample carries a **nonzero unique live rival actor identity**
  from Unreal `RivalCar->GetUniqueID()`. A respawned/replaced rival that
  occupies the same position with the same apparent velocity must earn a
  **new uninterrupted 0.18-second window**. A missing/zero identity fails
  closed; an identity change cannot reuse the old motor-force credit.
- The actual follower chassis world-space position is also monitored
  independently against its measured forward, lateral and Z velocities.
  When **both** cars jump together, their unchanged relative gap would
  otherwise fool a relative-only motion check. More than 2m of unexplained
  player world movement or 75cm unexplained player vertical movement revokes
  eligibility, even if the ahead, lane and 3D relative offsets remain legal.
  Missing/nonfinite follower coordinates fail closed. This is source-only
  until actual UE5.8 physics, world-origin, restart and teleport tests.
- The rival's **independent absolute world-space trajectory** is
  reconstructed from the measured player position and physical rival
  offset, then compared with the rival's own 3D velocity. This prevents
  two separately tolerated residuals (player travel and relative gap)
  from combining into a much larger rival displacement. The same 2m
  three-axis and 75cm vertical safety ceilings apply to each chassis
  **independently** as well as to their relative geometry. Invalid
  rival world reconstructions reset eligibility; standalone tests cover
  2.55m hidden lead relocations, 1.2m hidden vertical movement and
  physically consistent high-speed/cornering trajectories.
- Consecutive **relative 3D world positions** must also agree with the two
  measured chassis-velocity vectors: planar forward/lateral components and
  the actual human/rival Chaos vertical velocities over the current physics
  delta. Missing, nonfinite or implausibly large vertical velocities fail
  closed, rather than assuming zero motion. More than **2 m** of unexplained
  total 3D residual relative movement resets qualification immediately.
  An independent **75 cm vertical residual** ceiling also rejects smaller
  unexplained Z relocations that could fit inside the full 2.3 m vertical
  eligibility zone and the wider 2 m three-axis tolerance, even if the
  rival remains inside an otherwise legal drafting zone. This is a conservative source-only guard
  with allowances for ordinary tick-order skew, **not** proof of a genuine
  engine-verified teleport or collision detector. The first valid frame
  establishes a new baseline without granting a boost.
- Drafting must remain continuously eligible for **0.18 seconds** before
  granting help; an invalid frame, stale/missing rival, teleport,
  sideways-slide excess or input interruption clears the entire
  confirmation window.
- A qualified position yields at most **+6% additive motor force**.
  Greater distance or lateral separation reduces the benefit. The
  existing baseline motor force, frame-stable damping, top-speed cap,
  steering, service/hand brakes, car damage/condition and AI controller
  remain authoritative. This does **not** alter saved career data,
  credits, lap time clocks, legitimate gate awards or official records.
- When nitro and drafting overlap, the independent bonuses share a
  single **40% total extra motor-force cap**. Nitro has priority because
  it consumes limited charge. Drafting can contribute only the remaining
  share (as little as 2% when nitro grants its full 38%). The HUD shows
  the **actually granted** drafting percentage, not the unrestricted
  geometric eligibility estimate.
- [Cross-mechanic budget tests](https://github.com/sanskarIN/latebrake/blob/main/Tests/Standalone/ArcadeBoostArbitrationTests.cpp)
  reject nonfinite inputs and excessive additive force.

- An optional **SLIPSTREAM** active readout appears only in the
  expanded desktop race HUD. Compact phone controls and authoritative
  race rows remain unchanged.

## Build-independent source checks

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R SlipstreamDraftBoost --output-on-failure
python3 -m unittest discover -s Tests/Editor -p SlipstreamDraftBindingTests.py -v
```

Tests reject malformed/time-hitch samples, NaN/Infinity position and
motion, reverse/too-distant/oncoming rival, wrong lane, vertical
separation, denied driver states, braking, low speeds, invalid headings,
measured excessive left/right chassis sliding on either vehicle,
nonfinite lateral movement, 40,000 varying-speed sideways-motion cases,
12,000 adversarial in-lane teleport traces and rotated/smooth relative travel,
12,000 blocked physical sightline frames and post-occlusion reacquisition,
vertical velocity provenance, plausible airborne-lean changes, impossible
in-lane vertical relocation and reset/reacquisition, near-unit input headings
with angular spoofing, near-edge lane/gap measurements, and over-fast approach. Separate exact traction tests reject fabricated
contact, 0–2 supporting corners and unsupported/airborne lead/follower
states; Python source assertions verify both real Unreal bindings. They also check a 5,000-run stress sample, all
real racer/unreal binding guards and reset paths. This verifies only
portable logic and static Unreal-source wiring.

## Required real-engine and installed-game tests

1. Compile licensed UE 5.8 targets with actual UHT; cook an original
   circuit and record exact editor/package/engine/source SHA256 hashes.
2. Test sustained drafting behind the real AI on straight segments
   with measured gap and heading. Confirm extra acceleration is
   repeatable but bounded at both 30/60/120 FPS and different garage
   configurations. Verify driving feel and stability.
3. Check starting grid, countdown, menu, manually failed pause,
   focus loss, vehicle recovery, fresh restart, opposing headings,
   AI route completion and Time Attack. No phantom boost may survive.
4. Put an actual BlockAll/Visibility-blocking solid circuit barrier
   between the player and the real rival without moving either car outside
   the policy's allowed gap. Verify 0% draft, then remove the barrier and
   require a new unbroken 0.18s confirmation. Repeat on both straights,
   corner approaches, adjacent lanes, and during rival recovery. Check
   world-query instrumentation, no false barriers on ordinary clean asphalt,
   and the exact sampled actor identities on real Unreal 5.8.
5. Compare control/input handoff, standalone engine car physics,
   motor-force application and normal steering/braking on Win64.
   Confirm no collision or vehicle penetration is needed to draft.
6. Verify this cannot affect legitimate lap/checkpoint validation,
   game saves or credit awards; preserve genuine engine logs and
   observed hardware results. Test Android rendering separately.

Until verified on installed packages and
[playable milestone #148](https://github.com/sanskarIN/latebrake/issues/148)
is qualified, this remains an unverified **source-only** feature.
