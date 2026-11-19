#pragma once

#include "QualifyingSectorReview.h"
#include "QualifyingSession.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::competition {

// Pure, read-only consistency checker for two independently calculated
// qualifying displays. A matching receipt proves C++ model agreement, not
// authenticity of upstream Unreal/Chaos signals or official race results.
enum class GridAuditFault : std::uint8_t {
    None,
    MissingEvidence,
    InvalidCount,
    DuplicateActor,
    ForeignActor,
    InconsistentQualification,
    InvalidLapTime,
    ConflictingBestLap,
    InvalidTheoreticalPace
};

struct GridAuditSettings final {
    double maximumBestLapDifferenceSeconds = 0.05;
    double maximumTheoreticalOvershootSeconds = 0.05;
    bool requireIdenticalQualificationFlags = true;
};

struct AuditedGridEntry final {
    std::uint64_t actor = 0;
    int gridPosition = 0;
    int sectorLeaderboardPosition = 0;
    double measuredBestLapSeconds =
        std::numeric_limits<double>::infinity();
    double theoreticalSectorSumSeconds =
        std::numeric_limits<double>::infinity();
    double theoreticalGainSeconds = 0.0;
    bool hasValidLap = false;
    bool lapAgrees = false;
};

struct GridAuditReceipt final {
    std::array<AuditedGridEntry, MaximumQualifyingCars> byGridSlot{};
    GridAuditFault fault = GridAuditFault::MissingEvidence;
    std::size_t count = 0;
    std::size_t qualifiedCount = 0;
    std::size_t sectorQualifiedCount = 0;
    std::size_t sectorOrderingDifferences = 0;
    double maximumObservedBestLapDifferenceSeconds = 0.0;
    double maximumObservedTheoreticalGainSeconds = 0.0;
    int faultSlot = -1;
    bool fullyAgrees = false;
};

inline bool ValidGridAuditSettings(
    const GridAuditSettings& s) noexcept {
    return std::isfinite(s.maximumBestLapDifferenceSeconds) &&
           std::isfinite(s.maximumTheoreticalOvershootSeconds) &&
           s.maximumBestLapDifferenceSeconds >= 0.0 &&
           s.maximumBestLapDifferenceSeconds <= 0.5 &&
           s.maximumTheoreticalOvershootSeconds >= 0.0 &&
           s.maximumTheoreticalOvershootSeconds <= 0.5;
}

inline GridAuditReceipt AuditQualifyingGrid(
    const ProvisionalQualifyingGrid& grid,
    const ProvisionalSectorOrder& sectorOrder,
    GridAuditSettings settings = {}) noexcept {
    GridAuditReceipt output{};
    if (!grid.valid || !sectorOrder.valid ||
        !ValidGridAuditSettings(settings)) {
        return output;
    }
    if (grid.count == 0 ||
        grid.count > MaximumQualifyingCars ||
        sectorOrder.count != grid.count ||
        grid.qualifiedCount > grid.count ||
        sectorOrder.timedDrivers > grid.count) {
        output.fault = GridAuditFault::InvalidCount;
        return output;
    }
    output.count = grid.count;
    output.qualifiedCount = grid.qualifiedCount;
    output.sectorQualifiedCount = sectorOrder.timedDrivers;

    std::size_t observedQualifying = 0;
    std::size_t observedSectorQualifying = 0;
    for (std::size_t i = 0; i < grid.count; ++i) {
        output.faultSlot = static_cast<int>(i);
        const std::uint64_t actor = grid.actors[i];
        if (actor == 0) {
            output.fault = GridAuditFault::ForeignActor;
            return output;
        }
        for (std::size_t preceding = 0; preceding < i; ++preceding) {
            if (grid.actors[preceding] == actor) {
                output.fault = GridAuditFault::DuplicateActor;
                return output;
            }
        }
        int matchingSector = -1;
        for (std::size_t j = 0; j < sectorOrder.count; ++j) {
            const auto& fromSectors = sectorOrder.entries[j];
            if (fromSectors.actor == actor) {
                if (matchingSector != -1) {
                    output.fault = GridAuditFault::DuplicateActor;
                    return output;
                }
                matchingSector = static_cast<int>(j);
            }
        }
        if (matchingSector == -1) {
            output.fault = GridAuditFault::ForeignActor;
            return output;
        }

        const auto& sectors = sectorOrder.entries[
            static_cast<std::size_t>(matchingSector)];
        auto& audited = output.byGridSlot[i];
        audited.actor = actor;
        audited.gridPosition = static_cast<int>(i + 1);
        audited.sectorLeaderboardPosition = matchingSector + 1;
        audited.hasValidLap = grid.qualified[i];
        if (grid.qualified[i]) ++observedQualifying;
        if (sectors.hasEvidence) ++observedSectorQualifying;
        if (matchingSector != static_cast<int>(i)) {
            ++output.sectorOrderingDifferences;
        }

        if (settings.requireIdenticalQualificationFlags &&
            sectors.hasEvidence != grid.qualified[i]) {
            output.fault = GridAuditFault::InconsistentQualification;
            return output;
        }
        if (!grid.qualified[i]) {
            if (grid.bestLapOrdinals[i] != 0 ||
                sectors.reviewedLaps != 0 ||
                grid.bestLapSeconds[i] !=
                    std::numeric_limits<double>::infinity() ||
                sectors.measuredBestLapSeconds !=
                    std::numeric_limits<double>::infinity() ||
                sectors.bestSectorCombinationSeconds !=
                    std::numeric_limits<double>::infinity()) {
                output.fault = GridAuditFault::InconsistentQualification;
                return output;
            }
            audited.lapAgrees = true;
            continue;
        }

        const double gridLap = grid.bestLapSeconds[i];
        const double sectorLap = sectors.measuredBestLapSeconds;
        const double theoretical = sectors.bestSectorCombinationSeconds;
        if (!std::isfinite(gridLap) ||
            !std::isfinite(sectorLap) ||
            !std::isfinite(theoretical) ||
            gridLap <= 0.0 || sectorLap <= 0.0 ||
            grid.bestLapOrdinals[i] <= 0 ||
            sectors.reviewedLaps <= 0) {
            output.fault = GridAuditFault::InvalidLapTime;
            return output;
        }
        const double difference = std::abs(gridLap - sectorLap);
        output.maximumObservedBestLapDifferenceSeconds =
            std::max(output.maximumObservedBestLapDifferenceSeconds,
                     difference);
        if (difference > settings.maximumBestLapDifferenceSeconds) {
            output.fault = GridAuditFault::ConflictingBestLap;
            return output;
        }
        // Sector minimums, each measured on a real completed lap, cannot
        // be slower than the independently measured best valid whole lap,
        // modulo a bounded source clock difference.
        if (theoretical > gridLap +
                settings.maximumTheoreticalOvershootSeconds ||
            theoretical <= 0.0) {
            output.fault = GridAuditFault::InvalidTheoreticalPace;
            return output;
        }
        audited.measuredBestLapSeconds = gridLap;
        audited.theoreticalSectorSumSeconds = theoretical;
        audited.theoreticalGainSeconds =
            std::max(0.0, gridLap - theoretical);
        audited.lapAgrees = true;
        output.maximumObservedTheoreticalGainSeconds =
            std::max(output.maximumObservedTheoreticalGainSeconds,
                     audited.theoreticalGainSeconds);
    }

    if (observedQualifying != grid.qualifiedCount ||
        observedSectorQualifying != sectorOrder.timedDrivers) {
        output.fault = GridAuditFault::InvalidCount;
        output.faultSlot = -1;
        return output;
    }
    output.fault = GridAuditFault::None;
    output.faultSlot = -1;
    output.fullyAgrees = true;
    return output;
}

} // namespace latebrake::competition
