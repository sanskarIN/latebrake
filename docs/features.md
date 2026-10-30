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
