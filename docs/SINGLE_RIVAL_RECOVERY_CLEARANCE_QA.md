# Native single-rival recovery preflight

The actual autonomous actor now obtains the local human's current chassis
identity, position, heading and planar velocity immediately before a
recovery attempt. The historical recovery pose is preferred. If its
padded footprint is occupied, the selector considers four bounded lateral
alternatives on the same longitudinal coordinate. Every candidate uses
existing swept occupancy, complete asphalt and barrier checks. Unknown
human state or invalid primary geometry refuses recovery. A refused
preflight consumes the existing attempt ledger and does not move the actor.

This is source-level original-circuit preflight for one human and one
rival, not a real collision guarantee or five-rival runtime. The existing
engine placement and post-move physics verifier remains mandatory. No
player recovery fee, checkpoint, award or save policy changes.

Native acceptance: obstruct each historical rival recovery position with
the actual player, observe clear lateral selection or refusal, check no
forward route progress, inspect the verified/failed recovery counters,
then clear the area and verify a later independently qualified attempt.
Test missing player, pause, restart and physics loss. Licensed Unreal
compile, cook and device playtest remain separate.
