#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::competition {

// This is a deterministic season-review ledger, not a replacement for the
// authoritative Unreal race director, save format or official award system.
inline constexpr std::size_t MaximumSeasonDrivers = 6;
inline constexpr std::size_t MaximumSeasonRounds = 24;
inline constexpr std::array<int, MaximumSeasonDrivers> SeasonPositionPoints{
    25, 18, 15, 12, 10, 8
};

enum class SeasonAdmission : std::uint8_t {
    Accepted,
    NotConfigured,
    WrongSeason,
    WrongRound,
    InvalidField,
    InvalidRoster,
    InvalidClassification,
    UntrustedResult,
    Complete
};

enum class FinishState : std::uint8_t {
    Classified,
    Retired,
    Disqualified,
    DidNotStart
};

struct SeasonSettings final {
    std::uint64_t seasonIdentity = 0;
    int scheduledRounds = 0;
    int requiredRaceLaps = 0;
    bool requireFullGrid = true;
    bool awardFastestLapBonus = true;
    double minimumLegalRaceSeconds = 10.0;
    double maximumLegalRaceSeconds = 86400.0;
    double minimumLegalLapSeconds = 2.0;
};

inline bool ValidSeasonSettings(const SeasonSettings& s) noexcept {
    return s.seasonIdentity != 0 &&
           s.scheduledRounds > 0 &&
           s.scheduledRounds <= static_cast<int>(MaximumSeasonRounds) &&
           s.requiredRaceLaps > 0 &&
           s.requiredRaceLaps <= 1000 &&
           std::isfinite(s.minimumLegalRaceSeconds) &&
           std::isfinite(s.maximumLegalRaceSeconds) &&
           std::isfinite(s.minimumLegalLapSeconds) &&
           s.minimumLegalRaceSeconds > 0.0 &&
           s.maximumLegalRaceSeconds >= s.minimumLegalRaceSeconds &&
           s.maximumLegalRaceSeconds <= 172800.0 &&
           s.minimumLegalLapSeconds >= 1.0 &&
           s.minimumLegalLapSeconds <= s.maximumLegalRaceSeconds;
}

struct SeasonRaceResult final {
    std::uint64_t actor = 0;
    FinishState state = FinishState::DidNotStart;
    int officialPosition = 0;
    int completedLaps = 0;
    double officialRaceSeconds = 0.0;
    double fastestLegalLapSeconds = 0.0;
    std::uint64_t lastVerifiedFrame = 0;
    bool realPhysicalActor = false;
    bool officialClassificationVerified = false;
    bool originalRaceSessionVerified = false;
    bool physicalFinishGateVerified = false;
    bool fastestLapVerified = false;
};

struct SeasonRoundEvidence final {
    std::uint64_t seasonIdentity = 0;
    std::uint64_t raceSessionIdentity = 0;
    std::uint64_t circuitIdentity = 0;
    int roundNumber = 0;
    int resultCount = 0;
    bool originalCircuitVerified = false;
    bool gameModeResultsVerified = false;
    bool raceClockClosed = false;
    bool controlStateSafe = false;
    double officialClockSeconds = 0.0;
    std::array<SeasonRaceResult, MaximumSeasonDrivers> participants{};
};

struct DriverSeasonSummary final {
    std::uint64_t actor = 0;
    int points = 0;
    int roundsEntered = 0;
    int classifiedFinishes = 0;
    int fastestLapBonuses = 0;
    int wins = 0;
    int podiums = 0;
    std::array<int, MaximumSeasonDrivers> positionCounts{};
    std::array<int, MaximumSeasonRounds> pointsPerRound{};
};

struct SeasonRanking final {
    std::array<DriverSeasonSummary, MaximumSeasonDrivers> sorted{};
    std::size_t count = 0;
    int completedRounds = 0;
    int remainingRounds = 0;
    bool valid = false;
};

struct AcceptedRoundSummary final {
    std::uint64_t session = 0;
    std::uint64_t circuit = 0;
    std::array<int, MaximumSeasonDrivers> awardedPoints{};
    int roundNumber = 0;
    int classifiedFinishers = 0;
    std::uint64_t fastestBonusActor = 0;
    bool valid = false;
};

class ChampionshipSeason final {
public:
    bool Begin(const SeasonSettings& settings,
               const std::array<std::uint64_t, MaximumSeasonDrivers>& actors,
               std::size_t driverCount) noexcept {
        if (active_ || !ValidSeasonSettings(settings) ||
            driverCount < 2 || driverCount > MaximumSeasonDrivers ||
            (settings.requireFullGrid &&
             driverCount != MaximumSeasonDrivers)) {
            return false;
        }
        for (std::size_t i = 0; i < driverCount; ++i) {
            if (actors[i] == 0) return false;
            for (std::size_t j = 0; j < i; ++j) {
                if (actors[i] == actors[j]) return false;
            }
        }
        settings_ = settings;
        count_ = driverCount;
        for (std::size_t i = 0; i < count_; ++i) {
            drivers_[i] = DriverSeasonSummary{};
            drivers_[i].actor = actors[i];
        }
        active_ = true;
        return true;
    }

    SeasonAdmission Accept(const SeasonRoundEvidence& round) noexcept {
        if (!active_) return SeasonAdmission::NotConfigured;
        if (round.seasonIdentity != settings_.seasonIdentity) {
            return SeasonAdmission::WrongSeason;
        }
        if (completedRounds_ >= settings_.scheduledRounds) {
            return SeasonAdmission::Complete;
        }
        if (round.roundNumber != completedRounds_ + 1) {
            return SeasonAdmission::WrongRound;
        }
        if (round.raceSessionIdentity == 0 ||
            round.circuitIdentity == 0 ||
            round.resultCount != static_cast<int>(count_) ||
            !std::isfinite(round.officialClockSeconds) ||
            round.officialClockSeconds < settings_.minimumLegalRaceSeconds ||
            round.officialClockSeconds > settings_.maximumLegalRaceSeconds) {
            return SeasonAdmission::InvalidField;
        }
        if (!round.originalCircuitVerified ||
            !round.gameModeResultsVerified ||
            !round.raceClockClosed ||
            !round.controlStateSafe) {
            return SeasonAdmission::UntrustedResult;
        }
        // Duplicate session epochs are rejected even when the stated round
        // number differs. The history is bounded and never overwritten.
        for (int i = 0; i < completedRounds_; ++i) {
            if (history_[static_cast<std::size_t>(i)].session ==
                round.raceSessionIdentity) {
                return SeasonAdmission::WrongRound;
            }
        }

        std::array<std::size_t, MaximumSeasonDrivers> resultByDriver{};
        std::array<bool, MaximumSeasonDrivers> matched{};
        std::array<bool, MaximumSeasonDrivers + 1> seenPositions{};
        int classifiedCount = 0;
        for (std::size_t r = 0; r < count_; ++r) {
            const auto& car = round.participants[r];
            std::size_t index = count_;
            for (std::size_t d = 0; d < count_; ++d) {
                if (drivers_[d].actor == car.actor) {
                    index = d;
                    break;
                }
            }
            if (index == count_ || matched[index]) {
                return SeasonAdmission::InvalidRoster;
            }
            matched[index] = true;
            resultByDriver[index] = r;
            if (!car.realPhysicalActor ||
                !car.officialClassificationVerified ||
                !car.originalRaceSessionVerified ||
                car.lastVerifiedFrame == 0) {
                return SeasonAdmission::UntrustedResult;
            }
            if (!std::isfinite(car.officialRaceSeconds) ||
                !std::isfinite(car.fastestLegalLapSeconds) ||
                car.officialRaceSeconds < 0.0 ||
                car.officialRaceSeconds > round.officialClockSeconds ||
                car.completedLaps < 0 ||
                car.completedLaps > settings_.requiredRaceLaps) {
                return SeasonAdmission::InvalidField;
            }
            if (car.state == FinishState::Classified) {
                if (car.officialPosition <= 0 ||
                    car.officialPosition > static_cast<int>(count_) ||
                    seenPositions[static_cast<std::size_t>(
                        car.officialPosition)] ||
                    car.completedLaps != settings_.requiredRaceLaps ||
                    !car.physicalFinishGateVerified ||
                    car.officialRaceSeconds <
                        settings_.minimumLegalRaceSeconds) {
                    return SeasonAdmission::InvalidClassification;
                }
                seenPositions[static_cast<std::size_t>(
                    car.officialPosition)] = true;
                ++classifiedCount;
            } else if (car.officialPosition != 0 ||
                       car.physicalFinishGateVerified) {
                return SeasonAdmission::InvalidClassification;
            }
            if (car.fastestLapVerified) {
                if (car.state != FinishState::Classified ||
                    car.fastestLegalLapSeconds <
                        settings_.minimumLegalLapSeconds ||
                    car.fastestLegalLapSeconds >
                        car.officialRaceSeconds) {
                    return SeasonAdmission::InvalidField;
                }
            } else if (car.fastestLegalLapSeconds != 0.0) {
                return SeasonAdmission::InvalidField;
            }
        }
        for (int p = 1; p <= classifiedCount; ++p) {
            if (!seenPositions[static_cast<std::size_t>(p)]) {
                return SeasonAdmission::InvalidClassification;
            }
        }

        std::size_t fastest = count_;
        for (std::size_t d = 0; d < count_; ++d) {
            const auto& candidate = round.participants[resultByDriver[d]];
            if (!candidate.fastestLapVerified) continue;
            if (fastest == count_) {
                fastest = d;
                continue;
            }
            const auto& previous =
                round.participants[resultByDriver[fastest]];
            if (candidate.fastestLegalLapSeconds <
                previous.fastestLegalLapSeconds) {
                fastest = d;
            }
        }
        // All checks above are read-only. Commit a whole round atomically.
        AcceptedRoundSummary accepted{};
        accepted.valid = true;
        accepted.session = round.raceSessionIdentity;
        accepted.circuit = round.circuitIdentity;
        accepted.roundNumber = round.roundNumber;
        accepted.classifiedFinishers = classifiedCount;
        if (settings_.awardFastestLapBonus && fastest != count_) {
            accepted.fastestBonusActor = drivers_[fastest].actor;
        }
        for (std::size_t d = 0; d < count_; ++d) {
            const auto& source = round.participants[resultByDriver[d]];
            DriverSeasonSummary& destination = drivers_[d];
            int award = 0;
            ++destination.roundsEntered;
            if (source.state == FinishState::Classified) {
                const std::size_t rank =
                    static_cast<std::size_t>(source.officialPosition - 1);
                award = SeasonPositionPoints[rank];
                ++destination.positionCounts[rank];
                ++destination.classifiedFinishes;
                if (rank == 0) ++destination.wins;
                if (rank < 3) ++destination.podiums;
            }
            if (accepted.fastestBonusActor == source.actor) {
                ++award;
                ++destination.fastestLapBonuses;
            }
            destination.points += award;
            destination.pointsPerRound[
                static_cast<std::size_t>(completedRounds_)] = award;
            accepted.awardedPoints[d] = award;
        }
        history_[static_cast<std::size_t>(completedRounds_)] = accepted;
        ++completedRounds_;
        return SeasonAdmission::Accepted;
    }

    SeasonRanking Ranking() const noexcept {
        SeasonRanking output{};
        if (!active_) return output;
        output.valid = true;
        output.count = count_;
        output.completedRounds = completedRounds_;
        output.remainingRounds = settings_.scheduledRounds - completedRounds_;
        for (std::size_t i = 0; i < count_; ++i) {
            output.sorted[i] = drivers_[i];
        }
        for (std::size_t i = 1; i < count_; ++i) {
            const auto candidate = output.sorted[i];
            std::size_t j = i;
            while (j > 0 && BetterThan(candidate, output.sorted[j - 1])) {
                output.sorted[j] = output.sorted[j - 1];
                --j;
            }
            output.sorted[j] = candidate;
        }
        return output;
    }

    const AcceptedRoundSummary* Round(int number) const noexcept {
        if (!active_ || number <= 0 || number > completedRounds_) {
            return nullptr;
        }
        return &history_[static_cast<std::size_t>(number - 1)];
    }

    const DriverSeasonSummary* Driver(std::uint64_t actor) const noexcept {
        if (!active_ || actor == 0) return nullptr;
        for (std::size_t i = 0; i < count_; ++i) {
            if (drivers_[i].actor == actor) return &drivers_[i];
        }
        return nullptr;
    }

    int CompletedRounds() const noexcept { return completedRounds_; }
    bool Active() const noexcept { return active_; }

private:
    static bool BetterThan(const DriverSeasonSummary& a,
                           const DriverSeasonSummary& b) noexcept {
        if (a.points != b.points) return a.points > b.points;
        if (a.wins != b.wins) return a.wins > b.wins;
        if (a.podiums != b.podiums) return a.podiums > b.podiums;
        for (std::size_t i = 0; i < MaximumSeasonDrivers; ++i) {
            if (a.positionCounts[i] != b.positionCounts[i]) {
                return a.positionCounts[i] > b.positionCounts[i];
            }
        }
        return false;
    }

    SeasonSettings settings_{};
    std::array<DriverSeasonSummary, MaximumSeasonDrivers> drivers_{};
    std::array<AcceptedRoundSummary, MaximumSeasonRounds> history_{};
    std::size_t count_ = 0;
    int completedRounds_ = 0;
    bool active_ = false;
};

} // namespace latebrake::competition
