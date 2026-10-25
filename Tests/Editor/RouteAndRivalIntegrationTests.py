"""Source contract for on-track route guidance and earlier rival corners.

Does not execute UHT, Chaos, a real steering wheel or an installed game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
RIVAL = ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp"
ROUTE_H = ROOT / "Source/Latebrake/Public/Track/CircuitRouteAdvisor.h"
ROUTE = ROOT / "Source/Latebrake/Private/Track/CircuitRouteAdvisor.cpp"
CORNER = ROOT / "Source/Latebrake/Private/AI/RivalCornerAnticipation.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"

class RouteAndRivalIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = HUD.read_text()
        cls.rival = RIVAL.read_text()
        cls.route_h = ROUTE_H.read_text()
        cls.route = ROUTE.read_text()
        cls.corner = CORNER.read_text()
        cls.cmake = CMAKE.read_text()

    def test_actual_hud_uses_next_scored_gate_not_ghost_estimate(self):
        for marker in ('#include "Track/CircuitRouteAdvisor.h"',
                       "latebrake::AdviseCircuitRoute(",
                       "Race.GetNextCheckpoint()",
                       "Car->GetActorRotation().Yaw",
                       "bWayfindingEligible",
                       "RouteAdvice.Valid",
                       "DIRECT TO NEXT GATE",
                       "NEXT GATE %.0fm"):
            self.assertIn(marker, self.hud)
        self.assertIn("IsOnPavedSurface(position)", self.route)
        self.assertIn("!IsCircuitGate(nextGate)", self.route)
        self.assertIn("std::remainder(", self.route)
        self.assertIn("enum class RouteHint", self.route_h)
        self.assertNotIn("PassCheckpoint(", self.route)

    def test_actual_rival_uses_existing_pilot_before_corner_brake(self):
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        for marker in ('#include "AI/RivalCornerAnticipation.h"',
                       "latebrake::AnticipateRivalCorner(",
                       "latebrake::PlanRivalTurn(",
                       "Route.GetNextWaypoint()",
                       "SetAutonomousControls("):
            self.assertIn(marker, self.rival)
        self.assertLess(tick.index("latebrake::PlanRivalTurn("),
                        tick.index("SetAutonomousControls(static_cast<float>(Command.Throttle)"))
        self.assertIn("commanded.Throttle = 0.0;", self.corner)
        self.assertIn("commanded.Brake = true;", self.corner)
        self.assertIn("if (commanded.Arrived || commanded.Brake)", self.corner)
        self.assertIn("!IsCircuitGate(nextGate)", self.corner)
        self.assertNotIn("Route.ReportRecoveryResult", self.corner)

    def test_cpp_targets_compile_real_policy_units(self):
        for marker in ("CircuitRouteAdvisorTests.cpp",
                       "Private/Track/CircuitRouteAdvisor.cpp",
                       "NAME CircuitRouteAdvisor",
                       "RivalCornerAnticipationTests.cpp",
                       "Private/AI/RivalCornerAnticipation.cpp",
                       "NAME RivalCornerAnticipation"):
            self.assertIn(marker, self.cmake)

if __name__ == "__main__":
    unittest.main()
