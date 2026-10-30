"""Real pawn speed-responsive camera source binding (not UE rendering QA)."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class SpeedReactiveCameraBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy = read("Source/Latebrake/Public/Vehicles/SpeedReactiveCameraFov.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_signed_real_physics_drives_local_camera_only(self):
        start = self.pawn.index("const FVector FlatVelocity(Velocity.X")
        end = self.pawn.index("// Observe the real chassis travel direction", start)
        section = self.pawn[start:end]
        self.assertIn("FVector::DotProduct(FlatVelocity, Forward)", section)
        self.assertIn("!FMath::IsFinite(LongitudinalSpeed)", section)
        self.assertLess(section.index("!FMath::IsFinite(LongitudinalSpeed)"),
                        section.index("UpdateSpeedCameraFov("))
        self.assertIn("static_cast<double>(LongitudinalSpeed) * 0.036", section)
        update = self.pawn.split("void ALatebrakeCarPawn::UpdateSpeedCameraFov(", 1)[1]
        update = update.split("void ALatebrakeCarPawn::ChangeCamera()", 1)[0]
        self.assertIn("IsLocallyControlled()", update)
        self.assertIn("Cast<APlayerController>(GetController())", update)
        self.assertIn("bHumanPlayer, LookBackCamera.IsHeld()", update)
        self.assertIn("LookBackCamera.IsHeld()", update)
        self.assertIn("FollowCamera->SetFieldOfView(", update)
        self.assertIn("ReactiveFov.Sample(", update)

    def test_pause_camera_cycle_and_recovery_discard_old_visual_state(self):
        clear = self.pawn.split("void ALatebrakeCarPawn::ClearDriverInputs()", 1)[1]
        clear = clear.split("void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0]
        self.assertIn("ResetSpeedCameraFov();", clear)
        apply = self.pawn.split("void ALatebrakeCarPawn::ApplyCameraPreset()", 1)[1]
        apply = apply.split("void ALatebrakeCarPawn::ResetSpeedCameraFov()", 1)[0]
        self.assertIn("ReactiveFov.Reset();", apply)
        self.assertIn("View.FieldOfViewDegrees", apply)
        self.assertIn("ResetSpeedCameraFov();", self.header)
        self.assertIn("SpeedReactiveCameraFov ReactiveFov;", self.header)
        self.assertIn("ResetSpeedCameraFov();", self.pawn.split(
            "if (!Chassis->IsSimulatingPhysics()", 1)[1].split(
            "const bool bArcade", 1)[0])

    def test_portable_visual_policy_is_strict_bounded_and_non_authoritative(self):
        for required in ("std::isfinite(", "std::clamp(", "std::exp(",
                         "lookingBack", "forwardSpeedKmH < 0.0",
                         "frameSeconds > 0.25", "105.0", "Reset();"):
            self.assertIn(required, self.policy)
        for forbidden in ("AddForce(", "SaveGame", "Award", "PassCheckpoint(",
                          "UCLASS(", "GameMode", "Chaos", "GetWorld()"):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("NAME SpeedReactiveCameraFov", self.cmake)
        self.assertIn("SpeedReactiveCameraFovTests.cpp", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)


if __name__ == "__main__":
    unittest.main()
