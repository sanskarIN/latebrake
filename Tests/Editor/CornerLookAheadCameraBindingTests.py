"""Source-only contract for visual chase camera look-ahead in Unreal pawn.

Does not run UHT, cook a map or demonstrate installed camera motion.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class CornerLookAheadCameraBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header = source("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy = source("Source/Latebrake/Public/Vehicles/CornerLookAheadCamera.h")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")
        cls.tests = source("Tests/Standalone/CornerLookAheadCameraTests.cpp")

    def test_authoritative_chaos_sample_first_and_visual_update_only(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        samples = ("!IsRacePhysicsReady()", "GetPhysicsLinearVelocity()",
                   "SurfaceTraction.Observe({", "UpdateSpeedCameraFov(",
                   "UpdateCornerCameraYaw(", "SlipAngleMeter.Observe({")
        self.assertEqual([tick.index(v) for v in samples],
                         sorted(tick.index(v) for v in samples))
        self.assertIn("static_cast<double>(LongitudinalSpeed)", tick)
        self.assertIn("static_cast<double>(SmoothedSteering)", tick)
        impl = self.pawn.split("void ALatebrakeCarPawn::UpdateCornerCameraYaw(", 1)[1].split(
            "void ALatebrakeCarPawn::ChangeCamera()", 1)[0]
        for marker in ("Cast<APlayerController>(GetController())",
                       "CornerLookAhead.Sample(", "CameraBoom->SetRelativeRotation(",
                       "LookBackCamera.SpringArmYawDegrees() + YawLead"):
            self.assertIn(marker, impl)
        for forbidden in ("AddForce", "AddTorque", "SaveRecords", "RegisterCheckpoint"):
            self.assertNotIn(forbidden, impl)

    def test_focus_pause_and_preset_restore_static_camera(self):
        self.assertIn("CornerLookAhead.Reset();", self.pawn)
        self.assertIn("ResetCornerCameraYaw();", self.pawn)
        self.assertIn("ResetCornerCameraYaw();", self.header)
        self.assertIn("CornerLookAheadCamera.h", self.header)
        self.assertIn("CornerLookAheadCamera CornerLookAhead;", self.header)
        self.assertIn("LookBackReleased(); ResetSpeedCameraFov(); ResetCornerCameraYaw();",
                      self.header)
        reset = self.pawn.split("void ALatebrakeCarPawn::ResetCornerCameraYaw()",1)[1].split(
            "void ALatebrakeCarPawn::UpdateCornerCameraYaw(",1)[0]
        self.assertIn("CameraBoom->SetRelativeRotation(", reset)
        self.assertIn("LookBackCamera.SpringArmYawDegrees()", reset)
        self.assertIn("ResetCornerCameraYaw();", self.pawn.split(
            "void ALatebrakeCarPawn::ClearDriverInputs()",1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()",1)[0])

    def test_pure_camera_policy_bounded_source_only(self):
        for term in ("CameraView::Hood", "lookingBack", "humanDriver",
                     "std::isfinite(forwardSpeedCmPerSecond)",
                     "std::isfinite(steeringAxis)", "std::exp(", "maxYaw",
                     "std::clamp(", "void Reset()", "HasActiveSample()"):
            self.assertIn(term, self.policy)
        for forbidden in ("AddForce(", "AddTorque(", "SaveRecords(",
                          "RegisterCheckpoint("):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("CornerLookAheadCameraTests.cpp", self.cmake)
        self.assertIn("NAME CornerLookAheadCamera", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("20000", self.tests)


if __name__ == "__main__":
    unittest.main()
