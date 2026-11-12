"""Real Unreal source binding for the shared four-corner wheel geometry.

This inspects source only; licensed Unreal 5.8 UHT and real driving QA
are mandatory and intentionally NOT implied by these assertions.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
ANCHORS = (ROOT / "Source/Latebrake/Public/Vehicles/ChassisWheelAnchors.h").read_text(encoding="utf-8")
TESTS = (ROOT / "Tests/Standalone/ChassisWheelGeometryTests.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")


class UnrealWheelAnchorBindingTests(unittest.TestCase):
    def test_one_four_corner_map_controls_construction_rays_and_visuals(self):
        self.assertIn('#include "Vehicles/ChassisWheelAnchors.h"', PAWN)
        ctor = PAWN.split("ALatebrakeCarPawn::ALatebrakeCarPawn()", 1)[1].split(
            "void ALatebrakeCarPawn::BeginPlay()", 1
        )[0]
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1
        )[0]
        visual = PAWN.split("void ALatebrakeCarPawn::ApplyWheelVisualPose()", 1)[1].split(
            "void ALatebrakeCarPawn::UpdateWheelVisuals(", 1
        )[0]
        self.assertIn(
            "latebrake::ChassisWheelAnchors[static_cast<unsigned>(Index)]", ctor
        )
        self.assertIn("Anchor.ForwardCm", ctor)
        self.assertIn("Anchor.RightCm", ctor)
        self.assertIn("Anchor.SpindleHeightCm", ctor)
        self.assertIn(
            "ConfigureDecorativeMesh(Wheels[Index], Cylinder.Object, Location", ctor
        )
        self.assertIn("latebrake::ChassisProbeOffset(", tick)
        self.assertIn("ChassisForward * static_cast<float>(Probe.ForwardCm)", tick)
        self.assertIn("ChassisRight * static_cast<float>(Probe.RightCm)", tick)
        self.assertIn("WheelWorldAnchors[Index] = Origin;", tick)
        self.assertIn("GetPhysicsLinearVelocityAtPoint(Origin)", tick)
        self.assertIn("World->LineTraceSingleByChannel(", tick)
        self.assertIn("latebrake::ChassisVisualOffset(", visual)
        self.assertIn("Position.SpindleHeightCm", visual)
        self.assertIn("FVector RootScale", visual)
        self.assertIn("Wheels[Index]->SetRelativeLocation(", visual)
        self.assertNotIn("ProbeX[Index]", tick)
        self.assertNotIn("ProbeY[Index]", tick)
        self.assertNotIn("WheelX = Index", visual)
        self.assertNotIn("WheelY = Index", visual)
        self.assertLess(tick.index("latebrake::ChassisProbeOffset("),
                        tick.index("latebrake::CalculateSprungChassis(SpringSample)"))
        self.assertLess(tick.index("latebrake::CalculateSprungChassis(SpringSample)"),
                        tick.index("ApplySprungForces(SpringResponse, WheelWorldAnchors);"))

    def test_geometry_is_ordered_and_cannot_invent_grip_or_force(self):
        for expected in (
            "ChassisWheelCount = 4",
            "ChassisHalfWheelbaseCm = 122.0",
            "ChassisHalfTrackCm = 94.0",
            "ChassisWheelSpindleHeightCm = -29.0",
            "ChassisWheelVisualRadiusCm = 29.0",
            "ChassisWheelAnchors{{",
            "ChassisProbeOffset(unsigned corner)",
            "ChassisVisualOffset(",
            "std::array<ChassisWheelAnchor, ChassisWheelCount>",
        ):
            with self.subTest(expected=expected):
                self.assertIn(expected, ANCHORS)
        for banned in (
            "UWorld", "FVector", "UCLASS(", "AddForce(", "AddTorque(",
            "SpawnActor(", "SetActorLocation(", "AwardCredits(",
            "SaveGame", "GetWorld(",
        ):
            self.assertNotIn(banned, ANCHORS)
        self.assertIn("ChassisWheelGeometryTests.cpp", CMAKE)
        self.assertIn("NAME ChassisWheelGeometry", CMAKE)
        self.assertIn("latebrake_chassis_wheel_geometry_tests PRIVATE /W4 /WX", CMAKE)
        self.assertIn("SuspensionTargetRideDistanceCm", TESTS)
        self.assertIn("CalculateSprungChassis(", TESTS)
        self.assertIn("roll < -1.0 && pitch < -1.0", TESTS)


if __name__ == "__main__":
    unittest.main()
