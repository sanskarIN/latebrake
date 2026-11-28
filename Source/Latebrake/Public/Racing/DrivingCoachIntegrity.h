#pragma once

#include "Racing/DrivingSessionStats.h"
#include "Racing/DrivingCoachReview.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Cross-check the new coach against the original telemetry authority rather
// than trusting independently accumulated UI values. The result is advisory:
// it does not validate the Unreal physics source, race timing or save data.
enum class DrivingCoachIntegrityStatus : std::uint8_t
{
    NoSamples,
    LimitedSamples,
    VerifiedSourceAgreement,
    InvalidCoachReadout,
    CrossSourceMismatch,
    DistributionMismatch
};

inline const char* DrivingCoachIntegrityLabel(
    DrivingCoachIntegrityStatus status) noexcept
{
    switch(status)
    {
    case DrivingCoachIntegrityStatus::NoSamples:
        return "NO ACCEPTED SOURCE SAMPLES";
    case DrivingCoachIntegrityStatus::LimitedSamples:
        return "COACH SOURCE CONSISTENT; COVERAGE LIMITED";
    case DrivingCoachIntegrityStatus::VerifiedSourceAgreement:
        return "COACH AGREES WITH ORIGINAL SAMPLED TELEMETRY";
    case DrivingCoachIntegrityStatus::InvalidCoachReadout:
        return "COACH READOUT INVALID: WITHHOLD ADVICE";
    case DrivingCoachIntegrityStatus::CrossSourceMismatch:
        return "COACH/SOURCE COUNTERS DISAGREE: WITHHOLD ADVICE";
    case DrivingCoachIntegrityStatus::DistributionMismatch:
        return "SAMPLED SPEED PARTITION UNAVAILABLE: WITHHOLD ADVICE";
    default:return "COACH VERIFICATION UNAVAILABLE";
    }
}

struct DrivingCoachIntegrityResult final
{
    DrivingCoachIntegrityStatus Status=DrivingCoachIntegrityStatus::NoSamples;
    bool Valid=false;
    bool HasIndependentDistribution=false;
    bool AdviceAllowed=false;
    bool IndependentlyVerifiedChassis=false;
    bool OfficialTimingVerified=false;
    int SourceSamples=0;
    std::uint64_t CoachSamples=0;
    double SourceSeconds=0.0;
    double CoachSeconds=0.0;
    double SourceDistanceKm=0.0;
    double CoachDistanceKm=0.0;
    double SourceBrakingSeconds=0.0;
    double SourceSlipSeconds=0.0;
    double SourcePeakSpeedKmh=0.0;
    int SourceBrakeApplications=0;
};

inline bool DrivingCoachNumbersAgree(double a,double b) noexcept
{
    return std::isfinite(a) && std::isfinite(b) && a>=0.0 && b>=0.0 &&
        std::abs(a-b)<=1e-6*std::max({1.0,a,b});
}

inline DrivingCoachIntegrityResult VerifyDrivingCoachIntegrity(
    const DrivingSessionStats& stats,
    const DrivingCoachReadout& coach) noexcept
{
    DrivingCoachIntegrityResult result;
    result.SourceSamples=stats.GetAcceptedSamples();
    result.CoachSamples=coach.AcceptedSamples;
    result.SourceSeconds=stats.GetActiveSampledSeconds();
    result.CoachSeconds=coach.ObservedSeconds;
    result.SourceDistanceKm=stats.GetDistanceKm();
    result.CoachDistanceKm=coach.DistanceKm;
    result.SourceBrakingSeconds=stats.GetBrakingSeconds();
    result.SourceSlipSeconds=stats.GetHighSlipSeconds();
    result.SourcePeakSpeedKmh=stats.GetPeakSpeedKmH();
    result.SourceBrakeApplications=stats.GetBrakeApplications();
    if(result.SourceSamples<=0)
    {
        result.Status=DrivingCoachIntegrityStatus::NoSamples;
        return result;
    }
    if(!ValidDrivingCoachReadout(coach))
    {
        result.Status=DrivingCoachIntegrityStatus::InvalidCoachReadout;
        return result;
    }
    if(coach.AcceptedSamples!=static_cast<std::uint64_t>(result.SourceSamples) ||
       coach.AcceptedBrakeEpisodes!=
           static_cast<std::uint64_t>(result.SourceBrakeApplications) ||
       !DrivingCoachNumbersAgree(coach.ObservedSeconds,result.SourceSeconds) ||
       !DrivingCoachNumbersAgree(coach.DistanceKm,result.SourceDistanceKm) ||
       !DrivingCoachNumbersAgree(coach.BrakeSeconds,result.SourceBrakingSeconds) ||
       !DrivingCoachNumbersAgree(coach.HighSlipSeconds,result.SourceSlipSeconds) ||
       !DrivingCoachNumbersAgree(coach.PeakSpeedKmh,result.SourcePeakSpeedKmh))
    {
        result.Status=DrivingCoachIntegrityStatus::CrossSourceMismatch;
        return result;
    }
    const auto speed=stats.GetSpeedDistribution();
    if(speed.Valid)
    {
        if(speed.TotalFrames!=result.SourceSamples ||
           !DrivingCoachNumbersAgree(speed.TotalSeconds,result.SourceSeconds) ||
           !DrivingCoachNumbersAgree(speed.TotalDistanceCm/100000.0,
                result.SourceDistanceKm) ||
           !DrivingCoachNumbersAgree(speed.TotalBrakingSeconds,
                result.SourceBrakingSeconds))
        {
            result.Status=DrivingCoachIntegrityStatus::DistributionMismatch;
            return result;
        }
        result.HasIndependentDistribution=true;
    }
    const bool sufficient=coach.ObservedSeconds>=5.0 &&
        coach.AcceptedSamples>=20 &&
        coach.Evidence!=DrivingCoachEvidence::Limited;
    if(sufficient && !result.HasIndependentDistribution)
    {
        result.Status=DrivingCoachIntegrityStatus::DistributionMismatch;
        return result;
    }
    result.Valid=true;
    result.AdviceAllowed=sufficient;
    result.Status=sufficient ?
        DrivingCoachIntegrityStatus::VerifiedSourceAgreement :
        DrivingCoachIntegrityStatus::LimitedSamples;
    return result;
}

inline DrivingCoachIntegrityResult VerifyDrivingCoachIntegrity(
    const DrivingSessionStats& stats) noexcept
{
    return VerifyDrivingCoachIntegrity(stats,stats.GetCoachReview());
}
} // namespace latebrake
