"""Prevent original Unreal rival traffic braking from steering through an aborted pass.

Static source checks only, not a UE5.8 build or installed-game test.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = (ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp").read_text(encoding="utf-8")
PASS = (ROOT / "Source/Latebrake/Public/AI/RivalStraightPassAssist.h").read_text(encoding="utf-8")
RESOLVE = (ROOT / "Source/Latebrake/Public/AI/RivalPassTrafficResolution.h").read_text(encoding="utf-8")
TRAFFIC = (ROOT / "Source/Latebrake/Public/AI/RivalTrafficAwareness.h").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
NAV = (ROOT / "mkdocs.yml").read_text(encoding="utf-8")


class UnrealRivalPassTrafficResolutionBindingTests(unittest.TestCase):
    def test_actual_physical_ai_applies_traffic_and_hold_before_reconciling(self):
        for fragment in (
            '#include "AI/RivalPassTrafficResolution.h"',
            "StraightPassAssist.Apply(Command, {",
            "UGameplayStatics::GetPlayerPawn(this, 0)",
            "Player->GetVelocity()",
            "const latebrake::PilotCommand SafePassCommand = Passing.Command;",
            "latebrake::YieldRivalToPlayer(SafePassCommand, {",
            "const latebrake::PilotCommand HeldTrafficCommand =",
            "FollowingBrakeHold.Apply(",
            "latebrake::ResolveRivalPassTraffic(",
            "Command, Passing, HeldTrafficCommand",
            "if (PassResolution.AbortPassing)",
            "StraightPassAssist.Reset();",
            "const latebrake::PilotCommand FollowingCommand = PassResolution.Command;",
            "latebrake::ModerateRivalYawRate(FollowingCommand, {",
            "SpinThrottleGuard.Apply(",
            "SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)",
        ):
            self.assertIn(fragment, PAWN)
        entry = PAWN.split("const latebrake::RivalPassOutput Passing =", 1)[1]
        order = (
            "StraightPassAssist.Apply(Command, {",
            "YieldRivalToPlayer(SafePassCommand, {",
            "FollowingBrakeHold.Apply(",
            "latebrake::ResolveRivalPassTraffic(",
            "const latebrake::PilotCommand FollowingCommand = PassResolution.Command;",
            "latebrake::ModerateRivalYawRate(FollowingCommand, {",
            "SpinThrottleGuard.Apply(",
            "SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)",
        )
        self.assertEqual([entry.index(x) for x in order],
                         sorted(entry.index(x) for x in order))

    def test_traffic_emergency_is_never_downgraded_into_lateral_pass(self):
        for marker in (
            "if (pass.Passing && (trafficAndHold.Brake ||",
            "trafficAndHold.Arrived))",
            "trafficAndHold.Throttle = 0.0;",
            "trafficAndHold.Steering = plannedRoute.Steering;",
            "return {trafficAndHold, true};",
            "std::isfinite(p.Throttle)",
            "std::isfinite(p.Steering)",
            "return {{}, true};",
        ):
            self.assertIn(marker, RESOLVE)
        self.assertIn("ConservativeContactClearanceCm = 380.0", TRAFFIC)
        self.assertIn("timeToContact <= CriticalTimeToContactSeconds", TRAFFIC)

    def test_pass_starts_early_enough_and_only_on_long_real_roads(self):
        for marker in (
            "StartAheadMinimumCm = 1600.0",
            "StartAheadMaximumCm = 2400.0",
            "MinimumPassingStraightLengthCm = 5000.0",
            "MinimumEntryCornerRunwayCm = 4200.0",
            "MinimumEntryContactTimeSeconds = 1.8",
            "MaximumEntryClosingSpeedCmPerSecond = 900.0",
            "emergencyRunway / closingSpeed",
            "length < MinimumPassingStraightLengthCm",
            "toGate < MinimumEntryCornerRunwayCm",
            "playerAhead > StartAheadMaximumCm + 400.0",
        ):
            self.assertIn(marker, PASS)
        self.assertIn("MinimumCornerClearanceCm = 2150.0", PASS)
        self.assertIn("IsOnPavedSurface(s.Player)", PASS)

    def test_release_gate_and_scoring_authority_remain_closed(self):
        for src in (PASS, RESOLVE):
            for forbidden in (
                "SetActorLocation(", "SetPhysicsLinearVelocity(",
                "AddForce(", "PassCheckpoint(", "SaveGame",
                "AwardCredits(", "EarnCredits(", "GetWorld(", "SpawnActor(",
            ):
                self.assertNotIn(forbidden, src)
        self.assertIn("RivalPassTrafficResolutionTests.cpp", CMAKE)
        self.assertIn("NAME RivalPassTrafficResolution", CMAKE)
        self.assertIn("RIVAL_PASS_TRAFFIC_QA.md", NAV)


if __name__ == "__main__":
    unittest.main()
