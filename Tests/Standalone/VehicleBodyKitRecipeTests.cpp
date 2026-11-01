#include "Vehicles/VehicleBodyKitRecipe.h"
#include "Garage/GarageState.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace
{
void Check(bool valid, const char* reason)
{
    if (!valid)
    {
        std::cerr << "VehicleBodyKit FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using latebrake::BodyKitForVehicle;
    using latebrake::VehicleCount;
    const auto starter = BodyKitForVehicle(0);
    Check(std::strcmp(starter.Name, "STREET") == 0 &&
              !starter.HasHoodScoop && !starter.HasRearFins,
          "starter car retains a clean silhouette");
    Check(std::strcmp(BodyKitForVehicle(-1).Name, starter.Name) == 0 &&
              std::strcmp(BodyKitForVehicle(999).Name, starter.Name) == 0,
          "corrupted save IDs cannot reveal unowned kit variants");

    for (int id = 0; id < VehicleCount; ++id)
    {
        const auto kit = BodyKitForVehicle(id);
        Check(latebrake::GarageState::Definition(id).Id == id &&
                  kit.Name && kit.Name[0] != '\0',
              "every existing owned car has one named kit");
        Check(std::isfinite(kit.SplitterWidthScale) &&
                  kit.SplitterWidthScale >= 0.8 &&
                  kit.SplitterWidthScale <= 1.25,
              "splitter remains inside starter's geometric envelope");
        Check(std::isfinite(kit.SideSkirtLengthScale) &&
                  kit.SideSkirtLengthScale >= 0.8 &&
                  kit.SideSkirtLengthScale <= 1.30,
              "skirts cannot exceed intended bounding silhouette");
        Check(std::isfinite(kit.DiffuserWidthScale) &&
                  kit.DiffuserWidthScale >= 0.8 &&
                  kit.DiffuserWidthScale <= 1.25,
              "diffuser must retain bounded dimensions");
        Check(std::isfinite(kit.ScoopHeightCm) &&
                  kit.ScoopHeightCm >= 0.0 && kit.ScoopHeightCm <= 25.0 &&
                  (kit.HasHoodScoop == (kit.ScoopHeightCm > 0.0)),
              "scoop visibility and vertical extent agree");
        Check(std::isfinite(kit.RearFinHeightCm) &&
                  kit.RearFinHeightCm >= 0.0 && kit.RearFinHeightCm <= 38.0 &&
                  (kit.HasRearFins == (kit.RearFinHeightCm > 0.0)),
              "rear fin visibility and height agree");
        for (int other = 0; other < id; ++other)
        {
            const auto previous = BodyKitForVehicle(other);
            Check(std::strcmp(kit.Name, previous.Name) != 0,
                  "distinct cars must not share the same display kit label");
            Check(kit.SplitterWidthScale != previous.SplitterWidthScale ||
                      kit.SideSkirtLengthScale != previous.SideSkirtLengthScale ||
                      kit.DiffuserWidthScale != previous.DiffuserWidthScale ||
                      kit.HasHoodScoop != previous.HasHoodScoop ||
                      kit.HasRearFins != previous.HasRearFins,
                  "body-kit silhouette must have at least one visible difference");
        }
    }
    // Repeated selection and corrupt-ID fallback never mutate the recipe
    // or any ownership/reward state in this constexpr-only geometry policy.
    for (int i = 0; i < 100000; ++i)
    {
        const int id = i % VehicleCount;
        const auto a = BodyKitForVehicle(id);
        const auto b = BodyKitForVehicle(id);
        Check(std::strcmp(a.Name, b.Name) == 0 &&
                  a.SplitterWidthScale == b.SplitterWidthScale &&
                  a.HasRearFins == b.HasRearFins,
              "recipe is deterministic across long garage cycles");
    }
    std::cout << "VehicleBodyKit: all variants verified\n";
    return EXIT_SUCCESS;
}
