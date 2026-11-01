# Matched-gate player/rival split timing — source QA

**Source-only Unreal 5.8 development, not a verified playable build.**

The real player and prototype rival each have their *own* accepted forward
gate progression. After both have crossed the **same ordinal gate on the
same lap**, the expanded desktop HUD can display:

`SPLIT L1 G2: RIVAL +2.00s (NOT PLACE)`

A positive signed delta means the rival arrived later; a negative value
means the rival crossed earlier. It is **not** official race placement,
a physically measured road separation or an online timing leaderboard.

The tracker uses the same local, post-GO GameMode clock for both vehicles,
excluding artificial player recovery penalties from this *observation*
only. Penalties still count toward the actual race clock and rewards.
No new checkpoint/reward/garage authority or SaveGame fields are added.

## Defenses and verification

- No result until both physical gate traces accept the same cumulative
  gate ordinal, not simply the same gate index across different laps.
- Duplicate/out-of-order ordinals, zero/negative/NaN/infinite timestamps,
  reversed clocks and out-of-bounds configurations fail closed.
- Fixed, allocation-free 32-entry history per driver; overwritten slots
  must carry the precise full ordinal to match.
- A successful restart or new race configuration clears all history; a
  rejected career replay leaves the prior finished race intact.
- Gameplay pause, focus loss, menu, missing rival and race finish use
  existing GameMode eligibility guards.
- Compact/touch HUD layouts do not gain a new information row.

Portable C++20 and Python source-binding tests validate these contracts.
Actual licensed Unreal/UHT build, authored cooked map, installed Win64,
camera/HUD positioning and Android hardware qualification remain open
under issues #148 and #13. This feature does not turn source archives
into installers.
