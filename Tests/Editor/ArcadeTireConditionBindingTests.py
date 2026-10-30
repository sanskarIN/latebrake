"""Portable source-contract checks; not an Unreal Editor/UHT build."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class ArcadeTireConditionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Vehicles/ArcadeTireCondition.h")
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.guide = read("docs/ARCADE_TIRE_CONDITION_QA.md")

    def test_physics_sampling_and_bounded_existing_grip(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        for term in (
            "!IsRacePhysicsReady()",
            "const FVector Velocity = Chassis->GetPhysicsLinearVelocity()",
            "const float Speed = FlatVelocity.Size()",
            "const float LateralSpeed = FVector::DotProduct(FlatVelocity, Right)",
            "if (!FMath::IsFinite(Speed)",
            "TireCondition.Observe({",
            "static_cast<double>(Speed)",
            "static_cast<double>(LateralSpeed), bEffectiveBraking, true",
            "TireCondition.GripMultiplier()",
            "latebrake::FrameStableDampingRate(",
            "Chassis->AddForce(-Right * LateralSpeed * Mass * StableGripRate)",
        ):
            self.assertIn(term, tick)
        self.assertLess(tick.index("!IsRacePhysicsReady()"),
                        tick.index("TireCondition.Observe({"))
        self.assertLess(tick.index("TireCondition.Observe({"),
                        tick.index("Chassis->AddForce(-Right"))
        self.assertLess(tick.index("TireCondition.GripMultiplier()"),
                        tick.index("FrameStableDampingRate(GripRate, DeltaSeconds)"))
        self.assertIn("TireCondition.Suspend();", tick)

    def test_per_race_reset_and_both_driver_types(self):
        self.assertIn('#include "Vehicles/ArcadeTireCondition.h"', self.pawn_h)
        self.assertIn("latebrake::ArcadeTireCondition TireCondition;", self.pawn_h)
        self.assertIn("GetTireCondition() const", self.pawn_h)
        self.assertIn("TireCondition.Reset();", self.pawn_h)
        clear = self.pawn.split("void ALatebrakeCarPawn::ClearDriverInputs()", 1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0]
        self.assertIn("TireCondition.Suspend();", clear)
        self.assertIn("Car->ResetDrivingStats();", self.mode)
        self.assertIn("RivalCar->ResetDrivingStats();", self.mode)
        self.assertLess(self.mode.index("RivalCar->ResetDrivingStats();"),
                        self.mode.index("RivalCar->ConfigurePilot("))
        self.assertIn("RivalCar->ResetPilot();", self.mode)
        self.assertNotIn("IsLocallyControlled()", self.policy)

    def test_existing_authority_not_mutated(self):
        for forbidden in (
            "AddForce(", "SaveGame", "RaceProgress", "AwardCredits(",
            "PassCheckpoint(", "UCLASS(", "UE_LOG(", "Credits",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("MaxFrameSeconds = 0.10", self.policy)
        self.assertIn("MinimumGripMultiplier = 0.68", self.policy)
        self.assertIn("std::isfinite", self.policy)
        self.assertIn("std::clamp", self.policy)

    def test_hud_and_ci_are_explicitly_source_only(self):
        self.assertIn("Car->GetTireCondition().HasValidSample()", self.hud)
        self.assertIn("ARCADE GRIP", self.hud)
        self.assertIn("TIRE WEAR", self.hud)
        self.assertIn("!bCompact", self.hud)
        self.assertIn("ArcadeTireConditionTests.cpp", self.cmake)
        self.assertIn("NAME ArcadeTireCondition", self.cmake)
        self.assertIn("Unreal Engine 5.8", self.guide)
        self.assertIn("not playable", self.guide.lower())


if __name__ == "__main__":
    unittest.main()
