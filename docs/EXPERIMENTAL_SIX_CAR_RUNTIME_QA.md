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
