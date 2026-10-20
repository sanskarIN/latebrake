#pragma once

#include "Career/CareerChallenges.h"
#include <algorithm>
#include <array>
#include <cstdint>

namespace latebrake
{
inline constexpr int CareerSaveSchema = 2;

struct CareerSnapshot final
{
    int Schema = CareerSaveSchema;
    int SelectedEvent = -1;
    std::array<std::uint8_t, CareerEventCount> Medals{};
};

struct CareerAward final
{
    bool ValidFinish = false;
    bool FirstCompletion = false;
    bool MedalImproved = false;
    CareerMedal Medal = CareerMedal::None;
    int Credits = 0;
    int ChampionshipBonus = 0;
    int NewlyAvailable = -1;
};

class CareerProgress final
{
public:
    void Load(const CareerSnapshot& saved) noexcept
    {
        Data = {};
        // Legacy profiles (schema <= 0), and newer, unknown saves start
        // with no career state. Unrelated garage and time records are kept
        // separately in the Unreal SaveGame object.
        if (saved.Schema != CareerSaveSchema)
        {
            return;
        }
        for (int id = 0; id < CareerEventCount; ++id)
        {
            const auto value = saved.Medals[static_cast<std::size_t>(id)];
            if (value < 1 || value > 3) continue;
            const CareerEvent& event = CareerEvents[static_cast<std::size_t>(id)];
            if (event.Prerequisite >= 0 &&
                Data.Medals[static_cast<std::size_t>(event.Prerequisite)] == 0)
            {
                continue; // Never migrate impossible locked completions.
            }
            Data.Medals[static_cast<std::size_t>(id)] = value;
        }
        if (IsCareerId(saved.SelectedEvent) &&
            Status(saved.SelectedEvent) != CareerStatus::Locked)
        {
            Data.SelectedEvent = saved.SelectedEvent;
        }
    }

    CareerSnapshot Snapshot() const noexcept { return Data; }
    int SelectedEvent() const noexcept { return Data.SelectedEvent; }
    CareerMedal Medal(int id) const noexcept
    {
        return IsCareerId(id)
            ? static_cast<CareerMedal>(Data.Medals[static_cast<std::size_t>(id)])
            : CareerMedal::None;
    }

    CareerStatus Status(int id) const noexcept
    {
        const CareerEvent* event = GetCareerEvent(id);
        if (!event) return CareerStatus::Locked;
        const auto medal = Medal(id);
        if (medal == CareerMedal::Gold) return CareerStatus::Mastered;
        if (medal != CareerMedal::None) return CareerStatus::Completed;
        if (event->Prerequisite >= 0 &&
            Medal(event->Prerequisite) == CareerMedal::None)
        {
            return CareerStatus::Locked;
        }
        return CareerStatus::Available;
    }

    bool Select(int id) noexcept
    {
        if (Status(id) == CareerStatus::Locked) return false;
        Data.SelectedEvent = id;
        return true;
    }

    int CompletedCount() const noexcept
    {
        int count = 0;
        for (int id = 0; id < CareerEventCount; ++id)
            if (Medal(id) != CareerMedal::None) ++count;
        return count;
    }

    static int MedalPoints(CareerMedal medal) noexcept
    {
        switch (medal)
        {
        case CareerMedal::Bronze: return 3;
        case CareerMedal::Silver: return 6;
        case CareerMedal::Gold: return 10;
        default: return 0;
        }
    }

    int ChampionshipPoints(int series) const noexcept
    {
        const auto* championship = GetChampionship(series);
        if (!championship) return 0;
        int result = 0;
        for (int id : championship->Rounds) result += MedalPoints(Medal(id));
        return result;
    }

    bool ChampionshipComplete(int series) const noexcept
    {
        const auto* championship = GetChampionship(series);
        if (!championship) return false;
        for (int id : championship->Rounds)
            if (Medal(id) == CareerMedal::None) return false;
        return true;
    }

    int NextAvailable() const noexcept
    {
        for (int id = 0; id < CareerEventCount; ++id)
            if (Status(id) == CareerStatus::Available) return id;
        return -1;
    }

    CareerAward CommitFinish(int id, const CareerFinish& result) noexcept
    {
        CareerAward award;
        const CareerEvent* event = GetCareerEvent(id);
        if (!event || id != Data.SelectedEvent ||
            Status(id) == CareerStatus::Locked ||
            !IsValidCareerFinish(*event, result))
        {
            return award;
        }
        award.ValidFinish = true;
        award.Medal = EvaluateCareerMedal(*event, result);
        const CareerMedal prior = Medal(id);
        if (static_cast<int>(award.Medal) <= static_cast<int>(prior))
        {
            return award;
        }
        award.MedalImproved = true;
        const bool wasSeriesComplete =
            event->Series >= 0 && ChampionshipComplete(event->Series);
        Data.Medals[static_cast<std::size_t>(id)] =
            static_cast<std::uint8_t>(award.Medal);
        if (prior == CareerMedal::None)
        {
            award.FirstCompletion = true;
            award.Credits = event->FirstCredits;
            if (event->Series >= 0 && !wasSeriesComplete &&
                ChampionshipComplete(event->Series))
            {
                const auto* series = GetChampionship(event->Series);
                award.ChampionshipBonus = series->FirstCompletionBonus;
                award.Credits += award.ChampionshipBonus;
            }
            for (int next = 0; next < CareerEventCount; ++next)
            {
                if (CareerEvents[static_cast<std::size_t>(next)].Prerequisite == id &&
                    Status(next) == CareerStatus::Available)
                {
                    award.NewlyAvailable = next;
                    break;
                }
            }
        }
        return award;
    }

private:
    CareerSnapshot Data{};
};
} // namespace latebrake
