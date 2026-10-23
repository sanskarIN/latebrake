# Contributing

Latebrake is an open-source, community-developed racing game. Contributions are welcome when they are focused, testable and honest about what was actually verified.

## Where to help

- **Portable C++20 logic:** deterministic laps, input state, simulation guards and regression tests.
- **Unreal source integration:** small game class fixes with explicit compile and device-qualification requirements.
- **Docs:** clarify setup steps, UX, features, architecture and accurate release/QA boundaries.
- **QA evidence:** reproductions with exact source revision, platform, engine version, steps and actual game logs when available.
- **Accessibility and polish:** design proposals for remapping, responsive UI and readable HUD controls.

## Suggested process

1. Review [open issues](https://github.com/sanskarIN/latebrake/issues) and [the roadmap](ROADMAP.md), or open an issue for a clearly documented bug.
2. Work on a focused feature/fix branch from `main`.
3. Include regression tests when the code is engine-independent. Reproduce the failure first where possible.
4. Run `cmake` + `ctest` for portable code or `python -m mkdocs build` for documentation changes.
5. Open a pull request with what changed, how it was tested, and what still needs Unreal/Android verification.
6. Do not upload unverifiable executables, leaked licensed assets, unofficial engine installers or proprietary third-party car/track art.

## Source build is not installed gameplay

If a change touches Unreal physics, input, maps, packaging or SaveGame, mention that passing hosted C++ and script CI does **not** count as engine validation. If you have an authorized Unreal installation, include your own test evidence; otherwise leave the runtime QA requirement open.

## Edit this documentation site

Site content is in `docs/` and navigation/theme is in `mkdocs.yml`. The Pages workflow builds on pull requests and publishes only after changes reach `main`.

```bash
python -m pip install -r docs-requirements.txt
python -m mkdocs build
```

[View the repository](https://github.com/sanskarIN/latebrake) · [Report an issue](https://github.com/sanskarIN/latebrake/issues/new) · [Project website](https://sanskarin.github.io/)
