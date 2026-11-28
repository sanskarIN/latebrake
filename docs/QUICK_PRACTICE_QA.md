# Quick Race practice presets

The existing circuit is reused, with genuine ordered gate and active-clock
requirements. No new track or multiplayer capability is claimed.

| Choice | Laps | Local rival | Persistent finish records or rewards |
| --- | ---: | --- | --- |
| Standard / Expert / Rookie | 3 | Yes | Existing rules |
| Solo time attack | 1 | No | Existing rules |
| Solo practice | 3 | No | No |
| Solo practice | 5 | No | No |
| Rival practice | 5 | Standard | No |
| Rival practice | 10 | Standard | No |

## Native verification checklist

- Browse the eight choices with keyboard N, gamepad D-pad Right and mobile
  browse; verify the choice description and lap count before starting.
- Start every practice and verify actual HUD lap target, complete ordered
  gate sequence, start lights and player grid readiness.
- Solo sessions must have no rival pawn. Rival practice must have exactly
  one local Standard opponent and the requested physical lap target.
- Restart halfway and from Results. The active practice stays unchanged.
- Browse while paused, then resume without launching: the active attempt
  must not acquire a new lap target or opponent tier.
- Finish and compare the profile before and after: no completed-race count,
  credits, medals, best event times or best lap records may change.
- Practice must also work with the experimental fleet command-line flag
  without spawning extra cars or triggering its observational finish path.
- Return to normal circuit or time attack through menu or direct hotkey.
  Confirm the original lap count and persistence policy are restored.
- Launch an unlocked career event from practice. Only after successful
  profile selection does the active practice cease; save failure must
  retain the current attempt.
- Verify recovery, pause, focus loss and pit-service paths on the longer
  sessions, and that the menu is usable on a compact touch display.

Portable tests verify the menu state space, preset definitions and source
bindings. Actual Unreal physics, presentation and profile isolation still
require a native playtest.

## Whole-run practice timing

Ten-lap practice results distinguish the full attempt from the retained eight-lap detail window. Whole-attempt mean, deviation, fastest/slowest lap identities, fee-free totals and streaks are computed from authoritative ordered lap completions using bounded constant-memory statistics. No extrapolation or extra persistent record is created. Invalid or missing active-clock evidence suppresses the full-run summary. Verify the first two laps remain included in the ten-lap aggregate even after they leave the recent window.

## Session-only practice comparisons

Completed practice attempts can be compared against earlier attempts using exactly the same practice preset, car ID, engine/grip levels and handling mode. Records exist only in the current GameMode instance; no SaveGame changes are made. Mid-attempt tuning, handling or player-pawn changes permanently disqualify that attempt's comparison, even when the original configuration is restored. A valid ordered timed finish is still required. Verify repeat attempts with matching and different configurations, returning to a previously used configuration, changing handling during a run, and leaving/re-entering the level. Results must clearly distinguish session bests from saved personal bests.

## Session reference at each ordered gate

A complete practice with positive active time in every sector can become a session-local reference for the same preset, vehicle, engine/grip levels and handling. Each cumulative gate and sector is compared against a single prior completed attempt, with charged fee deltas separate from active-clock differences. The reference changes only on a faster complete comparable run. Missing or zero-active-time sectors, inconsistent gate order and configuration changes suppress exact-gate comparisons without changing the legitimate race result. Verify the live HUD, full results rows, resets, fee attribution and different configurations. No comparison writes to the saved profile.

## Direct desktop mode chooser

When the desktop menu has sufficient room, Quick Race shows eight mode cards below the existing navigation dock. A card chooses a pending preset only; a separate SELECT starts the race. Each card discloses lap count, requested local rivals and whether a finish can update saved records or remains session-only. The experimental fleet flag and read-only profile are reflected in normal mode cards. Stale, resized, unrendered or focus-suspended cards cannot select. Compact and touch layouts retain Browse/Select. Verify all cards, navigation, hover, narrow-window fallback and intentional launch separately in native gameplay.

## Practice results inspection panel

A complete matched practice reference unlocks a desktop Results inspection panel. Choose a gate, page through actual lap rows, and select a row to see its cumulative, active-clock and fee deltas. Diverging bars use one symmetric scale across all retained rows of that gate; negative is faster, positive slower. A missing comparison is not shown as a zero bar. The existing sector-motion panel remains the fallback when a complete practice comparison or sufficient desktop space is absent. Compact/touch layouts keep the text results. Native QA must exercise ten laps, multiple pages, resize/focus transitions and separate gameplay restart actions.
