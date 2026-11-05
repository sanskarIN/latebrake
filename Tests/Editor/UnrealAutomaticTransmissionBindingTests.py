"""Source binding of an actual gearbox force multiplier to the UE car.

This does not compile Unreal Engine, validate motor/gear sounds, demonstrate
a real torque-converter or qualify a packaged 3D Windows game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CPP = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
HEADER = ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Vehicles/AutomaticTransmission.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
NAV = ROOT / "mkdocs.yml"


class UnrealAutomaticTransmissionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = CPP.read_text(encoding="utf-8")
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")

    def test_real_shared_pawn_owns_automatic_shift_state(self):
        self.assertIn("latebrake::AutomaticTransmission Transmission;", self.header)
        self.assertIn("GetTransmissionReadout() const noexcept", self.header)
        self.assertIn("Transmission.Reset();", self.header)
        self.assertIn("Transmission.Suspend();", self.cpp)
        self.assertIn('#include "Vehicles/AutomaticTransmission.h"', self.cpp)
        tick = self.cpp.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        for token in (
            "Transmission.Advance({",
            "static_cast<double>(LongitudinalSpeed)",
            "static_cast<double>(EffectiveThrottle)",
            "static_cast<double>(MaxSpeed)",
            "if (!TransmissionFrame.Valid)",
            "RejectOptionalDriveAfterSpring(SpringResponse);",
            "TransmissionFrame.PropulsionFactor",
            "latebrake::GearAwareMotorPowerFactor(",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
        ):
            self.assertIn(token, tick)
        self.assertLess(
            tick.index("if (!TransmissionFrame.Valid)"),
            tick.index("Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *")
        )

    def test_gear_never_changes_checkpoints_or_existing_boost_budget(self):
        for forbidden in (
            "Race.PassCheckpoint(", "SaveRecords(", "EarnCredits(",
            "SetActorRotation(", "SetPhysicsLinearVelocity(",
            "AddForce(", "GetWorld(", "AddTorqueInRadians(",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("ShiftInterruptionSeconds = 0.18;", self.policy)
        self.assertIn("ForwardGearCount = 4;", self.policy)
        self.assertIn("directionOpposed", self.policy)
        self.assertIn("if (shifting) propulsion *= 0.22;", self.policy)

    def test_hud_is_optional_and_labeled_estimated(self):
        section = self.hud.split(
            "const latebrake::AutomaticTransmissionState Gear =", 1
        )[1].split("// Saved solo PB is a deadline", 1)[0]
        for token in (
            "Car->GetTransmissionReadout()",
            "Gear.Valid",
            "AUTO GEAR %s | RPM EST %d%s",
            "Gear.Shifting",
        ):
            self.assertIn(token, section)
        self.assertIn("if (!bCompact && !bTouchHUD &&", self.hud)
        self.assertLess(
            self.hud.index("TIME  %02d:%05.2f"),
            self.hud.index("AUTO GEAR %s | RPM EST %d%s")
        )

    def test_strict_cpp_and_site_docs(self):
        self.assertIn(
            "AutomaticTransmissionTests.cpp",
            CMAKE.read_text(encoding="utf-8")
        )
        self.assertIn(
            "NAME AutomaticTransmission",
            CMAKE.read_text(encoding="utf-8")
        )
        self.assertIn(
            "AUTOMATIC_TRANSMISSION_QA.md",
            NAV.read_text(encoding="utf-8")
        )


if __name__ == "__main__":
    unittest.main()
