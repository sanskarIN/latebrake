#pragma once

namespace latebrake
{
// Advisory, run-local driving metrics. Never changes physics, credits,
// checkpoints, lap results, save data, or any authoritative race clock.
struct DrivingSample final
{
    double DeltaSeconds = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double LateralSpeedCmPerSecond = 0.0;
    bool Braking = false;
};

class DrivingSessionStats final
{
public:
    void Reset() noexcept;
    void Suspend() noexcept;
    bool Observe(DrivingSample sample) noexcept;

    double GetDistanceKm() const noexcept { return DistanceCm / 100000.0; }
    double GetPeakSpeedKmH() const noexcept { return PeakSpeedKmH; }
    double GetActiveSampledSeconds() const noexcept { return ActiveSeconds; }
    double GetBrakingSeconds() const noexcept { return BrakingSeconds; }
    double GetHighSlipSeconds() const noexcept { return HighSlipSeconds; }
    int GetBrakeApplications() const noexcept { return BrakeApplications; }
    int GetAcceptedSamples() const noexcept { return AcceptedSamples; }

private:
    double DistanceCm = 0.0;
    double PeakSpeedKmH = 0.0;
    double ActiveSeconds = 0.0;
    double BrakingSeconds = 0.0;
    double HighSlipSeconds = 0.0;
    int BrakeApplications = 0;
    int AcceptedSamples = 0;
    bool PreviousBraking = false;
};
} // namespace latebrake
