#include "Vehicles/ChassisGroundContactTraction.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "ChassisGroundContactTraction FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::ChassisGroundContactSample Sample(int hits, double depth=36.0,
                                             double normal=1.0)
{
    latebrake::ChassisGroundContactSample s{};
    s.DeltaSeconds = 1.0 / 60.0;
    s.ChassisVerticalSpeedCmPerSecond = 0.0;
    s.PhysicsReady = true;
    for (int i=0; i<4; ++i)
    {
        s.Probes[static_cast<unsigned>(i)] =
            {i < hits, depth, normal};
    }
    return s;
}
}

int main()
{
    using namespace latebrake;
    for (int n=0; n<=4; ++n)
    {
        const auto result = EvaluateChassisGroundContact(Sample(n));
        const double desired[] = {0.0, 0.0, 0.45, 0.75, 1.0};
        Check(result.Valid && result.SupportingProbes==n &&
              result.ContactTractionScale==desired[n] &&
              result.HasDriveContact==(n>=2),
              "physical ground support scales 0/1/2/3/4 observed probes");
    }
    for (const double height : {0.0, 36.0, 85.0, 115.0})
    {
        const auto result=EvaluateChassisGroundContact(Sample(4,height));
        Check(result.Valid && result.ContactTractionScale==1.0,
              "resting chassis near track remains fully drivable");
    }
    const auto airborne = EvaluateChassisGroundContact(Sample(4, 180.0));
    Check(airborne.Valid && !airborne.HasDriveContact &&
          airborne.ContactTractionScale==0.0,
          "physical spawn/drop above road never has invented wheel traction");
    const auto steep = EvaluateChassisGroundContact(Sample(4, 36.0, 0.3));
    Check(steep.Valid && steep.ContactTractionScale == 0.0,
          "near-vertical walls cannot impersonate the supporting road");
    const auto hillside = EvaluateChassisGroundContact(Sample(4,36.0,0.72));
    Check(hillside.Valid && hillside.HasDriveContact,
          "sloping real WorldStatic ground can support contact");
    auto partial=Sample(4);
    partial.Probes[0].GroundNormalZ=0.45;
    partial.Probes[1].DistanceCm=180.0;
    const auto limit=EvaluateChassisGroundContact(partial);
    Check(limit.Valid && limit.SupportingProbes==2 &&
          limit.ContactTractionScale==0.45,
          "wheel probes crossing a road edge lose physical traction");
    auto invalid=Sample(4);
    invalid.Probes[1].DistanceCm=
        std::numeric_limits<double>::quiet_NaN();
    Check(!EvaluateChassisGroundContact(invalid).Valid,
          "nonfinite confirmed physics trace rejects traction");
    invalid=Sample(4);
    invalid.Probes[2].GroundNormalZ=1.5;
    Check(!EvaluateChassisGroundContact(invalid).Valid,
          "impossible confirmed hit normal rejects traction");
    invalid=Sample(4);
    invalid.PhysicsReady=false;
    Check(!EvaluateChassisGroundContact(invalid).Valid,
          "suspended or missing qualified car must not gain drive");
    invalid=Sample(4);
    invalid.DeltaSeconds=0.25;
    Check(!EvaluateChassisGroundContact(invalid).Valid,
          "physics hitch invalidates contact, not extrapolates motion");
    invalid=Sample(4);
    invalid.ChassisVerticalSpeedCmPerSecond=
        std::numeric_limits<double>::infinity();
    Check(!EvaluateChassisGroundContact(invalid).Valid,
          "untrusted vertical chassis velocity cannot grant motor force");

    for (int i=0;i<80000;++i)
    {
        const int n=i%5;
        auto probe=Sample(n,static_cast<double>(i%180),
                          i%7==0 ? 0.4 : 0.9);
        probe.DeltaSeconds=1.0/(30.0+static_cast<double>(i%91));
        probe.ChassisVerticalSpeedCmPerSecond=
            static_cast<double>((i%3000)-1500);
        const auto result=EvaluateChassisGroundContact(probe);
        Check(result.Valid && result.SupportingProbes>=0 &&
              result.SupportingProbes<=4 &&
              std::isfinite(result.ContactTractionScale) &&
              result.ContactTractionScale>=0.0 &&
              result.ContactTractionScale<=1.0 &&
              (result.HasDriveContact==(result.SupportingProbes>=2)),
              "80k changing physical ground traces remain bounded");
    }
    std::cout << "Ground-contact traction for real Chaos rigid chassis passed.\n";
    return EXIT_SUCCESS;
}
