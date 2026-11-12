#pragma once

#include "Telemetry/TelemetryFilters.h"
#include "Telemetry/TelemetryValidation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::engineering
{
struct KinematicEstimatorSettings final
{
    double VelocityTimeConstantSeconds = 0.08;
    double AccelerationTimeConstantSeconds = 0.12;
    double YawRateTimeConstantSeconds = 0.15;
    double MaximumAccelerationCmS2 = 8000.0;
    double MaximumJerkCmS3 = 80000.0;
    double StationarySpeedCmS = 60.0;
    double MaximumSlipRadians = 1.4;
    double MinimumCurvatureSpeedCmS = 180.0;
    double MinimumTimeSeconds = 0.001;
};

inline bool ValidKinematicEstimatorSettings(
    const KinematicEstimatorSettings& s) noexcept
{
    return Finite(s.VelocityTimeConstantSeconds) &&
           s.VelocityTimeConstantSeconds > 0.0 &&
           Finite(s.AccelerationTimeConstantSeconds) &&
           s.AccelerationTimeConstantSeconds > 0.0 &&
           Finite(s.YawRateTimeConstantSeconds) &&
           s.YawRateTimeConstantSeconds > 0.0 &&
           Finite(s.MaximumAccelerationCmS2) &&
           s.MaximumAccelerationCmS2 > 0.0 &&
           Finite(s.MaximumJerkCmS3) &&
           s.MaximumJerkCmS3 > 0.0 &&
           Finite(s.StationarySpeedCmS) &&
           s.StationarySpeedCmS >= 0.0 &&
           Finite(s.MaximumSlipRadians) &&
           s.MaximumSlipRadians > 0.0 &&
           Finite(s.MinimumCurvatureSpeedCmS) &&
           s.MinimumCurvatureSpeedCmS >= 0.0 &&
           Finite(s.MinimumTimeSeconds) &&
           s.MinimumTimeSeconds > 0.0;
}

struct MotionConsistency final
{
    double MeasuredSpeedCmS = 0.0;
    double PositionDerivedSpeedCmS = 0.0;
    double SensorDifferenceCmS = 0.0;
    double VelocityDirectionDifferenceRadians = 0.0;
    double HeadingRateErrorRadians = 0.0;
    double MeasuredAccelerationG = 0.0;
    double ReconstructedAccelerationG = 0.0;
    double AccelerationDiscrepancyG = 0.0;
    double DistanceDiscrepancyCm = 0.0;
    bool KinematicallyPlausible = false;
    bool Ready = false;
};

struct WheelMotionEstimate final
{
    double ContactWheelCount = 0.0;
    double MeanSignedWheelSpeedRadS = 0.0;
    double MeanAbsoluteWheelSpeedRadS = 0.0;
    double FastestWheelRadS = 0.0;
    double SlowestWheelRadS = 0.0;
    double MeanSlipRatio = 0.0;
    double MeanSlipAngleRadians = 0.0;
    double HighestSlipRatio = 0.0;
    double LowestNormalLoadNewtons = 0.0;
    double HighestNormalLoadNewtons = 0.0;
    double TotalNormalLoadNewtons = 0.0;
    double LeftRightLoadDifferenceNewtons = 0.0;
    double FrontRearLoadDifferenceNewtons = 0.0;
    double MeanContactTireTemperatureC = 0.0;
    double MeanTireWear = 0.0;
    bool AllWheelsContact = false;
    bool Qualified = false;
};

inline WheelMotionEstimate MeasureWheelMotion(
    const TelemetryWheels& wheels) noexcept
{
    WheelMotionEstimate result;
    double signedSpeed = 0.0;
    double absoluteSpeed = 0.0;
    double slip = 0.0;
    double angle = 0.0;
    double temperature = 0.0;
    double wear = 0.0;
    result.SlowestWheelRadS = 1.0e10;
    result.LowestNormalLoadNewtons = 1.0e10;
    for (int i = 0; i < 4; ++i)
    {
        const std::size_t n = static_cast<std::size_t>(i);
        if (!wheels.Contact[n])
        {
            continue;
        }
        const double ws = wheels.AngularSpeedRadS[n];
        const double load = wheels.NormalLoadNewtons[n];
        if (!Finite(ws) || !Finite(load) ||
            !Finite(wheels.SlipRatio[n]) ||
            !Finite(wheels.SlipAngleRadians[n]) ||
            !Finite(wheels.TireTemperatureCelsius[n]) ||
            !Finite(wheels.TireWearFraction[n]))
        {
            return {};
        }
        result.ContactWheelCount += 1.0;
        signedSpeed += ws;
        absoluteSpeed += std::abs(ws);
        slip += wheels.SlipRatio[n];
        angle += wheels.SlipAngleRadians[n];
        temperature += wheels.TireTemperatureCelsius[n];
        wear += wheels.TireWearFraction[n];
        result.HighestSlipRatio = std::max(
            result.HighestSlipRatio, std::abs(wheels.SlipRatio[n]));
        result.FastestWheelRadS = std::max(
            result.FastestWheelRadS, std::abs(ws));
        result.SlowestWheelRadS = std::min(
            result.SlowestWheelRadS, std::abs(ws));
        result.LowestNormalLoadNewtons = std::min(
            result.LowestNormalLoadNewtons, load);
        result.HighestNormalLoadNewtons = std::max(
            result.HighestNormalLoadNewtons, load);
        result.TotalNormalLoadNewtons += load;
        if (i == 0 || i == 2)
        {
            result.LeftRightLoadDifferenceNewtons += load;
        }
        else
        {
            result.LeftRightLoadDifferenceNewtons -= load;
        }
        if (i < 2)
        {
            result.FrontRearLoadDifferenceNewtons += load;
        }
        else
        {
            result.FrontRearLoadDifferenceNewtons -= load;
        }
    }
    if (result.ContactWheelCount < 1.0)
    {
        return {};
    }
    const double inv = 1.0 / result.ContactWheelCount;
    result.MeanSignedWheelSpeedRadS = signedSpeed * inv;
    result.MeanAbsoluteWheelSpeedRadS = absoluteSpeed * inv;
    result.MeanSlipRatio = slip * inv;
    result.MeanSlipAngleRadians = angle * inv;
    result.MeanContactTireTemperatureC = temperature * inv;
    result.MeanTireWear = wear * inv;
    result.AllWheelsContact = result.ContactWheelCount == 4.0;
    result.Qualified = Finite(result.TotalNormalLoadNewtons) &&
        result.TotalNormalLoadNewtons >= 0.0;
    return result;
}

class TelemetryKinematicEstimator final
{
public:
    void Reset() noexcept
    {
        VelocityFilter.Reset();
        AccelerationFilter.Reset();
        YawFilter.Reset();
        YawRateFilter.Reset();
        SpeedNoise.Reset();
        AccelerationNoise.Reset();
        PreviousPosition = {};
        PreviousVelocity = {};
        PreviousYaw = 0.0;
        PreviousAccelerationCmS2 = 0.0;
        TotalDistanceCm = 0.0;
        AcceptedFrames = 0;
        HasPrevious = false;
        Output = {};
        LastConsistency = {};
    }

    bool Configure(KinematicEstimatorSettings settings) noexcept
    {
        if (!ValidKinematicEstimatorSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    bool Observe(const TelemetryFrame& frame,
                 const TelemetryFrameDecision& gate) noexcept
    {
        if (!gate.Accepted || !Finite(gate.ValidatedDeltaSeconds) ||
            gate.ValidatedDeltaSeconds < Settings.MinimumTimeSeconds ||
            gate.ValidatedDeltaSeconds > MaximumFrameDeltaSeconds)
        {
            Reset();
            return false;
        }
        if (gate.StartNewSegment || gate.ResetFilters)
        {
            Reset();
        }
        const double dt = gate.ValidatedDeltaSeconds;
        const Vector2 position = PlanarPosition(frame);
        const Vector2 measuredVelocity = PlanarVelocity(frame);
        const Vector2 measuredAcceleration = PlanarAcceleration(frame);
        const double heading = WrapRadians(frame.Dynamics.YawRadians);
        if (!Finite(position) || !Finite(measuredVelocity) ||
            !Finite(measuredAcceleration) || !Finite(heading))
        {
            Reset();
            return false;
        }
        const bool hadPrevious = HasPrevious;
        Vector2 derivedVelocity = measuredVelocity;
        Vector2 derivedAcceleration = measuredAcceleration;
        double derivedYawRate = frame.Dynamics.AngularVelocityRadS.Z;
        double previousLongitudinalAcceleration =
            PreviousAccelerationCmS2;
        if (hadPrevious)
        {
            derivedVelocity = (position - PreviousPosition) / dt;
            derivedAcceleration =
                (measuredVelocity - PreviousVelocity) / dt;
            derivedYawRate = AngleDifference(PreviousYaw, heading) / dt;
        }
        if (!Finite(derivedVelocity) ||
            !Finite(derivedAcceleration) ||
            !Finite(derivedYawRate))
        {
            Reset();
            return false;
        }
        if (Length(derivedAcceleration) >
            Settings.MaximumAccelerationCmS2 * 2.0)
        {
            derivedAcceleration =
                NormalizeOrZero(derivedAcceleration) *
                Settings.MaximumAccelerationCmS2;
        }
        const Vector2 filteredVelocity = VelocityFilter.Observe(
            measuredVelocity, dt, Settings.VelocityTimeConstantSeconds);
        const Vector2 filteredAcceleration = AccelerationFilter.Observe(
            measuredAcceleration, dt,
            Settings.AccelerationTimeConstantSeconds);
        const double filteredHeading = YawFilter.Observe(
            heading, dt, Settings.YawRateTimeConstantSeconds);
        const double filteredYawRate = YawRateFilter.Observe(
            derivedYawRate, dt, Settings.YawRateTimeConstantSeconds);
        const Vector2 headingForward = HeadingUnit(filteredHeading);
        const Vector2 headingLeft = PerpendicularLeft(headingForward);
        const double speed = Length(filteredVelocity);
        const double rawSpeed = Length(measuredVelocity);
        const double longitudinal =
            Dot(filteredAcceleration, headingForward);
        const double lateral =
            Dot(filteredAcceleration, headingLeft);
        const double slip = speed >= Settings.StationarySpeedCmS ?
            SlipAngle(filteredVelocity, filteredHeading) : 0.0;
        const double jerk = hadPrevious ?
            Jerk(previousLongitudinalAcceleration, longitudinal, dt) : 0.0;
        const double boundedJerk = std::clamp(
            jerk, -Settings.MaximumJerkCmS3,
            Settings.MaximumJerkCmS3);
        if (hadPrevious)
        {
            const double segmentDistance =
                Distance(PreviousPosition, position);
            TotalDistanceCm = SaturatingAdd(
                TotalDistanceCm, segmentDistance);
        }
        Output.PositionCm = position;
        Output.VelocityCmS = measuredVelocity;
        Output.AccelerationCmS2 = measuredAcceleration;
        Output.SmoothedVelocityCmS = filteredVelocity;
        Output.SmoothedAccelerationCmS2 = filteredAcceleration;
        Output.SpeedCmS = speed;
        Output.SpeedKmh = speed * CmPerSecondToKmh;
        Output.ForwardSpeedCmS = Dot(filteredVelocity, headingForward);
        Output.LateralSpeedCmS = Dot(filteredVelocity, headingLeft);
        Output.LongitudinalAccelerationCmS2 = longitudinal;
        Output.LateralAccelerationCmS2 = lateral;
        Output.YawRadians = filteredHeading;
        Output.YawRateRadS = filteredYawRate;
        Output.SlipAngleRadians = std::clamp(
            slip, -Settings.MaximumSlipRadians,
            Settings.MaximumSlipRadians);
        Output.SlipAngleDegrees =
            Output.SlipAngleRadians * 180.0 / Pi;
        Output.EstimatedCurvature =
            rawSpeed >= Settings.MinimumCurvatureSpeedCmS ?
            filteredYawRate / rawSpeed : 0.0;
        Output.JerkCmS3 = boundedJerk;
        Output.DistanceCm = TotalDistanceCm;
        Output.Valid = true;

        MotionConsistency consistency;
        consistency.MeasuredSpeedCmS = rawSpeed;
        consistency.PositionDerivedSpeedCmS =
            Length(derivedVelocity);
        consistency.SensorDifferenceCmS =
            std::abs(consistency.PositionDerivedSpeedCmS - rawSpeed);
        consistency.VelocityDirectionDifferenceRadians =
            AngleBetween(measuredVelocity, derivedVelocity);
        consistency.HeadingRateErrorRadians =
            std::abs(filteredYawRate -
                     frame.Dynamics.AngularVelocityRadS.Z) * dt;
        consistency.MeasuredAccelerationG =
            Length(measuredAcceleration) / GravityCmS2;
        consistency.ReconstructedAccelerationG =
            Length(derivedAcceleration) / GravityCmS2;
        consistency.AccelerationDiscrepancyG =
            Length(derivedAcceleration - measuredAcceleration) /
            GravityCmS2;
        consistency.DistanceDiscrepancyCm = hadPrevious ?
            std::abs(Distance(PreviousPosition, position) -
                     rawSpeed * dt) : 0.0;
        consistency.KinematicallyPlausible =
            consistency.SensorDifferenceCmS < 1600.0 &&
            consistency.AccelerationDiscrepancyG < 7.0 &&
            consistency.DistanceDiscrepancyCm < 180.0;
        consistency.Ready = hadPrevious;
        LastConsistency = consistency;

        SpeedNoise.Observe(rawSpeed);
        AccelerationNoise.Observe(
            Length(measuredAcceleration) / GravityCmS2);
        PreviousPosition = position;
        PreviousVelocity = measuredVelocity;
        PreviousYaw = heading;
        PreviousAccelerationCmS2 = longitudinal;
        HasPrevious = true;
        AcceptedFrames = SaturatingIncrement(AcceptedFrames);
        return true;
    }

    TelemetryKinematics Snapshot() const noexcept
    {
        return Output;
    }

    MotionConsistency Consistency() const noexcept
    {
        return LastConsistency;
    }

    double TravelledDistanceCm() const noexcept
    {
        return TotalDistanceCm;
    }

    double AverageSensorSpeedCmS() const noexcept
    {
        return SpeedNoise.Mean();
    }

    double SensorSpeedDeviationCmS() const noexcept
    {
        return SpeedNoise.Deviation();
    }

    double AverageRawAccelerationG() const noexcept
    {
        return AccelerationNoise.Mean();
    }

    std::uint64_t QualifiedFrames() const noexcept
    {
        return AcceptedFrames;
    }

private:
    KinematicEstimatorSettings Settings{};
    VectorFilter VelocityFilter;
    VectorFilter AccelerationFilter;
    AngularFilter YawFilter;
    ExponentialFilter YawRateFilter;
    RunningStatistics SpeedNoise;
    RunningStatistics AccelerationNoise;
    Vector2 PreviousPosition{};
    Vector2 PreviousVelocity{};
    double PreviousYaw = 0.0;
    double PreviousAccelerationCmS2 = 0.0;
    double TotalDistanceCm = 0.0;
    std::uint64_t AcceptedFrames = 0;
    bool HasPrevious = false;
    TelemetryKinematics Output{};
    MotionConsistency LastConsistency{};
};

class MotionAnomalyMonitor final
{
public:
    void Reset() noexcept
    {
        Rejected = 0;
        Suspect = 0;
        Stable = 0;
        MaximumDiscrepancyCmS = 0.0;
        LongestSuspectSeconds = 0.0;
        CurrentSuspectSeconds = 0.0;
        RecentDiscrepancy.Reset();
    }

    bool Observe(MotionConsistency check, double dt) noexcept
    {
        if (!Finite(dt) || dt <= 0.0 || dt > 1.0 ||
            !check.Ready || !Finite(check.SensorDifferenceCmS) ||
            !Finite(check.AccelerationDiscrepancyG))
        {
            Rejected = SaturatingIncrement(Rejected);
            CurrentSuspectSeconds = 0.0;
            return false;
        }
        RecentDiscrepancy.Push(check.SensorDifferenceCmS);
        MaximumDiscrepancyCmS = std::max(
            MaximumDiscrepancyCmS, check.SensorDifferenceCmS);
        if (!check.KinematicallyPlausible)
        {
            Suspect = SaturatingIncrement(Suspect);
            CurrentSuspectSeconds += dt;
            LongestSuspectSeconds = std::max(
                LongestSuspectSeconds, CurrentSuspectSeconds);
            return false;
        }
        Stable = SaturatingIncrement(Stable);
        CurrentSuspectSeconds = 0.0;
        return true;
    }

    std::uint64_t StableFrames() const noexcept { return Stable; }
    std::uint64_t SuspectFrames() const noexcept { return Suspect; }
    std::uint64_t RejectedFrames() const noexcept { return Rejected; }
    double MaxSpeedDiscrepancyCmS() const noexcept
    {
        return MaximumDiscrepancyCmS;
    }
    double LongestSuspectPeriodSeconds() const noexcept
    {
        return LongestSuspectSeconds;
    }
    double MedianRecentDiscrepancyCmS() const noexcept
    {
        return RecentDiscrepancy.Percentile(0.5);
    }

private:
    std::uint64_t Rejected = 0;
    std::uint64_t Suspect = 0;
    std::uint64_t Stable = 0;
    double MaximumDiscrepancyCmS = 0.0;
    double LongestSuspectSeconds = 0.0;
    double CurrentSuspectSeconds = 0.0;
    RollingWindow<25> RecentDiscrepancy;
};
} // namespace latebrake::engineering
