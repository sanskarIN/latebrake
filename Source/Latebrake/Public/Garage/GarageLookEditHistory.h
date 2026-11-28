#pragma once

#include "Garage/GarageAppearance.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// An in-memory, bounded stack of *successfully committed* appearance edits.
// The caller owns all authorization and SaveGame transactions. Preview alone
// cannot mutate this ledger, and no undo may spend or refund currency.
inline constexpr std::size_t GarageLookHistoryDepth=12;

enum class GarageLookHistoryStatus : std::uint8_t
{
    Ready,
    InvalidVehicle,
    InvalidLook,
    Unchanged,
    Empty,
    StaleCurrent,
    OutOfOrder,
    ReplayDisallowed
};

enum class GarageLookHistoryDirection : std::uint8_t
{
    Undo,
    Redo
};

struct GarageLookHistoryEdit final
{
    bool Valid=false;
    int VehicleId=-1;
    GarageLook Before{};
    GarageLook After{};
    std::uint64_t Revision=0;
};

struct GarageLookHistoryProposal final
{
    bool Valid=false;
    GarageLookHistoryStatus Status=GarageLookHistoryStatus::Empty;
    GarageLookHistoryDirection Direction=GarageLookHistoryDirection::Undo;
    int VehicleId=-1;
    GarageLook ExpectedCurrent{};
    GarageLook ProposedLook{};
    std::uint64_t ExpectedRevision=0;
    bool ModifiesSave=false;
    bool SpendsCredits=false;
    bool GrantsOwnership=false;
};

struct GarageLookHistorySummary final
{
    std::size_t UndoCount=0;
    std::size_t RedoCount=0;
    std::uint64_t Revision=0;
    std::uint64_t DroppedUndo=0;
    std::uint64_t DroppedRedo=0;
    std::uint64_t StaleRefusals=0;
    bool Valid=false;
};

// Fixed-capacity LIFO with oldest-entry eviction. The newest edit is always
// at Size-1; each pop returns a copy because callers may commit only after
// the source save has succeeded. All pushes/pops are nonthrowing.
template<std::size_t Capacity>
class GarageHistoryStack final
{
public:
    static_assert(Capacity>0);
    void Reset() noexcept {Size=0;Items={};}
    std::size_t Count() const noexcept {return Size;}
    bool Peek(GarageLookHistoryEdit& out) const noexcept
    {
        if(Size==0)return false;
        out=Items[Size-1];
        return true;
    }
    bool Pop(GarageLookHistoryEdit& out) noexcept
    {
        if(!Peek(out))return false;
        Items[--Size]={};
        return true;
    }
    bool Push(const GarageLookHistoryEdit& edit) noexcept
    {
        bool evicted=false;
        if(Size==Capacity)
        {
            for(std::size_t i=1;i<Size;++i)
                Items[i-1]=Items[i];
            --Size;
            evicted=true;
        }
        Items[Size++]=edit;
        return evicted;
    }
private:
    std::array<GarageLookHistoryEdit,Capacity> Items{};
    std::size_t Size=0;
};

class GarageLookEditHistory final
{
public:
    void Reset() noexcept
    {
        for(auto& stack:Undo) stack.Reset();
        for(auto& stack:Redo) stack.Reset();
        Revisions.fill(0);
        DroppedUndo.fill(0);
        DroppedRedo.fill(0);
        StaleRefusals.fill(0);
    }

    void ResetVehicle(int vehicleId) noexcept
    {
        if(!GarageState::IsVehicle(vehicleId))return;
        const auto slot=static_cast<std::size_t>(vehicleId);
        Undo[slot].Reset();
        Redo[slot].Reset();
        Increase(Revisions[slot]);
    }

    bool RecordCommittedChange(int vehicleId,GarageLook before,
        GarageLook after) noexcept
    {
        if(!GarageState::IsVehicle(vehicleId) ||
            !ValidGarageLook(before) || !ValidGarageLook(after) ||
            SameGarageLook(before,after))
            return false;
        const auto slot=static_cast<std::size_t>(vehicleId);
        GarageLookHistoryEdit top;
        // External change or a new profile invalidates the older chain.
        // Never assume the car still shows what a stale stack recorded.
        if(Undo[slot].Peek(top) && !SameGarageLook(top.After,before))
            ResetVehicle(vehicleId);
        if(Redo[slot].Peek(top) && !SameGarageLook(top.Before,before))
            Redo[slot].Reset();
        if(Redo[slot].Count()>0)
        {
            DroppedRedo[slot]+=static_cast<std::uint64_t>(Redo[slot].Count());
            Redo[slot].Reset();
        }
        Increase(Revisions[slot]);
        const GarageLookHistoryEdit next{true,vehicleId,before,after,
            Revisions[slot]};
        if(Undo[slot].Push(next))Increase(DroppedUndo[slot]);
        return true;
    }

    GarageLookHistoryProposal Preview(GarageLookHistoryDirection direction,
        int vehicleId,GarageLook current) noexcept
    {
        GarageLookHistoryProposal proposal;
        proposal.Direction=direction;
        proposal.VehicleId=vehicleId;
        if(!GarageState::IsVehicle(vehicleId))
        {
            proposal.Status=GarageLookHistoryStatus::InvalidVehicle;
            return proposal;
        }
        if(!ValidGarageLook(current))
        {
            proposal.Status=GarageLookHistoryStatus::InvalidLook;
            return proposal;
        }
        const auto slot=static_cast<std::size_t>(vehicleId);
        GarageLookHistoryEdit edit;
        const auto& stack=direction==GarageLookHistoryDirection::Undo ?
            Undo[slot]:Redo[slot];
        if(!stack.Peek(edit))
        {
            proposal.Status=GarageLookHistoryStatus::Empty;
            return proposal;
        }
        const GarageLook expected=direction==GarageLookHistoryDirection::Undo ?
            edit.After:edit.Before;
        const GarageLook target=direction==GarageLookHistoryDirection::Undo ?
            edit.Before:edit.After;
        if(!SameGarageLook(expected,current))
        {
            proposal.Status=GarageLookHistoryStatus::StaleCurrent;
            Increase(StaleRefusals[slot]);
            return proposal;
        }
        if(!edit.Valid || edit.VehicleId!=vehicleId ||
            !ValidGarageLook(target) || SameGarageLook(target,current))
        {
            proposal.Status=GarageLookHistoryStatus::OutOfOrder;
            return proposal;
        }
        proposal.Valid=true;
        proposal.Status=GarageLookHistoryStatus::Ready;
        proposal.ExpectedCurrent=current;
        proposal.ProposedLook=target;
        proposal.ExpectedRevision=Revisions[slot];
        // This value only means the caller must SAVE the changes before
        // calling Commit. It does not authorize the save or grant ownership.
        proposal.ModifiesSave=true;
        return proposal;
    }

    // May only be called after a successful transaction for the exact
    // proposal. Any intermediate edit, wrong vehicle or mismatch refuses.
    bool Commit(const GarageLookHistoryProposal& proposal,
        GarageLook persisted) noexcept
    {
        if(!proposal.Valid ||
            proposal.Status!=GarageLookHistoryStatus::Ready ||
            !GarageState::IsVehicle(proposal.VehicleId) ||
            !ValidGarageLook(persisted) ||
            !SameGarageLook(persisted,proposal.ProposedLook) ||
            !proposal.ModifiesSave || proposal.SpendsCredits ||
            proposal.GrantsOwnership)
            return false;
        const auto slot=static_cast<std::size_t>(proposal.VehicleId);
        if(Revisions[slot]!=proposal.ExpectedRevision)return false;
        auto& source=proposal.Direction==GarageLookHistoryDirection::Undo ?
            Undo[slot]:Redo[slot];
        auto& destination=proposal.Direction==GarageLookHistoryDirection::Undo ?
            Redo[slot]:Undo[slot];
        GarageLookHistoryEdit actual;
        if(!source.Peek(actual) || !actual.Valid ||
            actual.VehicleId!=proposal.VehicleId ||
            !SameGarageLook(actual.Before,
                proposal.Direction==GarageLookHistoryDirection::Undo ?
                    proposal.ProposedLook:proposal.ExpectedCurrent) ||
            !SameGarageLook(actual.After,
                proposal.Direction==GarageLookHistoryDirection::Undo ?
                    proposal.ExpectedCurrent:proposal.ProposedLook))
            return false;
        GarageLookHistoryEdit popped;
        if(!source.Pop(popped))return false;
        Increase(Revisions[slot]);
        popped.Revision=Revisions[slot];
        if(destination.Push(popped))
        {
            if(proposal.Direction==GarageLookHistoryDirection::Undo)
                Increase(DroppedRedo[slot]);
            else
                Increase(DroppedUndo[slot]);
        }
        return true;
    }

    GarageLookHistorySummary Summary(int vehicleId) const noexcept
    {
        GarageLookHistorySummary out;
        if(!GarageState::IsVehicle(vehicleId))return out;
        const auto slot=static_cast<std::size_t>(vehicleId);
        out.Valid=true;
        out.UndoCount=Undo[slot].Count();
        out.RedoCount=Redo[slot].Count();
        out.Revision=Revisions[slot];
        out.DroppedUndo=DroppedUndo[slot];
        out.DroppedRedo=DroppedRedo[slot];
        out.StaleRefusals=StaleRefusals[slot];
        return out;
    }

private:
    static void Increase(std::uint64_t& value) noexcept
    {
        if(value<std::numeric_limits<std::uint64_t>::max())++value;
    }
    std::array<GarageHistoryStack<GarageLookHistoryDepth>,VehicleCount> Undo{};
    std::array<GarageHistoryStack<GarageLookHistoryDepth>,VehicleCount> Redo{};
    std::array<std::uint64_t,VehicleCount> Revisions{};
    std::array<std::uint64_t,VehicleCount> DroppedUndo{};
    std::array<std::uint64_t,VehicleCount> DroppedRedo{};
    std::array<std::uint64_t,VehicleCount> StaleRefusals{};
};
} // namespace latebrake
