"""Real Unreal visual preview: report actual committed component and
requested lamp output without claiming engine-rendered picture quality."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")
class UnrealVisualReadoutBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy=read("Source/Latebrake/Public/Vehicles/VehicleVisualPlaytestReadout.h")
        cls.tests=read("Tests/Standalone/VisualPlaytestReadoutTests.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
    def test_engine_derived_observation_not_3d_claim(self):
        for name in ("IsOriginalVehicleSurfaceCommitted",
                     "GetOriginalVehicleTintParameter",
                     "AreFrontLampsRequestedOn"):
            self.assertIn(name,self.header)
            self.assertIn("Car->"+name+"()",self.hud)
        self.assertIn("bFrontLampOutputRequested = FrontPlan.OutputIntensity",self.pawn)
        self.assertIn("IsValid(FrontLeftLamp) && IsValid(FrontRightLamp)",self.pawn)
        self.assertIn("#if !UE_BUILD_SHIPPING",self.hud)
        self.assertIn("MenuPage::Settings:",self.hud)
        self.assertIn("LIVE CAR MESH:",self.hud)
        self.assertIn("PAINT PARAM:",self.hud)
        self.assertIn("FRONT BEAMS REQUESTED:",self.hud)
        self.assertIn("VERIFY PIXELS IN ENGINE",self.hud)
    def test_fail_closed_policy_and_standalone_build(self):
        for token in ("VisualPlaytestSurface::LegacyCubeFallback",
                      "VisualPlaytestSurface::SourceMaterialIncomplete",
                      "VisualPlaytestSurface::SourceGeneratedSurface",
                      "VerifiedByEngineRendering = false"):
            self.assertIn(token,self.policy)
        self.assertIn("150000",self.tests)
        self.assertIn("NAME VisualPlaytestReadout",self.cmake)
    def test_qa_readout_cannot_change_driving_or_save_state(self):
        section=self.hud.split("case latebrake::MenuPage::Settings:",1)[1].split(
            "case latebrake::MenuPage::Profile:",1)[0]
        self.assertIn("ReportVisualPlaytest(",section)
        for forbidden in ("ApplyGarageToPlayer(", "Garage.Unlock(",
                          "SetMassOverrideInKg(", "EarnCredits("):
            self.assertNotIn(forbidden,section)
if __name__=="__main__":
    unittest.main()
