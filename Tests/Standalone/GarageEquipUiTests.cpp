#include "Garage/GarageEquipUiPolicy.h"
#include "UI/HudMenuMouseControls.h"
#include <cstdlib>
#include <iostream>
#include <cstring>

namespace
{
void Check(bool condition,const char* why)
{
    if(!condition){std::cerr<<"Unreal garage equip UX failure: "<<why<<'\n';
        std::exit(EXIT_FAILURE);}
}

void BoundaryAndAuthority()
{
    using namespace latebrake;
    struct Case{bool menu, suspended, future, career, owned; MenuPage page;
        int viewed,equipped; GarageEquipUiState expected;};
    const Case cases[]={
        {true,false,false,true,true,MenuPage::Garage,2,0,GarageEquipUiState::Ready},
        {true,false,false,true,true,MenuPage::Garage,0,0,GarageEquipUiState::Equipped},
        {true,false,false,true,false,MenuPage::Garage,2,0,GarageEquipUiState::LockedVehicle},
        {true,false,false,true,true,MenuPage::Garage,-1,0,GarageEquipUiState::InvalidVehicle},
        {true,false,false,true,true,MenuPage::Garage,6,0,GarageEquipUiState::InvalidVehicle},
        {true,false,false,true,true,MenuPage::Garage,1,99,GarageEquipUiState::InvalidVehicle},
        {true,false,false,false,true,MenuPage::Garage,2,0,GarageEquipUiState::CareerLocked},
        {true,false,true,true,true,MenuPage::Garage,2,0,GarageEquipUiState::ReadOnly},
        {true,true,false,true,true,MenuPage::Garage,2,0,GarageEquipUiState::Suspended},
        {false,false,false,true,true,MenuPage::Garage,2,0,GarageEquipUiState::NotInGarage},
        {true,false,false,true,true,MenuPage::Results,2,0,GarageEquipUiState::NotInGarage},
    };
    for(const auto& c:cases){
        auto s=EvaluateGarageEquipUi(c.menu,c.page,c.suspended,c.future,
            c.career,c.owned,c.viewed,c.equipped);
        Check(s==c.expected,"precise status for all forbidden reasons");
        Check(CanActivateGarageEquip(s)==(s==GarageEquipUiState::Ready),
            "only owned unequipped car can be activated");
        Check(std::strlen(GarageEquipUiCaption(s))>0 &&
            std::strlen(GarageEquipUiCaption(s))<=13,
            "bounded, nonempty readable button status");
    }
}

void ButtonRenderAndClickAgreement()
{
    using namespace latebrake;
    MenuDetailPager pager;
    pager.Synchronize(MenuPage::Garage,16,4);
    const auto plan=CalculateDesktopMenuMouseLayout(
        1280,720,CalculateHudLayout(1280,720),false,true);
    Check(plan.Valid,"full-size desktop has a mouse button dock");
    const auto select=plan.Buttons[4].Bounds;
    const double x=select.X+select.Width/2.0;
    const double y=select.Y+select.Height/2.0;
    for(const bool allowed:{false,true}){
        const auto a=AvailableDesktopMenuActions(MenuPage::Garage,pager,allowed);
        Check(a.Activate==allowed,"availability reflects same authoritative state");
        Check(DesktopMenuMouseActionEnabled(DesktopMenuMouseAction::Activate,a)
            ==allowed,"rendered button enabled equals click authority");
        Check((HitTestDesktopMenuMouse(plan,x,y,a) ==
            DesktopMenuMouseAction::Activate)==allowed,
            "disabled Garage equip button cannot dispatch even on click");
        for (const int index : {0,1,5})
        {
            const auto& button=plan.Buttons[static_cast<std::size_t>(index)];
            Check(DesktopMenuMouseActionEnabled(button.Action,a),
                "page and close buttons stay visually enabled regardless of equip");
            Check(HitTestDesktopMenuMouse(plan,
                button.Bounds.X+button.Bounds.Width/2.0,
                button.Bounds.Y+button.Bounds.Height/2.0,a)==button.Action,
                "mouse can leave a locked, equipped or read-only garage");
        }
    }
    auto other=AvailableDesktopMenuActions(MenuPage::Results,pager,false);
    Check(other.Activate &&
        HitTestDesktopMenuMouse(plan,x,y,other)==DesktopMenuMouseAction::Activate,
        "retry/other menu buttons are not globally disabled");
}

void DeterministicStress()
{
    using namespace latebrake;
    unsigned seed=0x9e3779b9u;
    constexpr int iterations=300000;
    for(int i=0;i<iterations;i++){
        seed=seed*1664525u+1013904223u;
        const bool menu=(seed&1u)!=0, suspended=(seed&2u)!=0,
            future=(seed&4u)!=0, career=(seed&8u)!=0,
            owned=(seed&16u)!=0;
        const auto page=(seed&32u)!=0?MenuPage::Garage:MenuPage::Results;
        int viewed=static_cast<int>((seed>>10)%9u)-1;
        int equipped=static_cast<int>((seed>>17)%9u)-1;
        const auto status=EvaluateGarageEquipUi(menu,page,suspended,
            future,career,owned,viewed,equipped);
        const bool expected=menu&&page==MenuPage::Garage&&!suspended&&
            !future&&career&&owned&&GarageState::IsVehicle(viewed)&&
            GarageState::IsVehicle(equipped)&&viewed!=equipped;
        Check(CanActivateGarageEquip(status)==expected,
            "300k cases match an independent explicit permission oracle");
        Check(GarageEquipUiCaption(status)[0]!='\0',
            "each state maps to an observable UI caption");
        const DesktopMenuMouseAvailability availability{
            (seed&64u)!=0,(seed&128u)!=0,expected};
        Check(DesktopMenuMouseActionEnabled(
                  DesktopMenuMouseAction::PreviousPage,availability) &&
              DesktopMenuMouseActionEnabled(
                  DesktopMenuMouseAction::NextPage,availability) &&
              DesktopMenuMouseActionEnabled(
                  DesktopMenuMouseAction::Close,availability),
            "equip authority never leaks into read-only menu navigation");
        Check(DesktopMenuMouseActionEnabled(
                  DesktopMenuMouseAction::Activate,availability)==expected &&
              DesktopMenuMouseActionEnabled(
                  DesktopMenuMouseAction::DetailsUp,availability)==availability.DetailsUp &&
              DesktopMenuMouseActionEnabled(
                  DesktopMenuMouseAction::DetailsDown,availability)==availability.DetailsDown,
            "equip and paging keep independent authority");
    }
}
} // namespace
int main()
{
    BoundaryAndAuthority();
    ButtonRenderAndClickAgreement();
    DeterministicStress();
    std::cout<<"Unreal garage equip button authority + captions passed.\n";
    return EXIT_SUCCESS;
}
