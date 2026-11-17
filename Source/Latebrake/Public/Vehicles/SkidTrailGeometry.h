#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
struct SkidVector final
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;

    SkidVector operator+(SkidVector other) const noexcept
    {
        return {X + other.X, Y + other.Y, Z + other.Z};
    }
    SkidVector operator-(SkidVector other) const noexcept
    {
        return {X - other.X, Y - other.Y, Z - other.Z};
    }
    SkidVector operator*(double scale) const noexcept
    {
        return {X * scale, Y * scale, Z * scale};
    }
};

inline double SkidDot(SkidVector a, SkidVector b) noexcept
{
    return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}

inline SkidVector SkidCross(SkidVector a, SkidVector b) noexcept
{
    return {a.Y*b.Z - a.Z*b.Y, a.Z*b.X - a.X*b.Z, a.X*b.Y - a.Y*b.X};
}

inline double SkidLength(SkidVector value) noexcept
{
    return std::hypot(value.X, value.Y, value.Z);
}

inline bool IsFiniteSkidVector(SkidVector value, double bound) noexcept
{
    return std::isfinite(value.X) && std::isfinite(value.Y) && std::isfinite(value.Z) &&
        std::abs(value.X) <= bound && std::abs(value.Y) <= bound && std::abs(value.Z) <= bound;
}

inline bool IsSkidUnitNormal(SkidVector value) noexcept
{
    return IsFiniteSkidVector(value, 1.001) && value.Z >= 0.68 &&
        std::abs(SkidDot(value, value) - 1.0) <= 0.002;
}

struct SkidStrip final
{
    bool Valid = false;
    SkidVector Center{};
    SkidVector Forward{};
    SkidVector Right{};
    SkidVector Up{};
    double LengthCm = 0.0;
    double WidthCm = 0.0;
    double ThicknessCm = 0.0;
    double Intensity = 0.0;
};

// Fit a tiny, non-colliding cuboid to two measured contact points. The
// output is a decoration, never a physical road/contact reconstruction.
inline SkidStrip MakeSkidStrip(SkidVector begin, SkidVector end,
                              SkidVector beginNormal, SkidVector endNormal,
                              double intensity) noexcept
{
    if (!IsFiniteSkidVector(begin, 1.0e8) || !IsFiniteSkidVector(end, 1.0e8) ||
        !IsSkidUnitNormal(beginNormal) || !IsSkidUnitNormal(endNormal) ||
        !std::isfinite(intensity) || intensity < 0.0 || intensity > 1.0)
        return {};
    beginNormal = beginNormal * (1.0 / SkidLength(beginNormal));
    endNormal = endNormal * (1.0 / SkidLength(endNormal));
    if (SkidDot(beginNormal, endNormal) < 0.98) return {};
    const auto sumNormal = beginNormal + endNormal;
    const auto normal = sumNormal * (1.0 / SkidLength(sumNormal));
    const auto chord = end - begin;
    const double heightResidual = SkidDot(chord, normal);
    if (std::abs(heightResidual) > 4.0) return {};
    const auto tangent = chord - normal * heightResidual;
    const double length = SkidLength(tangent);
    if (!std::isfinite(length) || length < 0.5 || length > 180.001) return {};
    const auto forward = tangent * (1.0 / length);
    auto right = SkidCross(normal, forward);
    const double rightLength = SkidLength(right);
    if (rightLength <= 0.99 || !std::isfinite(rightLength)) return {};
    right = right * (1.0 / rightLength);
    const auto up = SkidCross(forward, right);
    constexpr double Thickness = 0.08;
    const auto center = (begin + end) * 0.5 + up * (0.20 + Thickness * 0.5);
    return {true, center, forward, right, up, length + 0.5,
        18.0 * (0.8 + 0.2 * intensity), Thickness, intensity};
}
} // namespace latebrake
