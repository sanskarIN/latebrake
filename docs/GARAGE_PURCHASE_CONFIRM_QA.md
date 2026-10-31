# Garage spending confirmation — source QA

The prototype garage now requires an explicit **second press of the same
purchase shortcut within 8 real seconds** before spending credits:

| Shortcut | First press | Second identical press |
| --- | --- | --- |
| Q | Preview next locked car and its price | Attempt to unlock |
| U | Preview next engine level and its price | Attempt engine upgrade |
| I | Preview next grip level and its price | Attempt grip upgrade |

**E** still cycles between already-owned vehicles without spending credits.
Insufficient funds, already-completed upgrades, locked career editing and
unavailable cars never create a spend offer.

The on-screen garage feedback names the proposed purchase, the exact
price and the matching key to press again. A different purchase, changed
credits/ownership/tuning/selected car, invalid or backward clock, or
expiry requires a fresh preview. Offers are in memory only, never
persisted. Switching menu pages, opening/closing the menu, selecting
another owned vehicle and race restart cancel the pending authorization.

## Engineering safety

The portable `GaragePurchaseConfirmation` gate does **not** debit credits.
On the second press it authorizes only a single attempt to run the existing
`GarageState::Unlock` or `GarageState::Upgrade`. The game mode retains
its previous career-edit eligibility, price and funds checks, and
`ProfileWriteCheckpoint` rollback on save failure.

Time uses the platform monotonic clock rather than the paused Unreal world
clock, so an open menu cannot extend the 8-second confirmation window
indefinitely. This does not add financial purchases, microtransactions or
online services; all credits are virtual in-game rewards.

## Verification

```sh
python -m unittest discover -s Tests/Editor -p '*Tests.py'
cmake -S Tests/Standalone -B build/test
cmake --build build/test --parallel
ctest --test-dir build/test --output-on-failure
```

Portable `GaragePurchaseConfirmation` regression checks consumed
approvals, wrong key, changed balances and target, deadline, non-finite
clocks, insufficient credits and thousands of repeated sequences. Editor
source contract checks the ordering of authorization, existing garage
mutator, save and rollback.

**Not a verified game build:** UE 5.8 Editor/UHT, functional input
feedback, gamepad/keyboard repeat behavior, SaveGame failure scenarios
and an installed Windows playtest must still be tested. See
[playable qualification #148](https://github.com/sanskarIN/latebrake/issues/148)
and [production UI milestone #236](https://github.com/sanskarIN/latebrake/issues/236).
