#pragma once

// Immutable, Unreal-independent event definitions. All event-specific
// race rules are configured from this catalog, never from UI strings.
namespace latebrake
{
enum class EventKind
{
    Circuit = 0,
    TimeAttack = 1
};

struct RaceEvent final
{
    EventKind Kind;
    int CheckpointCount;
    int RequiredLaps;
    bool SpawnRival;
    const char* DisplayName;
};

constexpr bool IsValidEvent(EventKind kind) noexcept
{
    return kind == EventKind::Circuit || kind == EventKind::TimeAttack;
}

constexpr RaceEvent GetRaceEvent(EventKind kind) noexcept
{
    switch (kind)
    {
    case EventKind::TimeAttack: return {EventKind::TimeAttack, 4, 1, false, "Time Attack"};
    case EventKind::Circuit:
    default: return {EventKind::Circuit, 4, 3, true, "Circuit Race"};
    }
}
} // namespace latebrake
