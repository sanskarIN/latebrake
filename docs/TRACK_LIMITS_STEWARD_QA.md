# Starter-circuit race steward — experimental source integration

Status: Unmerged racing-game source candidate. Portable policy C++ can be
tested in CI; a genuine Unreal 5.8 Editor/Game compilation and installed
Win64 visual/physics test are still required by issue #148.

## Game behavior

The existing starter circuit consists of four pavement rectangles. The new
TrackLimitsSteward queries the same IsOnPavedSurface classifier used by the
current vehicle/offroad logic. No new surface is fabricated or claimed.
Only the actual local player chassis, under the same authorized GO/clock
eligibility as the race, may accrue a track-limits violation.

- Driving off pavement at at least 8 km/h for 0.6 seconds triggers a
  warning. A continuing qualifying offroad excursion for 1.5 seconds
  records one violation. The warning does not change the race clock.
- Three separately qualified excursions yield one five-second time penalty.
  The sixth and ninth incursions likewise add five seconds each, using
  RaceProgress::AddTimePenalty. Fees are already part of lap, sector,
  finish and saved record times. They are not added again in the HUD.
- To prevent repeated scoring while the car remains off pavement, the
  new episode requires at least 0.5 seconds continuously ON pavement.
  A brief clipped edge cannot repeatedly trigger a new violation.
- Slow/stationary offroad frames do not accrue new exposure. Invalid
  location, velocity, delta time, giant >250ms frames, missing/unauthorized
  driver, countdown, results, pause, focus loss and teleport suspend active
  exposure. GameMode also checks actual pawn identity, the pawn's monotonic
  teleport revision and the authoritative checkpoint-session epoch before
  every update, skipping the first unmatched sample rather than stitching
  exposure across a discontinuity. Previously recorded race-local strikes
  persist until an accepted RestartRace; a latched excursion must rejoin.
- The penalty callback is transactional: a refused five-second
  RaceProgress charge cannot be reported as an applied penalty or a
  recorded third violation. Every attempt is capped and deterministic.
- HUD is read-only. It displays an active warning and the number of
  recorded strikes; it never directly alters the clock, career save or
  car physics. All UI must be checked for 720p and Android line budgets.

## Current verification

- Real production RaceProgress clock integration test: exactly one five-second fee on the third excursion, with sector, finished-lap and reset accounting.
- Standalone C++20 strict warnings-as-errors policy tests: 30/60/120
  Hz, reverse movement, disabled authorization, pause/reset, no double
  counting, clean rejoin, invalid numbers and atomic failed penalties.
- Python source-binding test checks that the real GameMode is responsible
  for calling the policy and charging the existing race clock.
- This does not prove Unreal UHT/Game builds, physics contact, meaningful
  corner-cut fairness on real car trajectories, gamepad/mobile UX, the
  new curved circuit, Windows installer, or frame time.

## Required human qualification before merge

On an authorized UE 5.8 workstation, build the real game and Starter Map,
then drive three genuine distinct offroad excursions with steering, brakes
and throttle. Capture real player HUD and RaceProgress clock (+5 sec only
after third). Repeat with a stationary car, short road re-entry, lost focus,
manual pause, failed restart, verified recovery, 30/60/120 FPS, AI nearby,
career and solo Time Attack. Inspect whether the warning/penalty is fair.
Do not call the source-only checks a playable release.

Important: this implementation applies solely to the ORIGINAL four-strip
track. PR #316's proposed 1.70-km 9-m continuous road is a separate draft;
its future runtime activation requires swapping this classifier and
independently qualifying proper wheel/road contact in real Unreal.

## Scale target

200,000 maintainable lines of game code can be a long-term milestone, not a
release criterion. Prioritize working vehicle assets, track collision,
career/event behavior, lighting/audio, AI, loading/streaming, UI and
installer QA. Count production gameplay separately from tests and CI,
and never manufacture repetitive code to hit a number.
