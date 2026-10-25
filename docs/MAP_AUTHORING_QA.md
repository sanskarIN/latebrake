# v0.5.41 — Unreal starter-map receipt integrity (published source-only)

**Status: v0.5.41 published SOURCE-ONLY prerelease; not a playable Unreal game.**
The independent source release workflow passed synthetic receipt and C++20
policy checks, but did not run a genuine licensed Unreal Editor or cook a map.

## Purpose

The genuine circuit map does **not** ship as a compiled `.umap` in Git.
A licensed Unreal 5.8 Editor must create or load, save and serialize
`/Game/Maps/LatebrakeCircuit` before packaging. The previous script wrote
an Editor success receipt listing the map package and file size, but a file
of the same size with different bytes could satisfy the receipt. Also,
calling `New-StarterMap.ps1` directly skipped the UE version/uproject
compatibility checks supplied by other build stages.

## Hardened authoring sequence

On a Windows workstation with authorized **Unreal Engine 5.8** installed:

```powershell
pwsh ./scripts/Build-Editor.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8'
pwsh ./scripts/New-StarterMap.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8'
pwsh ./scripts/Build-PlayableCandidate.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8' -Platform Win64 -OutputDirectory 'D:/LatebrakeCandidateWin64'
```

The map stage:

1. Requires the project's `EngineAssociation` and installed engine's
   `Build.version` to both identify **5.8** before launching Editor.
2. Deletes any previous receipt so a swallowed Editor Python exception
   cannot pass due to an older successful invocation.
3. Requires the Editor Python script to confirm the running engine reports
   version 5.8, create/load `/Game/Maps/LatebrakeCircuit`, and successfully
   save a genuine binary level.
4. Writes a **version 1 receipt** with the fixed map package, operation
   (`created` or `loaded`), engine version `5.8`, byte length and a
   streaming **SHA256 of the actual saved level**.
5. Rejects malformed/stale receipts, old schema, mismatched engine or map
   identity, unexpected operation, missing map, symlinks, wrong length,
   invalid digest and same-size byte modification, then independently
   checks the serialized Unreal package header.
6. The candidate pipeline pins source HEAD and map SHA256 before UAT and
   checks identity again after UAT; it does not claim the receipt alone
   proves which map actually cooked.

To inspect the map receipt on the authorized machine after genuine Editor
authoring:

```powershell
pwsh ./scripts/Test-MapAuthoringReceipt.ps1 -MapPath './Content/Maps/LatebrakeCircuit.umap' -ReceiptPath './Saved/LatebrakeAutomation/starter-map-result.json'
```

To test the policy *without* Unreal, CI exercises disposable **synthetic
non-UE bytes**; those files are never shipped or treated as valid levels:

```powershell
pwsh ./scripts/Test-MapAuthoringReceiptPolicy.ps1
```

## Evidence still needed

A well-formed receipt proves only that recorded bytes and the inspected
file agree. The version fields are emitted by authoring scripts; they
are **not cryptographic proof** of genuine engine execution. Even a map
with Unreal package magic may fail to load or cook. Authentic UHT/editor
compilation, level load/cook and game startup, gate collisions, an installed
Win64 executable, Android package and real-device driving remain unverified.

See [executable build pipeline](EXECUTABLE_BUILD_PIPELINE.md),
[checkpoint QA issue #31](https://github.com/sanskarIN/latebrake/issues/31)
and [Android QA issue #13](https://github.com/sanskarIN/latebrake/issues/13).
