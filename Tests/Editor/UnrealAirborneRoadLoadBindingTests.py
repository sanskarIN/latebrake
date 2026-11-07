"""Actual 4-corner spring load must gate rolling resistance, not aero drag.

Source binding only: no licensed Unreal Engine/UHT or packaged Windows QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
POLICY = (ROOT / "Source/Latebrake/Public/Vehicles/ProgressiveRoadLoad.h").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
STRESS = (ROOT / "Tests/Standalone/AirborneRoadLoadTests.cpp").read_text(encoding="utf-8")
QA = (ROOT / "docs/PROGRESSIVE_ROAD_LOAD_QA.md").read_text(encoding="utf-8")


class UnrealAirborneRoadLoadBindingTests(unittest.TestCase):
    def test_same_measured_spring_frame_controls_rolling_force(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(",1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(",1)[0]
        measured = (
            "const double SprungRoadLoadFraction = FMath::Clamp(\n"
            "        SpringResponse.TotalUpForce / SpringResponse.WeightForce, 0.0, 1.0);"
        )
        self.assertEqual(tick.count(measured), 1)
        self.assertIn("bArcade, Surface.OffPavement, SprungRoadLoadFraction", tick)
        self.assertLess(tick.index("latebrake::CalculateSprungChassis(SpringSample)"),
                        tick.index(measured))
        self.assertLess(tick.index(measured),
                        tick.index("latebrake::CalculateProgressiveRoadLoad({"))
        self.assertLess(tick.index("latebrake::CalculateProgressiveRoadLoad({"),
                        tick.index("Chassis->AddForce(FVector("))
        local = tick.split("const double SprungRoadLoadFraction =",1)[1].split(
            "if (!RoadLoad.Valid)",1)[0]
        self.assertNotIn("LineTraceSingle", local)
        self.assertNotIn("GetActorUpVector()", local)
        self.assertIn("CalculateAxleTractionBudget(GroundContact, SpringResponse)",tick)

    def test_airborne_keeps_aero_while_roll_and_grass_fade(self):
        for required in (
            "double SprungLoadFraction = 1.0;",
            "!std::isfinite(sample.SprungLoadFraction)",
            "sample.SprungLoadFraction < 0.0",
            "sample.SprungLoadFraction > 1.0",
            "const double support = sample.SprungLoadFraction;",
            "(RoadLoadOffPavementResistance - 1.0) * support",
            "quadratic * speed * speed + rolling * rollingRamp * support",
        ):
            self.assertIn(required,POLICY)
        self.assertNotIn("Chassis->SetPhysicsLinearVelocity", POLICY)
        for forbidden in ("AddForce(", "SaveGame(", "PassCheckpoint(", "AwardCredits("):
            self.assertNotIn(forbidden,POLICY)
        self.assertIn("NAME AirborneRoadLoad",CMAKE)
        self.assertIn("120000",STRESS)
        self.assertIn("roof-down body",STRESS)
        self.assertIn("Contact-gated road friction",QA)


if __name__ == "__main__":
    unittest.main()
