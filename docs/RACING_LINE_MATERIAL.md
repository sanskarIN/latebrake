# Racing-line material authoring — requires genuine Unreal Engine 5.8

**Status:** Editor automation source prepared, real `.uasset` not yet generated
or engine-verified. The existing 88-mark guide is still an unqualified
source candidate. Generating a material is not proof of packaged rendering.

## Purpose

The existing basic engine cube mesh may not expose a `Color` vector
parameter. Requesting teal on straight guide marks and amber on corner marks
is therefore **not guaranteed** to change their appearance.

The new *project-owned* Material at
`/Game/Materials/M_RacingLineGuide` uses a real `Color` vector parameter
connected to **Base Color** and explicitly supports **Instanced Static
Meshes**. The actual GameMode loads it only when present and checks the
required parameter; missing/mismatched material falls back to the original
neutral stock cube material. This preserves early prototype playability,
without claiming polished colors on an absent asset.

## Author the authentic binary asset

On an authorized Windows Unreal Engine 5.8 installation, with the repository
at the intended racing-line PR source revision, run in PowerShell 7:

```powershell
git lfs install
pwsh ./scripts/New-RacingLineMaterial.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
```

The wrapper verifies the real Editor executable, UE 5.8 Build.version
and project EngineAssociation, then removes any **previous authoring receipt**
before invoking Unreal Editor Python. It requires both a saved material
and a **new Editor-written receipt**, comparing asset identity, Color
parameter, instanced-mesh usage, byte length and exact SHA256. A zero
Editor process exit code alone is never accepted as authoring success.

The receipt is stored at
`Saved/LatebrakeAutomation/racing-line-material-result.json`.
It is ignored by Git. This file is not a credential or proof of licensed
engine provenance, compiled shader correctness or rendered colors.
Inspect the material in the real Unreal Editor and retain the real log.

To revalidate an existing, freshly generated receipt without relaunching:

```powershell
pwsh ./scripts/Test-RacingLineMaterialReceipt.ps1 -MaterialPath './Content/Materials/M_RacingLineGuide.uasset' -ReceiptPath './Saved/LatebrakeAutomation/racing-line-material-result.json'
```

The checker also runs in public CI against synthetic byte fixtures.
Those files are **not** genuine Unreal material assets.

The automation refuses to overwrite an existing material graph. It
validates the `Color` parameter and instanced-mesh usage on existing
assets, and fails if an incompatible asset occupies the path. Material
shader compilation errors and failed saving also fail the script.

Once inspected in the real Unreal Material Editor, commit the **genuine**
`.uasset` through the project's existing Git LFS rules:

```powershell
git lfs ls-files
git add Content/Materials/M_RacingLineGuide.uasset
git lfs ls-files
git commit -m "assets(track): add Unreal-authored racing-line tint material"
```

The project already tracks `*.uasset` via LFS. The clean, committed
checkout is mandatory for `Test-SourceProvenance.ps1`: an untracked
material file deliberately blocks a reproducible Win64 cook. Keep the
actual Editor-created binary and any changed source revision together.
A source-only GitHub check does not run Unreal, generate the asset or
upload a real Windows game.

## Cook and validate

`Config/DefaultEngine.ini` includes `/Game/Materials` in the cooked
directories because the runtime C++ reference is loaded by string.
This inclusion does **not** automatically create an absent material.

During actual game startup the GameMode logs exactly one material source
observation:

- `LATEBRAKE_RACING_LINE_MATERIAL authored=yes` — a project-owned material
  loaded and exposed a `Color` vector parameter; still not proof of
  correct GPU output.
- `LATEBRAKE_RACING_LINE_MATERIAL authored=no` — absent or incompatible
  material; the old stock material is used instead.

The local Windows source-log parser accepts `--require-authored-material`.
A **strict** `-RequireRacingLineToggle` private playtest now requires this
authored-material marker in addition to the 88 guide marks, complete
track startup and an observed `L` key OFF/ON sequence.

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
# After successful real packaging, use its exact printed candidate directory:
pwsh ./scripts/Run-WindowsPlaytest.ps1 `
  -CandidateDirectory 'D:\LatebrakeCandidates\YourActualCandidate' `
  -RequireRacingLineToggle
```

**Visual QA still required:** verify teal/amber against asphalt in the
actual Win64 game at 720p and 1080p, both chase cameras, daylight/shadow,
different display settings and colorblind-safe readability; check the
asset survives the final cooked archive and remains entirely non-colliding.
Retain the source SHA, real log SHA, candidate manifest and screenshots.

Neither `authored=yes` nor source-only CI is a claim of working colors,
UE compilation, installed gameplay, release approval or performance.
