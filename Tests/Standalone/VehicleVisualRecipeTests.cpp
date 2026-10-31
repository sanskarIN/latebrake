#include "Vehicles/VehicleVisualRecipe.h"
#include "Garage/GarageState.h"

#include <cstdlib>
#include <iostream>

void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Vehicle visual: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    const auto arrow = VisualRecipeForVehicle(0);
    const auto kestrel = VisualRecipeForVehicle(1);
    const auto phantom = VisualRecipeForVehicle(2);
    Require(!arrow.HasRearWing && arrow.WingSpanScale == 0.0,
            "Arrow retains clean silhouette");
    Require(kestrel.HasRearWing && phantom.HasRearWing,
            "both purchased profiles have distinct wing outlines");
    Require(kestrel.HoodLengthScale > arrow.HoodLengthScale &&
            phantom.HoodLengthScale < arrow.HoodLengthScale,
            "front body length differs for all cars");
    Require(kestrel.RearDeckLengthScale < arrow.RearDeckLengthScale &&
            phantom.RearDeckLengthScale > arrow.RearDeckLengthScale,
            "rear body length differs for all cars");
    Require(kestrel.WingSpanScale > phantom.WingSpanScale &&
            phantom.WingHeightCm > kestrel.WingHeightCm,
            "rear wings are visibly differentiated");
    for (int id = 0; id < 3; ++id)
    {
        const auto spec = GarageState::Definition(id);
        const auto visual = VisualRecipeForVehicle(id);
        Require(spec.Id == id && spec.CabinLength > 0 && spec.CabinHeight > 0,
                "matching garage tuning and roof profile");
        Require(visual.HoodLengthScale >= 0.5 &&
                visual.HoodLengthScale <= 1.5 &&
                visual.RearDeckLengthScale >= 0.5 &&
                visual.RearDeckLengthScale <= 1.5 &&
                visual.RoofWidthScale >= 0.8 &&
                visual.RoofWidthScale <= 1.2,
                "bounded engine primitive mesh dimensions");
    }
    const auto corrupt = VisualRecipeForVehicle(-11);
    const auto outOfRange = VisualRecipeForVehicle(500);
    Require(!corrupt.HasRearWing && !outOfRange.HasRearWing &&
            corrupt.HoodLengthScale == arrow.HoodLengthScale &&
            outOfRange.RearDeckLengthScale == arrow.RearDeckLengthScale,
            "invalid car IDs use neutral safe shape");
    std::cout << "Vehicle visual recipe tests passed.\n";
    return EXIT_SUCCESS;
}
