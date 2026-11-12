import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
PRIVATE = ROOT / "Source/Latebrake/Private"
PUBLIC = ROOT / "Source/Latebrake/Public"

class OriginalAsphaltUnrealPreviewBindingTests(unittest.TestCase):
    def test_native_preview_is_explicitly_opt_in(self):
        game=(PRIVATE / "Game/LatebrakeRaceGameMode.cpp").read_text(encoding="utf-8")
        self.assertIn("LatebrakeExperimentalAsphaltSkin", game)
        self.assertIn("SpawnOriginalAsphaltSkinPreview", game)
        self.assertIn("#if !UE_BUILD_SHIPPING", game)
        self.assertIn("BuildOriginalCircuitPaintPreview", game)

    def test_bridge_does_not_touch_physical_collision(self):
        bridge=(PRIVATE / "Track/OriginalAsphaltUnrealPreviewBridge.cpp").read_text(encoding="utf-8")
        self.assertIn("OriginalPhysicalAsphaltSkin author", bridge)
        self.assertIn("author.ValidateMesh(result.Mesh)", bridge)
        self.assertIn("BuildUnrealSections", bridge)
        self.assertIn("CreateMeshSection_LinearColor", bridge)
        self.assertIn("false, // Authoritative asphalt collision remains unchanged.", bridge)
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::NoCollision)", bridge)
        self.assertIn("SetGenerateOverlapEvents(false)", bridge)
        self.assertIn("SetCanEverAffectNavigation(false)", bridge)
        self.assertIn("SetCastShadow(false)", bridge)
        self.assertNotIn("SetSimulatePhysics(true)", bridge)
        self.assertNotIn("SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)", bridge)
        self.assertNotIn("Race.Credit", bridge)
        self.assertNotIn("RecordCollisionBlock", bridge)

    def test_original_physical_road_sources_remain_the_only_geometry(self):
        source=(PUBLIC / "Track/OriginalPhysicalAsphaltSkin.h").read_text(encoding="utf-8")
        self.assertIn("PlannedCollisionBlock", source)
        self.assertIn("OriginalSkinPhysicalAsphalt", source)
        self.assertIn("SkinSlabTop", source)
        self.assertIn("ValidateOriginalAsphalt", source)
        self.assertIn("DoNotModifyCollision", source)
        self.assertIn("DoNotAwardRaceCredits", source)
        self.assertIn("IgnoredOverlappingSections", source)

if __name__ == "__main__":
    unittest.main()
