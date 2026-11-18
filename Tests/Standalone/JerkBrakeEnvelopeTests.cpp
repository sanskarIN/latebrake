#include "AI/JerkBrakeEnvelope.h"

#include <cassert>
#include <cmath>
#include <limits>

using namespace latebrake;

int main()
{
    const JerkBrakeEnvelopeSettings settings{};
    const auto stopped=EvaluateJerkBrakeEnvelope(0,0,1000,settings);
    assert(stopped.Valid && !stopped.RequiresEmergencyBrake);
    assert(std::abs(stopped.RequiredGapCm-485.0)<1e-9);
    const auto contact=EvaluateJerkBrakeEnvelope(0,0,485,settings);
    assert(contact.Valid && contact.RequiresEmergencyBrake);

    // For identical speeds, no dynamic closing distance is invented.
    const auto matched=EvaluateJerkBrakeEnvelope(3000,3000,1000);
    assert(matched.Valid && matched.ClosingSpeedCmps==0);
    assert(matched.RampDistanceCm==0);

    // Jerk-limited braking must never undercut instantaneous full
    // deceleration for a positive closing speed.
    for(double speed=25;speed<=12000;speed+=125)
    {
        const auto r=EvaluateJerkBrakeEnvelope(speed,0,100000);
        assert(r.Valid);
        const double instantaneous=speed*speed/
            (2.0*settings.MaximumDecelerationCmps2);
        assert(r.RampDistanceCm+r.ConstantBrakeDistanceCm+1e-7
            >=instantaneous);
        assert(r.RequiredGapCm>=485+instantaneous);
        assert(r.StopDurationSeconds>=r.RampDurationSeconds);
        const auto before=EvaluateJerkBrakeEnvelope(
            speed,0,r.RequiredGapCm-0.01);
        const auto after=EvaluateJerkBrakeEnvelope(
            speed,0,r.RequiredGapCm+0.01);
        assert(before.Valid && before.RequiresEmergencyBrake);
        assert(after.Valid && !after.RequiresEmergencyBrake);
    }

    // Continuous at the ramp-to-constant-braking transition.
    const double transition=settings.MaximumDecelerationCmps2*
        settings.MaximumDecelerationCmps2/
        (2.0*settings.MaximumJerkCmps3);
    const auto left=EvaluateJerkBrakeEnvelope(transition-1e-5,0,1000);
    const auto right=EvaluateJerkBrakeEnvelope(transition+1e-5,0,1000);
    assert(left.Valid && right.Valid);
    assert(std::abs(left.RequiredGapCm-right.RequiredGapCm)<0.01);

    // A slower obstacle increases closing demand; a faster one does not.
    const auto slow=EvaluateJerkBrakeEnvelope(5000,1000,1000);
    const auto fast=EvaluateJerkBrakeEnvelope(5000,4500,1000);
    assert(slow.Valid && fast.Valid);
    assert(slow.RequiredGapCm>fast.RequiredGapCm);
    assert(!EvaluateJerkBrakeEnvelope(-1,0,1000).Valid);
    assert(!EvaluateJerkBrakeEnvelope(100,0,-1).Valid);
    assert(!EvaluateJerkBrakeEnvelope(100,0,
        std::numeric_limits<double>::infinity()).Valid);
    auto invalid=settings;
    invalid.MaximumJerkCmps3=0;
    assert(!EvaluateJerkBrakeEnvelope(100,0,1000,invalid).Valid);
    invalid=settings;
    invalid.ReactionSeconds=std::numeric_limits<double>::quiet_NaN();
    assert(!EvaluateJerkBrakeEnvelope(100,0,1000,invalid).Valid);
}
