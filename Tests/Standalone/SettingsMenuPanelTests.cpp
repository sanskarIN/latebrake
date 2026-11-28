#include "UI/SettingsMenuPanel.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

namespace
{
using namespace latebrake;
void Require(bool value,const char* reason)
{
    if(!value)
    {
        std::cerr<<"SettingsMenuPanel FAIL: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

SettingsMenuAccess OpenAccess()
{
    return {true,MenuPage::Settings,true,false,false,false,true,true};
}

SettingsPanelLayout DesktopLayout(double w,double h,bool touch=false,MenuPage page=MenuPage::Settings)
{
    const auto hud=CalculateHudLayout(w,h);
    const auto dock=CalculateDesktopMenuMouseLayout(w,h,hud,touch,true);
    if(!dock.Valid)return {};
    const HudMenuReadablePanel text{true,hud.Left,hud.Top,
        dock.Left-hud.Left-12.0*hud.Scale,h-hud.Top-20.0,40.0};
    return CalculateSettingsPanelLayout(w,h,hud,dock,text,touch,page);
}

void TestAccessAndSelection()
{
    auto access=OpenAccess();
    SettingsMenuSelection selection;
    Require(selection.Current()==SettingsControl::SpeedUnits,"default speed choice");
    auto before=selection.Revision();
    Require(selection.Browse(access) && selection.Current()==SettingsControl::HudDetail &&
        selection.Revision()!=before,"browse chooses without mutating a setting");
    Require(selection.Browse(access) && selection.Current()==SettingsControl::CircuitLighting,
        "lighting selectable");
    Require(selection.Browse(access) && selection.Current()==SettingsControl::CameraMotion,
        "camera effects selectable");
    Require(selection.Browse(access) && selection.Current()==SettingsControl::HudScale,
        "HUD scale selectable");
    Require(selection.Browse(access) && selection.Current()==SettingsControl::SpeedUnits,
        "browse wraps after fifth control");
    Require(selection.Select(access,SettingsControl::CircuitLighting),"direct card selection");
    selection.Reset();
    Require(selection.Current()==SettingsControl::SpeedUnits,"page transition reset");
    for(auto page:{MenuPage::Closed,MenuPage::Garage,MenuPage::QuickRace,MenuPage::Results,MenuPage::Career})
    {
        access.Page=page;
        before=selection.Revision();
        Require(!selection.Browse(access) && !selection.Select(access,SettingsControl::HudDetail) &&
            selection.Revision()==before,"other pages cannot change cursor");
        for(auto control:SettingsControls)Require(!access.Allows(control),"other page mutation rejected");
    }
    access=OpenAccess();
    access.MenuOpen=false;
    Require(!selection.Browse(access),"closed menu rejects browse");
    access=OpenAccess();
    access.FocusSuspended=true;
    Require(!selection.Browse(access),"OS focus guard");
    access=OpenAccess();
    access.LocalPlayer=false;
    Require(!selection.Select(access,SettingsControl::HudDetail),"remote driver denied");
    access=OpenAccess();
    Require(!selection.Select(access,static_cast<SettingsControl>(255)),"invalid control");
    access.ProfileReadOnly=true;
    Require(!access.Allows(SettingsControl::SpeedUnits) && access.Allows(SettingsControl::HudDetail),
        "future profile stays protected; session settings remain available");
    access.ManualPauseSuspended=true;
    Require(!access.Allows(SettingsControl::CircuitLighting),"manual pause preserves lighting guard");
    access.ManualPauseSuspended=false;
    access.ActualLightsReady=false;
    Require(!access.Allows(SettingsControl::CircuitLighting),"no light components means no preview");
    Require(selection.Browse(access),"read-only selection still inspectable");
    Require(access.Allows(SettingsControl::CameraMotion),"camera effects independent of save and lighting");
    access.ActualCameraReady=false;
    Require(!access.Allows(SettingsControl::CameraMotion),"missing camera denies action");
}

void TestDescriptions()
{
    SettingsMenuSnapshot snapshot;
    snapshot.Access=OpenAccess();
    snapshot.Selected=SettingsControl::SpeedUnits;
    auto speed=DescribeSettingsControl(snapshot,SettingsControl::SpeedUnits);
    auto hud=DescribeSettingsControl(snapshot,SettingsControl::HudDetail);
    auto light=DescribeSettingsControl(snapshot,SettingsControl::CircuitLighting);
    Require(speed.Selected && speed.Enabled && speed.Saved &&
        std::string_view(speed.Value)=="KM/H","units are saved profile preference");
    Require(!hud.Selected && hud.Enabled && !hud.Saved &&
        std::string_view(hud.Value)=="BALANCED","HUD is session only");
    Require(light.Enabled && !light.Saved && std::string_view(light.Value)=="DAY",
        "lighting is visual preview only");
    snapshot.MilesPerHour=true;
    snapshot.HudMode=HudTelemetryMode::Focused;
    snapshot.Dusk=true;
    snapshot.Selected=SettingsControl::CircuitLighting;
    Require(std::string_view(DescribeSettingsControl(snapshot,SettingsControl::SpeedUnits).Value)=="MPH",
        "actual persisted units displayed");
    Require(std::string_view(DescribeSettingsControl(snapshot,SettingsControl::HudDetail).Value)=="FOCUSED",
        "actual HUD density displayed");
    Require(std::string_view(DescribeSettingsControl(snapshot,SettingsControl::CircuitLighting).Value)=="DUSK",
        "actual scene preset displayed");
    snapshot.Access.ProfileReadOnly=true;
    speed=DescribeSettingsControl(snapshot,SettingsControl::SpeedUnits);
    Require(!speed.Enabled && std::string_view(speed.ShortStatus)=="READ ONLY","locked profile shown");
    snapshot.Access.ActualLightsReady=false;
    light=DescribeSettingsControl(snapshot,SettingsControl::CircuitLighting);
    Require(!light.Enabled && light.Selected &&
        std::string_view(light.ShortStatus)=="UNAVAILABLE","unavailable selected light disclosed");
    snapshot.Access.FocusSuspended=true;
    Require(!DescribeSettingsControl(snapshot,SettingsControl::HudDetail).Enabled,"focus disables action");
    snapshot.Access.FocusSuspended=false;
    snapshot.CameraMotion=CameraMotionMode::Reduced;
    auto camera=DescribeSettingsControl(snapshot,SettingsControl::CameraMotion);
    Require(camera.Enabled && !camera.Saved && std::string_view(camera.Value)=="REDUCED",
        "actual session camera preference");
    snapshot.Access.ActualCameraReady=false;
    Require(!DescribeSettingsControl(snapshot,SettingsControl::CameraMotion).Enabled,
        "unavailable camera shown");
    snapshot.HudScale=HudScalePreference::Larger;
    auto size=DescribeSettingsControl(snapshot,SettingsControl::HudScale);
    Require(size.Enabled && !size.Saved && std::string_view(size.Value)=="LARGER",
        "HUD sizing available independently of camera, lights and profile write");
    snapshot.HudScale=static_cast<HudScalePreference>(255);
    Require(!DescribeSettingsControl(snapshot,SettingsControl::HudScale).Enabled,
        "unknown HUD sizing preference fails closed");
    snapshot.HudScale=HudScalePreference::Automatic;
    snapshot.HudMode=static_cast<HudTelemetryMode>(255);
    Require(!DescribeSettingsControl(snapshot,SettingsControl::HudDetail).Enabled,"invalid mode fails closed");
}

void TestGeometry()
{
    Require(DesktopLayout(1280,720).Valid,"ordinary desktop cards");
    Require(DesktopLayout(1920,1080).Valid,"full HD cards");
    Require(!DesktopLayout(800,600).Valid,"compact uses text");
    Require(!DesktopLayout(1920,1080,true).Valid,"touch retains existing browse pad");
    Require(!DesktopLayout(1920,1080,false,MenuPage::Garage).Valid,"no collision with garage controls");
    std::mt19937 generator(118u);
    std::uniform_int_distribution<int> sizeW(900,3840),sizeH(600,2160);
    int accepted=0;
    for(int i=0;i<3000;++i)
    {
        const double w=sizeW(generator),h=sizeH(generator);
        const auto layout=DesktopLayout(w,h);
        if(!layout.Valid)continue;
        ++accepted;
        Require(ContainsSettingsRect({0,0,w,h},layout.Panel),"panel within viewport");
        Require(ContainsSettingsRect(layout.Panel,layout.Header),"visible header");
        const auto hud=CalculateHudLayout(w,h);
        const auto dock=CalculateDesktopMenuMouseLayout(w,h,hud,false,true);
        for(const auto& b:dock.Buttons)
            Require(!SettingsRectsOverlap(b.Bounds,layout.Panel),"no existing dock overlap");
        for(std::size_t n=0;n<layout.Rows.size();++n)
        {
            const auto& row=layout.Rows[n];
            Require(row.Control==SettingsControls[n] && row.Bounds.Height>=60 &&
                ContainsSettingsRect(layout.Panel,row.Bounds),"complete accessible card");
            Require(ContainsSettingsRect(row.Bounds,row.Title) &&
                ContainsSettingsRect(row.Bounds,row.Value) &&
                ContainsSettingsRect(row.Bounds,row.Status),"font zones contained");
            Require(!SettingsRectsOverlap(row.Title,row.Value) &&
                !SettingsRectsOverlap(row.Value,row.Status),"caption zones separated");
            for(std::size_t j=0;j<n;++j)
                Require(!SettingsRectsOverlap(row.Bounds,layout.Rows[j].Bounds),"cards do not overlap");
        }
    }
    Require(accepted>1500,"desktop sweep exercised visible layouts");
    const auto hud=CalculateHudLayout(1280,720);
    const auto dock=CalculateDesktopMenuMouseLayout(1280,720,hud,false,true);
    Require(!CalculateSettingsPanelLayout(1280,720,hud,dock,
        {true,0,0,1280,720,40},false,MenuPage::Settings).Valid,"reject colliding text lane");
    Require(!CalculateSettingsPanelLayout(std::numeric_limits<double>::infinity(),720,hud,dock,
        {true,0,0,100,720,40},false,MenuPage::Settings).Valid,"invalid viewport");
}

void TestPointerReceipt()
{
    auto layout=DesktopLayout(1920,1080);
    Require(layout.Valid,"valid pointer test layout");
    SettingsMenuSnapshot snapshot{OpenAccess(),SettingsControl::SpeedUnits,false,
        HudTelemetryMode::Balanced,false,7};
    SettingsPanelPresentation shown{true,layout,snapshot,{}};
    shown.Rendered.fill(true);
    for(std::size_t i=0;i<layout.Rows.size();++i)
    {
        const auto& b=layout.Rows[i].Bounds;
        const auto hit=HitPresentedSettingsPanel(shown,snapshot,1920,1080,
            b.X+b.Width*0.5,b.Y+b.Height*0.5);
        Require(hit.Consumed && hit.Action==SettingsControls[i],"one visible row one action");
    }
    const auto& b=layout.Rows[1].Bounds;
    const double x=b.X+10,y=b.Y+10;
    const auto rejects=[&](SettingsMenuSnapshot changed,double w=1920,double h=1080)
    {
        const auto hit=HitPresentedSettingsPanel(shown,changed,w,h,x,y);
        Require(hit.Consumed && hit.Action==SettingsControl::Count,"stale input consumed without mutation");
    };
    auto changed=snapshot;++changed.Revision;rejects(changed);
    changed=snapshot;changed.MilesPerHour=true;rejects(changed);
    changed=snapshot;changed.Dusk=true;rejects(changed);
    changed=snapshot;changed.CameraMotion=CameraMotionMode::Reduced;rejects(changed);
    changed=snapshot;changed.HudScale=HudScalePreference::Larger;rejects(changed);
    changed=snapshot;changed.Access.ActualCameraReady=false;rejects(changed);
    changed=snapshot;changed.HudMode=HudTelemetryMode::Detailed;rejects(changed);
    changed=snapshot;changed.Selected=SettingsControl::HudDetail;rejects(changed);
    changed=snapshot;changed.Access.FocusSuspended=true;rejects(changed);
    changed=snapshot;changed.Access.ProfileReadOnly=true;rejects(changed);
    rejects(snapshot,1600,900);
    shown.Rendered[1]=false;
    rejects(snapshot);
    shown.Rendered[1]=true;
    shown.Layout.Rows[1].Control=SettingsControl::SpeedUnits;
    rejects(snapshot);
    shown.Layout.Rows[1].Control=SettingsControl::HudDetail;
    auto hit=HitPresentedSettingsPanel(shown,snapshot,1920,1080,-1,-1);
    Require(!hit.Consumed,"outside lane cannot intercept actual navigation");
    hit=HitPresentedSettingsPanel(shown,snapshot,1920,1080,
        layout.Header.X+1,layout.Header.Y+1);
    Require(hit.Consumed && hit.Action==SettingsControl::Count,"header is read only");
    shown.Valid=false;
    Require(!HitPresentedSettingsPanel(shown,snapshot,1920,1080,x,y).Consumed,
        "unpresented panel no hit");
}
}

int main()
{
    TestAccessAndSelection();
    TestDescriptions();
    TestGeometry();
    TestPointerReceipt();
    std::cout<<"Settings selection, guarded activation and pointer layout passed.\n";
    return EXIT_SUCCESS;
}
