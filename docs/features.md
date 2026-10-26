# Features & systems

This page distinguishes source-level features from tested game builds. All gameplay below describes code in the repository, **not confirmed functionality of an installed game**.

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
