#pragma once

#include "QualifyingStartGridPlan.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::competition {

// A bounded, source-only race-director review. Neither a signal's booleans
// nor a provisional grid constitute authenticated Unreal/Chaos evidence.
// A live GameMode and real marshal controller must independently enforce
// flags, vehicle speeds, passing restrictions and official results.
inline constexpr std::size_t MaximumRaceControlSignals = 64;

enum class ProvisionalRacePhase : std::uint8_t {
    Unconfigured,
    Formation,
    Green,
    FullCourseYellow,
    RedFlag,
    Finished
};

enum class ProvisionalRaceCommand : std::uint8_t {
    StartGreen,
    DeployYellow,
    ResumeGreen,
    StopRed,
    Reform,
    Finish,
    Abandon
};

enum class RaceControlAdmission : std::uint8_t {
    Accepted,
    NoSession,
    WrongSession,
    UnverifiedSource,
    InvalidTiming,
    StaleSignal,
    InvalidTransition,
    MinimumHoldNotElapsed,
    CapacityReached
};

struct RaceControlSettings final {
    std::uint64_t raceSession = 0;
    std::uint64_t qualifyingSession = 0;
    std::uint64_t circuit = 0;
    std::uint64_t authorizedMarshal = 0;
    double sessionStartSeconds = 0.0;
    double maximumClockSeconds = 86400.0;
    double minimumFormationSeconds = 5.0;
    double minimumYellowSeconds = 3.0;
    double minimumRedSeconds = 8.0;
    double formationAdvisorySpeedMps = 8.0;
    double yellowAdvisorySpeedMps = 20.0;
    int maximumSignals = 48;
};

inline bool ValidRaceControlSettings(
    const RaceControlSettings& s) noexcept {
    return s.raceSession != 0 &&
           s.qualifyingSession != 0 &&
           s.raceSession != s.qualifyingSession &&
           s.circuit != 0 &&
           s.authorizedMarshal != 0 &&
           std::isfinite(s.sessionStartSeconds) &&
           std::isfinite(s.maximumClockSeconds) &&
           std::isfinite(s.minimumFormationSeconds) &&
           std::isfinite(s.minimumYellowSeconds) &&
           std::isfinite(s.minimumRedSeconds) &&
           std::isfinite(s.formationAdvisorySpeedMps) &&
           std::isfinite(s.yellowAdvisorySpeedMps) &&
           s.sessionStartSeconds >= 0.0 &&
           s.maximumClockSeconds > s.sessionStartSeconds &&
           s.maximumClockSeconds <= 172800.0 &&
           s.minimumFormationSeconds > 0.0 &&
           s.minimumYellowSeconds > 0.0 &&
           s.minimumRedSeconds > 0.0 &&
           s.minimumFormationSeconds <= 3600.0 &&
           s.minimumYellowSeconds <= 3600.0 &&
           s.minimumRedSeconds <= 3600.0 &&
           s.formationAdvisorySpeedMps > 0.0 &&
           s.yellowAdvisorySpeedMps >
               s.formationAdvisorySpeedMps &&
           s.yellowAdvisorySpeedMps <= 90.0 &&
           s.maximumSignals >= 1 &&
           s.maximumSignals <=
               static_cast<int>(MaximumRaceControlSignals);
}

struct PhysicalRaceControlSignal final {
    std::uint64_t raceSession = 0;
    std::uint64_t circuit = 0;
    std::uint64_t marshal = 0;
    std::uint64_t engineFrame = 0;
    double engineClockSeconds = 0.0;
    ProvisionalRaceCommand command = ProvisionalRaceCommand::StartGreen;
    bool originalCircuitVerified = false;
    bool observedFromEngine = false;
    bool samePhysicalRosterVerified = false;
    bool engineClockVerified = false;
    bool raceControlAuthorized = false;
    bool gameModeAcknowledged = false;
};

struct ProvisionalRaceControlRecord final {
    ProvisionalRacePhase from = ProvisionalRacePhase::Unconfigured;
    ProvisionalRacePhase to = ProvisionalRacePhase::Unconfigured;
    ProvisionalRaceCommand command = ProvisionalRaceCommand::StartGreen;
    std::uint64_t frame = 0;
    std::uint64_t marshal = 0;
    double clockSeconds = 0.0;
    double previousPhaseSeconds = 0.0;
};

struct RaceControlReceipt final {
    RaceControlAdmission admission = RaceControlAdmission::NoSession;
    ProvisionalRacePhase from = ProvisionalRacePhase::Unconfigured;
    ProvisionalRacePhase to = ProvisionalRacePhase::Unconfigured;
    double previousPhaseSeconds = 0.0;
    int eventOrdinal = 0;
    bool accepted = false;
};

struct RaceControlAdvice final {
    ProvisionalRacePhase phase = ProvisionalRacePhase::Unconfigured;
    double recommendedSpeedCapMps = 0.0;
    bool speedCapRecommended = true;
    bool noPassingRecommended = true;
    bool mustHoldStartingGrid = true;
    int acceptedSignals = 0;
    int yellowPeriods = 0;
    int redPeriods = 0;
    int formationPeriods = 0;
    double completedFormationSeconds = 0.0;
    double completedGreenSeconds = 0.0;
    double completedYellowSeconds = 0.0;
    double completedRedSeconds = 0.0;
    std::uint64_t latestFrame = 0;
    bool valid = false;
};

class ProvisionalRaceControl final {
public:
    bool Begin(const RaceControlSettings& settings,
               const QualifyingStartGridPlan& proposedGrid) noexcept {
        if (active_ || !ValidRaceControlSettings(settings) ||
            !proposedGrid.valid ||
            proposedGrid.fault != QualifyingGridPlanFault::None ||
            proposedGrid.session != settings.qualifyingSession ||
            proposedGrid.count != MaximumQualifyingCars ||
            proposedGrid.qualifiedCount == 0 ||
            proposedGrid.qualifiedCount > MaximumQualifyingCars ||
            !latebrake::IsPlannedStartGridSafe()) {
            return false;
        }
        std::size_t qualified = 0;
        for (std::size_t slot = 0; slot < MaximumQualifyingCars; ++slot) {
            const auto& actor = proposedGrid.slots[slot];
            const auto& pose = latebrake::StartGridPoses[slot];
            if (actor.actor == 0 ||
                actor.plannedSlot != static_cast<int>(slot) ||
                !actor.rosterObserved ||
                actor.originalPose.Position.X != pose.Position.X ||
                actor.originalPose.Position.Y != pose.Position.Y ||
                actor.originalPose.HeadingDegrees != pose.HeadingDegrees) {
                return false;
            }
            if (actor.qualified) {
                if (!std::isfinite(actor.bestQualifyingSeconds) ||
                    actor.bestQualifyingSeconds <= 0.0) return false;
                ++qualified;
            } else if (actor.bestQualifyingSeconds !=
                       std::numeric_limits<double>::infinity()) {
                return false;
            }
            for (std::size_t prior = 0; prior < slot; ++prior) {
                if (actor.actor == proposedGrid.slots[prior].actor) {
                    return false;
                }
            }
        }
        if (qualified != proposedGrid.qualifiedCount) return false;
        settings_ = settings;
        active_ = true;
        phase_ = ProvisionalRacePhase::Formation;
        latestClockSeconds_ = settings.sessionStartSeconds;
        phaseStartSeconds_ = settings.sessionStartSeconds;
        formationPeriods_ = 1;
        return true;
    }

    RaceControlReceipt Observe(
        const PhysicalRaceControlSignal& source) noexcept {
        RaceControlReceipt receipt{};
        if (!active_) return receipt;
        receipt.from = phase_;
        receipt.to = phase_;
        if (source.raceSession != settings_.raceSession ||
            source.circuit != settings_.circuit) {
            receipt.admission = RaceControlAdmission::WrongSession;
            return receipt;
        }
        if (source.marshal != settings_.authorizedMarshal ||
            !source.originalCircuitVerified ||
            !source.observedFromEngine ||
            !source.samePhysicalRosterVerified ||
            !source.engineClockVerified ||
            !source.raceControlAuthorized ||
            !source.gameModeAcknowledged ||
            source.engineFrame == 0) {
            receipt.admission = RaceControlAdmission::UnverifiedSource;
            return receipt;
        }
        if (!std::isfinite(source.engineClockSeconds) ||
            source.engineClockSeconds < settings_.sessionStartSeconds ||
            source.engineClockSeconds > settings_.maximumClockSeconds) {
            receipt.admission = RaceControlAdmission::InvalidTiming;
            return receipt;
        }
        if (source.engineFrame <= latestFrame_ ||
            source.engineClockSeconds <= latestClockSeconds_) {
            receipt.admission = RaceControlAdmission::StaleSignal;
            return receipt;
        }
        const double elapsed =
            source.engineClockSeconds - phaseStartSeconds_;
        if (!std::isfinite(elapsed) || elapsed < 0.0) {
            receipt.admission = RaceControlAdmission::InvalidTiming;
            return receipt;
        }
        receipt.previousPhaseSeconds = elapsed;
        ProvisionalRacePhase requested = phase_;
        switch (source.command) {
        case ProvisionalRaceCommand::StartGreen:
            if (phase_ != ProvisionalRacePhase::Formation) {
                receipt.admission = RaceControlAdmission::InvalidTransition;
                return receipt;
            }
            if (elapsed < settings_.minimumFormationSeconds) {
                receipt.admission = RaceControlAdmission::MinimumHoldNotElapsed;
                return receipt;
            }
            requested = ProvisionalRacePhase::Green;
            break;
        case ProvisionalRaceCommand::DeployYellow:
            if (phase_ != ProvisionalRacePhase::Green) {
                receipt.admission = RaceControlAdmission::InvalidTransition;
                return receipt;
            }
            requested = ProvisionalRacePhase::FullCourseYellow;
            break;
        case ProvisionalRaceCommand::ResumeGreen:
            if (phase_ != ProvisionalRacePhase::FullCourseYellow) {
                receipt.admission = RaceControlAdmission::InvalidTransition;
                return receipt;
            }
            if (elapsed < settings_.minimumYellowSeconds) {
                receipt.admission = RaceControlAdmission::MinimumHoldNotElapsed;
                return receipt;
            }
            requested = ProvisionalRacePhase::Green;
            break;
        case ProvisionalRaceCommand::StopRed:
            if (phase_ != ProvisionalRacePhase::Formation &&
                phase_ != ProvisionalRacePhase::Green &&
                phase_ != ProvisionalRacePhase::FullCourseYellow) {
                receipt.admission = RaceControlAdmission::InvalidTransition;
                return receipt;
            }
            requested = ProvisionalRacePhase::RedFlag;
            break;
        case ProvisionalRaceCommand::Reform:
            if (phase_ != ProvisionalRacePhase::RedFlag) {
                receipt.admission = RaceControlAdmission::InvalidTransition;
                return receipt;
            }
            if (elapsed < settings_.minimumRedSeconds) {
                receipt.admission = RaceControlAdmission::MinimumHoldNotElapsed;
                return receipt;
            }
            requested = ProvisionalRacePhase::Formation;
            break;
        case ProvisionalRaceCommand::Finish:
            if (phase_ != ProvisionalRacePhase::Green &&
                phase_ != ProvisionalRacePhase::FullCourseYellow) {
                receipt.admission = RaceControlAdmission::InvalidTransition;
                return receipt;
            }
            requested = ProvisionalRacePhase::Finished;
            break;
        case ProvisionalRaceCommand::Abandon:
            if (phase_ != ProvisionalRacePhase::RedFlag) {
                receipt.admission = RaceControlAdmission::InvalidTransition;
                return receipt;
            }
            requested = ProvisionalRacePhase::Finished;
            break;
        default:
            receipt.admission = RaceControlAdmission::InvalidTransition;
            return receipt;
        }
        if (acceptedSignals_ >= settings_.maximumSignals) {
            receipt.admission = RaceControlAdmission::CapacityReached;
            return receipt;
        }
        // All validation above is read-only. Publish an indivisible
        // transition and journal event only after the complete check.
        ProvisionalRaceControlRecord record{};
        record.from = phase_;
        record.to = requested;
        record.command = source.command;
        record.frame = source.engineFrame;
        record.marshal = source.marshal;
        record.clockSeconds = source.engineClockSeconds;
        record.previousPhaseSeconds = elapsed;
        records_[static_cast<std::size_t>(acceptedSignals_)] = record;
        switch (phase_) {
        case ProvisionalRacePhase::Formation:
            completedFormationSeconds_ += elapsed;
            break;
        case ProvisionalRacePhase::Green:
            completedGreenSeconds_ += elapsed;
            break;
        case ProvisionalRacePhase::FullCourseYellow:
            completedYellowSeconds_ += elapsed;
            break;
        case ProvisionalRacePhase::RedFlag:
            completedRedSeconds_ += elapsed;
            break;
        default:
            break;
        }
        if (requested == ProvisionalRacePhase::FullCourseYellow) {
            ++yellowPeriods_;
        } else if (requested == ProvisionalRacePhase::RedFlag) {
            ++redPeriods_;
        } else if (requested == ProvisionalRacePhase::Formation) {
            ++formationPeriods_;
        }
        phase_ = requested;
        phaseStartSeconds_ = source.engineClockSeconds;
        latestClockSeconds_ = source.engineClockSeconds;
        latestFrame_ = source.engineFrame;
        ++acceptedSignals_;
        receipt.to = requested;
        receipt.admission = RaceControlAdmission::Accepted;
        receipt.eventOrdinal = acceptedSignals_;
        receipt.accepted = true;
        return receipt;
    }

    RaceControlAdvice Advice() const noexcept {
        RaceControlAdvice output{};
        if (!active_) return output;
        output.phase = phase_;
        output.acceptedSignals = acceptedSignals_;
        output.yellowPeriods = yellowPeriods_;
        output.redPeriods = redPeriods_;
        output.formationPeriods = formationPeriods_;
        output.completedFormationSeconds = completedFormationSeconds_;
        output.completedGreenSeconds = completedGreenSeconds_;
        output.completedYellowSeconds = completedYellowSeconds_;
        output.completedRedSeconds = completedRedSeconds_;
        output.latestFrame = latestFrame_;
        output.noPassingRecommended =
            phase_ != ProvisionalRacePhase::Green;
        output.mustHoldStartingGrid =
            phase_ == ProvisionalRacePhase::Formation ||
            phase_ == ProvisionalRacePhase::RedFlag ||
            phase_ == ProvisionalRacePhase::Finished;
        output.speedCapRecommended =
            phase_ != ProvisionalRacePhase::Green;
        switch (phase_) {
        case ProvisionalRacePhase::Formation:
            output.recommendedSpeedCapMps =
                settings_.formationAdvisorySpeedMps;
            break;
        case ProvisionalRacePhase::FullCourseYellow:
            output.recommendedSpeedCapMps =
                settings_.yellowAdvisorySpeedMps;
            break;
        case ProvisionalRacePhase::Green:
            output.recommendedSpeedCapMps =
                std::numeric_limits<double>::infinity();
            break;
        default:
            output.recommendedSpeedCapMps = 0.0;
            break;
        }
        output.valid = true;
        return output;
    }

    const ProvisionalRaceControlRecord* Record(
        std::size_t ordinalZeroBased) const noexcept {
        if (!active_ ||
            ordinalZeroBased >= static_cast<std::size_t>(acceptedSignals_)) {
            return nullptr;
        }
        return &records_[ordinalZeroBased];
    }

    bool Active() const noexcept { return active_; }
    ProvisionalRacePhase Phase() const noexcept { return phase_; }
    int AcceptedSignals() const noexcept { return acceptedSignals_; }

private:
    RaceControlSettings settings_{};
    std::array<ProvisionalRaceControlRecord,
               MaximumRaceControlSignals> records_{};
    ProvisionalRacePhase phase_ = ProvisionalRacePhase::Unconfigured;
    double latestClockSeconds_ = 0.0;
    double phaseStartSeconds_ = 0.0;
    double completedFormationSeconds_ = 0.0;
    double completedGreenSeconds_ = 0.0;
    double completedYellowSeconds_ = 0.0;
    double completedRedSeconds_ = 0.0;
    std::uint64_t latestFrame_ = 0;
    int acceptedSignals_ = 0;
    int yellowPeriods_ = 0;
    int redPeriods_ = 0;
    int formationPeriods_ = 0;
    bool active_ = false;
};

} // namespace latebrake::competition
