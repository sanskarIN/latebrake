import pathlib
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[2]
P=ROOT / "Source/Latebrake/Private/Vehicles"
H=ROOT / "Source/Latebrake/Public/Vehicles"

class OriginalChaosSuspensionPreviewBindingTests(unittest.TestCase):
    def test_four_real_corner_traces_and_force_authority(self):
        text=(P/"OriginalChaosSuspensionPreview.cpp").read_text(encoding="utf-8")
        self.assertIn("LineTraceSingleByChannel(",text)
        self.assertIn("ECC_WorldStatic",text)
        self.assertIn("GetCollisionObjectType()",text)
        self.assertIn("EComponentMobility::Static",text)
        self.assertIn("ChassisProbeOffset(",text)
        self.assertIn("GetPhysicsLinearVelocityAtPoint(",text)
        self.assertIn("MeasureSuspensionRayVelocity({",text)
        self.assertIn("CalculateSprungChassis(frame)",text)
        self.assertIn("AddForceAtLocation(",text)
        self.assertIn("receipt.PhysicalRaysCast!=4",text)
        self.assertIn("ForcesSubmitted",text)
        self.assertNotIn("SetPhysicsLinearVelocity(",text)
        self.assertNotIn("SetSimulatePhysics(false)",text)
        self.assertNotIn("Race.Credit",text)

    def test_native_experiment_off_by_default(self):
        pawn=(P/"LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
        self.assertIn("LatebrakeExperimentalSprungChassis",pawn)
        self.assertIn("ApplyOriginalChaosSprungPreview(",pawn)
        self.assertIn("#if !UE_BUILD_SHIPPING",pawn)
        self.assertIn("IsRacePhysicsReady()",pawn)
        self.assertIn("CanApplyPlayerDrive",pawn)
        self.assertIn("NeutralizeMotionWhenDrivingBlocked()",pawn)
        self.assertIn("NoChaosWheelSolver",pawn)

    def test_source_receipt_does_not_claim_wheel_runtime_qa(self):
        header=(H/"OriginalChaosSuspensionPreview.h").read_text(encoding="utf-8")
        self.assertIn("OriginalChaosChassisUsed",header)
        self.assertIn("WheelChaosIntegrationValidated=false",header)
        self.assertIn("NativeInstalledRaceValidated=false",header)
        self.assertIn("MissingCornerSamples",header)

if __name__=="__main__":
    unittest.main()
