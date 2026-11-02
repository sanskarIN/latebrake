"""Read-only HUD binds to physically observed, non-stale contact.

These source assertions cannot compile UE5.8 or verify on-screen pixels.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CAR_H = (ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h").read_text(encoding="utf-8")
CAR = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
HUD = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(encoding="utf-8")
MODEL = (ROOT / "Source/Latebrake/Public/Vehicles/ChassisContactReadout.h").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
DOCS = (ROOT / "docs/CHASSIS_GROUND_CONTACT_QA.md").read_text(encoding="utf-8")


class UnrealGroundContactHudBindingTests(unittest.TestCase):
    def test_real_chassis_updates_live_observation_before_any_hud_readout(self):
        for marker in (
            '#include "Vehicles/ChassisContactReadout.h"',
            "latebrake::ChassisGroundContactOutput LastGroundContact{};",
            "latebrake::ChassisContactReadout GetGroundContactReadout() const noexcept",
            "latebrake::DescribeChassisContact(LastGroundContact)",
            "LastGroundContact = {};",
        ):
            self.assertIn(marker, CAR_H)
        for marker in (
            "latebrake::EvaluateChassisGroundContact(GroundSample)",
            "if (!GroundContact.Valid)",
            "LastGroundContact = GroundContact;",
            "LastGroundContact = {};",
            "Chassis->IsSimulatingPhysics()",
            "Chassis->GetPhysicsLinearVelocity()",
        ):
            self.assertIn(marker, CAR)
        section = CAR.split("void ALatebrakeCarPawn::Tick(", 1)[1]
        self.assertLess(section.index("EvaluateChassisGroundContact(GroundSample)"),
                        section.index("LastGroundContact = GroundContact;"))
        self.assertLess(section.index("LastGroundContact = GroundContact;"),
                        section.index("Chassis->AddForce(Forward * DriveForce"))

    def test_hud_readout_cannot_change_physics_score_or_mobile_layout(self):
        for marker in (
            "if (!bCompact && VerticalPlan.MaxTelemetryLines >= 28)",
            "Car->GetGroundContactReadout()",
            "if (Contact.Live)",
            "ChassisContactStatus::Full",
            "ChassisContactStatus::Partial",
            'TEXT("ROAD")',
            'TEXT("PARTIAL")',
            'TEXT("AIRBORNE")',
            'TEXT("GROUND %s  %d/4 | TRACTION %d%%")',
            "Contact.QualifiedProbes",
            "Contact.DriveTractionPercent",
        ):
            self.assertIn(marker, HUD)
        for forbidden in (
            "PassCheckpoint(", "SaveGame", "AddForce(",
            "AwardCredits(", "SetActorLocation(", "UCLASS(",
        ):
            self.assertNotIn(forbidden, MODEL)
        self.assertIn("NAME ChassisContactReadout", CMAKE)
        self.assertIn("ChassisContactReadoutTests.cpp", CMAKE)
        self.assertIn("ground-contact readout", DOCS)

    def test_pure_policy_rejects_inconsistent_contact_and_stale_frames(self):
        for marker in (
            "sample.SupportingProbes < 0",
            "sample.SupportingProbes > 4",
            "std::isfinite(sample.ContactTractionScale)",
            "sample.HasDriveContact != (sample.SupportingProbes >= 2)",
            "std::abs(sample.ContactTractionScale - scale)",
            "ChassisContactStatus::Unavailable",
            "ChassisContactStatus::Airborne",
            "ChassisContactStatus::Partial",
            "ChassisContactStatus::Full",
        ):
            self.assertIn(marker, MODEL)


if __name__ == "__main__":
    unittest.main()
