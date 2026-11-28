#pragma once

#include "UI/HudVerticalPlan.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Presentation policy only; actual pause/finish/save status has its own
// reserved baselines. No physics or gameplay authority enters the queue.
enum class HudTelemetryMode : std::uint8_t { Focused, Balanced, Detailed };

constexpr bool ValidHudTelemetryMode(HudTelemetryMode m) noexcept
{
    return m==HudTelemetryMode::Focused || m==HudTelemetryMode::Balanced ||
        m==HudTelemetryMode::Detailed;
}

constexpr HudTelemetryMode NextHudTelemetryMode(HudTelemetryMode m) noexcept
{
    switch(m)
    {
        case HudTelemetryMode::Focused:return HudTelemetryMode::Balanced;
        case HudTelemetryMode::Balanced:return HudTelemetryMode::Detailed;
        case HudTelemetryMode::Detailed:return HudTelemetryMode::Focused;
        default:return HudTelemetryMode::Balanced;
    }
}

constexpr const char* HudTelemetryModeName(HudTelemetryMode m) noexcept
{
    switch(m)
    {
        case HudTelemetryMode::Focused:return "FOCUSED";
        case HudTelemetryMode::Detailed:return "DETAILED";
        default:return "BALANCED";
    }
}

constexpr bool CanCycleHudTelemetryMode(bool menuOpen,bool settingsPage,
    bool localDriver,bool focusSuspended) noexcept
{
    return menuOpen && settingsPage && localDriver && !focusSuspended;
}

// Enumerated in priority order, independent of source text or colors.
// Urgent late rows cannot be displaced by earlier optional diagnostics.
enum class HudTelemetryKind : std::uint8_t
{
    Blocking,
    Clock,
    Progress,
    Speed,
    Start,
    Urgent,
    Navigation,
    Pace,
    Vehicle,
    Context,
    Analysis,
    Decorative,
    Count
};

inline constexpr int HudTelemetryCandidateCapacity=160;
inline constexpr int HudTelemetryMaxRows=80;

constexpr bool ValidHudTelemetryKind(HudTelemetryKind kind) noexcept
{
    return static_cast<unsigned>(kind)<static_cast<unsigned>(HudTelemetryKind::Count);
}

constexpr bool HudTelemetryEssential(HudTelemetryKind kind) noexcept
{
    return ValidHudTelemetryKind(kind) &&
        static_cast<unsigned>(kind)<=static_cast<unsigned>(HudTelemetryKind::Navigation);
}

constexpr int HudTelemetryPriority(HudTelemetryKind kind) noexcept
{
    return ValidHudTelemetryKind(kind)?static_cast<int>(kind):100;
}

constexpr bool HudTelemetryAdmitted(HudTelemetryMode mode,HudTelemetryKind kind) noexcept
{
    if(!ValidHudTelemetryKind(kind))return false;
    if(!ValidHudTelemetryMode(mode))mode=HudTelemetryMode::Balanced;
    if(HudTelemetryEssential(kind))return true;
    switch(mode)
    {
        case HudTelemetryMode::Focused:return false;
        case HudTelemetryMode::Balanced:return kind==HudTelemetryKind::Pace ||
            kind==HudTelemetryKind::Vehicle || kind==HudTelemetryKind::Context;
        case HudTelemetryMode::Detailed:return true;
        default:return false;
    }
}

constexpr int HudTelemetryOptionalBudget(HudTelemetryMode mode) noexcept
{
    switch(mode)
    {
        case HudTelemetryMode::Focused:return 0;
        case HudTelemetryMode::Detailed:return HudTelemetryMaxRows;
        default:return 10;
    }
}

struct HudTelemetryCandidate final
{
    int SourceIndex=-1;
    HudTelemetryKind Kind=HudTelemetryKind::Count;
};

struct HudTelemetrySelection final
{
    bool Valid=false;
    int CandidateCount=0;
    int EssentialCount=0;
    int EligibleOptionalCount=0;
    int SuppressedByMode=0;
    int Refused=0;
    int MaxRows=0;
    int OptionalBudget=0;
    std::array<HudTelemetryCandidate,HudTelemetryCandidateCapacity> Candidates{};
};

class HudTelemetryQueue final
{
public:
    bool Add(int sourceIndex,HudTelemetryKind kind) noexcept
    {
        if(sourceIndex<0 || sourceIndex>=HudTelemetryCandidateCapacity ||
            !ValidHudTelemetryKind(kind) || Count>=HudTelemetryCandidateCapacity ||
            Seen[static_cast<std::size_t>(sourceIndex)])
        {
            ++Refused;
            return false;
        }
        Seen[static_cast<std::size_t>(sourceIndex)]=true;
        Entries[static_cast<std::size_t>(Count++)]={sourceIndex,kind};
        return true;
    }

    HudTelemetrySelection Select(HudTelemetryMode mode,int maxRows) const noexcept
    {
        HudTelemetrySelection out;
        if(maxRows<=0 || maxRows>HudTelemetryMaxRows)return out;
        if(!ValidHudTelemetryMode(mode))mode=HudTelemetryMode::Balanced;
        out.Valid=true;
        out.Refused=Refused;
        out.MaxRows=maxRows;
        out.OptionalBudget=HudTelemetryOptionalBudget(mode);
        // Stable bucket traversal. A fixed twelve-category scan is bounded
        // independently of text length and leaves same-kind source order.
        for(unsigned priority=0;priority<static_cast<unsigned>(HudTelemetryKind::Count);++priority)
        {
            const auto kind=static_cast<HudTelemetryKind>(priority);
            const bool admitted=HudTelemetryAdmitted(mode,kind);
            for(int i=0;i<Count;++i)
            {
                const auto c=Entries[static_cast<std::size_t>(i)];
                if(c.Kind!=kind)continue;
                if(!admitted)
                {
                    ++out.SuppressedByMode;
                    continue;
                }
                if(HudTelemetryEssential(kind))++out.EssentialCount;
                else ++out.EligibleOptionalCount;
                out.Candidates[static_cast<std::size_t>(out.CandidateCount++)]=c;
            }
        }
        return out;
    }

private:
    int Count=0;
    int Refused=0;
    std::array<bool,HudTelemetryCandidateCapacity> Seen{};
    std::array<HudTelemetryCandidate,HudTelemetryCandidateCapacity> Entries{};
};

struct HudTelemetryDrawReceipt final
{
    int Drawn=0;
    int EssentialDrawn=0;
    int OptionalDrawn=0;
    int Unfittable=0;
    int CapacityHidden=0;
    int OptionalHidden=0;
};

// The callback draws a real measured font row or returns false. Unfit
// text consumes no row and does not prevent the next candidate fitting.
template<typename Draw>
HudTelemetryDrawReceipt DrawSelectedHudTelemetry(const HudTelemetrySelection& selection,
    Draw draw)
{
    HudTelemetryDrawReceipt receipt;
    if(!selection.Valid || selection.MaxRows<1 || selection.MaxRows>HudTelemetryMaxRows ||
        selection.CandidateCount<0 ||
        selection.CandidateCount>HudTelemetryCandidateCapacity ||
        selection.OptionalBudget<0 || selection.OptionalBudget>HudTelemetryMaxRows)
        return receipt;
    std::array<bool,HudTelemetryCandidateCapacity> seen{};
    int previous=-1;
    for(int i=0;i<selection.CandidateCount;++i)
    {
        const auto c=selection.Candidates[static_cast<std::size_t>(i)];
        if(c.SourceIndex<0 || c.SourceIndex>=HudTelemetryCandidateCapacity ||
            !ValidHudTelemetryKind(c.Kind) ||
            seen[static_cast<std::size_t>(c.SourceIndex)] ||
            HudTelemetryPriority(c.Kind)<previous)
            return {};
        seen[static_cast<std::size_t>(c.SourceIndex)]=true;
        previous=HudTelemetryPriority(c.Kind);
    }
    for(int i=0;i<selection.CandidateCount;++i)
    {
        const auto c=selection.Candidates[static_cast<std::size_t>(i)];
        const bool essential=HudTelemetryEssential(c.Kind);
        if(receipt.Drawn>=selection.MaxRows)
        {
            ++receipt.CapacityHidden;
            continue;
        }
        if(!essential && receipt.OptionalDrawn>=selection.OptionalBudget)
        {
            ++receipt.OptionalHidden;
            continue;
        }
        if(!draw(c.SourceIndex))
        {
            ++receipt.Unfittable;
            continue;
        }
        ++receipt.Drawn;
        if(essential)++receipt.EssentialDrawn;
        else ++receipt.OptionalDrawn;
    }
    return receipt;
}
} // namespace latebrake
