"""Source wiring checks for the player-only animated chase rig.

These source contracts do not compile UE5.8, render camera footage or
qualify a cooked/installed driving game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class ChaseCameraTransitionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header = source("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy = source("Source/Latebrake/Public/Vehicles/ChaseCameraTransition.h")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")
        cls.tests = source("Tests/Standalone/ChaseCameraTransitionTests.cpp")

    def test_policy_uses_real_spring_arm_and_no_authority(self):
        apply = self.pawn.split(
            "void ALatebrakeCarPawn::ApplyCameraPreset()", 1)[1].split(
            "void ALatebrakeCarPawn::ResetSpeedCameraFov()", 1)[0]
        for required in (
            "CameraBoom->GetRelativeRotation().Pitch",
            "CameraBoom->SocketOffset.X",
            "CameraBoom->SocketOffset.Z",
            "ChaseRigTransition.Begin(DisplayedPose, View)",
            "ChaseRigTransition.Snap(View)",
            "ChaseRigTransition.Pose()",
            "Cast<APlayerController>(GetController())",
            "bBlendNextCameraPreset = false",
            "CameraBoom->bDoCollisionTest = View.CollisionProbeEnabled",
        ):
            self.assertIn(required, apply)

        update = self.pawn.split(
            "void ALatebrakeCarPawn::UpdateCornerCameraYaw(", 1)[1].split(
            "void ALatebrakeCarPawn::ChangeCamera()", 1)[0]
        self.assertLess(update.index("if (!bHumanPlayer)"),
                        update.index("ChaseRigTransition.Advance(FrameSeconds)"))
        self.assertIn("LookBackCamera.SpringArmYawDegrees() + YawLead", update)
        for forbidden in ("AddForce(", "AddTorque(", "RegisterCheckpoint(",
                          "SaveRecords(", "Race.Advance("):
            self.assertNotIn(forbidden, apply + update)

    def test_hood_and_rear_snap_then_unqualified_frames_cancel(self):
        change = self.pawn.split(
            "void ALatebrakeCarPawn::ChangeCamera()", 1)[1].split(
            "void ALatebrakeCarPawn::LookBackPressed()", 1)[0]
        self.assertIn("Previous != latebrake::CameraView::Hood", change)
        self.assertIn("Next != latebrake::CameraView::Hood", change)
        self.assertIn("!LookBackCamera.IsHeld()", change)
        self.assertIn("ApplyCameraPreset();", change)
        reset = self.pawn.split(
            "void ALatebrakeCarPawn::ResetCornerCameraYaw()", 1)[1].split(
            "void ALatebrakeCarPawn::UpdateCornerCameraYaw(", 1)[0]
        self.assertIn("ChaseRigTransition.Cancel();", reset)
        self.assertIn("CameraBoom->TargetArmLength =", reset)
        self.assertIn("CameraBoom->SocketOffset =", reset)
        self.assertIn("ResetCornerCameraYaw();", self.pawn.split(
            "void ALatebrakeCarPawn::ClearDriverInputs()", 1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0])
        self.assertIn("ChaseCameraTransition ChaseRigTransition;", self.header)
        self.assertIn("ChaseCameraTransition.h", self.header)

    def test_compiled_strict_source_policy_and_stall_guards(self):
        for required in ("IsSafeChaseCameraPose(", "IsSafeCameraPreset(",
                         "std::isfinite(frameSeconds)",
                         "frameSeconds > MaxFrameSeconds",
                         "DurationSeconds", "Elapsed / DurationSeconds",
                         "void Cancel()", "void Snap(", "bool Begin("):
            self.assertIn(required, self.policy)
        self.assertIn("ChaseCameraTransitionTests.cpp", self.cmake)
        self.assertIn("NAME ChaseCameraTransition", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("10000", self.tests)


if __name__ == "__main__":
    unittest.main()
