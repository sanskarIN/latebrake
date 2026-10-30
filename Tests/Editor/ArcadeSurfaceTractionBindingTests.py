"""Source integration contracts only; not Unreal Editor/UHT/runtime QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class ArcadeSurfaceTractionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Vehicles/ArcadeSurfaceTraction.h")
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.guide = read("docs/ARCADE_SURFACE_TRACTION_QA.md")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")

    def test_shared_real_chassis_location_and_fail_closed_force_order(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        markers = (
            "!IsRacePhysicsReady()",
            "const FVector Velocity = Chassis->GetPhysicsLinearVelocity()",
            "if (!FMath::IsFinite(Speed)",
            "const FVector SurfacePosition = GetActorLocation()",
            "SurfaceTraction.Observe({",
            "if (!Surface.Valid)",
            "NeutralizeMotionWhenDrivingBlocked();",
            "TireCondition.Observe({",
            "TireCondition.GripMultiplier() * Surface.LateralGripMultiplier",
            "FrameStableDampingRate(GripRate, DeltaSeconds)",
            "Chassis->AddForce(-Right * LateralSpeed * Mass * StableGripRate)",
        )
        self.assertTrue(all(marker in tick for marker in markers))
        positions = [tick.index(x) if x != "NeutralizeMotionWhenDrivingBlocked();"
                     else tick.index(x, tick.index("if (!Surface.Valid)"))
                     for x in markers]
        self.assertEqual(positions, sorted(positions))

    def test_normal_motor_and_both_bonuses_use_same_surface_scale(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        self.assertEqual(tick.count("static_cast<float>(Surface.MotorMultiplier)"), 3)
        self.assertIn("static_cast<float>(CombinedBoost.NitroExtra) *", tick)
        self.assertIn("static_cast<float>(DraftExtraForce) *", tick)
        self.assertLess(tick.index("SurfaceTraction.Observe({"),
                        tick.index("Chassis->AddForce(Forward * DriveForce"))
        self.assertIn("latebrake::AllocateArcadeBoost(", tick)
        self.assertIn("IsLocallyControlled()", tick)

    def test_race_restart_pause_focus_and_teleport_clear_surface_history(self):
        self.assertIn('#include "Vehicles/ArcadeSurfaceTraction.h"', self.pawn_h)
        self.assertIn("latebrake::ArcadeSurfaceTraction SurfaceTraction;", self.pawn_h)
        self.assertIn("SurfaceTraction.Reset();", self.pawn_h)
        self.assertIn("GetSurfaceTraction() const", self.pawn_h)
        clear = self.pawn.split("void ALatebrakeCarPawn::ClearDriverInputs()", 1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1
        )[0]
        self.assertIn("SurfaceTraction.Suspend();", clear)
        self.assertIn("SurfaceTraction.Suspend();", self.pawn)
        self.assertIn("ClearDriverInputs();", self.pawn)
        self.assertIn("bool ALatebrakeCarPawn::TryRecoverTo(", self.pawn)
        self.assertIn("OFF-PAVEMENT TRACTION", self.hud)
        self.assertIn("GetSurfaceTraction().IsOffPavement()", self.hud)
        self.assertIn("VerticalPlan.MaxTelemetryLines >= 28", self.hud)

    def test_paved_geometry_and_arbitrary_motor_boosts_not_in_policy(self):
        for marker in ("#include \"Track/CircuitLayout.h\"",
                       "IsOnPavedSurface(", "MaximumFrameSeconds = 0.10",
                       "MaximumWorldCoordinate = 25000.0",
                       "OffPavementConfirmSeconds = 0.15",
                       "OnPavementConfirmSeconds = 0.20",
                       "OffPavementMotorMultiplier = 0.78",
                       "OffPavementGripMultiplier = 0.72",
                       "std::isfinite", "Suspend();", "return {};"):
            self.assertIn(marker, self.policy)
        for forbidden in ("AddForce(", "PassCheckpoint(", "SaveGame",
                          "Credits", "Award", "UCLASS", "GetWorld("):
            self.assertNotIn(forbidden, self.policy)

    def test_ci_and_on_screen_advisory_are_source_only(self):
        self.assertIn("ArcadeSurfaceTractionTests.cpp", self.cmake)
        self.assertIn("NAME ArcadeSurfaceTraction", self.cmake)
        self.assertIn("latebrake_arcade_surface_traction_tests PRIVATE /W4 /WX", self.cmake)
        self.assertIn("OffTrackHint.Update(", self.hud)
        self.assertIn("Unreal Engine 5.8", self.guide)
        self.assertIn("not playable", self.guide.lower())


if __name__ == "__main__":
    unittest.main()
