"""Source-only integration checks for real pit hold blocker explanations.

These tests validate Unreal source wiring but DO NOT compile UE 5.8 or
demonstrate that the installed Windows/Android HUD renders correctly.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealPitGuidanceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Racing/RacePitService.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_real_observation_produces_status_and_does_not_grant_service(self):
        self.assertIn("PitServiceGuidance DiagnosePitService(", self.policy)
        self.assertIn("LastGuidance = DiagnosePitService(sample, Armed);", self.policy)
        self.assertIn("LastGuidance = PitServiceGuidance::Completed;", self.policy)
        self.assertIn("PitServiceGuidance Guidance() const noexcept", self.policy)
        self.assertIn("sample.PlanarSpeedCmPerSecond > 120.0", self.policy)
        self.assertIn("std::abs(sample.RequestedThrottle) > 0.05", self.policy)
        self.assertIn("sample.NitroHeld", self.policy)
        self.assertIn("sample.ButtonHeld", self.policy)
        self.assertIn("sample.StableGround", self.policy)
        self.assertIn("PitServiceGuidance::SettleChassis", self.policy)
        self.assertIn("if (!sample.MaintenanceNeeded)", self.policy)
        self.assertIn("if (!armed) return PitServiceGuidance::ExitToRearm;", self.policy)
        self.assertIn("void Suspend() noexcept", self.policy)
        self.assertIn("LastGuidance = PitServiceGuidance::Unavailable;", self.policy)
        diagnostic = self.policy.split("DiagnosePitService(", 1)[1].split(
            "// A single service is earned", 1)[0]
        for forbidden in ("Armed = ", "Stops = ", "HeldSeconds = ",
                          "SaveRecords(", "EarnCredits(", "RepairAtPit()"):
            self.assertNotIn(forbidden, diagnostic)

    def test_desktop_and_android_display_real_service_status(self):
        desktop = self.hud.split(
            "// An optional desktop pit notice", 1)[1].split(
            "// The independent off-track direction hint", 1)[0]
        for requirement in (
            "Pit.Guidance()", "PitServiceGuidance::SlowDown",
            "PitServiceGuidance::ReleaseThrottle",
            "PitServiceGuidance::ReleaseNitro",
            "PitServiceGuidance::NoMaintenance",
            "PitServiceGuidance::SettleChassis",
            "PitServiceGuidance::Servicing",
            "Pit.ProgressFraction()", "PIT SERVICED",
            "PIT: SLOW TO 4.3 KM/H OR LESS",
        ):
            self.assertIn(requirement, desktop)
        mobile = self.hud.split(
            "const latebrake::RacePitService& MobilePit", 1)[1].split(
            "DrawPad(Pads.CareerNext", 1)[0]
        self.assertIn("MobilePit.Guidance();", mobile)
        self.assertIn("MobilePit.ProgressFraction() * 100.0", mobile)
        self.assertIn("DrawPad(Pads.PitService, *PitPadLabel,", mobile)
        self.assertIn('TEXT("PIT SLOW")', mobile)
        self.assertIn('TEXT("LIFT GAS")', mobile)
        self.assertIn('TEXT("NO NITRO")', mobile)
        self.assertIn('TEXT("SETTLE CAR")', mobile)
        for code in ("RepairAtPit(", "ReplaceAtPit(", "RefillAtPit(",
                     "SaveRecords(", "EarnCredits(", "AddForce(", "TeleportTo("):
            self.assertNotIn(code, desktop)
            self.assertNotIn(code, mobile)

    def test_ground_stability_gate_is_real_unreal_input_not_hud_guess(self):
        pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        tick = pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1]
        observe = tick.split("if (PitService.Observe({", 1)[1].split(
            "CollisionDamage.RepairAtPit();", 1)[0]
        self.assertIn("Mode->HasLocalPlayerDriver(),", observe)
        self.assertIn("bPitNeedsService,", observe)
        self.assertIn("latebrake::HasStablePitGroundSupport(", observe)
        self.assertIn("GroundContact, static_cast<double>(Velocity.Z)", observe)
        self.assertIn("!sample.MaintenanceNeeded || !sample.StableGround", self.policy)
        self.assertLess(tick.index("EvaluateChassisGroundContact("),
                        tick.index("HasStablePitGroundSupport("))
        self.assertLess(tick.index("HasStablePitGroundSupport("),
                        tick.index("CollisionDamage.RepairAtPit();"))

    def test_new_diagnostic_is_mandatory_standalone_target(self):
        self.assertIn("RacePitGuidanceTests.cpp", self.cmake)
        self.assertIn("add_test(NAME RacePitGuidance", self.cmake)


if __name__ == "__main__":
    unittest.main()
