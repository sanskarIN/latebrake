#include "Vehicles/PortableBrakeThermalInertia.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool value, const char* label)
{
    if (!value)
    {
        std::cerr << "PortableBrakeThermal FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-8;
}
}

int main()
{
    using namespace latebrake;
    PortableBrakeThermalFrame frame;
    auto result = EstimatePortableBrakeThermalStep(frame);
    Check(result.Valid && Near(result.NextCelsius, 25.0) &&
          Near(result.EstimatedPadEfficiency, 1.0),
          "ambient neutral no brake means stable temperature");
    frame.RotorCelsius = 250.0;
    frame.BrakingHeatKilojoules = 300.0;
    frame.EffectiveHeatCapacityKilojoulesPerCelsius = 3.0;
    frame.PassiveCoolingRatePerSecond = 0.0;
    result = EstimatePortableBrakeThermalStep(frame);
    Check(result.Valid && Near(result.HeatAddedCelsius, 100.0) &&
          Near(result.NextCelsius, 350.0),
          "measured joules and heat capacity drive heating");
    frame.BrakingHeatKilojoules = 0.0;
    frame.RotorCelsius = 650.0;
    frame.StepSeconds = 0.4;
    frame.PassiveCoolingRatePerSecond = 0.5;
    const auto cooled = EstimatePortableBrakeThermalStep(frame);
    Check(cooled.Valid && cooled.NextCelsius < 650.0 &&
          cooled.NextCelsius >= frame.AmbientCelsius &&
          cooled.EstimatedPadEfficiency < 1.0,
          "cooling never underflows ambient and hot pad loses efficiency");

    frame.StepSeconds = 0.1;
    double iterated = frame.RotorCelsius;
    for (int step = 0; step < 4; ++step)
    {
        frame.RotorCelsius = iterated;
        const auto sample = EstimatePortableBrakeThermalStep(frame);
        Check(sample.Valid, "valid small time step");
        iterated = sample.NextCelsius;
    }
    Check(Near(iterated, cooled.NextCelsius),
          "exponential cooling is frame subdivision invariant");

    frame.StepSeconds = 0.4;
    frame.RotorCelsius = 650.0;
    frame.AirSpeedMetersPerSecond = 100.0;
    frame.AirflowCoolingRatePerSecondAt100Mps = 0.7;
    result = EstimatePortableBrakeThermalStep(frame);
    Check(result.Valid && result.NextCelsius < cooled.NextCelsius,
          "measured airflow increases cooling");

    frame.BrakingHeatKilojoules = 3000.0;
    frame.EffectiveHeatCapacityKilojoulesPerCelsius = 0.1;
    frame.RotorCelsius = 1100.0;
    result = EstimatePortableBrakeThermalStep(frame);
    Check(result.Valid && result.HeatClamped &&
          result.NextCelsius <= 1200.0 &&
          result.EstimatedPadEfficiency >= 0.45 &&
          result.EstimatedPadEfficiency <= 1.0,
          "thermal extremes remain finite and bounded");
    frame.PassiveCoolingRatePerSecond = 0.0;
    frame.AirflowCoolingRatePerSecondAt100Mps = 0.0;
    result = EstimatePortableBrakeThermalStep(frame);
    Check(result.Valid && result.EstimatedSevereFade &&
          Near(result.EstimatedPadEfficiency, 0.45),
          "upper-bound severe-fade reporting");

    frame.BrakingHeatKilojoules =
        std::numeric_limits<double>::quiet_NaN();
    Check(!EstimatePortableBrakeThermalStep(frame).Valid,
          "nonfinite energy fails closed");
    frame.BrakingHeatKilojoules = 0.0;
    frame.StepSeconds = 0.0;
    Check(!EstimatePortableBrakeThermalStep(frame).Valid,
          "zero dt invalid");
    frame.StepSeconds = 0.001;
    frame.RotorCelsius = 20.0;
    Check(!EstimatePortableBrakeThermalStep(frame).Valid,
          "rotor colder than stated ambient disallowed");
    frame.RotorCelsius = std::numeric_limits<double>::infinity();
    Check(!EstimatePortableBrakeThermalStep(frame).Valid,
          "nonfinite rotor invalid");
    frame.RotorCelsius = 100.0;
    frame.AmbientCelsius = -41.0;
    Check(!EstimatePortableBrakeThermalStep(frame).Valid,
          "unsupported ambient invalid");
    std::cout << "Portable brake thermal inertia tests passed.\n";
}
