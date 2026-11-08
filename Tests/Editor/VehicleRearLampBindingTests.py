"""Source-only binding of real chassis scale and point lights; NOT Unreal QA."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")
class VehicleRearLampBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header=read("Source/Latebrake/Public/Vehicles/VehicleRearLampLayout.h")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
    def test_root_and_all_attached_visuals_share_one_scale(self):
        self.assertIn('#include "Vehicles/VehicleRearLampLayout.h"',self.pawn)
        for axis in "XYZ":
            self.assertEqual(self.pawn.count("latebrake::VehicleRootScale"+axis),4)
        self.assertIn("Chassis->SetRelativeScale3D(RootScale);",self.pawn)
        self.assertIn("Component->SetRelativeLocation(RelativeLocation / RootScale);",self.pawn)
        self.assertNotIn("FVector(-385.f",self.pawn)
        self.assertNotIn("FVector(-390.f",self.pawn)
    def test_three_real_lights_and_focus_reset_still_wired(self):
        ctor=self.pawn.split("ALatebrakeCarPawn::ALatebrakeCarPawn()",1)[1].split(
            "void ALatebrakeCarPawn::BeginPlay()",1)[0]
        self.assertEqual(ctor.count("CreateDefaultSubobject<UPointLightComponent>"),3)
        for needle in ("latebrake::VehicleRearLampOffsets",
                       "Lamps[Index]->SetupAttachment(Chassis);",
                       "Lamps[Index]->SetRelativeLocation(LampLocations[Index] / ChassisScale);",
                       "Lamps[Index]->SetAttenuationRadius(",
                       "Lamps[Index]->SetCastShadows(false);",
                       "Lamps[Index]->SetIntensity(0.f);"):
            self.assertIn(needle,ctor)
        for needle in ("ApplySignalLights(true, bEffectiveBraking,",
                       "ApplySignalLights(false, false, 0.0);",
                       "RearLeftSignal->SetIntensity(",
                       "RearRightSignal->SetIntensity(",
                       "ReverseSignal->SetIntensity("):
            self.assertIn(needle,self.pawn)
    def test_strict_source_geometry_contract(self):
        for text in ("VehicleChassisHalfLengthCm == 190.0",
                     "VehicleChassisHalfWidthCm == 90.0",
                     "VehicleChassisHalfHeightCm == 32.5",
                     "IsRearLampNearPhysicalBumper(",
                     "VehicleRearLampRadiusCm = 170.0"):
            self.assertIn(text,self.header)
        self.assertIn("NAME VehicleRearLampLayout",self.cmake)
        self.assertIn("VehicleRearLampLayoutTests.cpp",self.cmake)
        self.assertIn("/W4 /WX",self.cmake)
        self.assertIn("-Wpedantic -Werror",self.cmake)
if __name__=="__main__":
    unittest.main()
