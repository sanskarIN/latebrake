# Unofficial rival finish-gate observation in Unreal Quick Race

**Source-only status:** This is an implementation and portable test suite,
not a real UE 5.8 compiled or installed playable game.

The original single offline rival uses independently accepted forward
checkpoint crossings. When the player completes a timing-eligible,
three-lap/four-gate Quick Race Circuit event, the actual Unreal GameMode
captures one *read-only* snapshot of the rival's verified route progress.
The Results Canvas shows one of these observations:

- **UNOFFICIAL: RIVAL ALREADY CLEARED THE FINAL GATE** — the rival route
  was already complete when the player finished.
- **UNOFFICIAL: RIVAL ROUTE NOT COMPLETE AT YOUR FINISH** — the rival
  had not completed the required verified gate sequence at that instant.

Neither observation is an official rank, win/loss, finish time or head-to-
head podium. Actors can tick in an arbitrary order within the same
render/physics frame, and physical order needs independent qualification.
No AI condition can change player eligibility, checkpoint progression,
career medals, earned credits, best times, SaveGame, or rewards.

Solo Time Attack, career races, invalid timing, missing/unready rival,
inconsistent lap counts and malformed gate counts show **no** inferred
finish result. Every new attempt clears the prior observation. A failed
SaveGame write may still show a valid *session-only* gate observation,
but the separate save warning remains prominent.

Portable regression validates all intermediate gate states, an
independently completed rival route, invalid configurations and session
readiness. Source-binding tests check the real GameMode and HUD wiring.

**Remaining real game QA:** actual UE 5.8/UHT compile, cooking the
original circuit, installed Windows/Android controller and visual
testing, a physical rival ahead/behind the final gate, same-frame
crossing ambiguity, interruptions, and reliable verified AI motion.
See [release gate #148](https://github.com/sanskarIN/latebrake/issues/148).
