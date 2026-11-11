#pragma once

#include "Telemetry/TelemetryMath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
class ExponentialFilter final
{
public:
    void Reset() noexcept
    {
        Initialized = false;
        Output = 0.0;
    }
    void Seed(double value) noexcept
    {
        Initialized = Finite(value);
        Output = Initialized ? value : 0.0;
    }
    double Observe(double value, double dt, double seconds) noexcept
    {
        if (!Finite(value) || !Finite(dt) || !Finite(seconds) ||
            dt <= 0.0 || dt > 1.0 || seconds <= 0.0)
        {
            return Initialized ? Output : 0.0;
        }
        if (!Initialized)
        {
            Seed(value);
        }
        else
        {
            Output = Lerp(Output, value, ExponentialWeight(dt, seconds));
        }
        return Output;
    }
    double Value() const noexcept { return Output; }
    bool Ready() const noexcept { return Initialized; }
private:
    bool Initialized = false;
    double Output = 0.0;
};

class AngularFilter final
{
public:
    void Reset() noexcept
    {
        Initialized = false;
        Angle = 0.0;
    }
    double Observe(double radians, double dt, double seconds) noexcept
    {
        if (!Finite(radians) || !Finite(dt) ||
            dt <= 0.0 || dt > 1.0 || seconds <= 0.0)
        {
            return Angle;
        }
        if (!Initialized)
        {
            Angle = WrapRadians(radians);
            Initialized = true;
        }
        else
        {
            Angle = WrapRadians(
                Angle + AngleDifference(Angle, radians) *
                ExponentialWeight(dt, seconds));
        }
        return Angle;
    }
    double Value() const noexcept { return Angle; }
private:
    bool Initialized = false;
    double Angle = 0.0;
};

class VectorFilter final
{
public:
    void Reset() noexcept
    {
        X.Reset();
        Y.Reset();
    }
    Vector2 Observe(Vector2 value, double dt, double seconds) noexcept
    {
        return {
            X.Observe(value.X, dt, seconds),
            Y.Observe(value.Y, dt, seconds)
        };
    }
    Vector2 Value() const noexcept { return {X.Value(), Y.Value()}; }
    bool Ready() const noexcept { return X.Ready() && Y.Ready(); }
private:
    ExponentialFilter X;
    ExponentialFilter Y;
};

class BoundedRateLimiter final
{
public:
    void Reset(double value = 0.0) noexcept
    {
        Output = Finite(value) ? value : 0.0;
        Initialized = Finite(value);
    }
    double Observe(double target, double dt,
                   double risePerSecond,
                   double fallPerSecond) noexcept
    {
        if (!Finite(target) || !Finite(dt) ||
            !Finite(risePerSecond) || !Finite(fallPerSecond) ||
            dt <= 0.0 || dt > 1.0 ||
            risePerSecond < 0.0 || fallPerSecond < 0.0)
        {
            return Output;
        }
        if (!Initialized)
        {
            Output = target;
            Initialized = true;
            return Output;
        }
        const double difference = target - Output;
        const double allowed = difference >= 0.0 ?
            risePerSecond * dt : fallPerSecond * dt;
        const double step = std::clamp(
            difference, -allowed, allowed);
        Output += step;
        return Output;
    }
    double Value() const noexcept { return Output; }
private:
    double Output = 0.0;
    bool Initialized = false;
};

class BoundedIntegrator final
{
public:
    void Reset(double startingValue = 0.0) noexcept
    {
        ValueCurrent = Finite(startingValue) ? startingValue : 0.0;
    }
    bool Step(double rate, double dt,
              double minimum, double maximum) noexcept
    {
        if (!Finite(rate) || !Finite(dt) || !Finite(minimum) ||
            !Finite(maximum) || minimum > maximum ||
            dt < 0.0 || dt > 1.0)
        {
            return false;
        }
        ValueCurrent = std::clamp(
            ValueCurrent + rate * dt, minimum, maximum);
        return true;
    }
    double Value() const noexcept { return ValueCurrent; }
private:
    double ValueCurrent = 0.0;
};

template<std::size_t Capacity>
class RollingWindow final
{
    static_assert(Capacity >= 3 && Capacity <= 256);
public:
    void Reset() noexcept
    {
        Values.fill(0.0);
        Count = 0;
        Next = 0;
        Sum = 0.0;
        SumSquares = 0.0;
    }
    bool Push(double value) noexcept
    {
        if (!Finite(value) || std::abs(value) > 1.0e12)
        {
            return false;
        }
        if (Count == Capacity)
        {
            const double old = Values[Next];
            Sum -= old;
            SumSquares -= old * old;
        }
        else
        {
            ++Count;
        }
        Values[Next] = value;
        Next = (Next + 1) % Capacity;
        Sum += value;
        SumSquares += value * value;
        return true;
    }
    std::size_t Size() const noexcept { return Count; }
    bool Empty() const noexcept { return Count == 0; }
    double Mean() const noexcept
    {
        return Count ? Sum / static_cast<double>(Count) : 0.0;
    }
    double Variance() const noexcept
    {
        if (!Count)
        {
            return 0.0;
        }
        const double count = static_cast<double>(Count);
        return std::max(0.0, SumSquares / count -
                        Mean() * Mean());
    }
    double StandardDeviation() const noexcept
    {
        return std::sqrt(Variance());
    }
    double RootMeanSquare() const noexcept
    {
        return Count ? std::sqrt(
            std::max(0.0, SumSquares / static_cast<double>(Count))) : 0.0;
    }
    double Minimum() const noexcept
    {
        if (!Count)
        {
            return 0.0;
        }
        double value = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < Count; ++i)
        {
            value = std::min(value, Values[i]);
        }
        return value;
    }
    double Maximum() const noexcept
    {
        if (!Count)
        {
            return 0.0;
        }
        double value = -std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < Count; ++i)
        {
            value = std::max(value, Values[i]);
        }
        return value;
    }
    double Latest() const noexcept
    {
        return Count ?
            Values[(Next + Capacity - 1) % Capacity] : 0.0;
    }
    double Oldest() const noexcept
    {
        return Count == Capacity ? Values[Next] :
            (Count ? Values[0] : 0.0);
    }
    double AtFromOldest(std::size_t index) const noexcept
    {
        if (index >= Count)
        {
            return 0.0;
        }
        const std::size_t first = Count == Capacity ? Next : 0;
        return Values[(first + index) % Capacity];
    }
    double Percentile(double fraction) const noexcept
    {
        if (!Count)
        {
            return 0.0;
        }
        std::array<double, Capacity> sorted{};
        // The fixed ring is often smaller than the standard library's
        // insertion-sort threshold. Keep the sort bounded to actual
        // samples to avoid GCC 15 -Warray-bounds in std::sort internals.
        for (std::size_t i = 0; i < Count; ++i)
        {
            const double current = AtFromOldest(i);
            std::size_t j = i;
            while (j > 0 && sorted[j - 1] > current)
            {
                sorted[j] = sorted[j - 1];
                --j;
            }
            sorted[j] = current;
        }
        const double rank = Clamp01(fraction) *
            static_cast<double>(Count - 1);
        const std::size_t lower = static_cast<std::size_t>(
            std::floor(rank));
        const std::size_t upper = std::min(lower + 1, Count - 1);
        return Lerp(sorted[lower], sorted[upper],
                    rank - static_cast<double>(lower));
    }
private:
    std::array<double, Capacity> Values{};
    std::size_t Count = 0;
    std::size_t Next = 0;
    double Sum = 0.0;
    double SumSquares = 0.0;
};

class RunningStatistics final
{
public:
    void Reset() noexcept
    {
        Count = 0;
        MeanValue = 0.0;
        SquareAccumulator = 0.0;
        MinimumValue = 0.0;
        MaximumValue = 0.0;
    }
    bool Observe(double value) noexcept
    {
        if (!Finite(value) || std::abs(value) > 1.0e12)
        {
            return false;
        }
        if (Count == 0)
        {
            MeanValue = value;
            MinimumValue = value;
            MaximumValue = value;
            Count = 1;
            return true;
        }
        if (Count == std::numeric_limits<std::uint64_t>::max())
        {
            return false;
        }
        ++Count;
        const double delta = value - MeanValue;
        MeanValue += delta / static_cast<double>(Count);
        const double deltaAfter = value - MeanValue;
        SquareAccumulator += delta * deltaAfter;
        MinimumValue = std::min(MinimumValue, value);
        MaximumValue = std::max(MaximumValue, value);
        return true;
    }
    std::uint64_t Size() const noexcept { return Count; }
    double Mean() const noexcept { return MeanValue; }
    double Minimum() const noexcept { return MinimumValue; }
    double Maximum() const noexcept { return MaximumValue; }
    double Variance() const noexcept
    {
        return Count > 1 ?
            std::max(0.0, SquareAccumulator /
                static_cast<double>(Count - 1)) : 0.0;
    }
    double Deviation() const noexcept
    {
        return std::sqrt(Variance());
    }
    double StandardError() const noexcept
    {
        return Count > 1 ?
            Deviation() / std::sqrt(static_cast<double>(Count)) : 0.0;
    }
    double CoefficientOfVariation() const noexcept
    {
        return SafeRatio(Deviation(), std::abs(MeanValue));
    }
private:
    std::uint64_t Count = 0;
    double MeanValue = 0.0;
    double SquareAccumulator = 0.0;
    double MinimumValue = 0.0;
    double MaximumValue = 0.0;
};

class AlphaBetaPositionFilter final
{
public:
    void Reset() noexcept
    {
        Initialized = false;
        Position = {};
        Velocity = {};
    }
    bool Observe(Vector2 measurement, double dt,
                 double alpha, double beta) noexcept
    {
        if (!Finite(measurement) || !Finite(dt) ||
            !Finite(alpha) || !Finite(beta) ||
            dt <= 0.0 || dt > 1.0 ||
            alpha <= 0.0 || alpha > 1.0 ||
            beta < 0.0 || beta > 1.0)
        {
            return false;
        }
        if (!Initialized)
        {
            Position = measurement;
            Velocity = {};
            Initialized = true;
            return true;
        }
        const Vector2 projected = Position + Velocity * dt;
        const Vector2 residual = measurement - projected;
        Position = projected + residual * alpha;
        Velocity += residual * (beta / dt);
        return Finite(Position) && Finite(Velocity);
    }
    Vector2 EstimatedPosition() const noexcept { return Position; }
    Vector2 EstimatedVelocity() const noexcept { return Velocity; }
    bool Ready() const noexcept { return Initialized; }
private:
    bool Initialized = false;
    Vector2 Position{};
    Vector2 Velocity{};
};

class OneEuroSignalFilter final
{
public:
    void Reset() noexcept
    {
        Signal.Reset();
        Derivative.Reset();
        HasPrevious = false;
        Previous = 0.0;
    }
    bool Configure(double minimumCutoffHz,
                   double beta, double derivativeCutoffHz) noexcept
    {
        if (!Finite(minimumCutoffHz) || !Finite(beta) ||
            !Finite(derivativeCutoffHz) ||
            minimumCutoffHz <= 0.0 || beta < 0.0 ||
            derivativeCutoffHz <= 0.0)
        {
            return false;
        }
        MinCutoff = minimumCutoffHz;
        Beta = beta;
        DerivativeCutoff = derivativeCutoffHz;
        Reset();
        return true;
    }
    double Observe(double value, double dt) noexcept
    {
        if (!Finite(value) || !Finite(dt) || dt <= 0.0 ||
            dt > 1.0)
        {
            return Signal.Value();
        }
        const double derivative = HasPrevious ?
            (value - Previous) / dt : 0.0;
        Previous = value;
        HasPrevious = true;
        const double smoothDerivative =
            Derivative.Observe(derivative, dt,
                               1.0 / (2.0 * Pi * DerivativeCutoff));
        const double cutoff = MinCutoff +
            Beta * std::abs(smoothDerivative);
        const double safeCutoff = std::clamp(cutoff, 0.01, 1000.0);
        return Signal.Observe(
            value, dt, 1.0 / (2.0 * Pi * safeCutoff));
    }
    double Value() const noexcept { return Signal.Value(); }
private:
    ExponentialFilter Signal;
    ExponentialFilter Derivative;
    double MinCutoff = 2.5;
    double Beta = 0.0;
    double DerivativeCutoff = 1.0;
    double Previous = 0.0;
    bool HasPrevious = false;
};

class HysteresisDetector final
{
public:
    void Reset() noexcept
    {
        On = false;
        TransitionCount = 0;
    }
    bool Configure(double enter, double exit) noexcept
    {
        if (!Finite(enter) || !Finite(exit) || enter <= exit)
        {
            return false;
        }
        EnterThreshold = enter;
        ExitThreshold = exit;
        Reset();
        return true;
    }
    bool Observe(double value) noexcept
    {
        if (!Finite(value))
        {
            return On;
        }
        const bool next = On ?
            value > ExitThreshold : value >= EnterThreshold;
        if (next != On)
        {
            ++TransitionCount;
        }
        On = next;
        return On;
    }
    bool Active() const noexcept { return On; }
    std::uint64_t Transitions() const noexcept
    {
        return TransitionCount;
    }
private:
    double EnterThreshold = 0.65;
    double ExitThreshold = 0.25;
    bool On = false;
    std::uint64_t TransitionCount = 0;
};

class StableDurationGate final
{
public:
    void Reset() noexcept
    {
        Elapsed = 0.0;
        Qualified = false;
    }
    bool Observe(bool condition, double dt,
                 double requiredSeconds) noexcept
    {
        if (!Finite(dt) || !Finite(requiredSeconds) ||
            dt <= 0.0 || dt > 1.0 || requiredSeconds <= 0.0)
        {
            Reset();
            return false;
        }
        if (!condition)
        {
            Reset();
            return false;
        }
        Elapsed = std::min(requiredSeconds, Elapsed + dt);
        Qualified = Elapsed >= requiredSeconds;
        return Qualified;
    }
    double StableSeconds() const noexcept { return Elapsed; }
    bool Ready() const noexcept { return Qualified; }
private:
    double Elapsed = 0.0;
    bool Qualified = false;
};

class PeakHold final
{
public:
    void Reset() noexcept
    {
        Peak = 0.0;
        HasPeak = false;
    }
    double Observe(double value, double dt, double decayPerSecond) noexcept
    {
        if (!Finite(value) || !Finite(dt) ||
            !Finite(decayPerSecond) || dt <= 0.0 ||
            dt > 1.0 || decayPerSecond < 0.0)
        {
            return Peak;
        }
        const double decayed = std::max(0.0, Peak - dt * decayPerSecond);
        Peak = HasPeak ? std::max(value, decayed) : value;
        HasPeak = true;
        return Peak;
    }
    double Value() const noexcept { return Peak; }
private:
    double Peak = 0.0;
    bool HasPeak = false;
};
} // namespace latebrake::engineering
