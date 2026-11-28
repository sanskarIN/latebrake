#include "UI/ProfileOverviewPanel.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok)
    {
        std::cerr<<"ProfileOverviewPanel: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b)
{
    return std::abs(a-b)<1e-7;
}
ProfileOverviewSource Example()
{
    ProfileOverviewSource s;
    s.Career.Medals[0]=1;
    s.Career.Medals[1]=2;
    s.Career.Medals[2]=3;
    s.Career.Medals[6]=3;
    s.Garage={1600,7,2,2,1};
    s.CompletedRaces=25;
    s.CircuitBest=145.25;
    s.BestLap=46.5;
    return s;
}
void RealProfileValues()
{
    const auto source=Example();
    const auto read=ReadProfileOverview(source);
    Require(read.Valid && read.Status==ProfileOverviewStatus::Current,
        "canonical local profile");
    Require(read.CompletedRaces==25 && read.Credits==1600 &&
        read.EquippedCar==2 && read.OwnedCars==3 &&
        read.EngineLevel==2 && read.GripLevel==1,"garage and race counts from source");
    Require(read.CareerCompleted==4 && read.Bronze==1 &&
        read.Silver==1 && read.Gold==2,"medal counts conserve completed events");
    Require(read.CircuitBestValid && Near(read.CircuitBest,145.25) &&
        read.BestLapValid && Near(read.BestLap,46.5),"observed stored best times");
    const auto empty=ReadProfileOverview({});
    Require(empty.Valid && empty.CompletedRaces==0 && empty.Credits==0 &&
        empty.CareerCompleted==0 && empty.OwnedCars==1 &&
        !empty.CircuitBestValid && !empty.BestLapValid,"starter state has no fake records");
}
void PersistenceAndCorruptData()
{
    auto source=Example();
    source.ReadOnly=true;
    auto read=ReadProfileOverview(source);
    Require(read.Status==ProfileOverviewStatus::ReadOnly &&
        !read.CircuitBestValid && !read.BestLapValid &&
        read.CircuitBest==0 && read.BestLap==0,"read-only profile does not assert saved records");
    source.SaveFailed=true;
    read=ReadProfileOverview(source);
    Require(read.Status==ProfileOverviewStatus::SaveFailed &&
        !read.CircuitBestValid && !read.BestLapValid,"failed persistence dominates status");
    source=Example();
    source.Garage={-100,-1,5,99,-1};
    source.Career.Medals.fill(0);
    source.Career.Medals[2]=3;
    source.Career.Medals[0]=255;
    source.CompletedRaces=-3;
    source.CircuitBest=std::numeric_limits<double>::infinity();
    source.BestLap=-1;
    read=ReadProfileOverview(source);
    Require(read.Valid && read.Credits==0 && read.EquippedCar==0 &&
        read.OwnedCars==1 && read.EngineLevel==3 && read.GripLevel==0 &&
        read.CompletedRaces==0 && read.CareerCompleted==0 &&
        !read.CircuitBestValid && !read.BestLapValid,"existing sanitation rules reused");
    source=Example();
    source.Career.Schema=CareerSaveSchema+1;
    source.CircuitBest=604800;
    source.BestLap=604801;
    read=ReadProfileOverview(source);
    Require(read.CareerCompleted==0 && read.CircuitBestValid &&
        !read.BestLapValid,"unknown career schema and record upper boundary");
    source=Example();
    source.CompletedRaces=std::numeric_limits<int>::max();
    source.Garage.Credits=std::numeric_limits<int>::max();
    read=ReadProfileOverview(source);
    Require(read.CompletedRaces==source.CompletedRaces &&
        read.Credits==source.Garage.Credits,"maximum legal counters not truncated");
}
struct View final
{
    double Width=0,Height=0;
    HudLayout Hud{};
    HudMenuVerticalPlan Vertical{};
    HudMenuReadablePanel Readable{};
    DesktopMenuMouseLayout Dock{};
    double SafeWidth=0;
    View(double w=1280,double h=720,bool touch=false):Width(w),Height(h)
    {
        Hud=CalculateHudLayout(w,h);
        Vertical=CalculateHudMenuVerticalPlan(w,h,Hud,touch);
        const auto text=CalculateHudTextBounds(w,h,Hud,touch);
        Dock=CalculateDesktopMenuMouseLayout(w,h,Hud,touch,true);
        SafeWidth=text.MaxWidthPixels;
        if(Dock.Valid)SafeWidth=std::min(SafeWidth,Dock.Left-text.Left-12*Hud.Scale);
        Readable=CalculateHudMenuReadablePanel(w,h,Hud,Vertical,SafeWidth,true);
    }
    ProfileOverviewLayout Plan(int pinned=2,bool profile=true,bool touch=false) const
    {
        return CalculateProfileOverviewLayout(Hud,Vertical,Readable,SafeWidth,
            Hud.Top+pinned*28*Hud.Scale,pinned,profile,touch);
    }
};
void CheckLayout(const View& view,const ProfileOverviewLayout& p,int pinned)
{
    Require(p.Valid && HudRadarContains({0,0,view.Width,view.Height},p.Panel),
        "viewport containment");
    Require(p.Panel.X==view.Hud.Left && p.Panel.Width<=view.SafeWidth+1e-7,
        "protected lane");
    const int remaining=view.Vertical.MaxDetailsRows-pinned-p.ReservedRows;
    Require(remaining>=ProfileOverviewMinDetails &&
        p.NextDetailY>=p.Panel.Y+p.Panel.Height &&
        p.NextDetailY+(remaining-1)*28*view.Hud.Scale<=
            view.Vertical.OverflowNoticeY-1.25*28*view.Hud.Scale+1e-7,
        "pager and warnings unaffected");
    if(view.Dock.Valid)
        for(const auto& button:view.Dock.Buttons)
            Require(!HudRadarOverlaps(p.Panel,button.Bounds),"right dock untouched");
    for(std::size_t i=0;i<p.Tiles.size();++i)
    {
        const auto& tile=p.Tiles[i];
        Require(HudRadarContains(p.Panel,tile.Bounds) &&
            HudRadarContains(tile.Bounds,tile.Label) &&
            HudRadarContains(tile.Bounds,tile.Value) &&
            !HudRadarOverlaps(tile.Label,tile.Value),"measured independent tile lanes");
        for(std::size_t j=0;j<i;++j)
            Require(!HudRadarOverlaps(tile.Bounds,p.Tiles[j].Bounds),"tile separation");
    }
}
ProfileOverviewMeasurements Measures(const ProfileOverviewLayout& p)
{
    ProfileOverviewMeasurements m;
    const auto fit=[](const DesktopMenuMouseRect& r)
    {return HudRaceStatusTextSize{r.Width*0.75,r.Height*0.75};};
    m.Header=fit(p.Header);m.Medals=fit(p.MedalLabel);m.Vehicles=fit(p.VehicleLabel);
    for(std::size_t i=0;i<p.Tiles.size();++i)
    {
        m.Labels[i]=fit(p.Tiles[i].Label);m.Values[i]=fit(p.Tiles[i].Value);
    }
    return m;
}
void ResponsiveAndMeasuredFallback()
{
    for(const auto& size:std::array<std::array<double,2>,6>{{
        {900,600},{1120,600},{1280,720},{1920,1080},{2560,1440},{3440,1440}}})
    {
        const View v(size[0],size[1]);CheckLayout(v,v.Plan(),2);
    }
    const View v;
    Require(!v.Plan(2,false).Valid && !v.Plan(2,true,true).Valid &&
        !View(640,480).Plan().Valid,"compact and unrelated page fallback");
    auto bad=v;bad.SafeWidth=200;
    Require(!bad.Plan().Valid,"narrow layout");
    bad=v;bad.Vertical.MaxDetailsRows=8;
    Require(!bad.Plan().Valid,"pager reserve");
    bad=v;bad.Readable.Valid=false;
    Require(!bad.Plan().Valid,"no orphan panel");
    const auto p=v.Plan();
    const auto valid=Measures(p);
    Require(CanDrawProfileOverview(p,valid),"all text fits");
    for(std::size_t i=0;i<p.Tiles.size();++i)
    {
        auto m=valid;m.Values[i].Width=p.Tiles[i].Value.Width+1;
        Require(!CanDrawProfileOverview(p,m),"long value must fall back");
        m=valid;m.Labels[i].Height=0;
        Require(!CanDrawProfileOverview(p,m),"missing label");
    }
    auto m=valid;m.Header.Width=p.Header.Width+1;
    Require(!CanDrawProfileOverview(p,m),"cannot omit status");
    m=valid;m.Medals.Width=p.MedalLabel.Width+1;
    Require(!CanDrawProfileOverview(p,m),"cannot omit medal legend");
    m=valid;m.Vehicles.Height=std::numeric_limits<double>::quiet_NaN();
    Require(!CanDrawProfileOverview(p,m),"nonfinite vehicle legend");
}
void AllCollectionFractions()
{
    const auto p=View().Plan();
    for(int bronze=0;bronze<=CareerEventCount;++bronze)
        for(int silver=0;silver<=CareerEventCount-bronze;++silver)
            for(int gold=0;gold<=CareerEventCount-bronze-silver;++gold)
                for(int cars=1;cars<=VehicleCount;++cars)
                {
                    auto r=ReadProfileOverview({});
                    r.Bronze=bronze;r.Silver=silver;r.Gold=gold;
                    r.CareerCompleted=bronze+silver+gold;r.OwnedCars=cars;
                    const auto bars=BuildProfileOverviewBars(p,r);
                    Require(bars.Valid,"valid medal and car partition");
                    const std::array<int,4> counts{{bronze,silver,gold,
                        CareerEventCount-r.CareerCompleted}};
                    double sum=0;
                    for(std::size_t i=0;i<counts.size();++i)
                    {
                        const auto& part=bars.Medals[i];
                        Require(Near(part.Width/p.MedalTrack.Width,
                            static_cast<double>(counts[i])/CareerEventCount),
                            "each segment proportional to canonical count");
                        if(i>0)Require(Near(part.X,bars.Medals[i-1].X+
                            bars.Medals[i-1].Width),"contiguous medal segments");
                        sum+=part.Width;
                    }
                    Require(Near(sum,p.MedalTrack.Width) &&
                        Near(bars.Vehicles.Width/p.VehicleTrack.Width,
                            static_cast<double>(cars)/VehicleCount),"full conservation");
                }
    auto r=ReadProfileOverview({});
    r.Gold=1;
    Require(!BuildProfileOverviewBars(p,r).Valid,"mismatched totals");
    r=ReadProfileOverview({});r.OwnedCars=0;
    Require(!BuildProfileOverviewBars(p,r).Valid,"starter must exist");
    r=ReadProfileOverview({});r.Bronze=std::numeric_limits<int>::max();
    Require(!BuildProfileOverviewBars(p,r).Valid,"malformed count bounded before arithmetic");
}
void RandomCanonicalProfiles()
{
    std::mt19937 rng(73194);
    int visible=0;
    for(int i=0;i<6000;++i)
    {
        auto s=Example();
        for(auto& m:s.Career.Medals)m=static_cast<std::uint8_t>(rng()%6);
        s.Garage={static_cast<int>(rng()%10000),static_cast<int>(rng()%150)-5,
            static_cast<int>(rng()%8),static_cast<int>(rng()%6),static_cast<int>(rng()%6)};
        s.CompletedRaces=static_cast<int>(rng()%2000);
        s.ReadOnly=rng()%10==0;s.SaveFailed=rng()%10==0;
        CareerProgress career;career.Load(s.Career);
        GarageState garage;garage.Load(s.Garage);
        const auto data=ReadProfileOverview(s);
        Require(data.Valid && data.CareerCompleted==career.CompletedCount() &&
            data.EquippedCar==garage.SelectedVehicle() && data.Credits==garage.Credits(),
            "canonical profile authority");
        const View v(240+rng()%3800,240+rng()%2000);
        const auto plan=v.Plan();
        if(!plan.Valid)continue;
        ++visible;CheckLayout(v,plan,2);
        Require(CanDrawProfileOverview(plan,Measures(plan)) &&
            BuildProfileOverviewBars(plan,data).Valid,"complete bounded profile card");
    }
    Require(visible>2000,"broad desktop layout coverage");
}
} // namespace
int main()
{
    RealProfileValues();PersistenceAndCorruptData();ResponsiveAndMeasuredFallback();
    AllCollectionFractions();RandomCanonicalProfiles();
    std::cout<<"Profile overview data, layout and progress tests passed\n";
}
