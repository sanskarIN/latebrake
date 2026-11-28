"""Career catalogue exposes locked routes without changing selection authority."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[2]


class CareerCatalogueBindingTests(unittest.TestCase):
    def test_visible_catalogue_is_read_only(self):
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text()
        block = hud.split("const auto DrawCareerCatalogue=[&]()", 1)[1].split(
            "switch (Mode->GetMenuPage())", 1)[0]
        for phrase in ("CareerCatalogue.Read(Mode->GetCareer().Snapshot()",
                       "Mode->GetGarageSnapshot()", "Catalogue.Section(Mode->GetMenuPage())",
                       "LOCKED EVENTS ARE READ-ONLY", "Row.FirstRouteEvent",
                       "Row.ConditionalAdditionalFunding", "Mode->GetMenuCareerEventId()"):
            self.assertIn(phrase, block)
        for forbidden in ("StartCareerEvent(", "SaveRecords(", "CommitFinish(", "Unlock("):
            self.assertNotIn(forbidden, block)
        for page in ("Career", "Championships", "Challenges"):
            section = hud.split(f"case latebrake::MenuPage::{page}:", 1)[1].split("break;", 1)[0]
            self.assertIn("DrawCareerCatalogue();", section)

    def test_authority_and_cache(self):
        mode = (ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text()
        self.assertIn("const int32 Target = GetMenuCareerEventId();", mode)
        self.assertNotIn("BuildCareerCatalogue(", mode)
        header = (ROOT / "Source/Latebrake/Public/UI/LatebrakeRaceHUD.h").read_text()
        self.assertIn("latebrake::CareerCatalogueCache CareerCatalogue;", header)
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text()
        self.assertIn("add_latebrake_portable_research_test(CareerCatalogue)", cmake)


if __name__ == "__main__":
    unittest.main()
