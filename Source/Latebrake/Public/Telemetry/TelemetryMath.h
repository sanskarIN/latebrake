#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>

namespace latebrake::engineering
{
inline constexpr double Pi = std::numbers::pi_v<double>;
inline constexpr double Tiny = 1.0e-10;
inline constexpr double CmPerMeter = 100.0;
inline constexpr double CmPerKilometer = 100000.0;
inline constexpr double CmPerSecondToKmh = 0.036;
inline constexpr double KmhToCmPerSecond = 1.0 / CmPerSecondToKmh;
inline constexpr double GravityCmS2 = 980.665;

struct Vector2 final
{
    double X = 0.0;
    double Y = 0.0;

    constexpr Vector2 operator+(Vector2 rhs) const noexcept
    {
        return {X + rhs.X, Y + rhs.Y};
    }
    constexpr Vector2 operator-(Vector2 rhs) const noexcept
    {
        return {X - rhs.X, Y - rhs.Y};
    }
    constexpr Vector2 operator*(double factor) const noexcept
    {
        return {X * factor, Y * factor};
    }
    constexpr Vector2 operator/(double factor) const noexcept
    {
        return {X / factor, Y / factor};
    }
    constexpr Vector2& operator+=(Vector2 rhs) noexcept
    {
        X += rhs.X;
        Y += rhs.Y;
        return *this;
    }
    constexpr Vector2& operator-=(Vector2 rhs) noexcept
    {
        X -= rhs.X;
        Y -= rhs.Y;
        return *this;
    }
};

struct Vector3 final
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;

    constexpr Vector3 operator+(Vector3 rhs) const noexcept
    {
        return {X + rhs.X, Y + rhs.Y, Z + rhs.Z};
    }
    constexpr Vector3 operator-(Vector3 rhs) const noexcept
    {
        return {X - rhs.X, Y - rhs.Y, Z - rhs.Z};
    }
    constexpr Vector3 operator*(double factor) const noexcept
    {
        return {X * factor, Y * factor, Z * factor};
    }
};

inline bool Finite(double value) noexcept
{
    return std::isfinite(value);
}

inline bool Finite(Vector2 v) noexcept
{
    return Finite(v.X) && Finite(v.Y);
}

inline bool Finite(Vector3 v) noexcept
{
    return Finite(v.X) && Finite(v.Y) && Finite(v.Z);
}

inline double Dot(Vector2 a, Vector2 b) noexcept
{
    return a.X * b.X + a.Y * b.Y;
}

inline double Cross(Vector2 a, Vector2 b) noexcept
{
    return a.X * b.Y - a.Y * b.X;
}

inline double Dot(Vector3 a, Vector3 b) noexcept
{
    return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}

inline double LengthSquared(Vector2 a) noexcept
{
    return Dot(a, a);
}

inline double LengthSquared(Vector3 a) noexcept
{
    return Dot(a, a);
}

inline double Length(Vector2 a) noexcept
{
    return std::hypot(a.X, a.Y);
}

inline double Length(Vector3 a) noexcept
{
    return std::hypot(a.X, a.Y, a.Z);
}

inline double Distance(Vector2 a, Vector2 b) noexcept
{
    return Length(a - b);
}

inline Vector2 PerpendicularLeft(Vector2 v) noexcept
{
    return {-v.Y, v.X};
}

inline Vector2 PerpendicularRight(Vector2 v) noexcept
{
    return {v.Y, -v.X};
}

inline Vector2 HeadingUnit(double radians) noexcept
{
    return {std::cos(radians), std::sin(radians)};
}

inline Vector2 Rotate(Vector2 v, double radians) noexcept
{
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    return {v.X * c - v.Y * s, v.X * s + v.Y * c};
}

inline Vector2 NormalizeOrZero(Vector2 v) noexcept
{
    const double length = Length(v);
    return Finite(length) && length > Tiny ? v / length : Vector2{};
}

inline Vector3 NormalizeOrZero(Vector3 v) noexcept
{
    const double length = Length(v);
    return Finite(length) && length > Tiny ? v * (1.0 / length) : Vector3{};
}

inline double Clamp01(double value) noexcept
{
    return Finite(value) ? std::clamp(value, 0.0, 1.0) : 0.0;
}

inline double ClampSigned(double value) noexcept
{
    return Finite(value) ? std::clamp(value, -1.0, 1.0) : 0.0;
}

inline double SafeRatio(
    double numerator, double denominator, double fallback = 0.0) noexcept
{
    if (!Finite(numerator) || !Finite(denominator) ||
        std::abs(denominator) < Tiny)
    {
        return fallback;
    }
    const double result = numerator / denominator;
    return Finite(result) ? result : fallback;
}

inline double SafeSquareRoot(double value) noexcept
{
    return Finite(value) && value > 0.0 ? std::sqrt(value) : 0.0;
}

inline double SafeAcos(double value) noexcept
{
    return std::acos(std::clamp(Finite(value) ? value : 1.0, -1.0, 1.0));
}

inline double SafeAsin(double value) noexcept
{
    return std::asin(std::clamp(Finite(value) ? value : 0.0, -1.0, 1.0));
}

inline double WrapRadians(double value) noexcept
{
    return Finite(value) ? std::remainder(value, 2.0 * Pi) : 0.0;
}

inline double WrapDegrees(double value) noexcept
{
    return Finite(value) ? std::remainder(value, 360.0) : 0.0;
}

inline double AngleDifference(double from, double to) noexcept
{
    return WrapRadians(to - from);
}

inline double AngleBetween(Vector2 first, Vector2 second) noexcept
{
    if (!Finite(first) || !Finite(second))
    {
        return 0.0;
    }
    return std::atan2(Cross(first, second), Dot(first, second));
}

inline double SignedDistanceToLine(
    Vector2 point, Vector2 origin, Vector2 unitDirection) noexcept
{
    return Cross(unitDirection, point - origin);
}

inline double ProgressAlongLine(
    Vector2 point, Vector2 origin, Vector2 unitDirection) noexcept
{
    return Dot(point - origin, unitDirection);
}

inline Vector2 ClosestPointOnSegment(
    Vector2 position, Vector2 begin, Vector2 end) noexcept
{
    const Vector2 edge = end - begin;
    const double lengthSquared = LengthSquared(edge);
    if (!Finite(position) || !Finite(begin) || !Finite(end) ||
        !Finite(lengthSquared) || lengthSquared < Tiny)
    {
        return begin;
    }
    const double t = Clamp01(Dot(position - begin, edge) / lengthSquared);
    return begin + edge * t;
}

inline double DistanceToSegment(
    Vector2 position, Vector2 begin, Vector2 end) noexcept
{
    return Distance(position, ClosestPointOnSegment(position, begin, end));
}

inline double Lerp(double a, double b, double t) noexcept
{
    return a + (b - a) * Clamp01(t);
}

inline Vector2 Lerp(Vector2 a, Vector2 b, double t) noexcept
{
    return a + (b - a) * Clamp01(t);
}

inline Vector3 Lerp(Vector3 a, Vector3 b, double t) noexcept
{
    return a + (b - a) * Clamp01(t);
}

inline double SmoothStep(double t) noexcept
{
    t = Clamp01(t);
    return t * t * (3.0 - 2.0 * t);
}

inline double SmootherStep(double t) noexcept
{
    t = Clamp01(t);
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

inline double ExponentialWeight(double dt, double timeConstant) noexcept
{
    if (!Finite(dt) || !Finite(timeConstant) ||
        dt <= 0.0 || timeConstant <= Tiny)
    {
        return 0.0;
    }
    return Clamp01(-std::expm1(-dt / timeConstant));
}

inline double SaturatingAdd(
    double a, double b, double limit = 1.0e15) noexcept
{
    if (!Finite(a) || !Finite(b) || !Finite(limit) || limit <= 0.0)
    {
        return 0.0;
    }
    return std::clamp(a + b, -limit, limit);
}

inline std::uint64_t SaturatingIncrement(std::uint64_t value) noexcept
{
    return value < std::numeric_limits<std::uint64_t>::max() ?
        value + 1U : value;
}

inline double BoundSpeed(double cmPerSecond) noexcept
{
    return Finite(cmPerSecond) ?
        std::clamp(cmPerSecond, 0.0, 16000.0) : 0.0;
}

inline bool NearlyEqual(
    double a, double b, double absoluteTolerance = 1.0e-8,
    double relativeTolerance = 1.0e-8) noexcept
{
    if (!Finite(a) || !Finite(b) ||
        !Finite(absoluteTolerance) || !Finite(relativeTolerance) ||
        absoluteTolerance < 0.0 || relativeTolerance < 0.0)
    {
        return false;
    }
    return std::abs(a - b) <=
        std::max(absoluteTolerance,
                 relativeTolerance * std::max(std::abs(a), std::abs(b)));
}

inline double ProjectedSpeed(Vector2 velocity, double heading) noexcept
{
    return Dot(velocity, HeadingUnit(heading));
}

inline double CrossTrackSpeed(Vector2 velocity, double heading) noexcept
{
    return Cross(HeadingUnit(heading), velocity);
}

inline double SlipAngle(Vector2 velocity, double heading) noexcept
{
    if (!Finite(velocity) || !Finite(heading) ||
        LengthSquared(velocity) < 100.0)
    {
        return 0.0;
    }
    return AngleDifference(heading, std::atan2(velocity.Y, velocity.X));
}

inline double Curvature(
    Vector2 previous, Vector2 current, Vector2 next) noexcept
{
    const double a = Distance(previous, current);
    const double b = Distance(current, next);
    const double c = Distance(next, previous);
    const double product = a * b * c;
    if (!Finite(product) || product < Tiny)
    {
        return 0.0;
    }
    return 2.0 * Cross(current - previous, next - previous) / product;
}

inline double TriangleArea(Vector2 a, Vector2 b, Vector2 c) noexcept
{
    return 0.5 * std::abs(Cross(b - a, c - a));
}

inline bool PointInConvexQuad(
    Vector2 point, const std::array<Vector2, 4>& corners) noexcept
{
    if (!Finite(point))
    {
        return false;
    }
    bool positive = false;
    bool negative = false;
    for (std::size_t i = 0; i < 4; ++i)
    {
        if (!Finite(corners[i]) ||
            !Finite(corners[(i + 1) % 4]))
        {
            return false;
        }
        const double cross = Cross(
            corners[(i + 1) % 4] - corners[i], point - corners[i]);
        positive = positive || cross > Tiny;
        negative = negative || cross < -Tiny;
        if (positive && negative)
        {
            return false;
        }
    }
    return true;
}

inline Vector2 Acceleration(
    Vector2 before, Vector2 after, double dt) noexcept
{
    if (!Finite(before) || !Finite(after) ||
        !Finite(dt) || dt <= Tiny)
    {
        return {};
    }
    return (after - before) / dt;
}

inline double Jerk(
    double accelerationBefore, double accelerationAfter,
    double dt) noexcept
{
    return SafeRatio(accelerationAfter - accelerationBefore, dt);
}

inline double BrakingDistance(
    double speedCmS, double decelerationCmS2,
    double reactionSeconds = 0.0) noexcept
{
    if (!Finite(speedCmS) || !Finite(decelerationCmS2) ||
        !Finite(reactionSeconds) || speedCmS < 0.0 ||
        decelerationCmS2 <= Tiny || reactionSeconds < 0.0)
    {
        return std::numeric_limits<double>::infinity();
    }
    return speedCmS * reactionSeconds +
        speedCmS * speedCmS / (2.0 * decelerationCmS2);
}

inline double SafeStoppingSpeed(
    double availableDistanceCm, double decelerationCmS2) noexcept
{
    if (!Finite(availableDistanceCm) ||
        !Finite(decelerationCmS2) ||
        availableDistanceCm <= 0.0 || decelerationCmS2 <= 0.0)
    {
        return 0.0;
    }
    return std::sqrt(2.0 * availableDistanceCm * decelerationCmS2);
}
} // namespace latebrake::engineering
