# Official Unreal Quick Race results: prior saved PB comparison

**Source-only until a licensed Unreal 5.8 Editor/UHT build and installed
Win64 playtest have passed.** This is NOT the separate 2D Win32 preview.

The actual GameMode captures the event's persisted best time at initial
profile load and after every accepted race restart. That pre-attempt
snapshot does not change when the official completed race is later
recorded. Only a timing-eligible, non-career Quick Race receives the
read-only Results comparison.

After a successful SaveGame write, Results distinguishes **FIRST SAVED
PB**, **NEW SAVED PB** with improved seconds, **MATCHED SAVED PB**, and a
slower race's seconds **BEHIND** the saved PB. Clock measurements include
real recovery time penalties. A failed save explicitly shows **SAVE
FAILED** and cannot report an earned saved record. Unrecordable finishes,
corrupt records and career medals cannot become a fake quick-race PB.

No changes to checkpoint acceptance, progression, award authority,
vehicle forces, save format, or personal-best calculation. Automated
portable C++20 and Python source-binding tests are not Unreal Editor
compiles, rendered UI tests or installed-game validation.

For manual QA on the genuine UE 5.8 package: run Circuit and solo Time
Attack for first, improved, tied, slower, recovery-penalty and failed
storage attempts; repeat on desktop and Android-sized views. Compare
pre/post SaveGame values against results text and verify separate career
behavior. See [playable release gate #148](https://github.com/sanskarIN/latebrake/issues/148).
