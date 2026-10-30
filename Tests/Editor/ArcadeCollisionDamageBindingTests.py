"""Shared pawn collision response contracts, not an Unreal physics test."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h").read_text(encoding="utf-8")
PAWN = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
HUD = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")


class ArcadeCollisionDamageBindingTests(unittest.TestCase):
    def test_real_chaos_hit_is_wired(self):
        self.assertIn("SetNotifyRigidBodyCollision(true)", PAWN)
        self.assertIn("OnComponentHit.AddDynamic(this,", PAWN)
        self.assertIn("&ALatebrakeCarPawn::HandleChassisHit", PAWN)
        self.assertIn("UFUNCTION()", HEADER)
        self.assertIn("void HandleChassisHit(", HEADER)
        callback = PAWN.split("void ALatebrakeCarPawn::HandleChassisHit(", 1)[1].split(
            "void ALatebrakeCarPawn::", 1)[0]
        for required in ("IsRacePhysicsReady()", "IsRaceStartGreen()",
                         "IsCircuitReadyForGameplay()", "GetPhysicsLinearVelocity()",
                         "GetMass()", "Hit.ImpactNormal.Z",
                         "CollisionDamage.ObserveImpact(",
                         "NormalImpulse.Size()"):
            self.assertIn(required, callback)
        self.assertIn("OtherActor == this", callback)

    def test_damage_scales_all_three_acceleration_paths(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertIn("CollisionDamage.Advance(", tick)
        self.assertIn("CollisionDamage.MotorMultiplier()", tick)
        self.assertIn("CollisionDamage.LateralGripMultiplier()", tick)
        self.assertIn("const float CombinedMotorScale =", tick)
        self.assertEqual(tick.count("static_cast<float>(CombinedMotorScale)"), 0)
        self.assertEqual(tick.count("CombinedMotorScale);"), 3)
        self.assertIn("CollisionDamage.HasEligibleFrame()", tick)

    def test_state_lifecycle_and_hud(self):
        self.assertIn("ArcadeCollisionDamage CollisionDamage;", HEADER)
        self.assertIn("CollisionDamage.Reset();", HEADER)
        self.assertIn("CollisionDamage.Suspend();", PAWN)
        self.assertIn("GetCollisionDamage()", HEADER)
        self.assertIn("Car->GetCollisionDamage().DamageFraction()", HUD)
        self.assertIn("IMPACT DAMAGE", HUD)
        self.assertIn("!bCompact", HUD)
        self.assertIn("ArcadeCollisionDamageTests.cpp", CMAKE)
        self.assertIn("NAME ArcadeCollisionDamage", CMAKE)


if __name__ == "__main__":
    unittest.main()
