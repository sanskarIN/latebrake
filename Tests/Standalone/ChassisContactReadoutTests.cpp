#include "Vehicles/ChassisContactReadout.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool yes, const char* message)
{
    if (!yes)
    {
        std::cerr << "ChassisContactReadout FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    const int expected[] = {0, 0, 45, 75, 100};
    for (int n = 0; n < 5; ++n)
    {
        ChassisGroundContactSample input{};
        input.DeltaSeconds = 1.0 / 60.0;
        input.PhysicsReady = true;
        for (int i = 0; i < 4; ++i)
            input.Probes[static_cast<unsigned>(i)] = {i < n, 36.0, 1.0};
        const auto sample = EvaluateChassisGroundContact(input);
        const auto report = DescribeChassisContact(sample);
        Require(report.Live && report.QualifiedProbes == n &&
                    report.DriveTractionPercent == expected[n],
                "HUD exactly reports physically applied contact scale");
        const auto requiredState = n == 4 ? ChassisContactStatus::Full :
            n >= 2 ? ChassisContactStatus::Partial :
            ChassisContactStatus::Airborne;
        Require(report.Status == requiredState,
                "HUD state must distinguish actual grounded and unsupported car");
    }

    Require(!DescribeChassisContact({}).Live,
            "unset run/restart cannot advertise a false grounded state");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Require(!DescribeChassisContact({true, 4, nan, true}).Live,
            "untrusted nonfinite contact cannot enter visible HUD");
    Require(!DescribeChassisContact({true, 2, 1.0, true}).Live,
            "inconsistent stored fraction cannot advertise full contact");
    Require(!DescribeChassisContact({true, 4, 1.0, false}).Live,
            "inconsistent suspension/drive state cannot enter the HUD");
    Require(!DescribeChassisContact({true, 5, 1.0, true}).Live,
            "imaginary fifth trace cannot be accepted");
    for (int i = 0; i < 25000; ++i)
    {
        const int count = i % 5;
        const auto scale = expected[count] / 100.0;
        const ChassisGroundContactOutput accepted{
            true, count, scale, count >= 2
        };
        const auto report = DescribeChassisContact(accepted);
        Require(report.Live &&
                report.DriveTractionPercent == expected[count] &&
                report.QualifiedProbes == count &&
                report.DriveTractionPercent >= 0 &&
                report.DriveTractionPercent <= 100,
                "25k contact readouts match exact force policy without drift");
    }
    std::cout << "Chassis contact HUD readouts verified.\n";
    return EXIT_SUCCESS;
}
