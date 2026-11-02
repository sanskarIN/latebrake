# Solo Time Attack: saved personal-best race clock

**SOURCE ONLY.** Actual Unreal Engine 5.8/UHT compilation, installed Windows
gameplay, Android touch behavior and a cooked project level remain unverified.

The expanded desktop HUD for a live **solo Quick Race Time Attack** now
shows the time remaining on a previously saved, valid Time Attack record:

- `PB CLOCK  23.50s LEFT  |  TARGET 90.50s`
- `PB CLOCK OVER  +1.50s  |  TARGET 90.50s`

This is **not** a predicted finish, sector pace, new PB, rank, ghost or
achievement. It compares the *same authoritative total race clock* used
for completion (including charged recoveries) to the prior saved record.
At equality the remaining budget is zero: a tie does not beat the PB.

The target is absent without a valid saved PB, in career and Circuit
events, before green, while paused or focus-suspended, in menus, with a
missing/unready driver, after finish, and on compact/touch HUDs where
core race controls take priority. The comparison has no access to credit,
saved profile, award, input or physics mutation.

C++20 `TimeAttackPersonalBestTarget` CTest covers start, partial clock,
equality, missed deadline, penalty-aware clock, invalid old records,
invalid clocks, seven-day bounds and the full event/session-state matrix.
Python source-binding checks assert that the Unreal Canvas HUD uses this
policy with the actual GameMode race and saved event PB. Real visual QA
is still required under [playable gate #148](https://github.com/sanskarIN/latebrake/issues/148).
