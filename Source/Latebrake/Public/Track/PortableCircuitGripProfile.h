#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace latebrake
{
// A source-only authored route profile, NOT a Chaos collision material,
// tire contact sample, dynamic wet weather or an actual map authoring step.
// Every segment belongs to the SAME continuous unwrapped lap distance.
struct PortableGripSegment final
{
    double StartMeters = 0.0;
    double EndMeters = 0.0;
    double DryMultiplier = 1.0;
    double WetMultiplier = 0.7;
    double RubberCoverage = 0.0;
    double LooseDebrisCoverage = 0.0;
};

struct PortableGripReadout final
{
    bool Valid = false;
    std::size_t SegmentIndex = 0;
    double DistanceMeters = 0.0;
    double DryComponent = 0.0;
    double WetComponent = 0.0;
    double RubberAdjustment = 0.0;
    double DebrisAdjustment = 0.0;
    double GripMultiplier = 0.0;
};

class PortableCircuitGripProfile final
{
public:
    static constexpr std::size_t Capacity = 64;

    bool Reset(double lapMeters) noexcept
    {
        Count = 0;
        CourseMeters = 0.0;
        if (!std::isfinite(lapMeters) ||
            lapMeters < 10.0 || lapMeters > 100000.0)
        {
            return false;
        }
        CourseMeters = lapMeters;
        return true;
    }

    bool Append(PortableGripSegment patch) noexcept
    {
        if (CourseMeters <= 0.0 || Count == Capacity ||
            !std::isfinite(patch.StartMeters) ||
            !std::isfinite(patch.EndMeters) ||
            !std::isfinite(patch.DryMultiplier) ||
            !std::isfinite(patch.WetMultiplier) ||
            !std::isfinite(patch.RubberCoverage) ||
            !std::isfinite(patch.LooseDebrisCoverage) ||
            patch.StartMeters < 0.0 ||
            patch.EndMeters <= patch.StartMeters ||
            patch.EndMeters > CourseMeters ||
            patch.DryMultiplier < 0.25 ||
            patch.DryMultiplier > 1.5 ||
            patch.WetMultiplier < 0.10 ||
            patch.WetMultiplier > 1.5 ||
            patch.RubberCoverage < 0.0 ||
            patch.RubberCoverage > 1.0 ||
            patch.LooseDebrisCoverage < 0.0 ||
            patch.LooseDebrisCoverage > 1.0 ||
            patch.RubberCoverage + patch.LooseDebrisCoverage > 1.0)
        {
            return false;
        }
        const double expectedStart = Count == 0
            ? 0.0 : Segments[Count - 1].EndMeters;
        // Exact contiguous bounds are intentional: source data comes from
        // one authoring space and gaps must not create invented asphalt.
        if (patch.StartMeters != expectedStart)
        {
            return false;
        }
        Segments[Count++] = patch;
        return true;
    }

    bool Complete() const noexcept
    {
        return Count > 0 && CourseMeters > 0.0 &&
            Segments[0].StartMeters == 0.0 &&
            Segments[Count - 1].EndMeters == CourseMeters;
    }

    std::size_t Size() const noexcept { return Count; }
    double LapDistanceMeters() const noexcept { return CourseMeters; }

    PortableGripReadout Evaluate(
        double stationMeters, double wetnessFraction) const noexcept
    {
        PortableGripReadout out;
        if (!Complete() || !std::isfinite(stationMeters) ||
            !std::isfinite(wetnessFraction) ||
            stationMeters < 0.0 ||
            stationMeters >= CourseMeters ||
            wetnessFraction < 0.0 || wetnessFraction > 1.0)
        {
            return out;
        }
        // The track finish belongs to the NEXT lap at station zero, never
        // to a nonexistent segment after the physical end of the profile.
        std::size_t lo = 0;
        std::size_t hi = Count;
        while (lo < hi)
        {
            const std::size_t middle = lo + (hi - lo) / 2;
            if (stationMeters >= Segments[middle].EndMeters)
            {
                lo = middle + 1;
            }
            else
            {
                hi = middle;
            }
        }
        if (lo >= Count)
        {
            return {};
        }
        const auto& patch = Segments[lo];
        if (stationMeters < patch.StartMeters)
        {
            return {};
        }
        const double dryContribution =
            (1.0 - wetnessFraction) * patch.DryMultiplier;
        const double wetContribution =
            wetnessFraction * patch.WetMultiplier;
        // Rubber gradually helps a dry groove while being conservative on
        // a wet course. Debris is always a penalty. Bounds still apply.
        const double rubber = patch.RubberCoverage *
            (0.08 * (1.0 - wetnessFraction) - 0.12 * wetnessFraction);
        const double debris = -0.18 * patch.LooseDebrisCoverage;
        const double grip = std::clamp(
            dryContribution + wetContribution + rubber + debris, 0.10, 1.6);
        if (!std::isfinite(grip))
        {
            return {};
        }
        out.Valid = true;
        out.SegmentIndex = lo;
        out.DistanceMeters = stationMeters;
        out.DryComponent = dryContribution;
        out.WetComponent = wetContribution;
        out.RubberAdjustment = rubber;
        out.DebrisAdjustment = debris;
        out.GripMultiplier = grip;
        return out;
    }

private:
    std::array<PortableGripSegment, Capacity> Segments{};
    std::size_t Count = 0;
    double CourseMeters = 0.0;
};
} // namespace latebrake
