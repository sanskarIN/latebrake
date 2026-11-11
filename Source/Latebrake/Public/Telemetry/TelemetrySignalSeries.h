#pragma once

#include "Telemetry/TelemetryFilters.h"
#include "Telemetry/TelemetryTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class SignalKind : std::uint8_t
{
    Speed,
    Throttle,
    Brake,
    Steering,
    YawRate,
    LongitudinalAcceleration,
    LateralAcceleration,
    SlipAngle,
    EngineRpm,
    BrakeTemperature,
    EngineTemperature,
    TireTemperature,
    TireWear,
    Fuel,
    TrackWetness,
    RoadFriction,
    GripScale,
    ChassisDamage,
    Unknown
};

struct SignalSample final
{
    double TimeSeconds = 0.0;
    double Value = 0.0;
    double WeightSeconds = 0.0;
    std::uint64_t SourceOrdinal = 0;
    bool Qualified = false;
};

struct SignalSummary final
{
    double Mean = 0.0;
    double Minimum = 0.0;
    double Maximum = 0.0;
    double StandardDeviation = 0.0;
    double Integral = 0.0;
    double AbsoluteIntegral = 0.0;
    double TotalVariation = 0.0;
    double MaximumRate = 0.0;
    double MinimumRate = 0.0;
    double TimeWeightedMean = 0.0;
    double RootMeanSquare = 0.0;
    double BeginningSeconds = 0.0;
    double EndingSeconds = 0.0;
    std::size_t Samples = 0;
    bool Valid = false;
};

inline double ReadTelemetrySignal(
    const TelemetryFrame& frame,
    SignalKind kind) noexcept
{
    switch (kind)
    {
    case SignalKind::Speed:
        return Length(Vector2{
            frame.Dynamics.WorldVelocityCmS.X,
            frame.Dynamics.WorldVelocityCmS.Y});
    case SignalKind::Throttle:
        return frame.Controls.Throttle;
    case SignalKind::Brake:
        return frame.Controls.Brake;
    case SignalKind::Steering:
        return frame.Controls.Steering;
    case SignalKind::YawRate:
        return frame.Dynamics.AngularVelocityRadS.Z;
    case SignalKind::LongitudinalAcceleration:
        return Dot(
            Vector2{
                frame.Dynamics.WorldAccelerationCmS2.X,
                frame.Dynamics.WorldAccelerationCmS2.Y},
            HeadingUnit(frame.Dynamics.YawRadians));
    case SignalKind::LateralAcceleration:
        return Cross(
            HeadingUnit(frame.Dynamics.YawRadians),
            Vector2{
                frame.Dynamics.WorldAccelerationCmS2.X,
                frame.Dynamics.WorldAccelerationCmS2.Y});
    case SignalKind::SlipAngle:
        return SlipAngle(
            Vector2{
                frame.Dynamics.WorldVelocityCmS.X,
                frame.Dynamics.WorldVelocityCmS.Y},
            frame.Dynamics.YawRadians);
    case SignalKind::EngineRpm:
        return frame.Dynamics.EngineRpm;
    case SignalKind::BrakeTemperature:
        return frame.Dynamics.BrakeTemperatureCelsius;
    case SignalKind::EngineTemperature:
        return frame.Dynamics.EngineTemperatureCelsius;
    case SignalKind::TireTemperature:
        return WheelMeanTemperature(frame.Wheels);
    case SignalKind::TireWear:
        return MeanWheelWear(frame.Wheels);
    case SignalKind::Fuel:
        return frame.Dynamics.FuelFraction;
    case SignalKind::TrackWetness:
        return frame.Environment.AmbientWetness;
    case SignalKind::RoadFriction:
        return frame.Environment.RoadFrictionEstimate;
    case SignalKind::GripScale:
        return frame.Dynamics.GripScale;
    case SignalKind::ChassisDamage:
        return frame.Dynamics.ChassisDamageFraction;
    default:
        return std::numeric_limits<double>::quiet_NaN();
    }
}

template<std::size_t Capacity>
class TelemetrySignalSeries final
{
    static_assert(Capacity >= 4 && Capacity <= 8192);
public:
    void Reset() noexcept
    {
        Samples.fill({});
        Start = 0;
        Count = 0;
        TotalObserved = 0;
        TotalRejected = 0;
        TotalOverwritten = 0;
        Integral = 0.0;
        AbsoluteIntegral = 0.0;
        TotalVariation = 0.0;
        LastValue = 0.0;
        LastTime = 0.0;
        HasPrevious = false;
        Statistics.Reset();
    }

    bool Append(SignalSample point) noexcept
    {
        if (!point.Qualified ||
            !Finite(point.TimeSeconds) ||
            !Finite(point.Value) ||
            !Finite(point.WeightSeconds) ||
            point.TimeSeconds < 0.0 ||
            point.WeightSeconds < 0.0 ||
            point.WeightSeconds > MaximumFrameDeltaSeconds ||
            std::abs(point.Value) > 1.0e12)
        {
            TotalRejected = SaturatingIncrement(TotalRejected);
            return false;
        }
        if (HasPrevious && point.TimeSeconds <= LastTime)
        {
            TotalRejected = SaturatingIncrement(TotalRejected);
            return false;
        }
        if (Count == Capacity)
        {
            ++Start;
            Start %= Capacity;
            ++TotalOverwritten;
        }
        else
        {
            ++Count;
        }
        Samples[(Start + Count - 1) % Capacity] = point;
        ++TotalObserved;
        Statistics.Observe(point.Value);
        Integral += point.Value * point.WeightSeconds;
        AbsoluteIntegral +=
            std::abs(point.Value) * point.WeightSeconds;
        if (HasPrevious)
        {
            TotalVariation += std::abs(point.Value - LastValue);
        }
        LastValue = point.Value;
        LastTime = point.TimeSeconds;
        HasPrevious = true;
        return true;
    }

    bool Observe(const TelemetryFrame& frame,
                 SignalKind kind) noexcept
    {
        const double value = ReadTelemetrySignal(frame, kind);
        return Append({
            frame.MonotonicSeconds,
            value,
            frame.DeltaSeconds,
            frame.Identity.FrameOrdinal,
            frame.Identity.Trust ==
                TelemetryTrust::EngineObserved &&
                frame.Race.EnginePhysicsReady
        });
    }

    SignalSample AtOldest(std::size_t index) const noexcept
    {
        return index < Count ?
            Samples[(Start + index) % Capacity] : SignalSample{};
    }

    SignalSample AtNewest(std::size_t index = 0) const noexcept
    {
        return index < Count ?
            Samples[(Start + Count - index - 1) % Capacity] :
            SignalSample{};
    }

    SignalSample Nearest(double timeSeconds) const noexcept
    {
        if (!Finite(timeSeconds) || Count == 0)
        {
            return {};
        }
        SignalSample best{};
        double bestError =
            std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < Count; ++i)
        {
            const SignalSample point = AtOldest(i);
            const double error =
                std::abs(point.TimeSeconds - timeSeconds);
            if (error < bestError)
            {
                best = point;
                bestError = error;
            }
        }
        return best;
    }

    SignalSample Interpolate(double timeSeconds,
                             double maximumGapSeconds = 0.10) const noexcept
    {
        if (!Finite(timeSeconds) ||
            !Finite(maximumGapSeconds) ||
            maximumGapSeconds <= 0.0 || Count == 0)
        {
            return {};
        }
        if (Count == 1)
        {
            return Nearest(timeSeconds);
        }
        for (std::size_t i = 1; i < Count; ++i)
        {
            const SignalSample a = AtOldest(i - 1);
            const SignalSample b = AtOldest(i);
            if (timeSeconds == a.TimeSeconds)
            {
                return a;
            }
            if (timeSeconds < a.TimeSeconds ||
                timeSeconds > b.TimeSeconds)
            {
                continue;
            }
            const double delta = b.TimeSeconds - a.TimeSeconds;
            if (delta <= 0.0 || delta > maximumGapSeconds)
            {
                return {};
            }
            const double t =
                (timeSeconds - a.TimeSeconds) / delta;
            return {
                timeSeconds,
                Lerp(a.Value, b.Value, t),
                Lerp(a.WeightSeconds, b.WeightSeconds, t),
                a.SourceOrdinal,
                a.Qualified && b.Qualified
            };
        }
        const SignalSample last = AtNewest();
        return timeSeconds == last.TimeSeconds ?
            last : SignalSample{};
    }

    SignalSummary Summarize() const noexcept
    {
        SignalSummary result;
        if (Count == 0)
        {
            return result;
        }
        RunningStatistics local;
        double totalWeight = 0.0;
        double weighted = 0.0;
        double squares = 0.0;
        double variation = 0.0;
        double minimumRate =
            std::numeric_limits<double>::infinity();
        double maximumRate =
            -std::numeric_limits<double>::infinity();
        SignalSample previous{};
        bool hasPrevious = false;
        for (std::size_t i = 0; i < Count; ++i)
        {
            const SignalSample current = AtOldest(i);
            local.Observe(current.Value);
            const double weight = current.WeightSeconds;
            totalWeight += weight;
            weighted += current.Value * weight;
            squares += current.Value * current.Value;
            if (hasPrevious)
            {
                const double dt =
                    current.TimeSeconds - previous.TimeSeconds;
                variation +=
                    std::abs(current.Value - previous.Value);
                if (dt > 0.0)
                {
                    const double rate =
                        (current.Value - previous.Value) / dt;
                    minimumRate = std::min(minimumRate, rate);
                    maximumRate = std::max(maximumRate, rate);
                }
            }
            previous = current;
            hasPrevious = true;
        }
        result.Mean = local.Mean();
        result.Minimum = local.Minimum();
        result.Maximum = local.Maximum();
        result.StandardDeviation = local.Deviation();
        result.TimeWeightedMean = SafeRatio(weighted, totalWeight);
        result.RootMeanSquare = std::sqrt(
            squares / static_cast<double>(Count));
        result.Integral = Integral;
        result.AbsoluteIntegral = AbsoluteIntegral;
        result.TotalVariation = variation;
        result.MaximumRate = Finite(maximumRate) ?
            maximumRate : 0.0;
        result.MinimumRate = Finite(minimumRate) ?
            minimumRate : 0.0;
        result.BeginningSeconds =
            AtOldest(0).TimeSeconds;
        result.EndingSeconds =
            AtNewest().TimeSeconds;
        result.Samples = Count;
        result.Valid = true;
        return result;
    }

    double TimeAbove(double threshold) const noexcept
    {
        if (!Finite(threshold))
        {
            return 0.0;
        }
        double result = 0.0;
        for (std::size_t i = 0; i < Count; ++i)
        {
            const SignalSample sample = AtOldest(i);
            if (sample.Value > threshold)
            {
                result += sample.WeightSeconds;
            }
        }
        return result;
    }

    double TimeBelow(double threshold) const noexcept
    {
        if (!Finite(threshold))
        {
            return 0.0;
        }
        double result = 0.0;
        for (std::size_t i = 0; i < Count; ++i)
        {
            const SignalSample sample = AtOldest(i);
            if (sample.Value < threshold)
            {
                result += sample.WeightSeconds;
            }
        }
        return result;
    }

    double TimeWithin(double minimum, double maximum) const noexcept
    {
        if (!Finite(minimum) || !Finite(maximum) ||
            minimum > maximum)
        {
            return 0.0;
        }
        double result = 0.0;
        for (std::size_t i = 0; i < Count; ++i)
        {
            const SignalSample sample = AtOldest(i);
            if (sample.Value >= minimum &&
                sample.Value <= maximum)
            {
                result += sample.WeightSeconds;
            }
        }
        return result;
    }

    std::size_t ThresholdCrossings(
        double threshold, bool rising) const noexcept
    {
        if (!Finite(threshold) || Count < 2)
        {
            return 0;
        }
        std::size_t count = 0;
        for (std::size_t i = 1; i < Count; ++i)
        {
            const double previous = AtOldest(i - 1).Value;
            const double current = AtOldest(i).Value;
            const bool crossed = rising ?
                previous < threshold && current >= threshold :
                previous > threshold && current <= threshold;
            count += crossed ? 1U : 0U;
        }
        return count;
    }

    std::size_t Size() const noexcept { return Count; }
    bool Empty() const noexcept { return Count == 0; }
    std::uint64_t AcceptedTotal() const noexcept
    {
        return TotalObserved;
    }
    std::uint64_t RejectedTotal() const noexcept
    {
        return TotalRejected;
    }
    std::uint64_t OverwrittenTotal() const noexcept
    {
        return TotalOverwritten;
    }

private:
    std::array<SignalSample, Capacity> Samples{};
    std::size_t Start = 0;
    std::size_t Count = 0;
    std::uint64_t TotalObserved = 0;
    std::uint64_t TotalRejected = 0;
    std::uint64_t TotalOverwritten = 0;
    double Integral = 0.0;
    double AbsoluteIntegral = 0.0;
    double TotalVariation = 0.0;
    double LastValue = 0.0;
    double LastTime = 0.0;
    bool HasPrevious = false;
    RunningStatistics Statistics;
};
} // namespace latebrake::engineering
