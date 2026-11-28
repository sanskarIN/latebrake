"""Source-only wiring contracts for read-only sampled driving coaching.

A green result does not qualify Unreal, a native vehicle or gameplay feel.
The portable test executable validates the actual finite state machine.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class DrivingCoachSourceBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.stats = source("Source/Latebrake/Private/Racing/DrivingSessionStats.cpp")
        cls.stats_header = source("Source/Latebrake/Public/Racing/DrivingSessionStats.h")
        cls.coach = source("Source/Latebrake/Public/Racing/DrivingCoachInsights.h")
        cls.pawn = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.pawn_header = source("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud = source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.build = source("Tests/Standalone/CMakeLists.txt")
        cls.test = source("Tests/Standalone/DrivingCoachInsightsTests.cpp")

    def test_existing_measured_chassis_sample_is_the_only_authority(self):
        self.assertIn("DrivingStats.Observe({static_cast<double>(DeltaSeconds)", self.pawn)
        self.assertIn("static_cast<double>(Speed), static_cast<double>(LateralSpeed)", self.pawn)
        self.assertIn("bEffectiveBraking", self.pawn)
        self.assertIn("DrivingCoachAccumulator Coach;", self.stats_header)
        self.assertIn("Coach.Observe({sample.DeltaSeconds", self.stats)
        self.assertEqual(len(re.findall(r"Coach\.Observe\(", self.stats)), 1)
        self.assertIn("Coach.Reset();", self.stats)
        self.assertIn("Coach.Suspend();", self.stats)
        self.assertIn("ValidDrivingCoachSample", self.coach)
        self.assertIn("return Coach.Review();", self.stats_header)
        self.assertIn("const latebrake::DrivingSessionStats& GetDrivingStats()", self.pawn_header)

    def test_coach_cannot_change_race_economy_or_player_physics(self):
        for file in (self.coach, self.stats_header):
            for forbidden in (
                "SaveRecords(", "AwardCredits(", "EarnCredits(",
                "RegisterCheckpoint(", "PassCheckpoint(", "RestartRace(",
                "SetActorTransform(", "SetSimulatePhysics(", "AddForce(",
                "AddTorque(", "GetAuthGameMode(", "GetWorld(",
                "UnrealEngine", "GameplayStatics", "UCLASS(",
            ):
                with self.subTest(forbidden=forbidden):
                    self.assertNotIn(forbidden, file)
        self.assertIn("NoGameplayAuthority=true", self.coach)
        self.assertIn("NoPersistentData=true", self.coach)
        self.assertIn("NoOfficialTiming=true", self.coach)

    def test_real_hud_is_results_only_and_explicitly_labeled(self):
        part = self.hud.split(
            "SAMPLED CAR TELEMETRY (ESTIMATED)", 1
        )[1].split("const auto PitSummary", 1)[0]
        self.assertIn("const auto Coaching=Car->GetDrivingStats().GetCoachReview();", part)
        self.assertIn("LOCAL DRIVING COACH | OBSERVED CHASSIS ONLY", part)
        self.assertIn("DrivingCoachMessageLabel(Coaching.Message)", part)
        self.assertIn("BuildDrivingCoachReview(Coaching)", part)
        self.assertIn("BuildDrivingCoachGoals(Coaching)", part)
        self.assertIn("DrivingCoachGoalLabel(", part)
        self.assertIn("DrivingCoachPaceTrendLabel(", part)
        self.assertIn("Coaching.AcceptedSamples", part)
        self.assertIn("Coaching.BrakeFraction", part)
        self.assertIn("Coaching.HighSlipFraction", part)
        self.assertIn("if (!bCompact)", part)
        self.assertNotIn("SaveRecords(", part)
        self.assertNotIn("ApplyGarageToPlayer(", part)
        self.assertLess(self.hud.index("if (latebrake::CanAwardFinishedRace(Race))",
                                       self.hud.index("SAMPLED CAR TELEMETRY (ESTIMATED)")-1000),
                        self.hud.index("LOCAL DRIVING COACH | OBSERVED CHASSIS ONLY"))

    def test_portable_matrix_integrates_new_regression(self):
        self.assertIn("DrivingCoachInsightsTests.cpp", self.build)
        self.assertIn("DrivingCoachReviewTests.cpp", self.build)
        self.assertIn("DrivingCoachPracticeGoalsTests.cpp", self.build)
        self.assertIn("add_test(NAME DrivingCoachInsights", self.build)
        self.assertIn("cxx_std_20", self.build)
        self.assertIn("/W4 /WX", self.build)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.build)
        for name in (
            "SampleValidation", "BrakeEpisodeLifecycle", "SlipEpisodeLifecycle",
            "SpeedBandBoundaries", "WindowsAndSuspensions",
            "RingRetentionAndMonotonicity", "RefusalDoesNotInventDriving",
            "EvidenceAndMessagePolicy", "DeterministicReferenceOracle",
        ):
            self.assertIn(name, self.test)


if __name__ == "__main__":
    unittest.main()
