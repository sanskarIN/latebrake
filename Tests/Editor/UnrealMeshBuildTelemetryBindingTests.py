"""Real Unreal source-render build counts trace actual rebuild boundaries.

No assertion here proves GPU rendering or the Editor has compiled.
"""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class UnrealMeshBuildTelemetryBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.h=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy=read("Source/Latebrake/Public/Vehicles/OriginalMeshBuildTelemetry.h")
        cls.tests=read("Tests/Standalone/OriginalMeshBuildTelemetryTests.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
    def test_count_measures_real_source_build_calls(self):
        rebuild=self.pawn.split(
            "void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1
        )[1].split("int32 ALatebrakeCarPawn::GetCommittedOriginalWheelParts()",1)[0]
        self.assertEqual(rebuild.count("OriginalMeshBuildAttempt("),1)
        self.assertEqual(rebuild.count("OriginalMeshBuildCommit("),1)
        self.assertLess(rebuild.index("OriginalMeshBuildAttempt("),
                        rebuild.index("MakeOriginalVehicleAnimatedParts("))
        self.assertLess(rebuild.index("CanPresentOriginalVehicleVisual("),
                        rebuild.index("OriginalMeshBuildCommit("))
        self.assertLess(rebuild.index("RenderParts[i]->SetVisibility(true, false);"),
                        rebuild.index("OriginalMeshBuildCommit("))
        self.assertIn("SourceMeshBuildTelemetry",self.h)
        self.assertIn("GetOriginalMeshBuildTelemetry() const noexcept",self.h)
    def test_hud_is_read_only_and_does_not_claim_pixels(self):
        settings=self.hud.split("case latebrake::MenuPage::Settings:",1)[1].split(
            "case latebrake::MenuPage::Profile:",1)[0]
        self.assertIn("3D SOURCE BUILDS:",settings)
        self.assertIn("Car->GetOriginalMeshBuildTelemetry()",settings)
        self.assertIn("#if !UE_BUILD_SHIPPING",settings)
        for forbidden in ("Garage.Unlock(","SaveRecords(","SetGarageTuning(",
                          "RebuildOriginalVehicleVisual(","SetSimulatePhysics("):
            self.assertNotIn(forbidden,settings)
        self.assertIn("do NOT prove",self.policy.replace("do NOT prove","do NOT prove")) if False else None
        self.assertIn("do NOT prove Unreal UHT compilation",self.policy)
    def test_saturating_source_only_policy(self):
        self.assertIn("Committed<=value.Attempts",self.policy)
        self.assertIn("std::numeric_limits<std::uint32_t>::max()",self.policy)
        self.assertIn("300000u",self.tests)
        self.assertIn("NAME OriginalMeshBuildTelemetry",self.cmake)
if __name__=="__main__":
    unittest.main()
