# Experimental six-car runtime — source candidate

Start a non-shipping Unreal build with -LatebrakeExperimentalSixCarGrid.
Only Quick Race Circuit requests the fleet. Default, career and solo events
retain the existing opponent configuration. Four additional physical rivals
are spawned only before GO, with collision-aware engine placement. A failed
spawn or subsequent roster change cannot silently become a two-car race.

Pre-GO validates six current bodies and fresh road probes on the original
original asphalt slabs (including their coplanar overlaps). The exact live
component, hit plane and weak UObject generation are checked. After GO,
roster identity and usable physics replace
grid-position checks. Pause/session changes invalidate permission until
the next admitted observation. All five other bodies are collected anew
before AI traffic or recovery preflight; the player still uses the original
paid recovery transaction. Passing is disabled in the experiment.

No finish awards, best times, career records, ranking or persistent rewards
are granted by this mode. HUD reports observed AI route progress only.
No new map, individual wheel suspension, multiplayer or native gameplay
qualification is claimed. Test default/solo/career isolation, 30/60/120 FPS,
all five AI gate sequences, obstructed spawn, failed recovery, stale/removed
actor, pause/resume, restart, finish without save, and cooked rendering.
Existing record display may still show the normal saved baseline.

Native UE5.8 compilation and the owner-controlled Win64 review are mandatory
before considering this experimental source for merge or release.

## Fleet review
Every observed rival gate and recovery has its own identity-bound receipt.
Live HUD gate counts compare only receipt-matched actor route observations;
missing or inconsistent evidence is explicitly unavailable. A five-rival
radar groups nearby measured positions without inventing off-map markers.
At the player's actual final gate, freeze an observational review before
revoking the attempt. Differences within 0.25 active-clock seconds are
uncertain, and no sampled comparison grants a place or persistent award.

## Pre-GO settling and visual identity
The optional fleet preserves only a bounded, already observed downward
velocity during pre-GO blocked-drive neutralization. This lets Chaos settle
from elevated spawn without adding force or accepting an unsupported grid.
Pause, menu, focus loss, invalid poses, fault and GO are excluded.
The five AI cars use the five cosmetic vehicle IDs and five distinct paints
other than the selected player appearance at spawn; their existing neutral
physics multipliers remain unchanged. This is a run-local visual plan, not
five newly simulated vehicle types or evidence of rendered pixels.

## Startup watchdog
The development grid must have 0.30 seconds of consecutive valid settled
observations before the real three-second countdown begins. Losing that
qualification while still red resets the countdown, never an active race.
The 15-second pre-GO active-tick budget includes countdown time, excludes
pauses and invalid/hitch samples, and faults until explicit restart if
exhausted. HUD diagnostics identify the blocking condition and slot. The
physical gantry and HUD cannot advertise GO while fleet driving is denied.
These time limits are conservative developer-mode choices, not native
playtest measurements or new race-time fees.

## Distinct AI lane targets
Five AI slots retain their original grid-side lane preference. For an
aligned physical car on the incoming straight, a bounded speed-dependent
lookahead targets a narrow offset inside the exact authored asphalt slab.
The target tapers near the real gate corridor and is constrained by the
padded body footprint. During corners, large heading errors, invalid
samples, and failed geometry checks it falls back to the original target.
Gate credit, route recovery distance, original nonexperimental AI, and
all downstream brake/traffic/steering guards are unchanged. This does not
claim overtaking, avoidance guarantees or measured handling quality.

## Queue-aware recovery
A stationary experimental rival can defer its ordinary eight-second stall
recovery only while a fresh, complete six-body observation shows a nearby
slow, aligned, nonoverlapping car ahead on pavement. The allowance is at
most 12 seconds per accepted route segment; missing evidence, moving
stalls, collisions and exhausted budgets use the existing recovery policy.
Pause and same-gate teleport do not replenish that allowance. A real
forward checkpoint is still processed before any recovery deferral.

## Known off-road opponents
Strict fleet forecasts still require every body on pavement by default.
Only the experimental runtime admits the separately identified degraded
context when the controlled car is on road and all five genuine blockers
remain valid within authored ground. Every blocker participates in the
same swept-contact and stopping-distance evaluation. Motor demand is
capped at 25 percent and speed over 900 cm/s requests braking. Unknown,
missing, out-of-ground or invalid actors and invalid brake evidence still
fail closed. An off-road subject uses the existing recovery path. These
are conservative prototype constants, not measured handling performance.
