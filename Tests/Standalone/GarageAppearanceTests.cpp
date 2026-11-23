#include "Garage/GarageAppearance.h"
#include "Vehicles/VehiclePaintRecipe.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

namespace
{
void Check(bool value, const char* why)
{
    if (!value)
    {
        std::cerr << "Garage appearance FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Same(latebrake::GarageLook a, latebrake::GarageLook b)
{
    return a.Paint == b.Paint && a.Stripe == b.Stripe &&
           a.Wheels == b.Wheels;
}

void TestLegacyDefaults()
{
    using namespace latebrake;
    GarageAppearanceState styles;
    const auto bytes = styles.Serialize();
    Check(bytes.size() == 18u, "fixed 18-byte six-car SaveGame layout");
    for (int id = 0; id < VehicleCount; ++id)
    {
        const GarageLook factory = FactoryGarageLook(id);
        Check(Same(styles.Get(id), factory), "legacy car defaults are unchanged");
        Check(ValidGarageLook(factory) && factory.Paint == id,
              "factory paint remains same ID/color");
        const auto oldPaint = PaintForVehicle(id);
        const auto selected = PaintForGarageLook(id, factory);
        Check(std::strcmp(oldPaint.Name, selected.Name) == 0 &&
              selected.Body.R == oldPaint.Body.R &&
              selected.Body.G == oldPaint.Body.G &&
              selected.Body.B == oldPaint.Body.B,
              "migrated factory model matches old material scheme");
    }
    Check(Same(styles.Get(-1), FactoryGarageLook(0)) &&
          Same(styles.Get(VehicleCount), FactoryGarageLook(0)),
          "invalid car IDs are never indexed");
    Check(std::strcmp(GarageLookChoiceName(GarageLookLayer::Paint, 7),
          "PEARL WHITE") == 0, "new palette is named");
    Check(std::strcmp(GarageLookChoiceName(GarageLookLayer::Wheels, 2),
          "TRACK") == 0, "rims have descriptive UI labels");
}

void TestSelectionsAndIsolation()
{
    using namespace latebrake;
    GarageAppearanceState styles;
    for (int id = 0; id < VehicleCount; ++id)
    {
        for (GarageLookLayer layer : {GarageLookLayer::Paint,
             GarageLookLayer::Stripe, GarageLookLayer::Wheels})
        {
            const int max = GarageLookChoiceCount(layer);
            Check(max > 0, "each style category has choices");
            for (int choice = 0; choice < max; ++choice)
            {
                Check(styles.Choose(id, layer, choice), "legal cosmetic choice");
                Check(ValidGarageLook(styles.Get(id)), "style remains bounded");
                const auto result = styles.Serialize();
                GarageAppearanceState restored;
                Check(restored.Load(result.data(), result.size()),
                      "roundtrip all valid car choices");
                Check(Same(restored.Get(id), styles.Get(id)),
                      "roundtrip selected vehicle exact");
            }
            const auto before = styles.Serialize();
            Check(!styles.Choose(id, layer, max) &&
                  !styles.Choose(id, layer, -1),
                  "out of range appearance is rejected");
            Check(styles.Serialize() == before,
                  "failed choice cannot mutate any car");
        }
        Check(!styles.Choose(id, static_cast<GarageLookLayer>(88), 0),
              "unknown layer is inert");
    }
    // Factory reset affects the requested car only, even after that
    // car had all three layers altered and a neighbor was customized.
    GarageAppearanceState restore;
    Check(restore.Choose(2, GarageLookLayer::Paint, 7) &&
          restore.Choose(2, GarageLookLayer::Stripe, 2) &&
          restore.Choose(2, GarageLookLayer::Wheels, 2) &&
          restore.Choose(3, GarageLookLayer::Paint, 6),
          "stage two different custom vehicles");
    const auto neighbor = restore.Get(3);
    Check(restore.ResetVehicle(2) &&
          Same(restore.Get(2), FactoryGarageLook(2)) &&
          Same(restore.Get(3), neighbor),
          "reset factory affects just one car");
    Check(!restore.ResetVehicle(-1) &&
          !restore.ResetVehicle(VehicleCount),
          "invalid reset ID is inert");

    const auto before = styles.Serialize();
    Check(!styles.Choose(-1, GarageLookLayer::Paint, 0) &&
          !styles.Choose(VehicleCount, GarageLookLayer::Paint, 0),
          "bad car IDs fail closed");
    Check(styles.Serialize() == before, "bad IDs never write memory");
    GarageAppearanceState empty;
    Check(!empty.Load(nullptr, GarageAppearanceState::SerializedBytes),
          "null save rejected");
    Check(!empty.Load(before.data(), before.size()-1),
          "truncated save rejected");
    Check(empty.Serialize() == GarageAppearanceState{}.Serialize(),
          "failed load restores factory cosmetics only");
    auto corrupted = before;
    corrupted[5u] = 255u;
    Check(!empty.Load(corrupted.data(), corrupted.size()),
          "one invalid field rejects all six");
    Check(empty.Serialize() == GarageAppearanceState{}.Serialize(),
          "invalid load makes no partial adjustments");
    Check(GarageAppearanceState::Checksum(before) !=
          GarageAppearanceState::Checksum(corrupted),
          "tampering with a field invalidates persisted checksum");
    Check(!styles.Cycle(-1, GarageLookLayer::Paint) &&
          !styles.Cycle(0, static_cast<GarageLookLayer>(255)),
          "invalid cycle cannot alter another state");
}

void TestDeterministicStress()
{
    using namespace latebrake;
    GarageAppearanceState styles;
    std::uint32_t seed = 0x9a4824f1u;
    for (int n = 0; n < 300000; ++n)
    {
        seed = seed * 1664525u + 1013904223u;
        const int id = static_cast<int>(seed % VehicleCount);
        const auto layer = static_cast<GarageLookLayer>((seed >> 13) % 3u);
        const auto previous = styles.Serialize();
        Check(styles.Cycle(id, layer), "cycle all production cars 300k times");
        const auto current = styles.Serialize();
        Check(previous != current, "each cycle changes a field");
        Check(GarageAppearanceState::Checksum(previous) !=
              GarageAppearanceState::Checksum(current),
              "every changed appearance affects checksum");
        GarageAppearanceState restored;
        Check(restored.Load(current.data(), current.size()) &&
              restored.Serialize() == current,
              "deterministic portable serialization roundtrip");
    }
    const auto pearl = PaintForGarageLook(2, {7u, 2u, 1u});
    Check(std::strcmp(pearl.Name, "PEARL WHITE") == 0 &&
          pearl.Body.R > 0.85 && pearl.Accent.R < 0.25,
          "white paint has genuinely contrasting trim");
    const auto dark = PaintForGarageLook(4, {6u, 1u, 2u});
    Check(std::strcmp(dark.Name, "GRAPHITE") == 0 &&
          dark.Body.R < 0.3 && dark.Accent.R > 0.7,
          "graphite palette is a distinct production look");
}
} // namespace

int main()
{
    TestLegacyDefaults();
    TestSelectionsAndIsolation();
    TestDeterministicStress();
    std::cout << "Production six-car GarageAppearance policy passed.\n";
    return EXIT_SUCCESS;
}
