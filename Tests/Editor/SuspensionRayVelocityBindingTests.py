"""Real source wiring for static-road ray-length damping; no Unreal runtime."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class SuspensionRayVelocityBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(
            encoding="utf-8")
        cls.tick = cls.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        cls.policy = (ROOT / "Source/Latebrake/Public/Vehicles/SprungChassisDynamics.h").read_text(
            encoding="utf-8")

    def test_same_static_surface_normal_and_true_corner_velocity_drive_damper(self):
        stages = (
            "World->LineTraceSingleByChannel(",
            "Hit.bBlockingHit && !Hit.bStartPenetrating",
            "if (bGroundHit)",
            "GetPhysicsLinearVelocityAtPoint(Origin)",
            "latebrake::MeasureSuspensionRayVelocity({",
            "if (!RayVelocity.Valid)",
            "SpringSample.Corners[static_cast<unsigned>(Index)] =",
            "latebrake::CalculateSprungChassis(SpringSample)",
            "if (!SpringResponse.Valid)",
            "ApplySprungForces(SpringResponse, WheelWorldAnchors);",
        )
        self.assertEqual([self.tick.index(s) for s in stages],
                         sorted(self.tick.index(s) for s in stages))
        sampler = self.tick.split("MeasureSuspensionRayVelocity({", 1)[1].split("});", 1)[0]
        for term in ("static_cast<double>(PointVelocity.X)",
                     "static_cast<double>(PointVelocity.Y)",
                     "static_cast<double>(PointVelocity.Z)",
                     "static_cast<double>(Hit.ImpactNormal.X)",
                     "static_cast<double>(Hit.ImpactNormal.Y), NormalZ"):
            self.assertIn(term, sampler)
        corner = self.tick.split("SpringSample.Corners[static_cast<unsigned>(Index)] =", 1)[1].split(
            "};", 1)[0]
        self.assertIn("true, Distance, NormalZ", corner)
        self.assertIn("RayVelocity.DistanceSpeedCmPerSecond", corner)
        self.assertNotIn("PointVelocity.Z", corner)
        self.assertEqual(self.tick.count("LineTraceSingleByChannel("), 1)
        self.assertEqual(self.tick.count("MeasureSuspensionRayVelocity({"), 1)

    def test_corrupt_sensor_rejects_entire_spring_frame_before_force(self):
        invalid = self.tick.split("if (!RayVelocity.Valid)", 1)[1].split(
            "GroundSample.Probes", 1)[0]
        self.assertIn("SpringSample.PhysicsReady = false;", invalid)
        self.assertNotIn("ApplySprungForces", invalid)
        self.assertNotIn("continue;", invalid)
        self.assertIn("if (!frame.PhysicsReady ||", self.policy)
        self.assertEqual(self.tick.count("ApplySprungForces(SpringResponse, WheelWorldAnchors);"), 1)

    def test_spring_and_anti_roll_both_use_distance_rate(self):
        self.assertIn("damping * observed.RayDistanceSpeedCmPerSecond", self.policy)
        self.assertIn("ro.RayDistanceSpeedCmPerSecond -", self.policy)
        self.assertIn("lo.RayDistanceSpeedCmPerSecond", self.policy)
        self.assertIn("SuspensionMaximumDistanceSpeedCmPerSecond = 15000.0", self.policy)
        self.assertNotIn("PointVerticalSpeedCmPerSecond", self.policy)

    def test_shared_bounds_strict_build_and_no_extra_authority(self):
        velocity = (ROOT / "Source/Latebrake/Public/Vehicles/SuspensionRayVelocity.h").read_text(
            encoding="utf-8")
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
        for term in ("s.NormalZ < SuspensionMinNormalZ",
                     "SuspensionMaximumDistanceSpeedCmPerSecond",
                     "s.NormalX * s.PointVelocityX",
                     "s.NormalY * s.PointVelocityY",
                     "s.NormalZ * s.PointVelocityZ) / s.NormalZ"):
            self.assertIn(term, velocity)
        self.assertIn("SuspensionRayVelocityTests.cpp", cmake)
        self.assertIn("NAME SuspensionRayVelocity", cmake)
        self.assertIn("latebrake_suspension_ray_velocity_tests PRIVATE /W4 /WX", cmake)
        for forbidden in ("GetWorld(", "AddForce(", "SetPhysicsLinearVelocity(",
                          "SaveGame(", "PassCheckpoint(", "AwardCredits("):
            self.assertNotIn(forbidden, velocity)


if __name__ == "__main__":
    unittest.main()
