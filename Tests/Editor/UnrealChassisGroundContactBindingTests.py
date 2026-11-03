"""Source wiring contract for actual Unreal WORLDSTATIC ground traction.

This suite never claims an Unreal 5.8/UHT compile, cooked road or real
suspension/independent tire contact. It checks the shared actor source.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
POLICY = (ROOT / "Source/Latebrake/Public/Vehicles/ChassisGroundContactTraction.h").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
DOCS = (ROOT / "docs/CHASSIS_GROUND_CONTACT_QA.md").read_text(encoding="utf-8")
NAV = (ROOT / "mkdocs.yml").read_text(encoding="utf-8")


class UnrealChassisGroundContactBindingTests(unittest.TestCase):
    def test_actual_player_and_rival_share_four_worldstatic_road_traces(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1]
        for marker in (
            '#include "Vehicles/ChassisGroundContactTraction.h"',
            '#include "CollisionQueryParams.h"',
            "GroundSample.ChassisVerticalSpeedCmPerSecond =",
            "Chassis->GetComponentLocation()",
            "GroundQuery.AddIgnoredActor(this);",
            "constexpr float ProbeForwardCm = 122.f;",
            "constexpr float ProbeSideCm = 94.f;",
            "for (int32 Index = 0; Index < 4; ++Index)",
            "World->LineTraceSingleByChannel(",
            "Hit, Origin, End, ECC_WorldStatic, GroundQuery",
            "Hit.bBlockingHit",
            "Hit.GetComponent()->GetCollisionObjectType() == ECC_WorldStatic",
            "static_cast<double>(Hit.Distance)",
            "static_cast<double>(Hit.ImpactNormal.Z)",
            "latebrake::EvaluateChassisGroundContact(GroundSample)",
            "if (!GroundContact.Valid)",
            "const float ContactTractionScale =",
        ):
            self.assertIn(marker, PAWN)
        self.assertLess(tick.index("Chassis->GetPhysicsLinearVelocity()"),
                        tick.index("World->LineTraceSingleByChannel("))
        self.assertLess(tick.index("latebrake::EvaluateChassisGroundContact("),
                        tick.index("CalculateProgressiveRoadLoad("))
        self.assertLess(tick.index("latebrake::EvaluateChassisGroundContact("),
                        tick.index("CalculateGripLimitedBraking("))

    def test_no_traction_without_observed_contact_and_no_untrusted_energy(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1]
        for marker in (
            "GroundContact.HasDriveContact",
            "TireCondition.Observe({",
            "bServiceBraking && GroundContact.HasDriveContact",
            "bHumanDriver && GroundContact.HasDriveContact",
            "bEffectiveBraking || !GroundContact.HasDriveContact",
            "bDraftTargetReady && GroundContact.HasDriveContact",
            "TransmissionFrame.PropulsionFactor *",
            "ContactTractionScale);",
            "if (GroundContact.HasDriveContact &&",
            "else if (GroundContact.HasDriveContact && !bEffectiveBraking)",
            "static_cast<float>(LateralForce.RightForce) *",
            "if (bEffectiveBraking && GroundContact.HasDriveContact)",
            "static_cast<float>(PhysicalBraking.ForwardForce) *",
            "TurnTorque * ContactTractionScale",
        ):
            self.assertIn(marker, tick)
        self.assertGreaterEqual(tick.count("ContactTractionScale"), 6)

    def test_physics_geometry_avoids_fake_wheel_rig_and_ground_invention(self):
        for marker in (
            "ChassisContactProbeTraceLengthCm = 280.0",
            "ChassisContactMaximumSupportedHeightCm = 115.0",
            "ChassisContactMinimumGroundNormalZ = 0.68",
            "ChassisContactMaxFrameSeconds = 0.10",
            "supporting == 3 ? 0.75",
            "supporting == 2 ? 0.45",
            "supporting >= 2",
            "std::isfinite(probe.DistanceCm)",
            "std::isfinite(probe.GroundNormalZ)",
            "std::isfinite(s.ChassisVerticalSpeedCmPerSecond)",
        ):
            self.assertIn(marker, POLICY)
        self.assertIn("pitch/roll-locked", DOCS)
        self.assertIn("NOT a playable", DOCS)
        for forbidden in (
            "AddForce(", "AddTorque(", "PassCheckpoint(", "AwardCredits(",
            "SetActorLocation(", "SetPhysicsLinearVelocity(",
            "SpawnActor(", "SaveGame", "GetWorld(", "UCLASS(",
        ):
            self.assertNotIn(forbidden, POLICY)
        self.assertIn("ChassisGroundContactTractionTests.cpp", CMAKE)
        self.assertIn("NAME ChassisGroundContactTraction", CMAKE)
        self.assertIn("CHASSIS_GROUND_CONTACT_QA.md", NAV)


if __name__ == "__main__":
    unittest.main()
