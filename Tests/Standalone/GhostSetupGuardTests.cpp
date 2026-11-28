#include "Racing/GhostSetupGuard.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"GhostSetupGuard: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
GhostSetupStamp Key(int car=0,int engine=0,int grip=0,int handling=0,std::uint64_t actor=1)
{
    return {actor,{car,engine,grip},handling};
}
void RecordLap(BestLapGhost& ghost,double duration)
{
    for(int i=1;i<=10;++i)
        Require(ghost.Record({duration*i/10,static_cast<double>(i)*10,0,0,0},true),"observed pose");
    Require(ghost.FinishLap(duration,true,true),"real eligible best lap");
}
void IsolateConfigurationChanges()
{
    auto ghost=std::make_unique<BestLapGhost>();
    GhostSetupGuard guard;
    auto key=Key();
    Require(RefreshGhostSetup(guard,*ghost,key,true)==GhostSetupTransition::InitialBinding &&
        guard.Matches(key),"initial physical configuration");
    RecordLap(*ghost,2);
    Require(ghost->HasBest() && ghost->GetBestSeconds()==2,"qualified ghost");
    Require(RefreshGhostSetup(guard,*ghost,key,false)==GhostSetupTransition::Unchanged &&
        ghost->HasBest(),"unchanged setup preserves reference");
    Require(ghost->Record({0.2,0,0,0,0},true),"next candidate begins");
    auto changed=key;changed.Tuning.Engine=1;
    Require(RefreshGhostSetup(guard,*ghost,changed,false)==GhostSetupTransition::Rebound &&
        !ghost->HasBest() && !ghost->IsCandidateAllowed(),"tuning invalidates prior ghost and current lap");
    Require(RefreshGhostSetup(guard,*ghost,key,false)==GhostSetupTransition::Rebound &&
        !ghost->HasBest() && !ghost->IsCandidateAllowed(),"switch-back never restores old reference");
    Require(!ghost->FinishLap(1.8,true,true),"mixed lap rejected at boundary");
    Require(ghost->IsCandidateAllowed(),"real ordered boundary reopens candidate");
    RecordLap(*ghost,1.5);
    Require(ghost->HasBest() && ghost->GetBestSeconds()==1.5,"fresh same-config lap");
    changed=key;changed.Handling=1;
    Require(RefreshGhostSetup(guard,*ghost,changed,true)==GhostSetupTransition::Rebound &&
        !ghost->HasBest() && ghost->IsCandidateAllowed(),"boundary handling change opens fresh trace");
    RecordLap(*ghost,2);
    changed.Actor=2;
    Require(RefreshGhostSetup(guard,*ghost,changed,false)==GhostSetupTransition::Rebound &&
        !ghost->HasBest() && !ghost->IsCandidateAllowed(),"different actor cannot inherit ghost");
}
void MissingAuthorityAndReset()
{
    auto ghost=std::make_unique<BestLapGhost>();
    GhostSetupGuard guard;
    Require(RefreshGhostSetup(guard,*ghost,{},true)==GhostSetupTransition::Unavailable &&
        !guard.IsBound() && !ghost->IsCandidateAllowed(),"missing initial authority");
    const auto key=Key();
    Require(RefreshGhostSetup(guard,*ghost,key,true)==GhostSetupTransition::InitialBinding &&
        ghost->IsCandidateAllowed(),"pre-clock setup may seed lap");
    RecordLap(*ghost,2);
    Require(RefreshGhostSetup(guard,*ghost,{},false)==GhostSetupTransition::Lost &&
        !guard.IsBound() && !ghost->HasBest(),"missing actual body revokes ghost");
    Require(RefreshGhostSetup(guard,*ghost,{},false)==GhostSetupTransition::Unavailable,
        "repeated unavailable observation");
    Require(RefreshGhostSetup(guard,*ghost,key,false)==GhostSetupTransition::Rebound &&
        !ghost->IsCandidateAllowed(),"return mid-lap cannot stitch");
    guard.Reset();ghost->ResetRun();
    Require(RefreshGhostSetup(guard,*ghost,key,true)==GhostSetupTransition::InitialBinding &&
        ghost->IsCandidateAllowed() && !ghost->HasBest(),"new attempt starts independently");
}
void InvalidKeys()
{
    for(const auto key:{Key(-1),Key(VehicleCount),Key(0,-1),Key(0,4),Key(0,0,4),
        Key(0,0,0,2),Key(0,0,0,0,0)})
    {
        GhostSetupGuard guard;
        Require(!key.Valid() && guard.Observe(key)==GhostSetupTransition::Unavailable &&
            !guard.Matches(key),"invalid physical stamp never bound");
    }
}
void AllSetupTransitions()
{
    auto ghost=std::make_unique<BestLapGhost>();
    GhostSetupGuard guard;
    std::mt19937 rng(71289);
    auto prior=Key();
    RefreshGhostSetup(guard,*ghost,prior,true);
    for(int i=0;i<1000;++i)
    {
        const auto next=Key(static_cast<int>(rng()%VehicleCount),static_cast<int>(rng()%4),
            static_cast<int>(rng()%4),static_cast<int>(rng()%2),1+rng()%3);
        const bool boundary=i%2==0;
        const auto transition=RefreshGhostSetup(guard,*ghost,next,boundary);
        Require(guard.Matches(next),"current binding exact");
        if(!(prior==next))
            Require(transition==GhostSetupTransition::Rebound && !ghost->HasBest() &&
                ghost->IsCandidateAllowed()==boundary,"all physical changes isolated");
        else Require(transition==GhostSetupTransition::Unchanged,"same key stable");
        prior=next;
    }
}
}
int main()
{
    IsolateConfigurationChanges();MissingAuthorityAndReset();InvalidKeys();AllSetupTransitions();
    std::cout<<"Ghost setup guard tests passed\n";
}
