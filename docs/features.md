# Features & systems

This page distinguishes source-level features from tested game builds. All gameplay below describes code in the repository, **not confirmed functionality of an installed game**.

## Start lights (v0.5.75 source-only pre-release)

A single three-second 3→2→1→GO gate controls the real race clock,
checkpoint scoring, player/recovery inputs and AI rival motion. An
unready start grid, paused menu, focus-loss latch or unqualified
circuit stops the countdown; a game restart clears it. Only the
fraction of a frame *after* GO counts as racing time. A temporary
GO notice appears in the race HUD after the official clock row.
Actual Unreal/installed QA is still required. See
[start-sequence QA](RACE_START_QA.md).

## Launch feedback (v0.5.76 published source preview)

The run-local, optional 0–5 km/h launch estimate uses signed forward
velocity from the real player chassis and the authoritative post-GO clock,
not virtual reaction time or a predicted countdown. The next
**published v0.5.77** source refinement interpolates only across short,
consecutive qualified physics samples; interruptions and larger gaps fail
closed. It never changes credits, competitive records, saves or vehicle
forces. Actual Unreal physics sampling and installed Win64/Android QA
remain unverified. [Launch QA](LAUNCH_SPEED_QA.md).

## Post-v0.5.85 engine heat and AI checkpoint feedback (source development)

- **Engine heat:** both physical driver types share a bounded, race-local
  synthetic motor-heat state. Valid sustained propulsion raises heat;
  coasting allows cooling. Above 67% heat, the real vehicle source can
  modestly reduce normal, nitro and drafting drive forces together,
  without altering braking, steering, or official race results.
- **Rival heat adaptation:** a separate autonomous driving layer may
  reduce high-speed throttle demand based on previous qualified engine
  heat, while preserving low-speed recovery and existing traffic braking.
- **Honest gate comparison:** accepted forward checkpoint counts for the
  human and physical rival can be compared on spacious HUDs without
  claiming official placement, road position or timing advantage.
- These are **source-only**, not verified Unreal or installed gameplay.
  See [engine heat QA](ARCADE_ENGINE_HEAT_QA.md),
  [rival cooling QA](RIVAL_ENGINE_HEAT_PACE_QA.md), and
  [accepted gate comparison QA](RIVAL_VERIFIED_GATE_GAP_QA.md).

## v0.5.85 race-local collision damage (source only, not installed gameplay)

- **Real chassis impact response:** high-speed, wall-like physical collision impulses can progressively reduce shared player/rival arcade motor drive and lateral grip, capped at 60% race-local wear. A 0.4-second contact cooldown guards callback bursts. Road-floor hits, low-speed bumps, invalid data and interrupted frames cannot award damage. Restart repairs the car; neither saved career currency nor official race results change. Source integration and portable tests only, not engine gameplay verification. [Collision QA](ARCADE_COLLISION_DAMAGE_QA.md).

## v0.5.85 brake thermal response (source only, not installed gameplay)

- **BRAKE HEAT:** the real shared player/rival chassis now uses validated signed speed and service-brake demand for a bounded prototype brake-heat model. Repeated high-speed service braking can reduce existing service-brake resistance to no less than 74%; cooling restores it. The separate handbrake floor, race timing, records and compact touch HUD are preserved. This is source-only, not Unreal 5.8 or installed-game validation. [Brake heat QA](ARCADE_BRAKE_HEAT_QA.md).

## v0.5.84 road-surface traction (not installed gameplay)

- **Off-pavement response:** after 0.15s of genuine off-road player/rival chassis positioning, normal motor and qualified boost additions use 78% of existing forces and lateral damping uses 72%; after 0.20s proven on-road return, baseline forces are restored. The source policy cannot touch race scoring, records or save files and is not a real wheel/tire physics simulation. [Surface traction QA](ARCADE_SURFACE_TRACTION_QA.md).

## v0.5.83 rival racecraft (not installed gameplay)

- **Autonomous corner pacing:** real AI steering request, last valid race-local grip estimate and actual lateral chassis motion may reduce throttle or request bounded braking before a high-speed corner. Conservative sample rejection and 0.12-second brake hysteresis cannot alter course steering, official timing, scoring or traffic-awareness priorities. [Rival grip QA](RIVAL_GRIP_PACE_QA.md).

## v0.5.82 source racing mechanics (not installed gameplay)

- **Arcade tire condition:** observed chassis slip and braking heat and wear adjust the bounded existing lateral-grip damping for both physical cars.
- **Rechargeable human nitro:** forward human demand alone, subject to race authorization, grip-independent finite energy, heat and cooldown, can earn up to +38% additional motor drive.
- **Real-rival slipstream:** the local human earns up to +6% extra motor drive only behind a valid aligned moving rival for at least 0.18 seconds.
- **Shared bonus budget:** concurrent nitro and drafting together cannot exceed +40% of baseline motor force. The desktop HUD shows the actual allocated draft share, not its uncapped eligibility.

See [tire QA](ARCADE_TIRE_CONDITION_QA.md), [nitro QA](NITRO_BOOST_QA.md) and [drafting QA](SLIPSTREAM_DRAFT_QA.md). All mechanisms need actual licensed Unreal Engine 5.8/UHT, original cooked map and installed driving tests.

## Racing

- Four ordered directional circuit gates; rejected crossings include wrong-way and implausible trajectories.
- Race clock, lap progression, recent timed laps, sector comparisons and best-sector theoretical lap.
- Circuit Race and one-lap Time Attack definitions; recovery adds a **five-second time penalty** when authorized.
- Race result validation, local record tracking and bounded offline credits.
- Experimental route-following rival pilot; its approximate telemetry is **not** official competitive standings.

Read [track design](TRACK.md), [AI and events](AI_AND_EVENTS.md), [sector timing](SECTOR_TIMING.md) and [finish insights](FINISH_INSIGHTS.md).

## Post-race source analysis (v0.5.71 source pre-release)

- **Recent lap distribution:** mean, median, population standard deviation,
  signed last-lap difference and consecutive faster laps over only the last
  eight retained valid and actively timed laps.
- **Sector analysis:** actual positive best split at each official gate and
  their **theoretical**, potentially mixed-lap sum — never an achieved record.
- **Sampled car statistics:** approximate speed-integrated distance,
  measured peak speed, brake applications, fraction of sample time braking
  and high lateral-slip. These are not official odometer readings or grades.

The Results screen uses bounded, reachable detail pages. These source
features have regression tests, but still need real Unreal Engine and
Windows/Android gameplay qualification; see
[Results insights QA](RESULTS_INSIGHTS_QA.md).

## Human analog controls (v0.5.74 source-only pre-release)

Player steering has a symmetric 12% center deadzone and signed
throttle/reverse has a 5.5% deadzone, with a smooth full-range
remapping of remaining analog travel. Full keyboard input and full
trigger/stick extremes remain intact. Invalid values are neutralized
without applying these filters to rival AI and phone touch geometry.
Requires actual UE and controller QA; see [analog input QA](ANALOG_INPUT_QA.md).

## Camera viewpoints (v0.5.72 source-only pre-release)

Press **C** (existing desktop/gamepad Camera action) to rotate through
CHASE, CLOSE CHASE, HOOD, HIGH CHASE. Each view has an explicit Unreal
spring-arm distance, angle, socket offset, collision and lag policy.
The selected mode is shown on spacious racing HUD layouts. On Android
the existing upper-middle pad displays **CAM 1–4** during gameplay, indicating the active view, while
its menu meaning stays **BROWSE**. Touch camera activation requires
a fresh press in a ready, unpaused, focused race. The steering and
gas/brake hit rectangles are unchanged. [Camera test plan](CAMERA_PRESETS_QA.md).

No screenshots or original cooked asset/runtime validation are claimed.

## Optional arcade drift score (v0.5.73 source-only pre-release)

The latest source work includes a **virtual drift-combo meter** driven
by observed car speed and lateral slip samples. It only scores sustained
arcade-style sideways chassis movement, handles short breaks without
duplicating combos, and stops on pause, app focus loss, recovery or
gameplay end. The existing Results detail pager can show unofficial
combo totals and sample time after a valid finish.

**These points are decorative; they do not grant career credits,
achievements, records, upgrades, payments or race wins.** The player
can continue normal circuit and time-attack racing without using the
meter. Physical validity and legible HUD output require actual licensed
Unreal Engine/device qualification. [Drift QA plan](DRIFT_COMBO_QA.md).

## Driving and controls

The prototype car is built from engine primitives and uses Chaos-backed rigid-body forces with Arcade and simulation-inspired presets. It is **not** a production tire/suspension simulation.

| Input | Action |
| --- | --- |
| W / S | Accelerate / reverse |
| A / D | Steer |
| Space | Brake |
| R | Recover the car with a penalty |
| T | Restart |
| C | Change camera distance |
| Esc | Pause / resume |
| 3 / 4 | Select Circuit / Time Attack |
| Q / E | Unlock next vehicle / select an owned vehicle |
| U / I | Upgrade engine / grip |

Android source also contains drawn touch pads for steering, GAS, BRAKE, PAUSE, RESTART and RECOVER. Multi-touch cancellation, viewport changes, pause lifecycle and residual steering/throttle safety have portable regression tests. **Android touch delivery has not been verified on a real device.** See [touch controls and QA](TOUCH_CONTROLS.md).

## Career and garage

- Three fictional vehicle profiles: **Arrow, Kestrel and Phantom**.
- Offline earned credits and vehicle upgrade rules.
- **12 source-defined career events**: two three-round championships and six standalone challenges.
- Award validation and one-time completion rewards; better medals do not repeat the original completion payment.
- Local profile persistence with validation and rollback on reported save failures.

Explore [garage](GARAGE.md), [career](CAREER.md), [championships](CHAMPIONSHIPS.md) and [challenges](CHALLENGES.md).

## Developer-focused integrity

Engine-independent modules cover startup completeness, course collision expectations, player physics readiness, checkpoint motion sessions, recovery authorization, archive provenance and signed-off build evidence. These guards deliberately **fail closed** on invalid conditions, and do not substitute for real gameplay testing.

Explore the [architecture](ARCHITECTURE.md), [testing reference](TESTING.md), and [executable build pipeline](EXECUTABLE_BUILD_PIPELINE.md).
