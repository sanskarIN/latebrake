"""Native Unreal two-front-spotlight runtime source wiring, not packaged QA."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def src(path):return (ROOT/path).read_text(encoding="utf-8")
class UnrealFrontLampBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn=src("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.h=src("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.plan=src("Source/Latebrake/Public/Vehicles/VehicleFrontLampRig.h")
        cls.cmake=src("Tests/Standalone/CMakeLists.txt")
        cls.tests=src("Tests/Standalone/FrontLampRigTests.cpp")
    def test_real_unreal_spotlight_components(self):
        for token in ("USpotLightComponent","FrontLeftLamp","FrontRightLamp",
                      "SetAttenuationRadius(", "SetInnerConeAngle(",
                      "SetOuterConeAngle(","SetCastShadows(false)",
                      "SetAffectGlobalIllumination(false)",
                      "SetIntensity(0.f)"):
            self.assertIn(token,self.pawn)
        self.assertIn("class USpotLightComponent;",self.h)
        self.assertIn("TObjectPtr<USpotLightComponent> FrontLeftLamp;",self.h)
        self.assertIn("TObjectPtr<USpotLightComponent> FrontRightLamp;",self.h)
    def test_correct_parent_scale_and_appearance_refresh(self):
        for token in ("RefreshFrontLampPositions();",
                      "PlanFrontLampRig(", "FVector(p.Xcm,p.Ycm,p.Zcm)/Scale",
                      "OriginalVehicleVisualOffsetZCm",
                      "VehicleChassisHalfLengthCm+4.0"):
            self.assertIn(token,self.pawn+self.plan)
        self.assertNotIn("AddForce(",self.plan)
    def test_pause_gate_applies_to_both_lamps(self):
        method=self.pawn.split("void ALatebrakeCarPawn::ApplySignalLights(",1
        )[1].split("void ALatebrakeCarPawn::ApplyWheelVisualPose()",1)[0]
        self.assertIn("bRaceDrivingAllowed, bEffectiveBraking",method)
        self.assertIn("Signals.RedRearIntensity > 0.0",method)
        self.assertIn("FrontLeftLamp->SetIntensity(",method)
        self.assertIn("FrontRightLamp->SetIntensity(",method)
        self.assertIn("PlanFrontLampRig(",method)
        self.assertIn("200000",self.tests)
        self.assertIn("NAME FrontLampRig",self.cmake)
if __name__=="__main__":
    unittest.main()
