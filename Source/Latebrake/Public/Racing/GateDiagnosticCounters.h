#pragma once

#include "Racing/CheckpointMotionTrace.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Bounded, allocation-free, engine-independent QA summary for one checkpoint
// window. No identifiers, coordinates, network transport or disk writes.
// Counts saturate instead of wrapping, including when merging snapshots.
class GateDiagnosticCounters final
{
public:
    static constexpr std::size_t TraceCategories =
        static_cast<std::size_t>(CheckpointTraceDecision::GateRejected) + 1U;
    static constexpr std::size_t GateCategories =
        static_cast<std::size_t>(CircuitGateDecision::OutsideVerticalCorridor) + 1U;

    void Observe(CheckpointTraceResult result) noexcept
    {
        Increase(Samples);
        const std::size_t trace = static_cast<std::size_t>(result.Trace);
        if (trace < TraceCount.size())
        {
            Increase(TraceCount[trace]);
        }

        // A first sample and an authorized teleport never evaluate a gate.
        // Only genuinely evaluated rejected movements have gate-reason stats.
        if (result.Trace == CheckpointTraceDecision::GateRejected)
        {
            const std::size_t gate = static_cast<std::size_t>(result.Gate);
            if (gate < GateRejectionCount.size() &&
                result.Gate != CircuitGateDecision::Crossed)
            {
                Increase(GateRejectionCount[gate]);
            }
        }
    }

    void Merge(const GateDiagnosticCounters& other) noexcept
    {
        Samples = SaturatedAdd(Samples, other.Samples);
        for (std::size_t i = 0; i < TraceCount.size(); ++i)
        {
            TraceCount[i] = SaturatedAdd(TraceCount[i], other.TraceCount[i]);
        }
        for (std::size_t i = 0; i < GateRejectionCount.size(); ++i)
        {
            GateRejectionCount[i] =
                SaturatedAdd(GateRejectionCount[i], other.GateRejectionCount[i]);
        }
    }

    void Reset() noexcept { *this = GateDiagnosticCounters{}; }

    std::uint32_t TotalSamples() const noexcept { return Samples; }

    std::uint32_t CountTrace(CheckpointTraceDecision type) const noexcept
    {
        const auto i = static_cast<std::size_t>(type);
        return i < TraceCount.size() ? TraceCount[i] : 0U;
    }

    std::uint32_t CountGateRejection(CircuitGateDecision type) const noexcept
    {
        const auto i = static_cast<std::size_t>(type);
        return i < GateRejectionCount.size() ? GateRejectionCount[i] : 0U;
    }

private:
    static std::uint32_t SaturatedAdd(std::uint32_t lhs,
                                      std::uint32_t rhs) noexcept
    {
        constexpr auto limit = std::numeric_limits<std::uint32_t>::max();
        return rhs > limit - lhs ? limit : static_cast<std::uint32_t>(lhs + rhs);
    }

    static void Increase(std::uint32_t& count) noexcept
    {
        count = SaturatedAdd(count, 1U);
    }

    std::uint32_t Samples = 0U;
    std::array<std::uint32_t, TraceCategories> TraceCount{};
    std::array<std::uint32_t, GateCategories> GateRejectionCount{};
};
} // namespace latebrake
