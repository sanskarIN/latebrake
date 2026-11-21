#pragma once

#include "Track/WorldScaleCandidateRacingLine.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Source-only swept chassis envelope check. A point on asphalt is not proof
// that a car with finite width fits, especially on chorded curved sections.
// This does not create Chaos collision or authorize actual vehicle movement.
struct WorldScaleSweptEnvelopeSettings final
{
    double HalfWidthCm = 98.0;
    double HalfLengthCm = 225.0;
    double EdgeMarginCm = 40.0;
    unsigned SamplesPerSegment = 12;
};

struct WorldScaleSweptEnvelopeAudit final
{
    bool Valid = false;
    unsigned SegmentsChecked = 0;
    unsigned ChassisPosesChecked = 0;
    unsigned CornerChecks = 0;
    double MinimumVerifiedCornerDistanceCm = 0.0;
};

inline bool ValidWorldScaleSweptEnvelopeSettings(
    const WorldScaleSweptEnvelopeSettings& s) noexcept
{
    return std::isfinite(s.HalfWidthCm) &&
        s.HalfWidthCm >= 70.0 && s.HalfWidthCm <= 130.0 &&
        std::isfinite(s.HalfLengthCm) &&
        s.HalfLengthCm >= 150.0 && s.HalfLengthCm <= 300.0 &&
        std::isfinite(s.EdgeMarginCm) &&
        s.EdgeMarginCm >= 0.0 && s.EdgeMarginCm <= 100.0 &&
        s.SamplesPerSegment >= 4u && s.SamplesPerSegment <= 64u;
}

inline bool WorldScaleChassisCornersOnAsphalt(
    CircuitPoint center, double heading,
    const WorldScaleSweptEnvelopeSettings& settings) noexcept
{
    if (!std::isfinite(center.X) || !std::isfinite(center.Y) ||
        !std::isfinite(heading) ||
        !ValidWorldScaleSweptEnvelopeSettings(settings))
        return false;

    const double forwardX = std::cos(heading);
    const double forwardY = std::sin(heading);
    const double rightX = -forwardY;
    const double rightY = forwardX;
    const double halfLength = settings.HalfLengthCm + settings.EdgeMarginCm;
    const double halfWidth = settings.HalfWidthCm + settings.EdgeMarginCm;
    for (double longitudinal : {-halfLength, halfLength})
    {
        for (double lateral : {-halfWidth, halfWidth})
        {
            const CircuitPoint corner{
                center.X + longitudinal * forwardX + lateral * rightX,
                center.Y + longitudinal * forwardY + lateral * rightY
            };
            if (!std::isfinite(corner.X) || !std::isfinite(corner.Y) ||
                !IsOnWorldScaleRoad(corner))
                return false;
        }
    }
    return true;
}

inline WorldScaleSweptEnvelopeAudit AuditWorldScaleSweptEnvelope(
    const WorldScaleLinePlan& plan,
    WorldScaleSweptEnvelopeSettings settings = {}) noexcept
{
    if (!plan.Valid || !ValidWorldScaleSweptEnvelopeSettings(settings) ||
        plan.VerifiedSegments != WorldScaleLineStationCount)
        return {};

    WorldScaleSweptEnvelopeAudit audit{};
    double minimumChord = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
    {
        const auto next = (i + 1u) % WorldScaleLineStationCount;
        const auto& a = plan.Nodes[i];
        const auto& b = plan.Nodes[next];
        if (!a.PhysicallySupportedAtSource ||
            !b.PhysicallySupportedAtSource ||
            !std::isfinite(a.CandidateCenter.X) ||
            !std::isfinite(a.CandidateCenter.Y) ||
            !std::isfinite(b.CandidateCenter.X) ||
            !std::isfinite(b.CandidateCenter.Y))
            return {};
        const double dx = b.CandidateCenter.X - a.CandidateCenter.X;
        const double dy = b.CandidateCenter.Y - a.CandidateCenter.Y;
        const double chord = std::hypot(dx, dy);
        if (!std::isfinite(chord) || chord <= 1.0)
            return {};
        minimumChord = std::min(minimumChord, chord);
        const double heading = std::atan2(dy, dx);
        for (unsigned sample = 0; sample <= settings.SamplesPerSegment; ++sample)
        {
            const double t = static_cast<double>(sample) /
                static_cast<double>(settings.SamplesPerSegment);
            const CircuitPoint center{
                a.CandidateCenter.X + dx * t,
                a.CandidateCenter.Y + dy * t
            };
            if (!WorldScaleChassisCornersOnAsphalt(center, heading, settings))
                return {};
            ++audit.ChassisPosesChecked;
            audit.CornerChecks += 4u;
        }
        ++audit.SegmentsChecked;
    }
    audit.MinimumVerifiedCornerDistanceCm = minimumChord;
    audit.Valid = audit.SegmentsChecked == WorldScaleLineStationCount;
    return audit;
}
} // namespace latebrake
