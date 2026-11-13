#pragma once

#include "Telemetry/TelemetryTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
struct TelemetryGateSettings final
{
    TelemetryTrust MinimumTrust = TelemetryTrust::EngineObserved;
    double MaximumSpeedCmS = MaximumPlanarSpeedCmS;
    double MaximumAccelerationCmS2 = 8000.0;
    double MaximumAngularSpeedRadS = 25.0;
    double MaximumFrameGapSeconds = MaximumFrameDeltaSeconds;
    double MaximumWorldJumpCm = 650.0;
    double MaximumSpeedDiscrepancyCmS = 1250.0;
    double WorldToleranceCm = 80.0;
    bool RequireRaceActive = true;
    bool RequireWheelData = false;
    bool RequireOnPavement = false;
    bool RequireNewOrdinal = true;
};

inline bool ValidTelemetryGateSettings(
    const TelemetryGateSettings& s) noexcept
{
    return Finite(s.MaximumSpeedCmS) &&
           s.MaximumSpeedCmS > 0.0 &&
           s.MaximumSpeedCmS <= 30000.0 &&
           Finite(s.MaximumAccelerationCmS2) &&
           s.MaximumAccelerationCmS2 > 0.0 &&
           Finite(s.MaximumAngularSpeedRadS) &&
           s.MaximumAngularSpeedRadS > 0.0 &&
           Finite(s.MaximumFrameGapSeconds) &&
           s.MaximumFrameGapSeconds > 0.0 &&
           s.MaximumFrameGapSeconds <= 1.0 &&
           Finite(s.MaximumWorldJumpCm) &&
           s.MaximumWorldJumpCm > 0.0 &&
           Finite(s.MaximumSpeedDiscrepancyCmS) &&
           s.MaximumSpeedDiscrepancyCmS >= 0.0 &&
           Finite(s.WorldToleranceCm) &&
           s.WorldToleranceCm >= 0.0;
}

inline bool ValidTelemetryControls(
    const TelemetryControls& c) noexcept
{
    return Finite(c.Throttle) &&
           Finite(c.Brake) &&
           Finite(c.Steering) &&
           Finite(c.Handbrake) &&
           Finite(c.Clutch) &&
           c.Throttle >= 0.0 && c.Throttle <= 1.0 &&
           c.Brake >= 0.0 && c.Brake <= 1.0 &&
           c.Steering >= -1.0 && c.Steering <= 1.0 &&
           c.Handbrake >= 0.0 && c.Handbrake <= 1.0 &&
           c.Clutch >= 0.0 && c.Clutch <= 1.0 &&
           c.Gear >= -1 && c.Gear <= 12;
}

inline bool ValidTelemetryDynamics(
    const TelemetryDynamics& d,
    const TelemetryGateSettings& s) noexcept
{
    if (!Finite(d.WorldPositionCm) ||
        !Finite(d.WorldVelocityCmS) ||
        !Finite(d.WorldAccelerationCmS2) ||
        !Finite(d.AngularVelocityRadS) ||
        !Finite(d.YawRadians) ||
        !Finite(d.PitchRadians) ||
        !Finite(d.RollRadians) ||
        !Finite(d.EngineRpm) ||
        !Finite(d.MotorTorqueNm) ||
        !Finite(d.BrakeTemperatureCelsius) ||
        !Finite(d.EngineTemperatureCelsius) ||
        !Finite(d.FuelFraction) ||
        !Finite(d.ChassisDamageFraction) ||
        !Finite(d.GripScale) ||
        !Finite(d.RoadSlopeRadians) ||
        !Finite(d.SuspensionCompressionCm) ||
        !Finite(d.RideHeightCm))
    {
        return false;
    }
    if (Length(d.WorldPositionCm) > MaximumWorldMagnitudeCm ||
        Length(d.WorldVelocityCmS) > s.MaximumSpeedCmS ||
        Length(d.WorldAccelerationCmS2) > s.MaximumAccelerationCmS2 ||
        Length(d.AngularVelocityRadS) > s.MaximumAngularSpeedRadS)
    {
        return false;
    }
    if (std::abs(d.PitchRadians) > Pi ||
        std::abs(d.RollRadians) > Pi ||
        std::abs(d.RoadSlopeRadians) > Pi / 2.0 ||
        d.EngineRpm < 0.0 || d.EngineRpm > 30000.0 ||
        std::abs(d.MotorTorqueNm) > 20000.0 ||
        d.BrakeTemperatureCelsius < -100.0 ||
        d.BrakeTemperatureCelsius > 2000.0 ||
        d.EngineTemperatureCelsius < -100.0 ||
        d.EngineTemperatureCelsius > 1000.0 ||
        d.FuelFraction < 0.0 || d.FuelFraction > 1.0 ||
        d.ChassisDamageFraction < 0.0 ||
        d.ChassisDamageFraction > 1.0 ||
        d.GripScale < 0.0 || d.GripScale > 4.0 ||
        d.SuspensionCompressionCm < -200.0 ||
        d.SuspensionCompressionCm > 300.0 ||
        d.RideHeightCm < -200.0 ||
        d.RideHeightCm > 600.0)
    {
        return false;
    }
    return true;
}

inline bool ValidTelemetryWheels(const TelemetryWheels& w) noexcept
{
    for (int i = 0; i < 4; ++i)
    {
        const std::size_t n = static_cast<std::size_t>(i);
        if (!Finite(w.AngularSpeedRadS[n]) ||
            !Finite(w.NormalLoadNewtons[n]) ||
            !Finite(w.SlipRatio[n]) ||
            !Finite(w.SlipAngleRadians[n]) ||
            !Finite(w.TireTemperatureCelsius[n]) ||
            !Finite(w.TireWearFraction[n]))
        {
            return false;
        }
        if (std::abs(w.AngularSpeedRadS[n]) > 1500.0 ||
            w.NormalLoadNewtons[n] < 0.0 ||
            w.NormalLoadNewtons[n] > 250000.0 ||
            std::abs(w.SlipRatio[n]) > 12.0 ||
            std::abs(w.SlipAngleRadians[n]) > Pi ||
            w.TireTemperatureCelsius[n] < -100.0 ||
            w.TireTemperatureCelsius[n] > 300.0 ||
            w.TireWearFraction[n] < 0.0 ||
            w.TireWearFraction[n] > 1.0)
        {
            return false;
        }
    }
    return true;
}

inline bool ValidTelemetryEnvironment(
    const TelemetryEnvironment& e) noexcept
{
    return Finite(e.AirTemperatureCelsius) &&
           Finite(e.TrackTemperatureCelsius) &&
           Finite(e.AmbientWetness) &&
           Finite(e.WindSpeedCmS) &&
           Finite(e.WindDirectionRadians) &&
           Finite(e.VisibilityFraction) &&
           Finite(e.RoadFrictionEstimate) &&
           e.AirTemperatureCelsius >= -100.0 &&
           e.AirTemperatureCelsius <= 90.0 &&
           e.TrackTemperatureCelsius >= -100.0 &&
           e.TrackTemperatureCelsius <= 150.0 &&
           e.AmbientWetness >= 0.0 &&
           e.AmbientWetness <= 1.0 &&
           e.WindSpeedCmS >= 0.0 &&
           e.WindSpeedCmS <= 10000.0 &&
           e.VisibilityFraction >= 0.0 &&
           e.VisibilityFraction <= 1.0 &&
           e.RoadFrictionEstimate >= 0.0 &&
           e.RoadFrictionEstimate <= 4.0;
}

inline bool ValidTelemetryRaceStatus(
    const TelemetryRaceStatus& race) noexcept
{
    return race.LapIndex >= 0 &&
           race.LapIndex <= MaximumTelemetryLaps &&
           race.NextGateIndex >= 0 &&
           race.NextGateIndex <= 256 &&
           race.CreditedGateOrdinal >= 0 &&
           race.CreditedGateOrdinal <= 32768 &&
           race.PositionOrdinal >= 0 &&
           race.PositionOrdinal <= MaximumTelemetryDrivers &&
           race.SectorIndex >= 0 &&
           race.SectorIndex <= MaximumTelemetrySectors &&
           Finite(race.CurrentLapSeconds) &&
           Finite(race.LastLapSeconds) &&
           Finite(race.RemainingDistanceCm) &&
           race.CurrentLapSeconds >= 0.0 &&
           race.LastLapSeconds >= 0.0 &&
           race.RemainingDistanceCm >= 0.0;
}

inline TelemetryReject ValidateFrameFields(
    const TelemetryFrame& f,
    const TelemetryGateSettings& s) noexcept
{
    if (!ValidTelemetryGateSettings(s))
    {
        return TelemetryReject::InvalidDynamics;
    }
    if (!f.Race.EnginePhysicsReady ||
        static_cast<int>(f.Identity.Trust) <
            static_cast<int>(s.MinimumTrust))
    {
        return TelemetryReject::MissingPhysics;
    }
    if (f.Identity.SessionToken == 0 ||
        f.Identity.ActorToken == 0 ||
        f.Identity.SessionRevision < 0)
    {
        return TelemetryReject::WrongSession;
    }
    if (f.Identity.DriverSlot < 0 ||
        f.Identity.DriverSlot >= MaximumTelemetryDrivers)
    {
        return TelemetryReject::WrongDriver;
    }
    if (!Finite(f.MonotonicSeconds) ||
        !Finite(f.RaceClockSeconds) ||
        !Finite(f.DeltaSeconds) ||
        !Finite(f.SpeedometerKmH) ||
        !Finite(f.InputLatencySeconds) ||
        f.MonotonicSeconds < 0.0 ||
        f.MonotonicSeconds > MaximumTelemetrySeconds ||
        f.RaceClockSeconds < 0.0 ||
        f.RaceClockSeconds > MaximumTelemetrySeconds ||
        f.DeltaSeconds <= 0.0 ||
        f.DeltaSeconds > s.MaximumFrameGapSeconds ||
        f.InputLatencySeconds < 0.0 ||
        f.InputLatencySeconds > 3.0 ||
        f.SpeedometerKmH < 0.0 ||
        f.SpeedometerKmH > 600.0)
    {
        return TelemetryReject::InvalidClock;
    }
    if (s.RequireRaceActive && !IsRaceMoving(f.Race.Phase))
    {
        return TelemetryReject::InvalidPhase;
    }
    if (!ValidTelemetryRaceStatus(f.Race))
    {
        return TelemetryReject::InvalidPhase;
    }
    if (!Finite(f.Dynamics.WorldPositionCm) ||
        Length(f.Dynamics.WorldPositionCm) > MaximumWorldMagnitudeCm)
    {
        return TelemetryReject::InvalidPosition;
    }
    if (!Finite(f.Dynamics.WorldVelocityCmS) ||
        Length(f.Dynamics.WorldVelocityCmS) > s.MaximumSpeedCmS)
    {
        return TelemetryReject::InvalidVelocity;
    }
    if (!Finite(f.Dynamics.YawRadians) ||
        !Finite(f.Dynamics.PitchRadians) ||
        !Finite(f.Dynamics.RollRadians) ||
        !Finite(f.Dynamics.AngularVelocityRadS))
    {
        return TelemetryReject::InvalidAttitude;
    }
    if (!ValidTelemetryControls(f.Controls))
    {
        return TelemetryReject::InvalidControls;
    }
    if (!ValidTelemetryDynamics(f.Dynamics, s))
    {
        return TelemetryReject::InvalidDynamics;
    }
    if (!ValidTelemetryEnvironment(f.Environment))
    {
        return TelemetryReject::InvalidEnvironment;
    }
    if (s.RequireOnPavement && !f.Environment.OnPhysicalPavement)
    {
        return TelemetryReject::InvalidEnvironment;
    }
    if (!ValidTelemetryWheels(f.Wheels))
    {
        return TelemetryReject::InvalidWheelReadout;
    }
    if (s.RequireWheelData && WheelContactFraction(f.Wheels) <= 0.0)
    {
        return TelemetryReject::InvalidWheelReadout;
    }
    return TelemetryReject::None;
}

class TelemetryFrameGate final
{
public:
    explicit TelemetryFrameGate(
        TelemetryGateSettings settings = {}) noexcept
        : Settings(settings)
    {
    }

    void Reset() noexcept
    {
        Previous = {};
        Counters = {};
        HasPrevious = false;
        HasSession = false;
        ActiveSessionToken = 0;
        ActiveActorToken = 0;
        ActiveRevision = -1;
        ActiveDriverSlot = -1;
    }

    void Suspend() noexcept
    {
        HasPrevious = false;
        Counters.InactiveFrames =
            SaturatingIncrement(Counters.InactiveFrames);
    }

    bool SetSettings(TelemetryGateSettings settings) noexcept
    {
        if (!ValidTelemetryGateSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    TelemetryFrameDecision Observe(
        const TelemetryFrame& frame) noexcept
    {
        Counters.Observed = SaturatingIncrement(Counters.Observed);
        const auto reject = [this](
            TelemetryReject reason, bool newSegment = false) noexcept
        {
            Counters.Rejected = SaturatingIncrement(Counters.Rejected);
            const std::size_t index = TelemetryRejectIndex(reason);
            if (index < Counters.RejectReasons.size())
            {
                Counters.RejectReasons[index] =
                    SaturatingIncrement(Counters.RejectReasons[index]);
            }
            if (reason == TelemetryReject::InvalidDynamics ||
                reason == TelemetryReject::InvalidWheelReadout ||
                reason == TelemetryReject::InvalidVelocity)
            {
                Counters.SensorFaults =
                    SaturatingIncrement(Counters.SensorFaults);
            }
            if (reason == TelemetryReject::ImpossibleJump ||
                reason == TelemetryReject::ClockReversed)
            {
                Counters.Discontinuities =
                    SaturatingIncrement(Counters.Discontinuities);
            }
            HasPrevious = false;
            return TelemetryFrameDecision{
                TelemetryQuality::Invalid, reason, false,
                newSegment, true, 0.0, 0.0, 0.0
            };
        };

        const TelemetryReject fields =
            ValidateFrameFields(frame, Settings);
        if (fields != TelemetryReject::None)
        {
            return reject(fields);
        }
        if (HasSession &&
            (frame.Identity.SessionToken != ActiveSessionToken ||
             frame.Identity.ActorToken != ActiveActorToken ||
             frame.Identity.DriverSlot != ActiveDriverSlot ||
             frame.Identity.SessionRevision != ActiveRevision))
        {
            Counters.EpochResets =
                SaturatingIncrement(Counters.EpochResets);
            HasSession = true;
            ActiveSessionToken = frame.Identity.SessionToken;
            ActiveActorToken = frame.Identity.ActorToken;
            ActiveDriverSlot = frame.Identity.DriverSlot;
            ActiveRevision = frame.Identity.SessionRevision;
            Previous = frame;
            HasPrevious = true;
            Counters.Accepted = SaturatingIncrement(Counters.Accepted);
            return {
                TelemetryQuality::Qualified, TelemetryReject::None,
                true, true, true, frame.DeltaSeconds, 0.0, 0.0
            };
        }
        if (!HasSession)
        {
            HasSession = true;
            ActiveSessionToken = frame.Identity.SessionToken;
            ActiveActorToken = frame.Identity.ActorToken;
            ActiveDriverSlot = frame.Identity.DriverSlot;
            ActiveRevision = frame.Identity.SessionRevision;
        }

        bool startSegment = !HasPrevious;
        double elapsed = frame.DeltaSeconds;
        double displacement = 0.0;
        double yawDelta = 0.0;
        if (HasPrevious)
        {
            if (Settings.RequireNewOrdinal &&
                frame.Identity.FrameOrdinal <= Previous.Identity.FrameOrdinal)
            {
                return reject(TelemetryReject::StaleObservation);
            }
            if (frame.MonotonicSeconds <= Previous.MonotonicSeconds ||
                frame.RaceClockSeconds < Previous.RaceClockSeconds)
            {
                Counters.ClockResets =
                    SaturatingIncrement(Counters.ClockResets);
                return reject(TelemetryReject::ClockReversed, true);
            }
            elapsed = frame.MonotonicSeconds - Previous.MonotonicSeconds;
            if (!Finite(elapsed) || elapsed <= 0.0 ||
                elapsed > Settings.MaximumFrameGapSeconds)
            {
                return reject(TelemetryReject::InvalidClock, true);
            }
            const Vector2 p = PlanarPosition(frame);
            const Vector2 previousP = PlanarPosition(Previous);
            displacement = Distance(p, previousP);
            yawDelta = AngleDifference(
                Previous.Dynamics.YawRadians, frame.Dynamics.YawRadians);
            const double speed = Length(PlanarVelocity(frame));
            const double priorSpeed = Length(PlanarVelocity(Previous));
            const double permitted = std::max(
                Settings.MaximumWorldJumpCm,
                std::max(speed, priorSpeed) * elapsed +
                    Settings.WorldToleranceCm);
            if (!Finite(displacement) || displacement > permitted)
            {
                return reject(TelemetryReject::ImpossibleJump, true);
            }
            const double impliedSpeed = displacement / elapsed;
            if (!Finite(impliedSpeed) ||
                impliedSpeed > Settings.MaximumSpeedCmS +
                    Settings.MaximumSpeedDiscrepancyCmS)
            {
                return reject(TelemetryReject::ImpossibleJump, true);
            }
            if (std::abs(yawDelta) >
                Settings.MaximumAngularSpeedRadS * elapsed +
                0.20)
            {
                return reject(TelemetryReject::InvalidAttitude, true);
            }
        }
        Previous = frame;
        HasPrevious = true;
        Counters.Accepted = SaturatingIncrement(Counters.Accepted);
        return {
            TelemetryQuality::Qualified, TelemetryReject::None,
            true, startSegment, startSegment, elapsed,
            displacement, yawDelta
        };
    }

    bool HasQualifiedPrevious() const noexcept
    {
        return HasPrevious;
    }

    const TelemetryFrame& LastAcceptedFrame() const noexcept
    {
        return Previous;
    }

    TelemetryCounters Snapshot() const noexcept
    {
        return Counters;
    }

    const TelemetryGateSettings& GetSettings() const noexcept
    {
        return Settings;
    }

    std::uint64_t CurrentSession() const noexcept
    {
        return ActiveSessionToken;
    }

    std::uint64_t CurrentActor() const noexcept
    {
        return ActiveActorToken;
    }

    int CurrentRevision() const noexcept
    {
        return ActiveRevision;
    }

private:
    TelemetryGateSettings Settings{};
    TelemetryFrame Previous{};
    TelemetryCounters Counters{};
    bool HasPrevious = false;
    bool HasSession = false;
    std::uint64_t ActiveSessionToken = 0;
    std::uint64_t ActiveActorToken = 0;
    int ActiveRevision = -1;
    int ActiveDriverSlot = -1;
};
} // namespace latebrake::engineering
