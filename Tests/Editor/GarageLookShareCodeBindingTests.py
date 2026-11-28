"""Source-only read-only visual sharing integration; no engine/package claims."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class GarageLookShareCodeBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.code = read("Source/Latebrake/Public/Garage/GarageLookShareCode.h")
        cls.mode = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.tests = read("Tests/Standalone/GarageLookShareCodeTests.cpp")
        cls.build = read("Tests/Standalone/CMakeLists.txt")

    def test_look_reference_is_purely_read_only(self):
        self.assertIn('#include "Garage/GarageLookShareCode.h"', self.hud)
        section = self.hud.split("case latebrake::MenuPage::Garage:", 1)[1].split(
            "case latebrake::MenuPage::Results:", 1)[0]
        self.assertIn("EncodeGarageLookShareCode(PreviewId,ViewedLook)", section)
        self.assertIn("GetGaragePreviewLook()", section)
        self.assertIn('TEXT("LOOK REFERENCE: %s | COSMETIC ONLY")', section)
        self.assertIn("UTF8_TO_TCHAR(LookCode.Text.data())", section)
        self.assertLess(section.index("const latebrake::GarageLook ViewedLook"),
                        section.index("LOOK REFERENCE:"))
        self.assertIn("GetGaragePreviewVehicleId() const", self.mode)
        self.assertIn("GetGaragePreviewLook() const", self.mode)
        self.assertNotIn("DecodeGarageLookShareCode(", section)
        for action in ("Garage.Unlock(", "Garage.EarnCredits(",
                       "Garage.Upgrade(", "SaveRecords("):
            self.assertNotIn(action, section)

    def test_code_never_encodes_session_or_mutates_authority(self):
        self.assertIn("GarageState::IsVehicle(vehicleId)", self.code)
        self.assertIn("ValidGarageLook(appearance)", self.code)
        self.assertIn("GarageLookCodeChecksum(payload)", self.code)
        self.assertIn("GarageLookCodeStatus::Noncanonical", self.code)
        for authority in ("SaveRecords(", "Unlock(", "EarnCredits(",
                          "SetGarageTuning(", "AddForce(", "SetActorLocation(",
                          "GetWorld(", "FPlatformTime"):
            self.assertNotIn(authority, self.code)

    def test_portable_roundtrip_is_in_ci_matrix(self):
        self.assertIn("GarageLookShareCodeTests.cpp", self.build)
        self.assertIn("NAME GarageLookShareCode", self.build)
        for proof in ("EncodeGarageLookShareCode", "DecodeGarageLookShareCode",
                      "ValidGarageLook", "GarageLookCodeStatus::InvalidChecksum",
                      "GarageLookCodeStatus::Noncanonical"):
            self.assertIn(proof, self.tests)


if __name__ == "__main__":
    unittest.main()
