# Native Garage playtest evidence — v0.6.0

This optional offline tool records **unverified human observations**. It
does not install or compile Unreal, launch a game, authenticate evidence,
approve a merge or enable a release. It is independent of the Garage code
candidate, currently PR #470, and can be reviewed on main separately.

## First obtain a genuine build

Review the live Garage PR head and use the protected-main
[trusted manual draft Unreal validation](UNREAL_DRAFT_PR_VALIDATION.md)
workflow. Select PR 470, the exact current reviewed SHA, and Editor first.
After Editor/UHT passes, dispatch Win64 at that same SHA. Download the
complete candidate only after the separate post-build identity check passes.

Follow [Windows personal playtesting](WINDOWS_PERSONAL_PLAYTEST.md). Use a
disposable local profile for save failure, future schema and purchase
tests; do not alter your normal saved progress. The diagnostic runner
produces a private schema-2 receipt and runtime log. The archive must
remain hash-identical after the session. The tool requires the genuine
receipt with the actual same-session log and a successful exit archive
seal; a normal process exit alone does not establish gameplay.

## Record observations

Run from the trusted tooling checkout with Python 3. Use the actual
private session paths and the **reviewed full 40-character commit SHA**.

```powershell
$Sha = Read-Host 'Exact reviewed Garage candidate SHA'
python scripts/Review-WindowsPlaytest.py garage-init --receipt 'D:/QA/session.json' --expected-source $Sha --output 'D:/QA/garage-checklist.json'
```

Edit only environment, result and notes fields in the checklist. There are
138 distinct manual scenarios:

| Group | Count |
| --- | ---: |
| Six car identities | 6 |
| Car × eight paint choices | 48 |
| Car × three stripe choices | 18 |
| Car × three rim choices | 18 |
| Car × four orbit views | 24 |
| Actual viewport layouts | 6 |
| Integration behaviors | 18 |

The matrix checks visible results, not merely pressed keys. Record a
descriptive observation for each pass or fail. Leave unperformed checks
as not_tested. Hardware or viewport that is unavailable remains not_tested.
Do not invent results to meet a deadline. A single-session checklist is
not a substitute for separate hardware-specific tests. Document additional
sessions separately, then optionally aggregate them with the command below.
Never combine inconsistent candidate identities.

```powershell
python scripts/Review-WindowsPlaytest.py garage-evaluate --receipt 'D:/QA/session.json' --expected-source $Sha --checklist 'D:/QA/garage-checklist.json' --output 'D:/QA/garage-summary.json'
```

This rechecks the candidate's complete closed-world manifest, session log
and exact source SHA. Reports group counts, unresolved scenarios and
blocking observations (including process failure or incomplete hardware
details). Even all self-reported passes cannot set gameplay_verified,
source_authenticity_verified or ready_for_release. Private JSON files
must remain outside the sealed candidate. Do not upload private receipts
or logs containing user paths or personal information.

## Combine separate sessions

For restart, hardware and viewport tests, initialize and record a new
checklist per separate genuine runtime receipt. Combine one to sixteen
receipt/checklist pairs only when the exact source, manifest, executable,
engine and map hashes agree:

```powershell
python scripts/Review-WindowsPlaytest.py garage-aggregate --expected-source $Sha --session 'D:/QA/session1.json' 'D:/QA/garage1.json' --session 'D:/QA/session2.json' 'D:/QA/garage2.json' --output 'D:/QA/garage-aggregate.json'
```

The tool rechecks each archive and runtime log independently. Duplicate
receipt hashes are rejected. An earlier fail stays a fail even when
another session reports a pass; the conflict is listed for manual review.
A not_tested observation does not erase a pass or failure in another
session. The aggregate remains unverified.

For PR #470, provide the owner with independently reviewed native build
and cook logs, exact SHA, observed screenshots and the manually checked
summary against issue #462. Review the integration's native QA guide.
Neither the recorder nor ordinary hosted source CI validates rendered
pixels, real collision physics or an installed playable release.
