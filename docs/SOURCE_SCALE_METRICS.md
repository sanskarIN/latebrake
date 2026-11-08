# Measuring LateBrake's production-source milestone

**v0.6.0 pre-release requires at least 200,000 meaningful nonblank,
noncomment production-source lines.** This target belongs to v0.6.0;
it is not deferred to the separate v1.0.0 stable milestone. Quantity is
necessary for that milestone, and never sufficient for game quality.

Run from a checkout (Git and Python 3 required):

    python3 scripts/MeasureSourceScale.py
    python3 scripts/MeasureSourceScale.py --json

Before publishing v0.6.0, explicitly run the quantity gate in addition to
all source, Unreal, cooked-package and installed-game acceptance checks:

    python3 scripts/MeasureSourceScale.py --json --require-milestone

This opt-in command exits 1 while the target is unmet and still prints its
complete JSON report. Normal PR source CI remains informational; useful
small changes and refactoring must not fail solely for a smaller codebase.
The existing core CI runs the report and its independent lexical fixtures.
No v0.6.0 publication is authorized by an informational source count.

## Scope and reproducibility

The audit uses `git ls-files --cached -z` and the current checkout's UTF-8
bytes. Stage new files before auditing a proposed change. Schema **2**
separates these categories:

| Category | Included files | Counts toward 200,000 |
| --- | --- | --- |
| `game_source` | `Source/` C++ `.cpp`, `.cc`, `.cxx`, `.h`, `.hpp`, `.inl` | Nonblank noncomment lines only |
| `unreal_build_rules` | `Source/` C# `.cs` target/module rules | No |
| `experimental_preview` | `Experiments/` source | No |
| `test_code` | `Tests/` source and fixtures | No |
| `automation_code` | `scripts/` Python/PowerShell | No |
| `ci_workflows` | Workflow YAML | No |

Production reports **physical lines**, **nonblank lines** (including
comments), and **nonblank noncomment lines** separately. Comment syntax
inside string/character/raw literals remains literal code. C++ continued
line comments and physical line mapping are handled before lexical
counting. C# rules are measured separately, including verbatim literals.
`goal_metric`, `milestone`, `remaining_game_source_lines` and `goal_reached`
refer exclusively to `game_source.nonblank_noncomment_lines`.

Documentation, binary assets, ignored/untracked outputs, symlinks (including
symlinked parent directories), conventional generated/vendor/test/build
folders within `Source/`, `.generated.h`/`.gen.cpp` files and exact duplicate
production-file bytes are excluded. Duplicate exclusion is deterministic;
partial or rewritten duplication, dead code, boilerplate and the actual
usefulness of code still require human review. This is a lexical source
audit, not a language compiler, semantic complexity score or proof that
all measured code is functional. Never add padding to satisfy the number.

## Audited baseline

At `51052bb856da498db9897292faef2b1389388c37` (`main`, November 8, 2026),
schema 2 reports **177 C++ production files**, **21,154 physical lines**,
**19,640 nonblank lines**, and **17,410 nonblank noncomment lines**.
The remaining quantity is **182,590**. The three Unreal build-rule files
(41 physical lines) are separate. Unmerged physics, road and steward
branches are independent candidates and must never be summed together.

Schema 1 used physical source lines, including C# build rules, to calculate
its goal. Its historical 21,195-line main result remains a physical count;
it does not satisfy this stricter production-code milestone.

Keep system completion and runtime acceptance alongside size: real UE5.8
wheel contact and handling, an original cooked circuit, AI, race progression,
art/audio/UI, performance and genuine installed Windows packages. See
[playable gate #148](https://github.com/sanskarIN/latebrake/issues/148).
