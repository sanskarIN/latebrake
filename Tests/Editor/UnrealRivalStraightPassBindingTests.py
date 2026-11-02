"""Source contracts for the actual single AI car's bounded passing lane.

These do not execute Chaos physics, verify collision avoidance, or prove a
cooked, installed or physically tested UE 5.8 racing game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
RIVAL_H = (ROOT / "Source/Latebrake/Public/AI/LatebrakeRivalCar.h").read_text(encoding="utf-8")
RIVAL = (ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp").read_text(encoding="utf-8")
POLICY = (ROOT / "Source/Latebrake/Public/AI/RivalStraightPassAssist.h").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
DOCS = (ROOT / "docs/RIVAL_STRAIGHT_PASS_QA.md").read_text(encoding="utf-8")
NAV = (ROOT / "mkdocs.yml").read_text(encoding="utf-8")


class UnrealRivalStraightPassBindingTests(unittest.TestCase):
    def test_shared_actor_uses_only_real_player_and_route(self):
        for marker in (
            '#include "AI/RivalStraightPassAssist.h"',
            "latebrake::RivalStraightPassAssist StraightPassAssist;",
        ):
            self.assertIn(marker, RIVAL_H)
        tick = RIVAL.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        for marker in (
            "Mode->IsRaceStartGreen()",
            "IsRacePhysicsReady()",
            "Route.ObserveVerifiedGate(",
            "RivalGateTrace.Sample(",
            "const latebrake::CircuitPoint NextTarget =",
            "latebrake::PlanRivalTurn(",
            "UGameplayStatics::GetPlayerPawn(this, 0)",
            "IsValid(Player) && Player->IsRacePhysicsReady()",
            "Player->GetActorLocation()",
            "Player->GetVelocity()",
            "StraightPassAssist.Apply(Command, {",
            "GetSurfaceTraction().HasValidSample()",
            "GetTireCondition().HasValidSample()",
            "GetCollisionDamage().HasEligibleFrame()",
            "!GetSurfaceTraction().IsOffPavement()",
            "const latebrake::PilotCommand SafePassCommand = Passing.Command;",
            "YieldRivalToPlayer(SafePassCommand, {",
            "FollowingBrakeHold.Apply(",
            "TrafficCommand.Brake && !SafePassCommand.Brake",
            "ModerateRivalYawRate(FollowingCommand, {",
            "SpinThrottleGuard.Apply(SafeTrafficCommand, {",
            "SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)",
        ):
            self.assertIn(marker, tick)
        for prev, after in (
            ("Route.ObserveVerifiedGate(", "latebrake::PlanRivalTurn("),
            ("latebrake::PlanRivalTurn(", "StraightPassAssist.Apply(Command, {"),
            ("Player->GetVelocity()", "StraightPassAssist.Apply(Command, {"),
            ("StraightPassAssist.Apply(Command, {",
             "YieldRivalToPlayer(SafePassCommand, {"),
            ("YieldRivalToPlayer(SafePassCommand, {",
             "FollowingBrakeHold.Apply("),
            ("FollowingBrakeHold.Apply(", "ModerateRivalYawRate("),
        ):
            self.assertLess(tick.index(prev), tick.index(after))

    def test_interruption_restart_recovery_and_finish_clear_pass(self):
        for marker in (
            "void ALatebrakeRivalCar::ResetPilot()",
            "void ALatebrakeRivalCar::ConfigurePilot(",
            "if (!Mode || !Mode->IsRaceStartGreen()",
            "if (!latebrake::IsCircuitGate(Next))",
            "if (Observation.NeedsRecovery)",
            "if (Observation.Completed)",
        ):
            self.assertIn(marker, RIVAL)
        self.assertGreaterEqual(RIVAL.count("StraightPassAssist.Reset();"), 6)
        self.assertIn("ActiveGate = -1;", POLICY)
        self.assertIn("if (ActiveGate != -1 && ActiveGate != s.NextGate)", POLICY)

    def test_no_passing_off_track_near_corners_or_into_player(self):
        for marker in (
            "LaneOffsetCm = 440.0",
            "MinimumCornerClearanceCm = 2150.0",
            "StartAheadMinimumCm = 1600.0",
            "StartAheadMaximumCm = 2400.0",
            "FinishBehindClearanceCm = 550.0",
            "MaximumPassDurationSeconds = 6.0",
            "IsOnPavedSurface(s.Rival)",
            "IsOnPavedSurface(s.Player)",
            "IsOnPavedSurface(aim)",
            "playerSide - selectedLane",
            "command.Throttle = 0.0;",
            "command.Brake = true;",
            "std::clamp(error / 0.82, -0.65, 0.65)",
            "std::isfinite(s.RivalHeadingRadians)",
            "std::isfinite(s.PlayerVelocityX)",
            "std::isfinite(command.Steering)",
            "return {{}, false, 0.0}",
        ):
            self.assertIn(marker, POLICY)

    def test_no_progress_award_game_teleport_or_direct_force_mutation(self):
        for forbidden in (
            "PassCheckpoint(", "ObserveVerifiedGate(", "AwardCredits(",
            "SaveGame", "SetPhysicsLinearVelocity(", "AddForce(",
            "SetActorLocation(", "UCLASS(", "GetWorld(", "SpawnActor(",
        ):
            self.assertNotIn(forbidden, POLICY)
        self.assertIn("not a playable", DOCS.lower())
        self.assertIn("RivalStraightPassAssistTests.cpp", CMAKE)
        self.assertIn("NAME RivalStraightPassAssist", CMAKE)
        self.assertIn("RIVAL_STRAIGHT_PASS_QA.md", NAV)


if __name__ == "__main__":
    unittest.main()
