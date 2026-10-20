#pragma once

#include <array>
#include <cstdint>

namespace latebrake
{
// Persisted IDs are immutable. Append new IDs; never reorder or reuse them.
// All events use the existing four-gate starter circuit, not imaginary maps.
enum class CareerObjective : std::uint8_t
{
    Time, Clean, Precision, Consistency, Vehicle, Endurance
};

enum class CareerMedal : std::uint8_t
{
    None = 0, Bronze = 1, Silver = 2, Gold = 3
};

enum class CareerStatus : std::uint8_t
{
    Locked, Available, Completed, Mastered
};

struct CareerEvent final
{
    int Id;
    const char* Name;
    int Series;                 // -1 for standalone challenges.
    int Round;                  // 0-based within its championship.
    int Prerequisite;           // -1 for immediately available.
    int Laps;
    CareerObjective Objective;
    double BronzeTarget;
    double SilverTarget;
    double GoldTarget;
    int RequiredVehicle;        // -1 = any currently owned car.
    int FirstCredits;           // Awarded once, only on first valid medal.
    bool SpawnRival;
};

inline constexpr std::array<CareerEvent, 12> CareerEvents{{
    {0, "Rookie Qualifier", 0, 0, -1, 1, CareerObjective::Time, 170.0, 140.0, 115.0, -1, 100, true},
    {1, "Rookie Sprint",    0, 1,  0, 2, CareerObjective::Time, 320.0, 270.0, 220.0, -1, 150, true},
    {2, "Rookie Finale",    0, 2,  1, 3, CareerObjective::Time, 480.0, 410.0, 335.0, -1, 225, true},
    {3, "Prologue Clean",   1, 0,  2, 2, CareerObjective::Clean, 2.0, 1.0, 0.0, -1, 200, false},
    {4, "Prologue Pace",    1, 1,  3, 2, CareerObjective::Time, 300.0, 250.0, 205.0, -1, 250, true},
    {5, "Prologue Finale",  1, 2,  4, 4, CareerObjective::Endurance, 700.0, 600.0, 510.0, -1, 350, true},
    {6, "Clock Hunter",   -1, 0, -1, 1, CareerObjective::Time, 160.0, 130.0, 100.0, -1, 100, false},
    {7, "Clean Circuit",  -1, 0,  0, 2, CareerObjective::Clean, 2.0, 1.0, 0.0, -1, 125, false},
    {8, "Gate Guardian",  -1, 0,  1, 2, CareerObjective::Precision, 300.0, 255.0, 210.0, -1, 150, false},
    {9, "Lap Consistency",-1, 0,  1, 3, CareerObjective::Consistency, 24.0, 15.0, 8.0, -1, 150, false},
    {10,"Kestrel Mastery",-1, 0,  2, 2, CareerObjective::Vehicle, 320.0, 275.0, 230.0, 1, 200, false},
    {11,"Long Haul",      -1, 0,  4, 6, CareerObjective::Endurance, 1100.0, 950.0, 800.0, -1, 300, false}
}};

struct ChampionshipDefinition final
{
    int Id;
    const char* Name;
    std::array<int, 3> Rounds;
    int FirstCompletionBonus;
};

inline constexpr std::array<ChampionshipDefinition, 2> Championships{{
    {0, "Rookie Cup", {0, 1, 2}, 350},
    {1, "Prologue Cup", {3, 4, 5}, 550}
}};

constexpr int CareerEventCount = static_cast<int>(CareerEvents.size());
constexpr bool IsCareerId(int id) noexcept
{
    return id >= 0 && id < CareerEventCount;
}

constexpr const CareerEvent* GetCareerEvent(int id) noexcept
{
    return IsCareerId(id) ? &CareerEvents[static_cast<std::size_t>(id)] : nullptr;
}

constexpr const ChampionshipDefinition* GetChampionship(int id) noexcept
{
    return id >= 0 && id < static_cast<int>(Championships.size())
        ? &Championships[static_cast<std::size_t>(id)] : nullptr;
}
} // namespace latebrake
