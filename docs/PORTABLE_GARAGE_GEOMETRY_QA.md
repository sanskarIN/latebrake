# Portable garage geometry integration

This is a source-only subset of draft PR #452 at `6fa9ad94113b3459fc86f9e41b4c6e726aed191d`. The selected portable files retain their exact authored blobs. No Unreal Pawn, GameMode, HUD, SaveGame schema, collision, input, native adapter, or release files are imported. Existing factory paint and the default rim choice remain supported by the pure source APIs. The geometry can be inspected without Unreal; this is not a playable or cooked 3D game.

The included tests cover six-car appearance isolation, gallery browsing, saved-look payload validation, three rim geometries, individual wheel pivots, material sections, OBJ/MTL export, visual transition scheduling, and read-only light/mesh diagnostics. The appearance payload is not yet integrated with the live SaveGame in this subset.

## Build the static preview tool

From an isolated checkout with CMake and a C++20 toolchain:

```powershell
cmake -S Tests/Standalone -B build/portable
cmake --build build/portable --config Release --target latebrake_original_obj_preview
````

Run the resulting executable with a new output directory. It writes 18 original car/rim OBJ and MTL pairs. Use a new empty directory to avoid overwriting same-named exported files. Inspect the body, four independent wheel groups and material names in a compatible 3D model viewer. The static source models have no engine shaders or physical collision authority.

Before importing the remaining PR #452 runtime work, reconcile its duplicated portable source with main, run exact-head source CI, then perform real Unreal 5.8 UHT/Editor, cook and installed gameplay validation. Do not infer native readiness from this portable slice.

## Combined source candidate

Includes reference timing clock (#454), cumulative garage budget and existing Canvas HUD (#458), measured lap penalty/sector review (#459), and portable crease-aware vehicle shading plus corrected showroom fallback from #452. No new native procedural mesh adapter, camera input binding, collision or SaveGame schema is imported. The existing HUD source now uses the budget and lap review; actual Unreal engine build/render QA remains separate. All exact combined CI checks must pass before merge. Avoid merging overlapping sibling PRs independently after this candidate lands.
