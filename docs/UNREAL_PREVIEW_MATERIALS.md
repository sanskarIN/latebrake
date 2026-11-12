# LateBrake native visual materials — Unreal Editor 5.8

Procedural six-car bodies, asphalt skin, guardrail steel and painted kerbs
already provide triangles and vertex colors. The fallback stock Unreal Cube
material does not guarantee vertex colors, so models might appear gray even
when the geometry exists. The editor authoring script creates real PBR
material assets rather than treating a source-only preview as a verified
render.

## Run on Windows 11 with installed Unreal Editor 5.8

From the repository root (with the Editor Python plugin enabled):

    pwsh scripts/New-PreviewMaterials.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8'

This calls licensed Unreal Editor to create/compile six actual Material
packages and rechecks a fresh engine receipt. See the receipt at:
Saved/LatebrakeAutomation/preview-materials-result.json.

New assets use Vertex Color for Base Color and physically motivated
roughness/metallic values. Existing assets are NEVER overwritten: the receipt
will say existing_unmodified_requires_visual_review because their graphs may
not be compatible. The wrapper checks each .uasset byte count and SHA-256;
it does NOT certify the visual render.

The native .uasset files are created in Content/Latebrake/Materials:
M_LatebrakeRoadPbr, M_LatebrakeSteelPbr, M_LatebrakeCarPaintPbr,
M_LatebrakeGlassPbr, M_LatebrakeRubberPbr, M_LatebrakeEmissionPbr.
They cannot be fabricated from source-only tests. Review the actual binary
assets and commit them only after native inspection.

## Nonshipping preview options

Enable only during an authentic Editor/Windows QA run as appropriate:

    -LatebrakeExperimentalCarMesh
    -LatebrakeExperimentalCircuitPaint
    -LatebrakeExperimentalAsphaltSkin
    -LatebrakeExperimentalArmco
    -LatebrakeExperimentalKerbs
    -LatebrakeExperimentalSprungChassis

Verify all six car colors, lamps, glass, tires, real bumper clearance,
road-edge striped paint, Armco metal sheen, actual collision and wheel
contact, race clock, recovery and lap rewards, and 30/60/120 FPS behavior.
Check both Editor visuals and actual cooked assets before release.
No source test, material receipt or mock validation certifies native gameplay.

If the material assets are absent, Unreal uses the stock engine material
fallback. A fallback is not proof of intended PBR colors.
