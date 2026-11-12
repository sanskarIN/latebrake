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

    // Both actual cars need at least three sound contact traces to
    // exchange arcade drafting force; pit service is still stricter
    // (four supports and a nearly stationary vertical chassis).
    for (int n = 0; n <= 4; ++n)
    {
        const auto contact = EvaluateChassisGroundContact(Sample(n));
        Check(HasQualifiedDraftGroundSupport(contact) == (n >= 3),
              "draft requires three or four real grounded corners");
        Check(contact.HasDriveContact == (n >= 2),
              "drafting never changes the normal drive-contact policy");
    }
    auto invalidDraft = EvaluateChassisGroundContact(Sample(4));
    invalidDraft.Valid = false;
    Check(!HasQualifiedDraftGroundSupport(invalidDraft),
          "unset physics cannot authorize ghost drafting");
    invalidDraft = EvaluateChassisGroundContact(Sample(4));
    invalidDraft.ContactTractionScale = 0.75;
    Check(!HasQualifiedDraftGroundSupport(invalidDraft),
          "fabricated full-support traction never grants arcade boost");
    invalidDraft = EvaluateChassisGroundContact(Sample(3));
    invalidDraft.ContactTractionScale = 1.0;
    Check(!HasQualifiedDraftGroundSupport(invalidDraft),
          "three supports cannot claim four-corner road grip");
    invalidDraft = EvaluateChassisGroundContact(Sample(4));
    invalidDraft.HasDriveContact = false;
    Check(!HasQualifiedDraftGroundSupport(invalidDraft),
          "inconsistent physical drive contact fails closed");
    const auto unsafeGround = EvaluateChassisGroundContact(Sample(4, 180.0));
    Check(!HasQualifiedDraftGroundSupport(unsafeGround),
          "a genuinely airborne car cannot supply a drafting target");
    for (int i = 0; i < 25000; ++i)
    {
        const auto s = Sample(i % 5);
        const auto contact = EvaluateChassisGroundContact(s);
        Check(HasQualifiedDraftGroundSupport(contact) == (i % 5 >= 3),
              "25k physical support states preserve deterministic draft policy");
    }

    // Four trusted supports and a nearly settled body are needed for
    // an actual pit service; normal drive remains eligible at two.
    for (int n = 0; n <= 4; ++n)
    {
        const auto contact = EvaluateChassisGroundContact(Sample(n));
        Check(HasStablePitGroundSupport(contact, 0.0) == (n == 4),
              "pit service demands all four measured road supports");
    }
    const auto settled = EvaluateChassisGroundContact(Sample(4));
    for (const double vertical : {-80.0, 0.0, 80.0})
    {
        Check(HasStablePitGroundSupport(settled, vertical),
              "settled chassis accepts pit service at boundary");
    }
    for (const double vertical : {-80.001, 80.001,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()})
    {
        Check(!HasStablePitGroundSupport(settled, vertical),
              "bounce or nonfinite vertical movement denies service");
    }
    auto fabricated = settled;
    fabricated.Valid = false;
    Check(!HasStablePitGroundSupport(fabricated, 0.0),
          "invalid contact may not authorize repairs");
    fabricated = settled;
    fabricated.HasDriveContact = false;
    Check(!HasStablePitGroundSupport(fabricated, 0.0),
          "missing actual drive contact may not authorize repairs");
    fabricated = settled;
    fabricated.ContactTractionScale = 0.45;
    Check(!HasStablePitGroundSupport(fabricated, 0.0),
          "partial contact traction may not authorize full pit repair");

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
