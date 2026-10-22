# SaveGame schema v2 — migration and safety

Previous releases stored best event and lap times, completed race count,
earned credits, owned vehicle mask, selected Arrow/Kestrel/Phantom profile,
and engine/grip tuning in the `LatebrakeProfile` slot.

v0.5.0 adds:
- `SaveSchemaVersion` (2 for the new career format; old/absent is 0)
- `ActiveCareerEventId` (-1 when no event has been selected)
- `CareerMedals` fixed-length array for immutable event IDs 0–11
- `bSpeedInMilesPerHour` optional HUD speed preference

On loading a legacy slot, the existing `GarageState::Load` and
`SanitizeBestTime` / `SanitizeCompletedRaces` rules still run.
Career data initializes empty and does not destroy valid historical
credits, vehicle ownership, upgrades, or racing records. A schema-2
record is interpreted only if the medal array has the expected length.
Malformed/unknown medals (outside 1..3), orphan completions with missing
prerequisites, and unknown selected event IDs are rejected.

On finishing a valid career event, the medal state and earned credit balance
are placed in the **same** new SaveGame object before `SaveGameToSlot`.
Repeat completion after loading cannot award initial credits or championship
bonus again. Unreleased v0.5.1 source uses
`Progression/ProfileWriteCheckpoint.h` to capture the last in-memory garage,
career, completed-race count and personal records **before** awarding a
new finish. Only a successful `SaveGameToSlot` commits the new values.
On a reported write failure the existing bank balance, medal unlocks,
race count and best times are restored immediately, and the HUD reports
an unsaved reward rather than a spendable win. A later genuine replay can
still earn its one-time first-completion award.

In unreleased v0.5.1 development, **starting** a career event has the same
write-before-action rule. The chosen career event and any vehicle required
for it are staged inside a profile checkpoint and persisted **before** the
live race clock, rival, player pawn or active event changes. If the SaveGame
write reports failure, the selection is reverted and launch is refused, with
the existing race left intact. This prevents an unsaved required vehicle
switch or event selection from masquerading as durable career progress.
This workflow has portable policy tests but still requires real Unreal
serialization and installed-game verification.

Vehicle selections, purchases, engine and grip upgrades similarly revert
their balance/ownership/tuning changes on a failed SaveGame write. The
speed-display unit toggle also restores the previous setting if its write
fails. Transient race results and the active running event are **not**
automatically resumed after disk repair; players can retry the event.

**Unreleased v0.5.2 time-record consistency:** Race finish evaluation
now applies the same positive, finite and seven-day upper bound used by
`SanitizeBestTime` when loading historical records. An unrepresentable
or invalid elapsed/best-lap clock cannot award quick-race credits, first
career medals, additional race completions or a stored personal best.
The best-time improvement function also preserves an existing valid
record when a new candidate is slower or malformed, and the SaveGame
writer sanitizes all three best fields before serializing. This avoids
a successful write containing an unrecordable `float` that would
otherwise be silently discarded on the next load. It does **not**
provide atomic filesystem writes or validate actual Unreal serialization.

**Limit of the guarantee:** this is only an in-memory rollback when Unreal
reports a failure. It does not make filesystem writes transactional, detect
a partially corrupted save, or guarantee durability after OS power loss.
There is no crash-safe transactional backup implemented yet; offline
storage integrity and interrupted-write recovery require real device tests.

If a future save schema greater than 2 is loaded, this build refuses
SaveGame writes to avoid silently downgrading it, and warns the player.
It can still read legacy garage fields for display. Back up existing saves
before manually testing migrations on real devices.

The new career snapshot's deterministic rules are tested via C++20 CI;
actual Unreal SaveGame serialization, partially written slot recovery and
OS background persistence remain unverified.
