#include "Vehicles/VehiclePaintRecipe.h"
#include "Garage/GarageState.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "VehiclePaintRecipe FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool ValidRgb(latebrake::VehiclePaintRgb value)
{
    return std::isfinite(value.R) && std::isfinite(value.G) &&
           std::isfinite(value.B) &&
           value.R >= 0.0 && value.R <= 1.0 &&
           value.G >= 0.0 && value.G <= 1.0 &&
           value.B >= 0.0 && value.B <= 1.0;
}

double Separation(latebrake::VehiclePaintRgb a,
                  latebrake::VehiclePaintRgb b)
{
    const double r = a.R - b.R;
    const double g = a.G - b.G;
    const double blue = a.B - b.B;
    return r * r + g * g + blue * blue;
}
}

int main()
{
    using latebrake::GarageState;
    using latebrake::VehicleCount;
    using latebrake::PaintForVehicle;
    const auto starter = PaintForVehicle(0);
    Check(std::strcmp(starter.Name, "SKY BLUE") == 0,
          "starter has canonical free visual identity");
    for (int invalid : {-200, -1, VehicleCount, 999999})
    {
        const auto other = PaintForVehicle(invalid);
        Check(std::strcmp(other.Name, starter.Name) == 0 &&
              Separation(other.Body, starter.Body) == 0.0 &&
              Separation(other.Accent, starter.Accent) == 0.0,
              "bad or corrupt garage IDs always use free starter paint");
    }
    for (int id = 0; id < VehicleCount; ++id)
    {
        const auto x = PaintForVehicle(id);
        Check(GarageState::Definition(id).Id == id,
              "paint identity follows immutable garage vehicle ID");
        Check(x.Name && x.Name[0] != '\0' &&
              ValidRgb(x.Body) && ValidRgb(x.Accent) && ValidRgb(x.Canopy),
              "every recipe has complete bounded material color values");
        Check(Separation(x.Body, x.Accent) > 0.13,
              "body and aero trim have a discernible contrast");
        Check(Separation(x.Body, x.Canopy) > 0.14,
              "cockpit canopy is visually distinct from painted shell");
        for (int prev = 0; prev < id; ++prev)
        {
            const auto other = PaintForVehicle(prev);
            Check(std::strcmp(x.Name, other.Name) != 0,
                  "every garage car has a unique paint name");
            Check(Separation(x.Body, other.Body) > 0.14,
                  "body palettes remain meaningfully different");
        }
    }
    for (int n = 0; n < 100000; ++n)
    {
        const int id = n % VehicleCount;
        const auto a = PaintForVehicle(id);
        const auto b = PaintForVehicle(id);
        Check(a.Name == b.Name && Separation(a.Body, b.Body) == 0.0 &&
              Separation(a.Accent, b.Accent) == 0.0,
              "repeated garage switches retain deterministic source paints");
    }
    std::cout << "VehiclePaintRecipe: six paint identities verified\n";
    return EXIT_SUCCESS;
}
