#pragma once

#include "Racing/DrivingSessionStats.h"

#include <cmath>

namespace latebrake
{
// Grounded chassis samples, not official distance, lap telemetry, a driver
// rating or a prediction. Active time may omit invalid/hitchy frames.
struct SampledDrivingReview final
{
    bool Valid = false;
    int AcceptedFrames = 0;
    int BrakeApplications = 0;
    double EstimatedDistanceKm = 0.0;
    double SampledSeconds = 0.0;
    double PeakSpeedKmH = 0.0;
    double BrakingPercent = 0.0;
    double HighLateralSlipPercent = 0.0;
};

inline SampledDrivingReview BuildSampledDrivingReview(
    const DrivingSessionStats& stats) noexcept
{
    SampledDrivingReview result;
    const int frames = stats.GetAcceptedSamples();
    const int brakes = stats.GetBrakeApplications();
    const double time = stats.GetActiveSampledSeconds();
    const double braking = stats.GetBrakingSeconds();
    const double slipping = stats.GetHighSlipSeconds();
    const double distance = stats.GetDistanceKm();
    const double peak = stats.GetPeakSpeedKmH();
    if (frames < 2 || brakes < 0 || brakes > frames ||
        !std::isfinite(time) || time < 0.25 || time > 10000000.0 ||
        !std::isfinite(braking) || braking < 0.0 || braking > time ||
        !std::isfinite(slipping) || slipping < 0.0 || slipping > time ||
        !std::isfinite(distance) || distance < 0.0 || distance > 10000.0 ||
        !std::isfinite(peak) || peak < 0.0 || peak > 360.0)
    {
        return result;
    }
    result.Valid = true;
    result.AcceptedFrames = frames;
    result.BrakeApplications = brakes;
    result.EstimatedDistanceKm = distance;
    result.SampledSeconds = time;
    result.PeakSpeedKmH = peak;
    result.BrakingPercent = 100.0 * braking / time;
    result.HighLateralSlipPercent = 100.0 * slipping / time;
    return result;
}
} // namespace latebrake
