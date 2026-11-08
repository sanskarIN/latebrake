# Measuring LateBrake's actual source-code scale

The long-term 200,000-line aspiration should track **authored production
game source**, not documentation, release workflow YAML or thousands of
tests mistaken for implemented gameplay.

Run from a checkout (Git and Python 3 required):

    python3 scripts/MeasureSourceScale.py
    python3 scripts/MeasureSourceScale.py --json

Counts are reproducible on the current tracked checkout using
"git ls-files --cached -z" and UTF-8 text. The report separates:

- game_source: Source/ .cpp, .h, .cs — the actual Unreal game modules.
- experimental_preview: Experiments/ source — not the Unreal game.
- test_code: Tests/ source/test policies — important, but not a
  shipped feature or production-game LOC.
- automation_code: scripts/ Python and PowerShell.
- ci_workflows: .github/workflows/ YAML.
- Documentation, assets, ignored/untracked generated code and symlinks
  are excluded from the game-source milestone.

Physical lines include blank lines and comments; the separate nonblank
count **still includes comments**, so neither is a semantic measure of
working functionality. The report never fails a build for being under
200,000 lines. It must not be misused to justify dead code or repeated
boilerplate. Code quantity does not prove collision, rendering, sound,
car behavior or a playable Windows build.

Milestone coverage should prioritize: real UE5.8 cooked drivable circuit;
licensed original 3D cars and environments; actual Chaos wheel contact;
reliable AI opponents; career, garage, garage saves and multiplayer when
single-player is verified; useful racing UI; graphics, audio and
performance optimization; packaging, crash logs and installed-device QA.
Every feature needs tests and user-visible acceptance evidence. See
issue #148 for mandatory playable-release gates.
