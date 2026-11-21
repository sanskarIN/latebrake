# Production C++ source-only mainline contribution

This branch imports **107 original production header-only C++20 models** and **24 strict standalone test translation units** from the owner's earlier tested [LateBrake racecraft draft #346](https://github.com/sanskarIN/latebrake/pull/346), frozen at `4f09f3587a37857a6b26d34defc6cb11e3bd09bf`. The systems cover fleet identity, tire contact/grip, suspension, braking, collision safety, AI traffic proposals, pit crew chronology, track rules, powertrain, race-control evidence, session telemetry, lap replay and driver coaching.

The import intentionally **does not** carry the old branch's changes to actual Unreal Engine GameMode, Chaos pawn/rival actors, private actor implementation files, build rules, race timers, credits or release automation. Future native Unreal 5.8 integration needs separate UHT/Editor/Win64 driving QA and does not follow from portable source tests.

Strict CI on this *current-main based tree* must compile the 24 imported C++20 portable tests with warnings as errors on Windows, Ubuntu and macOS, in addition to all existing tests. The accompanying source audit enforces **87,792 + 20,000 = 107,792 unique qualifying nonblank/noncomment production C++ lines** using `scripts/MeasureSourceScale.py --json`, not gross PR additions or any test/doc lines. Exact duplicate source files are excluded by the meter.

This source library is real C++ but is **not yet a playable six-car game, native installed build or verified physics simulator**. Do not claim that until physical Unreal integration is tested independently. Respect protected-main permissions, required CI and review rules; do not publish tags or releases through this contribution.
