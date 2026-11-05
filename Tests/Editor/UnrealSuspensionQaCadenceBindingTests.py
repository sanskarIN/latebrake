"""Source-only binding for bounded, non-shipping suspension event logs.

The real car's sampled spring load mask controls QA *only*, not drive,
race progression or Unreal's Chaos force solver. No runtime QA implied.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h").read_text(encoding="utf-8")
POLICY = (ROOT / "Source/Latebrake/Public/Vehicles/SuspensionQaCadence.h").read_text(encoding="utf-8")
TESTS = (ROOT / "Tests/Standalone/SuspensionQaCadenceTests.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")


class UnrealSuspensionQaCadenceBindingTests(unittest.TestCase):
    def test_real_spring_frame_controls_nonshipping_log_only(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1
        )[0]
        qa = tick.split("#if !UE_BUILD_SHIPPING", 1)[1].split("#endif", 1)[0]
        for marker in (
            "unsigned LoadedCornerMask = 0u;",
            "const auto& Wheel = SpringResponse.Corners[Corner];",
            "Wheel.Contact &&",
            "Wheel.UpForce > SpringResponse.WeightForce * 1.0e-6",
            "LoadedCornerMask |= 1u << Corner;",
            "SuspensionQaSchedule.ShouldReport(",
            "LoadedCornerMask, bPreGreen",
            "LATEBRAKE_SUSPENSION v=1",
            "SpringResponse.ContactCount",
            "SpringResponse.TotalUpForce / SpringResponse.WeightForce",
        ):
            self.assertIn(marker, qa)
        self.assertLess(tick.index("GripReattachment.Advance("),
                        tick.index("SuspensionQaSchedule.ShouldReport("))
        # Physics support is dispatched exactly once before optional
        # policies or QA. Non-shipping telemetry only observes it; logs
        # must never gate or double-submit any spring force.
        lift = "ApplySprungForces(SpringResponse, WheelWorldAnchors);"
        self.assertEqual(tick.count(lift), 1)
        self.assertLess(tick.index("if (!SpringResponse.Valid)"), tick.index(lift))
        self.assertLess(tick.index(lift), tick.index("GripReattachment.Advance("))
        self.assertLess(tick.index(lift),
                        tick.index("SuspensionQaSchedule.ShouldReport("))
        for forbidden in (
            "Chassis->AddForce", "Chassis->AddTorque", "Chassis->SetPhysics",
            "LineTraceSingleByChannel", "SetActorLocation", "PassCheckpoint(",
            "AwardCredits(", "SaveGame(", "Http", "WriteFile(", "SpawnActor(",
        ):
            self.assertNotIn(forbidden, qa)

    def test_real_reset_boundaries_and_no_shipping_state(self):
        self.assertIn('#include "Vehicles/SuspensionQaCadence.h"', HEADER)
        self.assertIn("latebrake::SuspensionQaCadence SuspensionQaSchedule;", HEADER)
        self.assertIn("void ResetSuspensionQa() noexcept", HEADER)
        self.assertIn("#if !UE_BUILD_SHIPPING", HEADER)
        self.assertIn("SuspensionQaSchedule.Reset();", HEADER)
        self.assertIn("GripReattachment.Reset(); ResetSuspensionQa();", HEADER)
        self.assertIn("if (bResetGripAuthority)", PAWN)
        self.assertIn("GripReattachment.Reset();\n        ResetSuspensionQa();", PAWN)
        self.assertGreaterEqual(PAWN.count("ResetSuspensionQa();"), 3)
        no_heading = PAWN.split("if (Forward.IsNearlyZero())", 1)[1].split(
            "\n    }", 1
        )[0]
        self.assertIn("ClearDriverInputs(false);", no_heading)
        self.assertIn("GripReattachment.Reset();", no_heading)
        self.assertNotIn("ResetSuspensionQa();", no_heading)

    def test_portable_stress_and_strict_unchanged_v1_schema(self):
        self.assertIn("SuspensionQaMinimumEventSeconds = 0.20", POLICY)
        self.assertIn("SuspensionQaHeartbeatSeconds = 2.0", POLICY)
        self.assertIn("SuspensionQaMaximumFrameSeconds = 0.10", POLICY)
        for marker in ("void Reset() noexcept", "std::isfinite(deltaSeconds)",
                       "loadedCornerMask > 0x0Fu", "isGrid != bLastGrid",
                       "bPendingTransition = bPendingTransition ||",
                       "loadedCornerMask != LastObservedLoadMask",
                       "isGrid != bLastObservedGrid",
                       "bPendingTransition = false;"):
            self.assertIn(marker, POLICY)
        for forbidden in ("UCLASS(", "GetWorld(", "AddForce(", "SaveGame(",
                          "AwardCredits(", "SetPhysics", "Http"):
            self.assertNotIn(forbidden, POLICY)
        self.assertIn("120000", TESTS)
        self.assertIn("24000", TESTS)
        self.assertIn("shortLossReports == 1", TESTS)
        self.assertIn("SuspensionQaCadenceTests.cpp", CMAKE)
        self.assertIn("NAME SuspensionQaCadence", CMAKE)


if __name__ == "__main__":
    unittest.main()
