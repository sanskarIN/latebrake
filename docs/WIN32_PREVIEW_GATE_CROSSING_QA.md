# Win32 2D preview — real directional gate crossing

**Scope:** the standalone native Win32/GDI V5 **2D experiment**, **NOT**
LateBrake's Unreal Engine 5.8 racer. No installed-device driving test has
been completed by these source changes.

## Problem fixed

A car previously could be accepted at a checkpoint merely by overlapping
its broad circular zone while facing in the nominal travel direction.
That is insufficient evidence of actually passing the painted gate, and
could allow accidental or misleading progress after a teleport/turn.

## Current scoring policy

The `update()` physics step now captures the current car's position
*before* moving. For the next ordered gate, both the pre-step and
post-step positions must be on the procedural road. Their movement segment
must go from strictly **upstream** to **on or beyond** the oriented
checkpoint plane. The final car position must be inside the gate zone,
the car must be moving forward faster than the gate's minimum speed and
point in the correct direction, and the existing ready latch must be
armed. No new lap, finish, credit, record, save schema or rival rule is
introduced. The same `creditGate()` process remains authoritative.

Recovery teleports are not counted as a crossing because prior position
is sampled from the next `update()` step, after teleport. Paused frames,
zero elapsed movement, stationary correct-facing cars, wrong-way
crossings and attempts starting beyond the gate also cannot score.

## Automated proof vs open QA

The Windows native V5 `--self-test` checks invalid gate IDs and cycles
through **all four oriented gates for three laps**, refusing wrong-way
movement, stationary on-gate positions, already-beyond starts and repeat
credits before admitting a forward movement across each gate. The
cross-platform Python source-binding test also guards the actual update
call and GitHub Actions executable smoke-test wiring.

These are deterministic synthetic tests, **not a substitute** for
installed Windows 10/11 keyboard/gamepad playtests. Hands-on QA should
drive all four layouts at normal and low speed, confirm checkpoints
remain reachable, test reverse and recovery near gates, and record real
FPS, rendering and installer behavior. The official UE 5.8 build and
release gate remains [#148](https://github.com/sanskarIN/latebrake/issues/148).
