# Native practice trial and setup evidence QA

Status: implementation on the integration branch; requires Unreal Engine native verification.
Portable C++ and source-binding tests are not a substitute for a cooked Windows or Android build.

## Preconditions

- Start from a clean test profile with Arrow owned and at least one unowned vehicle.
- Record credits, owned mask, equipped ID, engine/grip levels, saved appearance and
  persistent lap/event records before starting.
- Use the normal starter circuit and actual local player input.

## Practice configuration lifecycle

1. Open Quick Race and browse or click one of the four practice presets.
2. Enable trial (Q or the desktop panel). Choose Nova while it is still unowned.
3. Change engine and grip independently; verify the pending labels and configured
   multipliers. Do not launch yet.
4. Verify the active race pawn is unchanged while staging choices.
5. Select to start. Inspect the actual car model and compare measured acceleration
   and handling against the earlier owned configuration.
6. Restart the practice race. Its active trial configuration must remain latched.
7. Open Quick Race and stage a different trial configuration without starting it.
   Close the menu and verify the active race still uses the original setup.
8. Launch a normal circuit, time attack, and eligible career event. Verify the
   latest saved garage setup is restored and unowned trial cars cannot enter
   persistent competition.
9. Inspect the saved profile; practice itself must not change credits, ownership,
   tuning, styling, medals or saved best times.
10. Exercise focus loss, manual pause, page changes, narrow windows and stale
    mouse positions. Unavailable controls must not edit or launch anything.

## Timing and analysis

- Complete a valid practice attempt, then repeat exactly the same preset, car,
  upgrades and handling. Confirm the comparison, fee attribution and newest-first
  attempt list are populated from actual checkpoint timing.
- Switch handling mid-race and switch back. The attempt must not become
  comparable merely because the final configuration matches.
- Compare a finish with a charged recovery fee against a fee-free finish.
  The fee-inclusive and fee-free bests must be presented separately.
- Complete more than eight attempts. The recent list is bounded, while the
  full-session means, counts and bests include every accepted comparable finish.
- Complete a 10-lap practice and inspect the full gate comparison through all
  pages. No lap or gate is fabricated by missing samples.
- Compare two configurations within the same preset and handling. Verify
  that a multi-axis change is excluded from single-axis comparison summaries.
- Switch between solo and rival modes, and between handling modes. Their
  measured setup evidence must remain separate.
- Use Next Test and Retest Measured Best on a sufficiently tall desktop
  viewport. These must stage a pending setup only; SELECT remains necessary.
- On smaller windows, verify keyboard setup shortcuts and textual guidance
  remain readable. Touch mode retains the normal saved-setup practice flow.

## Completion evidence

Record the exact Git commit, Unreal build version, platform, frame timing,
input path, profile before/after and screenshots or video of the above.
Do not mark native verification complete solely from portable tests.
