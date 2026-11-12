"""Source contracts for per-rival identity-safe observational telemetry.

Does not compile UnrealHeaderTool or simulate five active Chaos pawns.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class GridRivalLedgerBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = source("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.header = source("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.rival = source("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.policy = source("Source/Latebrake/Public/AI/GridRivalLedger.h")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")
        cls.tests = source("Tests/Standalone/GridRivalLedgerTests.cpp")

    def test_actual_game_mode_owns_primary_actor_identity(self):
        self.assertIn('#include "AI/GridRivalLedger.h"', self.header)
        self.assertIn("latebrake::GridRivalLedger GridRivals;", self.header)
        self.assertIn("GetGridRivalStatus(int32 Slot) const noexcept", self.header)
        self.assertIn("GridRivals.Snapshot(Slot)", self.header)
        self.assertIn("IsPrimaryRivalActor(const ALatebrakeRivalCar* Actor)", self.header)
        self.assertIn("RivalCar.Get() == Actor", self.header)
        self.assertEqual(self.mode.count(
            "GridRivals.Configure(Race.GetCheckpointCount(), Race.GetRequiredLaps());"
        ), 2)
        self.assertIn("GridRivals.Bind(latebrake::PrimaryRivalGridSlot", self.mode)
        self.assertEqual(
            self.mode.count("GridRivals.Bind(latebrake::PrimaryRivalGridSlot"), 2)
        self.assertIn("static_cast<uint64>(RivalCar->GetUniqueID())", self.mode)
        self.assertIn("GridRivals.RecordAcceptedGate(", self.mode)
        self.assertIn("GridRivals.RecordRecoveryResult(", self.mode)

    def test_only_accepted_primary_physical_rival_can_submit(self):
        gate = self.rival.split("if (Observation.Advanced)", 1)[1].split(
            "if (Observation.NeedsRecovery)", 1)[0]
        self.assertIn("Mode->IsPrimaryRivalActor(this)", gate)
        self.assertIn("Mode->ObserveRivalVerifiedGate();", gate)
        self.assertLess(gate.index("Mode->IsPrimaryRivalActor(this)"),
                        gate.index("Mode->ObserveRivalVerifiedGate();"))
        rec = self.rival.split("if (Observation.NeedsRecovery)", 1)[1].split(
            "if (Observation.Completed)", 1)[0]
        self.assertLess(rec.index("const bool bReported = Route.ReportRecoveryResult(bRecovered);"),
                        rec.index("Mode->ObserveRivalRecoveryResult(this, bRecovered);"))
        self.assertIn("if (bReported)", rec)
        self.assertIn("SetAutonomousControls(0.f, 0.f, !bRecovered);", rec)
        self.assertIn("const ALatebrakeRivalCar* Actor, bool bEnginePlacementVerified",
                      self.mode)
        self.assertIn("if (!IsPrimaryRivalActor(Actor))", self.mode)
        # The new fleet owns a bounded *transient* extra-rival array,
        # but only behind a non-shipping opt-in. The fixed-capacity ledger
        # remains the only source of unofficial per-actor observations.
        self.assertIn("TArray<TObjectPtr<ALatebrakeRivalCar>> ExperimentalRivals;", self.header)
        self.assertIn("bool bExperimentalSixCarGrid = false;", self.header)
        self.assertIn("GridRivals.Matches(Slot, Identity)", self.mode)

    def test_no_progress_awards_or_teleport_authority(self):
        for forbidden in ("AddForce(", "AddTorque(", "SaveRecords(",
                          "PassCheckpoint(", "SpawnActor<", "SetActorLocation(",
                          "EarnCredits(", "UCLASS(", "UGameplayStatics"):
            self.assertNotIn(forbidden, self.policy)
        for marker in ("static constexpr int FirstRivalSlot",
                       "PrimaryRivalGridSlot", "Entries.fill({});",
                       "status.ActorToken == actorToken",
                       "ordinal != status.LastGateOrdinal + 1",
                       "elapsedSeconds <= status.LastGateSeconds",
                       "attemptOrdinal != status.RecoveryAttempts + 1",
                       "status.Finished = ordinal == TotalGates",
                       "bool Release(", "GridRivalStatus Snapshot("):
            self.assertIn(marker, self.policy)
        self.assertIn("GridRivalLedgerTests.cpp", self.cmake)
        self.assertIn("NAME GridRivalLedger", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("3600", self.tests) if "3600" in self.tests else self.assertIn(
            "for (int gates = 1; gates <= 16", self.tests)


if __name__ == "__main__":
    unittest.main()
