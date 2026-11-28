#include "Garage/GarageLookEditHistory.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string>

namespace
{
using namespace latebrake;
void Require(bool condition,const std::string& message)
{
    if(!condition)
    {
        std::cerr << "GarageLookEditHistory: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
GarageLook Appearance(int paint,int stripe=0,int wheels=0)
{
    return {static_cast<std::uint8_t>(paint),
        static_cast<std::uint8_t>(stripe),
        static_cast<std::uint8_t>(wheels)};
}
void EmptyAndInvalidCases()
{
    GarageLookEditHistory history;
    const auto initial=history.Summary(0);
    Require(initial.Valid && initial.UndoCount==0 && initial.RedoCount==0,
        "empty history per original car");
    Require(!history.Summary(-1).Valid && !history.Summary(VehicleCount).Valid,
        "unmapped ids cannot read stale history");
    Require(!history.RecordCommittedChange(-1,Appearance(0),Appearance(1)),
        "invalid vehicle cannot record edit");
    Require(!history.RecordCommittedChange(0,Appearance(8),Appearance(1)),
        "invalid paint never enters saved history");
    Require(!history.RecordCommittedChange(0,Appearance(0),Appearance(0)),
        "same-look attempts are not committed edits");
    const auto out=history.Preview(GarageLookHistoryDirection::Undo,-1,Appearance(0));
    Require(!out.Valid && out.Status==GarageLookHistoryStatus::InvalidVehicle,
        "invalid undo ID refused");
    const auto missing=history.Preview(GarageLookHistoryDirection::Redo,0,Appearance(0));
    Require(!missing.Valid && missing.Status==GarageLookHistoryStatus::Empty,
        "uncommitted undo/redo is unavailable");
    const auto invalid=history.Preview(GarageLookHistoryDirection::Undo,0,Appearance(0,3));
    Require(!invalid.Valid && invalid.Status==GarageLookHistoryStatus::InvalidLook,
        "malformed look cannot be accepted as current");
}
void UndoRedoTransactions()
{
    GarageLookEditHistory history;
    const GarageLook factory=Appearance(0);
    const GarageLook changed=Appearance(5,1,2);
    Require(history.RecordCommittedChange(0,factory,changed),"committed look A");
    const auto initial=history.Summary(0);
    Require(initial.UndoCount==1 && initial.RedoCount==0,
        "new commit enters undo stack only");
    const auto proposal=history.Preview(GarageLookHistoryDirection::Undo,0,changed);
    Require(proposal.Valid && proposal.ModifiesSave &&
        !proposal.GrantsOwnership && !proposal.SpendsCredits &&
        SameGarageLook(proposal.ExpectedCurrent,changed) &&
        SameGarageLook(proposal.ProposedLook,factory),
        "preview is a save-required intent without permission or economy");
    Require(history.Summary(0).UndoCount==1,
        "preview alone must never pop undo history");
    Require(!history.Commit(proposal,changed),
        "unsaved proposal cannot consume stack");
    Require(history.Commit(proposal,factory),"successful saved undo");
    Require(history.Summary(0).UndoCount==0 &&
        history.Summary(0).RedoCount==1,
        "committed undo transfers one edit to redo stack");
    Require(!history.Commit(proposal,factory),
        "reusing an old proposal is forbidden");
    const auto redo=history.Preview(GarageLookHistoryDirection::Redo,0,factory);
    Require(redo.Valid && SameGarageLook(redo.ProposedLook,changed),
        "redo proposes the exact original saved appearance");
    Require(history.Commit(redo,changed),"saved redo succeeds");
    Require(history.Summary(0).UndoCount==1 &&
        history.Summary(0).RedoCount==0,
        "redo returns one edit to undo chain");
    const auto stale=history.Preview(GarageLookHistoryDirection::Undo,0,factory);
    Require(!stale.Valid && stale.Status==GarageLookHistoryStatus::StaleCurrent &&
        history.Summary(0).StaleRefusals==1,
        "user cannot undo a look that is no longer shown");
}
void BranchingAndIndependentCars()
{
    GarageLookEditHistory history;
    const auto a=Appearance(0),b=Appearance(1),c=Appearance(2),d=Appearance(3);
    Require(history.RecordCommittedChange(0,a,b),"A-B committed");
    Require(history.RecordCommittedChange(0,b,c),"B-C committed");
    Require(history.RecordCommittedChange(1,a,d),"another vehicle recorded");
    auto undo=history.Preview(GarageLookHistoryDirection::Undo,0,c);
    Require(history.Commit(undo,b),"revert latest car zero edit");
    Require(history.Summary(0).UndoCount==1 && history.Summary(0).RedoCount==1,
        "car zero keeps separate undo/redo stacks");
    Require(history.Summary(1).UndoCount==1 && history.Summary(1).RedoCount==0,
        "car one unaffected");
    Require(history.RecordCommittedChange(0,b,d),"new branch after successful undo");
    Require(history.Summary(0).RedoCount==0 &&
        history.Summary(0).DroppedRedo==1,
        "branching invalidates old redo path");
    Require(!history.Preview(GarageLookHistoryDirection::Redo,0,d).Valid,
        "cannot replay an obsolete alternate future");
    undo=history.Preview(GarageLookHistoryDirection::Undo,0,d);
    Require(history.Commit(undo,b),"latest branch can be undone");
    undo=history.Preview(GarageLookHistoryDirection::Undo,0,b);
    Require(history.Commit(undo,a),"original edit still accessible");
    Require(history.Summary(0).UndoCount==0 &&
        history.Summary(0).RedoCount==2,
        "two successful rollbacks are retained");
    history.ResetVehicle(0);
    Require(history.Summary(0).UndoCount==0 &&
        history.Summary(0).RedoCount==0 &&
        history.Summary(1).UndoCount==1,
        "per-vehicle reset never alters another car's history");
}
void StaleChainAndSaveRefusals()
{
    GarageLookEditHistory history;
    const auto a=Appearance(0),b=Appearance(1),c=Appearance(2),d=Appearance(3);
    Require(history.RecordCommittedChange(2,a,b),"baseline fixture");
    const auto first=history.Preview(GarageLookHistoryDirection::Undo,2,b);
    // A newer successful save makes the earlier request stale.
    Require(history.RecordCommittedChange(2,b,c),"newer change");
    Require(!history.Commit(first,a),"old nonce cannot consume updated history");
    Require(history.Summary(2).UndoCount==2,"stale commit left all edits intact");
    const auto current=history.Preview(GarageLookHistoryDirection::Undo,2,c);
    Require(current.Valid,"fresh consent is eligible");
    Require(!history.Commit(current,c),"failed SaveRecords must not commit stack");
    Require(history.Summary(2).UndoCount==2,"failed saved state did not pop");
    // External profile replacement: the old edit chain is unsafe. A newly
    // committed style begins a fresh chain without fabricating a rollback.
    Require(history.RecordCommittedChange(2,d,a),"external source jump");
    Require(history.Summary(2).UndoCount==1 &&
        history.Summary(2).RedoCount==0,
        "stale cross-profile history invalidated by new successful edit");
}
void BoundedEvictionAndRedoEviction()
{
    GarageLookEditHistory history;
    GarageLook current=Appearance(0);
    for(std::size_t i=0;i<GarageLookHistoryDepth+13u;++i)
    {
        const GarageLook next=Appearance(static_cast<int>((i+1)%8));
        if(SameGarageLook(current,next))continue;
        Require(history.RecordCommittedChange(4,current,next),
            "bounded long edit chain");
        current=next;
    }
    const auto summary=history.Summary(4);
    Require(summary.UndoCount==GarageLookHistoryDepth && summary.DroppedUndo>0,
        "oldest committed edits evicted and bounded");
    for(std::size_t i=0;i<GarageLookHistoryDepth;++i)
    {
        const auto proposal=history.Preview(GarageLookHistoryDirection::Undo,4,current);
        Require(proposal.Valid,"retained edit can be undone");
        current=proposal.ProposedLook;
        Require(history.Commit(proposal,current),"saved undo accepted");
    }
    Require(history.Summary(4).UndoCount==0 &&
        history.Summary(4).RedoCount==GarageLookHistoryDepth,
        "redo stack bounded to same history depth");
    Require(!history.Preview(GarageLookHistoryDirection::Undo,4,current).Valid,
        "evicted edit cannot be restored or invented");
}
void RandomizedIndependentStateOracle()
{
    GarageLookEditHistory history;
    std::mt19937 random{0xA17D00};
    std::array<GarageLook,VehicleCount> current{};
    std::array<unsigned,VehicleCount> successfulSaves{};
    for(int id=0;id<VehicleCount;++id)current[static_cast<std::size_t>(id)]=
        FactoryGarageLook(id);
    for(int trial=0;trial<25000;++trial)
    {
        const int id=static_cast<int>(random()%VehicleCount);
        auto& observed=current[static_cast<std::size_t>(id)];
        const auto operation=random()%5;
        if(operation<=2)
        {
            const auto next=Appearance(static_cast<int>(random()%8),
                static_cast<int>(random()%3),static_cast<int>(random()%3));
            if(SameGarageLook(observed,next))continue;
            const bool saveFailed=random()%7==0;
            if(!saveFailed)
            {
                Require(history.RecordCommittedChange(id,observed,next),
                    "save-confirmed appearance updated ledger");
                observed=next;
                ++successfulSaves[static_cast<std::size_t>(id)];
            }
        }
        else
        {
            const auto direction=operation==3 ?
                GarageLookHistoryDirection::Undo:GarageLookHistoryDirection::Redo;
            const auto proposal=history.Preview(direction,id,observed);
            if(!proposal.Valid) continue;
            const bool saveFailed=random()%7==0;
            if(!saveFailed)
            {
                Require(history.Commit(proposal,proposal.ProposedLook),
                    "successful random save applies exact proposal");
                observed=proposal.ProposedLook;
                ++successfulSaves[static_cast<std::size_t>(id)];
            }
        }
        const auto summary=history.Summary(id);
        Require(summary.Valid && summary.UndoCount<=GarageLookHistoryDepth &&
            summary.RedoCount<=GarageLookHistoryDepth,
            "bounded independent stacks during random mutation sequence");
        if(summary.UndoCount>0)
            Require(history.Preview(GarageLookHistoryDirection::Undo,id,observed).Valid,
                "undo top always matches current saved look");
        else if(summary.RedoCount>0)
            Require(history.Preview(GarageLookHistoryDirection::Redo,id,observed).Valid,
                "redo top always matches current saved look");
    }
    unsigned total=0;
    for(auto value:successfulSaves)total+=value;
    Require(total>10000,"substantial randomized successful transaction coverage");
}
}
int main()
{
    EmptyAndInvalidCases();
    UndoRedoTransactions();
    BranchingAndIndependentCars();
    StaleChainAndSaveRefusals();
    BoundedEvictionAndRedoEviction();
    RandomizedIndependentStateOracle();
    std::cout << "Garage in-memory appearance edit history tests passed.\n";
    return EXIT_SUCCESS;
}
