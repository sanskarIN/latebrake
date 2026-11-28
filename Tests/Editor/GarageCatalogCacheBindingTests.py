"""Guard the native HUD's shared read-only catalogue cache."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GarageCatalogCacheBindingTests(unittest.TestCase):
    def test_shared_cache_for_all_three_render_consumers(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        renderer = read("Source/Latebrake/Private/UI/LatebrakeGarageVehicleCardsHUD.cpp")
        header = read("Source/Latebrake/Public/UI/LatebrakeRaceHUD.h")
        self.assertEqual(hud.count("GarageCatalogue.Read(Mode->GetGarageSnapshot())"), 2)
        self.assertNotIn("BuildGarageCatalogView(Mode->GetGarageSnapshot())", hud)
        self.assertIn("DescribeGarageVehicleCards(Snapshot,&GarageCatalogue)", renderer)
        self.assertIn("latebrake::GarageCatalogCache GarageCatalogue;", header)

    def test_cache_only_canonical_economy_not_appearance_or_authority(self):
        policy = read("Source/Latebrake/Public/Garage/GarageCatalogView.h")
        cache = policy.split("class GarageCatalogCache final", 1)[1]
        self.assertIn("garage.Load(saved)", cache)
        for field in ("Credits", "OwnedVehicleMask", "SelectedVehicle", "EngineLevel", "GripLevel"):
            self.assertIn(f"current.{field}!=Key.{field}", cache)
        self.assertIn("Cached=BuildGarageCatalogView(current)", cache)
        for forbidden in ("GarageLook", "SaveRecords(", "Unlock(", "Select("):
            self.assertNotIn(forbidden, cache)
        cards = read("Source/Latebrake/Public/UI/GarageVehicleCards.h")
        self.assertLess(cards.index("if(!appearance.Load("), cards.index("cache?cache->Read(saved)"))
        self.assertIn("PaintForGarageLook(id,look).Body", cards)
        self.assertIn("const auto data=DescribeGarageVehicleCards(current);", cards)
        self.assertIn("add_latebrake_portable_research_test(GarageCatalogCache)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
