#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <utility>

namespace latebrake::racecraft
{
inline constexpr double Pi = std::numbers::pi_v<double>;
inline constexpr double TwoPi = 2.0 * Pi;
inline constexpr double GravityCmS2 = 980.665;
inline constexpr double CentimetersPerMeter = 100.0;
inline constexpr double CentimetersPerKilometer = 100000.0;
inline constexpr double CmS2ToG = 1.0 / GravityCmS2;
inline constexpr double MaxSimulationFrameSeconds = 0.10;
inline constexpr double PhysicsEpsilon = 1.0e-9;
inline constexpr double MaximumPhysicalSpeedCmS = 18000.0;
inline constexpr double MaximumPhysicalForceN = 250000.0;

struct Vec2 final
{
    double X = 0.0;
    double Y = 0.0;
};
struct Vec3 final
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};
struct Pose2 final
{
    Vec2 Position{};
    double HeadingRadians = 0.0;
};
struct Segment2 final
{
    Vec2 A{};
    Vec2 B{};
};
struct Disc2 final
{
    Vec2 Center{};
    double RadiusCm = 0.0;
};
struct Rect2 final
{
    Vec2 Center{};
    Vec2 HalfSize{};
    double RotationRadians = 0.0;
};
struct Aabb2 final
{
    Vec2 Minimum{};
    Vec2 Maximum{};
};
struct SegmentProjection final
{
    Vec2 Point{};
    double AlongFraction = 0.0;
    double DistanceSquared = 0.0;
    double SignedSideCm = 0.0;
    double SegmentLengthCm = 0.0;
    bool Valid = false;
};
struct RayIntersection final
{
    Vec2 Position{};
    double AlongFirst = 0.0;
    double AlongSecond = 0.0;
    bool Collides = false;
    bool Parallel = false;
};
struct BrakeEnvelope final
{
    double DistanceCm = 0.0;
    double TimeSeconds = 0.0;
    double ReactionDistanceCm = 0.0;
    double BrakingDistanceCm = 0.0;
    double DecelerationCmS2 = 0.0;
    bool Valid = false;
};

inline bool Finite(double value) noexcept
{
    return std::isfinite(value);
}
inline bool Finite(Vec2 p) noexcept
{
    return Finite(p.X) && Finite(p.Y);
}
inline bool Finite(Vec3 p) noexcept
{
    return Finite(p.X) && Finite(p.Y) && Finite(p.Z);
}
inline bool Finite(Pose2 p) noexcept
{
    return Finite(p.Position) && Finite(p.HeadingRadians);
}
inline double Clamp01(double value) noexcept
{
    return Finite(value) ? std::clamp(value, 0.0, 1.0) : 0.0;
}
inline double SafeDivision(double numerator, double denominator,
                           double fallback = 0.0) noexcept
{
    return Finite(numerator) && Finite(denominator) &&
        std::abs(denominator) > PhysicsEpsilon ?
        numerator / denominator : fallback;
}
inline double Square(double v) noexcept { return v * v; }
inline double Lerp(double a, double b, double fraction) noexcept
{
    return a + (b - a) * Clamp01(fraction);
}
inline Vec2 Lerp(Vec2 a, Vec2 b, double f) noexcept
{
    return {Lerp(a.X, b.X, f), Lerp(a.Y, b.Y, f)};
}
inline Vec3 Lerp(Vec3 a, Vec3 b, double f) noexcept
{
    return {Lerp(a.X, b.X, f), Lerp(a.Y, b.Y, f),
            Lerp(a.Z, b.Z, f)};
}
inline Vec2 operator+(Vec2 a, Vec2 b) noexcept
{
    return {a.X + b.X, a.Y + b.Y};
}
inline Vec2 operator-(Vec2 a, Vec2 b) noexcept
{
    return {a.X - b.X, a.Y - b.Y};
}
inline Vec2 operator*(Vec2 a, double scale) noexcept
{
    return {a.X * scale, a.Y * scale};
}
inline Vec2 operator/(Vec2 a, double scale) noexcept
{
    return {SafeDivision(a.X, scale), SafeDivision(a.Y, scale)};
}
inline Vec3 operator+(Vec3 a, Vec3 b) noexcept
{
    return {a.X + b.X, a.Y + b.Y, a.Z + b.Z};
}
inline Vec3 operator-(Vec3 a, Vec3 b) noexcept
{
    return {a.X - b.X, a.Y - b.Y, a.Z - b.Z};
}
inline Vec3 operator*(Vec3 a, double scale) noexcept
{
    return {a.X * scale, a.Y * scale, a.Z * scale};
}
inline double Dot(Vec2 a, Vec2 b) noexcept
{
    return a.X * b.X + a.Y * b.Y;
}
inline double Cross(Vec2 a, Vec2 b) noexcept
{
    return a.X * b.Y - a.Y * b.X;
}
inline double Dot(Vec3 a, Vec3 b) noexcept
{
    return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}
inline double LengthSquared(Vec2 v) noexcept { return Dot(v, v); }
inline double LengthSquared(Vec3 v) noexcept { return Dot(v, v); }
inline double Length(Vec2 v) noexcept { return std::hypot(v.X, v.Y); }
inline double Length(Vec3 v) noexcept
{
    return std::hypot(v.X, std::hypot(v.Y, v.Z));
}
inline double Distance(Vec2 a, Vec2 b) noexcept
{
    return Length(a - b);
}
inline Vec2 Normalized(Vec2 value) noexcept
{
    const double n = Length(value);
    return Finite(n) && n > PhysicsEpsilon ?
        value / n : Vec2{};
}
inline Vec3 Normalized(Vec3 value) noexcept
{
    const double n = Length(value);
    return Finite(n) && n > PhysicsEpsilon ?
        value * (1.0 / n) : Vec3{};
}
inline Vec2 PerpendicularLeft(Vec2 value) noexcept
{
    return {-value.Y, value.X};
}
inline Vec2 PerpendicularRight(Vec2 value) noexcept
{
    return {value.Y, -value.X};
}
inline Vec2 Rotate(Vec2 p, double radians) noexcept
{
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    return {c * p.X - s * p.Y, s * p.X + c * p.Y};
}
inline Vec2 HeadingVector(double radians) noexcept
{
    return {std::cos(radians), std::sin(radians)};
}
inline Vec2 ToLocal(Pose2 frame, Vec2 world) noexcept
{
    return Rotate(world - frame.Position, -frame.HeadingRadians);
}
inline Vec2 ToWorld(Pose2 frame, Vec2 local) noexcept
{
    return frame.Position + Rotate(local, frame.HeadingRadians);
}
inline double WrappedAngle(double radians) noexcept
{
    if (!Finite(radians)) return 0.0;
    const double a = std::remainder(radians, TwoPi);
    return a < -Pi ? a + TwoPi : a > Pi ? a - TwoPi : a;
}
inline double AngleDifference(double from, double to) noexcept
{
    return WrappedAngle(to - from);
}
inline double BlendAngle(double a, double b, double t) noexcept
{
    return WrappedAngle(a + AngleDifference(a, b) * Clamp01(t));
}
inline double AngleOf(Vec2 v) noexcept
{
    return Finite(v) ? std::atan2(v.Y, v.X) : 0.0;
}
inline double SignedHeadingError(double heading, Vec2 direction) noexcept
{
    return AngleDifference(heading, AngleOf(direction));
}
inline double ForwardSpeed(Vec2 velocity, double yaw) noexcept
{
    return Dot(velocity, HeadingVector(yaw));
}
inline double SideSpeed(Vec2 velocity, double yaw) noexcept
{
    return Cross(HeadingVector(yaw), velocity);
}
inline double PlanarSlipAngle(Vec2 velocity, double yaw) noexcept
{
    return Length(velocity) <= PhysicsEpsilon ?
        0.0 : WrappedAngle(AngleOf(velocity) - yaw);
}
inline double Smoothstep(double edge0, double edge1,
                         double value) noexcept
{
    if (!Finite(edge0) || !Finite(edge1) ||
        !Finite(value) || edge1 <= edge0) return 0.0;
    const double t = Clamp01((value - edge0) / (edge1 - edge0));
    return t * t * (3.0 - 2.0 * t);
}
inline double EaseInOutQuint(double f) noexcept
{
    const double t = Clamp01(f);
    return t * t * t * (t * (6.0 * t - 15.0) + 10.0);
}
inline double Sign(double v) noexcept
{
    return v > 0.0 ? 1.0 : v < 0.0 ? -1.0 : 0.0;
}
inline double ExponentialResponse(double oldValue, double target,
                                  double timeConstant, double dt) noexcept
{
    if (!Finite(oldValue) || !Finite(target) ||
        !Finite(timeConstant) || !Finite(dt) ||
        dt <= 0.0) return oldValue;
    if (timeConstant <= PhysicsEpsilon) return target;
    return oldValue + (target - oldValue) *
        (-std::expm1(-dt / timeConstant));
}
inline double RateLimited(double previous, double target,
                          double ascentPerSecond,
                          double descentPerSecond,
                          double dt) noexcept
{
    if (!Finite(previous) || !Finite(target) ||
        !Finite(ascentPerSecond) || !Finite(descentPerSecond) ||
        !Finite(dt) || dt < 0.0 || ascentPerSecond < 0.0 ||
        descentPerSecond < 0.0) return previous;
    return previous + std::clamp(
        target - previous, -descentPerSecond * dt,
        ascentPerSecond * dt);
}
inline SegmentProjection ProjectToSegment(Vec2 p, Segment2 s) noexcept
{
    SegmentProjection result;
    if (!Finite(p) || !Finite(s.A) || !Finite(s.B)) return result;
    const Vec2 span = s.B - s.A;
    const double squared = LengthSquared(span);
    if (!Finite(squared) || squared <= PhysicsEpsilon) return result;
    const double fraction =
        std::clamp(Dot(p - s.A, span) / squared, 0.0, 1.0);
    result.Point = s.A + span * fraction;
    result.AlongFraction = fraction;
    result.DistanceSquared = LengthSquared(p - result.Point);
    result.SignedSideCm = SafeDivision(
        Cross(span, p - s.A), std::sqrt(squared));
    result.SegmentLengthCm = std::sqrt(squared);
    result.Valid = Finite(result.DistanceSquared) &&
        Finite(result.SignedSideCm);
    return result;
}
inline RayIntersection IntersectSegments(Segment2 a,
                                          Segment2 b) noexcept
{
    RayIntersection out;
    if (!Finite(a.A) || !Finite(a.B) ||
        !Finite(b.A) || !Finite(b.B)) return out;
    const Vec2 r = a.B - a.A;
    const Vec2 s = b.B - b.A;
    const double denominator = Cross(r, s);
    if (std::abs(denominator) <= PhysicsEpsilon)
    {
        out.Parallel = true;
        return out;
    }
    const Vec2 offset = b.A - a.A;
    out.AlongFirst = Cross(offset, s) / denominator;
    out.AlongSecond = Cross(offset, r) / denominator;
    out.Collides = out.AlongFirst >= 0.0 &&
        out.AlongFirst <= 1.0 && out.AlongSecond >= 0.0 &&
        out.AlongSecond <= 1.0;
    out.Position = a.A + r * out.AlongFirst;
    return out;
}
inline double PointSegmentDistance(Vec2 point,
                                   Segment2 line) noexcept
{
    const SegmentProjection p = ProjectToSegment(point, line);
    return p.Valid ? std::sqrt(p.DistanceSquared) :
        std::numeric_limits<double>::infinity();
}
inline std::array<Vec2, 4> RectCorners(Rect2 rect) noexcept
{
    const Vec2 x = Rotate({rect.HalfSize.X, 0.0},
                          rect.RotationRadians);
    const Vec2 y = Rotate({0.0, rect.HalfSize.Y},
                          rect.RotationRadians);
    return {{rect.Center + x + y, rect.Center - x + y,
             rect.Center - x - y, rect.Center + x - y}};
}
inline bool ValidRect(Rect2 rect) noexcept
{
    return Finite(rect.Center) && Finite(rect.HalfSize) &&
        Finite(rect.RotationRadians) &&
        rect.HalfSize.X > 0.0 && rect.HalfSize.Y > 0.0;
}
inline bool AabbContains(Aabb2 box, Vec2 point) noexcept
{
    return Finite(point) && Finite(box.Minimum) &&
        Finite(box.Maximum) &&
        point.X >= box.Minimum.X && point.X <= box.Maximum.X &&
        point.Y >= box.Minimum.Y && point.Y <= box.Maximum.Y;
}
inline bool AabbContains(Aabb2 box, Rect2 body) noexcept
{
    if (!ValidRect(body)) return false;
    const auto corners = RectCorners(body);
    return std::all_of(corners.begin(), corners.end(),
        [&](Vec2 corner) { return AabbContains(box, corner); });
}
inline double OverlapOnAxis(
    const std::array<Vec2, 4>& a,
    const std::array<Vec2, 4>& b, Vec2 axis) noexcept
{
    const double axisLength = Length(axis);
    if (axisLength <= PhysicsEpsilon) return 0.0;
    axis = axis / axisLength;
    double minA = std::numeric_limits<double>::infinity();
    double maxA = -minA;
    double minB = minA;
    double maxB = -minA;
    for (Vec2 p : a)
    {
        const double d = Dot(p, axis);
        minA = std::min(minA, d);
        maxA = std::max(maxA, d);
    }
    for (Vec2 p : b)
    {
        const double d = Dot(p, axis);
        minB = std::min(minB, d);
        maxB = std::max(maxB, d);
    }
    return std::min(maxA, maxB) - std::max(minA, minB);
}
inline bool RectsOverlap(Rect2 a, Rect2 b,
                         double clearance = 0.0) noexcept
{
    if (!ValidRect(a) || !ValidRect(b) ||
        !Finite(clearance) || clearance < 0.0) return true;
    const auto cornersA = RectCorners(a);
    const auto cornersB = RectCorners(b);
    const std::array<Vec2, 4> axes{{
        cornersA[1] - cornersA[0], cornersA[3] - cornersA[0],
        cornersB[1] - cornersB[0], cornersB[3] - cornersB[0]
    }};
    for (Vec2 edge : axes)
    {
        const double overlap = OverlapOnAxis(
            cornersA, cornersB, PerpendicularLeft(edge));
        if (overlap < -clearance) return false;
    }
    return true;
}
inline double ContactRadiusAlongAxis(Rect2 body, Vec2 axis) noexcept
{
    if (!ValidRect(body)) return 0.0;
    const Vec2 local = Rotate(
        Normalized(axis), -body.RotationRadians);
    return std::abs(local.X) * body.HalfSize.X +
        std::abs(local.Y) * body.HalfSize.Y;
}
inline double SignedClearanceToLine(Rect2 body,
                                    Segment2 edge) noexcept
{
    if (!ValidRect(body)) return -1.0;
    const Vec2 span = edge.B - edge.A;
    if (Length(span) <= PhysicsEpsilon) return -1.0;
    const Vec2 normal = Normalized(PerpendicularLeft(span));
    return Dot(body.Center - edge.A, normal) -
        ContactRadiusAlongAxis(body, normal);
}
inline BrakeEnvelope StoppingEnvelope(
    double currentCmS, double desiredCmS,
    double decelerationCmS2,
    double reactionSeconds) noexcept
{
    BrakeEnvelope out;
    if (!Finite(currentCmS) || !Finite(desiredCmS) ||
        !Finite(decelerationCmS2) || !Finite(reactionSeconds) ||
        currentCmS < 0.0 || desiredCmS < 0.0 ||
        desiredCmS > currentCmS ||
        decelerationCmS2 <= PhysicsEpsilon ||
        reactionSeconds < 0.0 || reactionSeconds > 4.0)
        return out;
    out.ReactionDistanceCm = currentCmS * reactionSeconds;
    out.BrakingDistanceCm = std::max(0.0,
        (Square(currentCmS) - Square(desiredCmS)) /
        (2.0 * decelerationCmS2));
    out.DistanceCm = out.ReactionDistanceCm +
        out.BrakingDistanceCm;
    out.TimeSeconds = reactionSeconds +
        (currentCmS - desiredCmS) / decelerationCmS2;
    out.DecelerationCmS2 = decelerationCmS2;
    out.Valid = Finite(out.DistanceCm) &&
        Finite(out.TimeSeconds);
    return out;
}
inline double SpeedForAvailableBrakingDistance(
    double targetSpeedCmS, double availableDistanceCm,
    double decelerationCmS2) noexcept
{
    if (!Finite(targetSpeedCmS) || !Finite(availableDistanceCm) ||
        !Finite(decelerationCmS2) || targetSpeedCmS < 0.0 ||
        availableDistanceCm < 0.0 || decelerationCmS2 < 0.0)
        return 0.0;
    return std::sqrt(Square(targetSpeedCmS) +
        2.0 * decelerationCmS2 * availableDistanceCm);
}
inline double MaximumCurveSpeedCmS(double curvaturePerCm,
                                    double lateralGripG,
                                    double safetyFactor = 0.88) noexcept
{
    if (!Finite(curvaturePerCm) || !Finite(lateralGripG) ||
        !Finite(safetyFactor) || lateralGripG <= 0.0 ||
        safetyFactor <= 0.0 || safetyFactor > 1.0)
        return 0.0;
    if (std::abs(curvaturePerCm) <= PhysicsEpsilon)
        return MaximumPhysicalSpeedCmS;
    return std::min(MaximumPhysicalSpeedCmS,
        std::sqrt(safetyFactor * lateralGripG *
            GravityCmS2 / std::abs(curvaturePerCm)));
}
inline double CurvatureFromThreePoints(
    Vec2 a, Vec2 b, Vec2 c) noexcept
{
    if (!Finite(a) || !Finite(b) || !Finite(c)) return 0.0;
    const double ab = Distance(a, b);
    const double bc = Distance(b, c);
    const double ac = Distance(a, c);
    if (ab <= PhysicsEpsilon || bc <= PhysicsEpsilon ||
        ac <= PhysicsEpsilon) return 0.0;
    return 2.0 * Cross(b - a, c - a) / (ab * bc * ac);
}
inline double KineticEnergyJ(double kg, double cmS) noexcept
{
    return Finite(kg) && Finite(cmS) && kg >= 0.0 ?
        0.5 * kg * Square(cmS / CentimetersPerMeter) : 0.0;
}
inline double ClampPhysicalSpeed(double cmS) noexcept
{
    return Finite(cmS) ?
        std::clamp(cmS, -MaximumPhysicalSpeedCmS,
                   MaximumPhysicalSpeedCmS) : 0.0;
}
inline double WrapDistance(double cm, double totalCm) noexcept
{
    if (!Finite(cm) || !Finite(totalCm) ||
        totalCm <= PhysicsEpsilon) return 0.0;
    const double remainder = std::fmod(cm, totalCm);
    return remainder < 0.0 ? remainder + totalCm : remainder;
}
inline double SignedShortestLapDistance(double a, double b,
                                        double lapCm) noexcept
{
    if (!Finite(a) || !Finite(b) || !Finite(lapCm) ||
        lapCm <= PhysicsEpsilon) return 0.0;
    return WrapDistance(b - a + lapCm * 0.5, lapCm) -
        lapCm * 0.5;
}
inline double IntegrateTrapezoid(double oldValue,
                                 double newValue,
                                 double dt) noexcept
{
    return Finite(oldValue) && Finite(newValue) &&
        Finite(dt) && dt >= 0.0 ?
        0.5 * (oldValue + newValue) * dt : 0.0;
}
inline double Bilinear(double a00, double a10, double a01,
                       double a11, double x, double y) noexcept
{
    return Lerp(Lerp(a00, a10, x), Lerp(a01, a11, x), y);
}
} // namespace latebrake::racecraft
