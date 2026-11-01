#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace latebrake
{
// Read-only observations from accepted forward gate crossings on one local
// post-GO clock. No official race placement, reward or physics authority.
struct RivalMatchedGateSplit final
{
    bool Valid = false;
    int GateOrdinal = 0;
    int LapNumber = 0;
    int GateNumber = 0;
    double PlayerSeconds = 0.0;
    double RivalSeconds = 0.0;
    // Positive: rival arrived later than player; negative: earlier.
    double RivalMinusPlayerSeconds = 0.0;
};

class RivalMatchedGateSplits final
{
public:
    static constexpr int HistoryCapacity = 32;
    static constexpr double MaxElapsedSeconds = 7.0 * 24.0 * 3600.0;

    void Configure(int checkpointCount, int requiredLaps) noexcept
    {
        ValidConfiguration = checkpointCount >= 1 && checkpointCount <= 256 &&
            requiredLaps >= 1 && requiredLaps <= 100;
        GateCount = ValidConfiguration ? checkpointCount : 0;
        TotalGates = ValidConfiguration ? checkpointCount * requiredLaps : 0;
        Reset();
    }

    void Reset() noexcept
    {
        PlayerSamples.fill({});
        RivalSamples.fill({});
        LastPlayerOrdinal = 0;
        LastRivalOrdinal = 0;
        LastPlayerSeconds = 0.0;
        LastRivalSeconds = 0.0;
        Latest = {};
    }

    bool RecordPlayer(int gateOrdinal, double elapsedSeconds) noexcept
    {
        return Record(true, gateOrdinal, elapsedSeconds);
    }

    bool RecordRival(int gateOrdinal, double elapsedSeconds) noexcept
    {
        return Record(false, gateOrdinal, elapsedSeconds);
    }

    RivalMatchedGateSplit LatestMatched() const noexcept { return Latest; }

private:
    struct Sample final
    {
        int Ordinal = 0;
        double Seconds = 0.0;
    };

    bool Record(bool player, int ordinal, double seconds) noexcept
    {
        if (!ValidConfiguration || ordinal < 1 || ordinal > TotalGates ||
            !std::isfinite(seconds) || seconds <= 0.0 ||
            seconds > MaxElapsedSeconds)
        {
            return false;
        }
        int& lastOrdinal = player ? LastPlayerOrdinal : LastRivalOrdinal;
        double& lastSeconds = player ? LastPlayerSeconds : LastRivalSeconds;
        if (ordinal <= lastOrdinal || seconds <= lastSeconds)
        {
            // A duplicate, stale gate or reversed clock must never overwrite
            // accepted timing or forge a newer matching split.
            return false;
        }
        auto& own = player ? PlayerSamples : RivalSamples;
        const auto& other = player ? RivalSamples : PlayerSamples;
        const std::size_t index =
            static_cast<std::size_t>((ordinal - 1) % HistoryCapacity);
        own[index] = {ordinal, seconds};
        lastOrdinal = ordinal;
        lastSeconds = seconds;
        const Sample matched = other[index];
        if (matched.Ordinal == ordinal && matched.Seconds > 0.0 &&
            (!Latest.Valid || ordinal > Latest.GateOrdinal))
        {
            const double playerTime = player ? seconds : matched.Seconds;
            const double rivalTime = player ? matched.Seconds : seconds;
            const double delta = rivalTime - playerTime;
            if (std::isfinite(delta))
            {
                Latest = {true, ordinal,
                    (ordinal - 1) / GateCount + 1,
                    (ordinal - 1) % GateCount + 1,
                    playerTime, rivalTime, delta};
            }
        }
        return true;
    }

    bool ValidConfiguration = false;
    int GateCount = 0;
    int TotalGates = 0;
    std::array<Sample, HistoryCapacity> PlayerSamples{};
    std::array<Sample, HistoryCapacity> RivalSamples{};
    int LastPlayerOrdinal = 0;
    int LastRivalOrdinal = 0;
    double LastPlayerSeconds = 0.0;
    double LastRivalSeconds = 0.0;
    RivalMatchedGateSplit Latest{};
};
} // namespace latebrake
