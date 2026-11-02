# Windows standalone preview: solo-event isolation QA

> **Scope:** optional native Win32/GDI **2D** preview only. This is not the
> Unreal Engine 5.8 game, a production car simulation, a multiplayer mode
> or evidence of a packaged Unreal Windows executable.

## User-visible behavior

| Mode | Laps | Simulated local AI rivals | Win condition |
| --- | ---: | ---: | --- |
| Circuit Race | 3 | 3 | Complete all ordered gates and laps |
| Time Attack | 1 | 0 | Complete within 65 seconds |
| Rival Duel | 2 | 3 | Finish the ordered laps before any rival reaches its final lap |
| Checkpoint Rush | 2 | 0 | Complete within 95 seconds |

Time Attack and Checkpoint Rush no longer silently run invisible rival
physics. Their unused rival slots stay zero initialized, and the simulation
does **not** advance them or apply rival collision damage. The rendering
and circuit minimap draw only the configured rivals, and the HUD calls out
solo clock challenges. Circuit Race and Rival Duel still contain three
**scripted local AI cars**, not remote players.

The per-race `Race::rivalCount` snapshot is set only in `beginRace()`.
Changing a pending event does not mutate the number of physics actors
mid-attempt. Invalid event indices fail closed before a race begins.
Rival Duel evaluates **each** active rival's completed laps for its loss
condition; it no longer assumes that AI index 2 is guaranteed to win.

## Automated verification (source-level)

1. `Tests/Standalone/PreviewRaceModesTests.cpp` compiles the actual
   `PreviewRaceModes.h` policy on strict C++20 toolchains. It verifies
   all four mode counts, lap counts and invalid mode rejection.
2. `Tests/Editor/Win32SoloRaceBindingTests.py` confirms gameplay,
   car rendering, minimap and headless self-test consume the policy
   rather than only documenting it.
3. The Windows preview build workflow compiles the actual native
   `LateBrakePreviewV5.exe` using MSVC; its `--self-test` walks all four
   mode configurations, checks no solo AI initialization or movement
   occurs, and exercises scripted rival event setup.
4. The isolated profile self-test still verifies no real player career
   files are written.

These checks are **not** real gamepad, graphical, FPS, collision-feel
or installed-system acceptance tests. Do not call a passing CI run a
playable Unreal build.

## Manual Windows checklist (not yet completed)

- From the Challenges menu start Time Attack. Confirm one-lap target,
  no red opponent car on the track/minimap, solo HUD label, and clock
  remains active while driving.
- Repeat Checkpoint Rush; confirm two laps and no AI contact damage.
- Start Circuit Race and Rival Duel. Confirm three visible moving
  local rivals, track and minimap agree, and rivalry remains active.
- In Rival Duel, allow a different AI slot to reach its target laps;
  the event must lose regardless of which rival finished first.
- Restart each event and switch modes between attempts. Confirm
  the new count and lap count apply only to the new race.
- Verify keyboard, controller, low-FPS fixed-step update, window
  focus/reconnection, Results and local progress save in a real GUI.

## Remaining limitations

The 2D preview is a separate experimental application. Unreal race
events, verified opponent placements, advanced overtaking behavior,
genuine Win64 cooked builds and all player-hardware QA remain separate
work. No prerelease or final game installer is claimed by this change.
