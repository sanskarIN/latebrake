#include "Racing/PracticeTrialSetup.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool condition,const char* why)
{
    if(!condition)
    {
        std::cerr<<"PracticeTrialSetup: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
PracticeTrialAccess Access()
{
    return {true,MenuPage::QuickRace,true,false,QuickPracticePreset::SoloThree};
}
bool Equal(GarageSnapshot a,GarageSnapshot b)
{
    return a.Credits==b.Credits && a.OwnedVehicleMask==b.OwnedVehicleMask &&
        a.SelectedVehicle==b.SelectedVehicle && a.EngineLevel==b.EngineLevel &&
        a.GripLevel==b.GripLevel;
}
void AllConfigurations()
{
    GarageState garage;
    garage.Load({900,1,0,1,2});
    const auto original=garage.Snapshot();
    PracticeTrialSetup setup;
    Require(!setup.Pending(original).Enabled,"saved configuration default");
    Require(setup.Edit(Access(),PracticeTrialAction::Toggle,original),"enable free trial");
    Require(setup.Pending(original).Key==MakeGarageTuningKey(original),"copy current garage");
    for(int id=0;id<VehicleCount;++id)
    {
        setup.Edit(Access(),PracticeTrialAction::ChooseCar,original,id);
        for(int engine=0;engine<4;++engine)
        {
            while(setup.Pending(original).Key.Engine!=engine)
                Require(setup.Edit(Access(),PracticeTrialAction::NextEngine,original),"cycle engine");
            for(int grip=0;grip<4;++grip)
            {
                while(setup.Pending(original).Key.Grip!=grip)
                    Require(setup.Edit(Access(),PracticeTrialAction::NextGrip,original),"cycle grip");
                const auto trial=setup.Pending(original);
                Require(trial.Enabled && trial.Key==GarageTuningKey{id,engine,grip},"exact configuration");
                Require(trial.Owned==(id==0),"ownership not fabricated");
                GarageState model;
                model.Load({0,KnownVehicleMask,id,engine,grip});
                const auto expected=model.Performance();
                Require(trial.Performance.Acceleration==expected.Acceleration &&
                    trial.Performance.Grip==expected.Grip &&
                    trial.Performance.TopSpeed==expected.TopSpeed,"matches existing handling model");
                Require(setup.BeginPractice(Access()),"launch");
                Require(setup.Active(Access().Preset,original).Key==trial.Key,"latched physical setup");
                Require(!setup.Active(QuickPracticePreset::None,original).Enabled,"never applies to normal race");
                Require(Equal(garage.Snapshot(),original),"no profile mutation");
            }
        }
    }
}

void PendingAndActiveIsolation()
{
    PracticeTrialSetup setup;
    const GarageSnapshot saved{2000,3,1,2,1};
    const GarageSnapshot changed{1300,7,2,3,0};
    Require(setup.Edit(Access(),PracticeTrialAction::Toggle,saved),"enable");
    Require(setup.Edit(Access(),PracticeTrialAction::ChooseCar,saved,5),"unowned car can test");
    Require(setup.BeginPractice(Access()),"launch");
    const auto active=setup.Active(Access().Preset,saved);
    Require(active.Key.Selected==5 && !active.Owned,"unowned trial latched");
    Require(setup.Edit(Access(),PracticeTrialAction::NextEngine,saved),"stage next tuning");
    Require(setup.Edit(Access(),PracticeTrialAction::NextCar,saved),"wrap to car zero");
    Require(setup.Pending(saved).Key.Selected==0,"pending wrapped");
    Require(setup.Active(Access().Preset,changed).Key==active.Key,"pending and saved changes never retune active trial");
    Require(setup.Edit(Access(),PracticeTrialAction::Toggle,changed),"pending disable");
    Require(!setup.Pending(changed).Enabled && setup.Active(Access().Preset,changed).Enabled,"toggle next race only");
    setup.EndPractice();
    Require(!setup.Active(Access().Preset,changed).Enabled,"exit restores owned configuration");
    Require(setup.Active(Access().Preset,changed).Key==MakeGarageTuningKey(changed),"restore latest persisted setup");
    Require(setup.BeginPractice(Access()),"ordinary practice launch");
    Require(!setup.Active(Access().Preset,changed).Enabled,"disabled pending remains disabled");
    Require(setup.Edit(Access(),PracticeTrialAction::Toggle,changed),"enable from new saved setup");
    Require(setup.Pending(changed).Key==MakeGarageTuningKey(changed),"no stale re-enable");
    Require(setup.Edit(Access(),PracticeTrialAction::NextGrip,changed),"change grip");
    Require(setup.Edit(Access(),PracticeTrialAction::Reset,changed),"reset to saved");
    Require(setup.Pending(changed).Key==MakeGarageTuningKey(changed),"reset exact");
    Require(!setup.Edit(Access(),PracticeTrialAction::Reset,changed),"idempotent reset");
}
void AuthorizationAndMalformedInput()
{
    const GarageSnapshot saved{200,1,0,0,0};
    PracticeTrialSetup setup;
    Require(setup.Edit(Access(),PracticeTrialAction::Toggle,saved),"enable");
    const auto original=setup.Pending(saved).Key;
    const PracticeTrialAction actions[]={PracticeTrialAction::Toggle,PracticeTrialAction::NextCar,
        PracticeTrialAction::ChooseCar,PracticeTrialAction::NextEngine,
        PracticeTrialAction::NextGrip,PracticeTrialAction::Reset};
    for(int flag=0;flag<5;++flag)
    {
        auto denied=Access();
        switch(flag)
        {
        case 0:denied.MenuOpen=false;break;
        case 1:denied.Page=MenuPage::Garage;break;
        case 2:denied.LocalDriver=false;break;
        case 3:denied.Suspended=true;break;
        case 4:denied.Preset=QuickPracticePreset::None;break;
        }
        for(auto action:actions)
            Require(!setup.Edit(denied,action,saved,5),"blocked context cannot edit");
        Require(!setup.BeginPractice(denied),"blocked context cannot launch");
        Require(setup.Pending(saved).Key==original,"blocked inputs preserve pending state");
        Require(!setup.Active(Access().Preset,saved).Enabled,"blocked launch leaves inactive");
    }
    for(int id:{-1,VehicleCount,std::numeric_limits<int>::min(),std::numeric_limits<int>::max()})
        Require(!setup.Edit(Access(),PracticeTrialAction::ChooseCar,saved,id),"invalid car denied");
    Require(!setup.Edit(Access(),static_cast<PracticeTrialAction>(255),saved),"invalid action denied");
    Require(setup.Pending(saved).Key==original,"malformed edits unchanged");
    Require(!ValidPracticeTrialKey({0,-1,0}) && !ValidPracticeTrialKey({0,4,0}) &&
        !ValidPracticeTrialKey({0,0,4}) && !ValidPracticeTrialKey({99,0,0}),"strict key validation");
    const GarageSnapshot corrupt{-50,-5,99,999,-50};
    PracticeTrialSetup clean;
    Require(clean.Edit(Access(),PracticeTrialAction::Toggle,corrupt),"sanitize saved input");
    Require(clean.Pending(corrupt).Key==GarageTuningKey{0,3,0},"saved sanitization reused");
}
void LooksAndPersistence()
{
    GarageAppearanceState looks;
    Require(looks.Choose(0,GarageLookLayer::Paint,7),"owned color");
    Require(looks.Choose(5,GarageLookLayer::Paint,1),"historical unowned appearance");
    const auto before=looks.Serialize();
    const GarageSnapshot saved{0,1,0,0,0};
    PracticeTrialSetup setup;
    setup.Edit(Access(),PracticeTrialAction::Toggle,saved);
    Require(SameGarageLook(PracticeTrialLook(setup.Pending(saved),looks),looks.Get(0)),"owned style visible");
    setup.Edit(Access(),PracticeTrialAction::ChooseCar,saved,5);
    Require(SameGarageLook(PracticeTrialLook(setup.Pending(saved),looks),FactoryGarageLook(5)),"unowned factory look");
    Require(looks.Serialize()==before,"no appearance mutation");
}
void RandomLifecycle()
{
    std::mt19937 rng(794611);
    PracticeTrialSetup setup;
    GarageState garage;
    garage.Load({12345,7,1,1,2});
    const auto saved=garage.Snapshot();
    for(int i=0;i<12000;++i)
    {
        const auto before=setup.Active(Access().Preset,saved);
        switch(rng()%5)
        {
        case 0:
            Require(setup.BeginPractice(Access()),"valid launch");
            Require(setup.Active(Access().Preset,saved).Key==setup.Pending(saved).Key,"launch matches pending");
            break;
        case 1:
            setup.EndPractice();
            Require(!setup.Active(Access().Preset,saved).Enabled,"exit");
            break;
        default:
            setup.Edit(Access(),static_cast<PracticeTrialAction>(rng()%6),saved,
                static_cast<int>(rng()%10)-2);
            Require(setup.Active(Access().Preset,saved).Key==before.Key &&
                setup.Active(Access().Preset,saved).Enabled==before.Enabled,"editing never affects active snapshot");
            break;
        }
        Require(ValidPracticeTrialKey(setup.Pending(saved).Key),"pending always valid");
        Require(ValidPracticeTrialKey(setup.Active(Access().Preset,saved).Key),"active always valid");
        Require(Equal(garage.Snapshot(),saved),"saved profile unchanged");
    }
}
}
int main()
{
    AllConfigurations();
    PendingAndActiveIsolation();
    AuthorizationAndMalformedInput();
    LooksAndPersistence();
    RandomLifecycle();
    std::cout<<"Practice trial setup tests passed\n";
}
