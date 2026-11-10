# Experimental six-physical-car grid — manual UE5.8 qualification

> **DRAFT, developer opt-in, source-only. Not a verified playable build.**

This candidate adds four *actual* optional Unreal `ALatebrakeRivalCar`
instances to the existing human + primary-rival single-player circuit.
They are **disabled by default** and fully compiled out from the
additional-spawn path in a **shipping** build. Source tests alone do not
prove that these new actors drive without colliding in Chaos.

## Activation (trusted local developer workstations only)

After building the exact proposed commit using a genuinely installed
UE5.8/UHT target, run a **non-shipping Editor or Development** game with
the single local argument:

    -LatebrakeExperimentalSixCarGrid

Without that argument, normal local racing retains the existing human +
one physical AI rival. A shipping build refuses extra actor spawning even
if the flag appears in the command line.

This flag does not change the save schema, career rewards or online
multiplayer state. It is intended exclusively for controlled physics QA;
it is **not** a public feature or supported packaged game switch.

## Implemented source behavior

- Builds on [PR #329 six-slot start-grid geometry](https://github.com/sanskarIN/latebrake/pull/329)
  and [PR #330 actor identity and recovery ledger](https://github.com/sanskarIN/latebrake/pull/330).
  The same 380×180cm padded start boxes and original asphalt barriers
  govern all six planned positions.
- Only a valid Circuit **non-career** event with an authored, ready
  original track, working primary rival, and successful six-slot geometry
  preflight may create the four additional physical AI actors.
- Extra slots 2–5 use `DontSpawnIfColliding`, real actor tokens and
  distinct route trackers. A failure while building the candidate grid
  destroys all partially created extras and disables this test mode for
  the rest of that run.
- Extra rivals are **not** player-controlled, and they never mutate
  official player checkpoint progression, garage purchases, best times,
  career medals, prize eligibility or the primary-rival HUD comparison.
  They may independently record **unofficial** accepted gate and recovery
  observations in the existing five-slot ledger.
- At each AI control decision the real pawn samples the human and all
  other live AI bodies' **actual** positions, headings and world velocities.
  It refuses acceleration on unknown/invalid actor samples. On a verified
  traffic snapshot, a bounded 0.4-second relative-motion swept
  oriented-rectangle test may remove throttle or add service braking,
  but may **not** steer or add acceleration.
- The existing player's courtesy-brake and primary-rival passing systems
  are still present. A new fleet emergency brake is passed through the
  existing passing-abort + 0.18s brake-hold policies. Supplemental rivals
  do **not** run the speculative player-passing steer.
- A stalled extra rival attempts recovery at its OWN fixed valid original
  grid slot; a primary rival retains its original next-gate recovery.
  All physical vehicles (human plus remaining AI bodies) must provide
  valid obstacle observations for recovery clearance. A blocked/unknown
  landing consumes a **failed** already-issued recovery attempt, without
  moving a body. Only after clearance does the existing real
  `TryRecoverTo` perform post-move physics verification.
- An approved new race attempt destroys and recreates the supplemental
  actor set. Event switching and disabled solo/career races destroy it.
  Stale actor tokens cannot record new session events.
- Normal one-rival traffic and career racing are preserved unless the
  explicit developer flag is supplied.

## Source regression

    cmake -S Tests/Standalone -B Tests/Standalone/build
    cmake --build Tests/Standalone/build --parallel 4
    ctest --test-dir Tests/Standalone/build --output-on-failure
    python3 -m unittest discover -s Tests/Editor -p '*Tests.py'

The new `RivalFleetTraffic` CTest verifies real-observation contract
handling, collisions between coarse frames, multiple distinct blockers,
heading rotations, malformed identities, safe rear-vehicle treatment,
and no steering/score mutation. The source-binding tests verify the
actual non-shipping toggle and live game-mode wiring.

## Required real-engine test campaign (still unverified)

1. Compile Unreal 5.8 Editor/UHT and Development Game for this **exact**
   PR SHA. Keep full build logs and compiler provenance.
2. Open and cook the actual original circuit. Verify the human plus five
   visible, independently simulated physical chassis start at their
   authored grid poses, all within real asphalt collision and without
   overlaps. A source-valid grid does not certify Unreal collision.
3. Repeat a stationary six-car grid launch, initial light sequence,
   straight, corner, turnaround, reverse, pause, restart, focus-loss
   recovery and event switch (Circuit, Time Attack, career).
4. Drive the human car through the pack at 30/60/120 FPS. Exercise
   front and lateral interactions, a car behind, slip/oversteer, emergency
   traffic braking and all actor identity changes. Check continuous
   contact versus source-only 0.4s linear predictions.
5. Force every rival to stall separately. Confirm independent recovery
   budgets, occupied landing rejection, real `TryRecoverTo` pose
   verification and honest failed/succeeded recovery counts.
6. Ensure **only** primary rival route feeds the existing unofficial HUD
   split and finish review. Extra rivals may not award wins, credits or
   affect official player results. Monitor actual UHT, Chaos exceptions,
   collisions, FPS percentiles, hitch frames and memory.
7. Run the normal one-rival Development build without the flag, and a
   shipping build with the flag. Confirm that neither enables extra
   spawns. Install an owner-built Win64 candidate only after provenance,
   original map, code signatures and installed startup log are verified.

## Hard limits

No placement/ranking authority, multiplayer networking, professional AI
quality or real race-physics guarantee is implied. The original 3D track,
actual chassis mesh, sound, effects and engine packaging still require
substantial work. Experimental AI on a simplistic course can collide,
stop or fail to finish. Real collisions can also change direction or
height inside the forecast horizon; mathematical swept XY rectangles
are not a substitute for full Chaos testing.

Keep [playable-release acceptance issue #148](https://github.com/sanskarIN/latebrake/issues/148)
**open** until full owner-verified engine, cooked-map and installed-package
evidence exists. This draft does not publish v0.5.98, alter PR #295's
frozen candidate, merge to main, or provide a real Win64 executable.
