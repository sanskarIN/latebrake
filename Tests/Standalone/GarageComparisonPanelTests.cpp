#include "UI/GarageComparisonPanel.h"
#include "UI/GarageShowroomLane.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

namespace
{
using namespace latebrake;

void Require(bool ok,const char* why)
{
    if(!ok)
    {
        std::cerr<<"GarageComparisonPanel: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a,double b,double tolerance=1e-7)
{
    return std::abs(a-b)<=tolerance;
}

GarageCatalogView Catalog(int selected=0,int owned=1,int credits=0,int engine=0,int grip=0)
{
    return BuildGarageCatalogView({credits,owned,selected,engine,grip});
}

void ConfiguredCarDifferences()
{
    const auto arrow=ReadGarageComparisonPanel(Catalog(),0);
    Require(arrow.Valid && arrow.ViewedId==0 && arrow.EquippedId==0 &&
        arrow.ViewedOwnership==GarageCatalogOwnership::Equipped &&
        arrow.AxisPercent==25.0,"equipped comparison is neutral");
    for(const auto& metric:arrow.Metrics)
        Require(Near(metric.Equipped,metric.Viewed) && metric.DeltaPercent==0 &&
            metric.Direction==GarageComparisonDirection::Equal,"identical configured values");
    const auto kestrel=ReadGarageComparisonPanel(Catalog(),1);
    Require(kestrel.Valid && kestrel.ViewedOwnership==GarageCatalogOwnership::LockedInsufficientCredits,
        "locked model still has read-only configuration");
    Require(Near(kestrel.Metrics[0].DeltaPercent,12.0) &&
        Near(kestrel.Metrics[1].DeltaPercent,-6.0) &&
        Near(kestrel.Metrics[2].DeltaPercent,-2.0),"exact original vehicle multipliers");
    Require(kestrel.Metrics[0].Direction==GarageComparisonDirection::Higher &&
        kestrel.Metrics[1].Direction==GarageComparisonDirection::Lower &&
        kestrel.Metrics[2].Direction==GarageComparisonDirection::Lower,"signed direction");
    const auto glider=ReadGarageComparisonPanel(Catalog(),4);
    Require(glider.Valid && Near(glider.Metrics[0].DeltaPercent,0) &&
        Near(glider.Metrics[1].DeltaPercent,23),"different vehicle strengths visible");
    const auto affordable=ReadGarageComparisonPanel(Catalog(0,1,100000),5);
    Require(affordable.Valid &&
        affordable.ViewedOwnership==GarageCatalogOwnership::LockedAffordable &&
        std::string_view(GarageComparisonOwnershipLabel(affordable.ViewedOwnership))=="LOCKED PREVIEW",
        "affordability is not ownership");
    const auto owned=ReadGarageComparisonPanel(Catalog(0,3),1);
    Require(owned.Valid && owned.ViewedOwnership==GarageCatalogOwnership::Owned,
        "owned is distinct from equipped");
}

void AllCarsAndSharedUpgrades()
{
    for(int selected=0;selected<VehicleCount;++selected)
    {
        for(int viewed=0;viewed<VehicleCount;++viewed)
        {
            const auto baseline=ReadGarageComparisonPanel(Catalog(selected,KnownVehicleMask),viewed);
            Require(baseline.Valid,"all owned pairs");
            for(int engine=0;engine<=3;++engine)
            {
                for(int grip=0;grip<=3;++grip)
                {
                    const auto catalogue=Catalog(selected,KnownVehicleMask,4000,engine,grip);
                    const auto row=ReadGarageComparisonPanel(catalogue,viewed);
                    Require(row.Valid && row.EquippedId==selected && row.ViewedId==viewed,
                        "selected and viewed identities independent");
                    for(std::size_t i=0;i<row.Metrics.size();++i)
                    {
                        const auto& metric=row.Metrics[i];
                        Require(static_cast<int>(metric.Metric)==static_cast<int>(i) &&
                            Near(metric.DeltaPercent,baseline.Metrics[i].DeltaPercent),
                            "shared upgrades cancel in the relative comparison");
                        Require(Near(metric.DeltaPercent,
                            (metric.Viewed/metric.Equipped-1.0)*100.0),"no guessed physical performance");
                        Require(std::abs(metric.DeltaPercent)<=row.AxisPercent &&
                            row.AxisPercent>=GarageComparisonMinimumAxisPercent,
                            "shared signed axis contains every metric");
                    }
                }
            }
        }
    }
}

void RejectMixedCatalogue()
{
    auto catalogue=Catalog();
    Require(!ReadGarageComparisonPanel(catalogue,-1).Valid &&
        !ReadGarageComparisonPanel(catalogue,VehicleCount).Valid,"invalid viewed identity");
    const auto nan=std::numeric_limits<double>::quiet_NaN();
    for(int fault=0;fault<10;++fault)
    {
        auto bad=catalogue;
        switch(fault)
        {
        case 0:bad.SelectedVehicle=-1;break;
        case 1:bad.Cars[1].VehicleId=2;break;
        case 2:bad.Cars[0].Ownership=GarageCatalogOwnership::Owned;break;
        case 3:bad.Cars[1].Ownership=GarageCatalogOwnership::Equipped;break;
        case 4:bad.Cars[1].Ownership=static_cast<GarageCatalogOwnership>(99);break;
        case 5:bad.Cars[1].Effective.Acceleration=nan;break;
        case 6:bad.Cars[0].Effective.Grip=0;break;
        case 7:bad.Cars[1].Effective.TopSpeed=-1;break;
        case 8:bad.Cars[1].Effective.Acceleration=100;break;
        case 9:bad.Cars[0].VehicleId=3;break;
        }
        Require(!ReadGarageComparisonPanel(bad,1).Valid,"mixed or nonfinite catalogue refused");
    }
    // The sanitized load must not create ownership from corrupted masks.
    const auto sanitized=ReadGarageComparisonPanel(Catalog(5,-1,-50,99,-3),5);
    Require(sanitized.Valid && sanitized.EquippedId==0 &&
        sanitized.ViewedOwnership==GarageCatalogOwnership::LockedInsufficientCredits,
        "honors the authoritative save sanitation");
}

struct MenuView final
{
    double Width=1280;
    double Height=720;
    HudLayout Hud{};
    HudMenuVerticalPlan Vertical{};
    HudMenuReadablePanel Readable{};
    GarageShowroomLane Showroom{};
    double SafeWidth=0;

    MenuView(double width=1280,double height=720,bool touch=false)
        :Width(width),Height(height)
    {
        Hud=CalculateHudLayout(width,height);
        Vertical=CalculateHudMenuVerticalPlan(width,height,Hud,touch);
        const auto text=CalculateHudTextBounds(width,height,Hud,touch);
        const auto dock=CalculateDesktopMenuMouseLayout(width,height,Hud,touch,true);
        Showroom=CalculateGarageShowroomLane(width,height,Hud,touch,true,MenuPage::Garage);
        SafeWidth=text.MaxWidthPixels;
        if(dock.Valid)
            SafeWidth=std::min(SafeWidth,dock.Left-text.Left-12.0*Hud.Scale);
        if(Showroom.Valid)
            SafeWidth=std::min(SafeWidth,Showroom.TextWidth);
        Readable=CalculateHudMenuReadablePanel(width,height,Hud,Vertical,SafeWidth,true);
    }

    GarageComparisonPanelLayout Plan(int pinned=2,bool garage=true,bool touch=false) const
    {
        return CalculateGarageComparisonPanelLayout(Hud,Vertical,Readable,SafeWidth,
            Hud.Top+pinned*28.0*Hud.Scale,pinned,garage,touch);
    }
};

void CheckLayout(const MenuView& view,const GarageComparisonPanelLayout& layout,int pinned)
{
    Require(layout.Valid && HudRadarContains({0,0,view.Width,view.Height},layout.Panel),
        "card inside viewport");
    const DesktopMenuMouseRect owner{view.Readable.X,view.Readable.Y,
        view.Readable.Width,view.Readable.Height};
    Require(HudRadarContains(owner,layout.Panel) &&
        layout.Panel.Width<=view.SafeWidth+1e-7,"protected left lane");
    Require(Near(layout.Panel.Y,view.Hud.Top+pinned*28.0*view.Hud.Scale),
        "purchase feedback remains above comparison");
    Require(layout.ReservedRows==5 &&
        Near(layout.NextDetailY,layout.Panel.Y+5*28.0*view.Hud.Scale) &&
        layout.NextDetailY>=layout.Panel.Y+layout.Panel.Height,
        "integral row reservation");
    const int remaining=view.Vertical.MaxDetailsRows-pinned-layout.ReservedRows;
    Require(remaining>=GarageComparisonMinimumDetailRows &&
        layout.NextDetailY+(remaining-1)*28.0*view.Hud.Scale<=
            view.Vertical.OverflowNoticeY-1.25*28.0*view.Hud.Scale+1e-7,
        "detail pager and save warning keep independent baselines");
    if(view.Showroom.Valid)
        Require(!HudRadarOverlaps(layout.Panel,view.Showroom.Aperture),"3D showroom unobscured");
    const auto dock=CalculateDesktopMenuMouseLayout(view.Width,view.Height,view.Hud,false,true);
    if(dock.Valid)
        Require(layout.Panel.X+layout.Panel.Width<=dock.Left,"purchase and menu dock separated");
    std::array<DesktopMenuMouseRect,11> parts{};
    parts[0]=layout.Title;parts[1]=layout.Context;
    for(std::size_t i=0;i<layout.Metrics.size();++i)
    {
        parts[2+3*i]=layout.Metrics[i].Label;
        parts[3+3*i]=layout.Metrics[i].Delta;
        parts[4+3*i]=layout.Metrics[i].Track;
    }
    for(std::size_t i=0;i<parts.size();++i)
    {
        Require(HudRadarContains(layout.Panel,parts[i]),"each element inside comparison");
        for(std::size_t j=0;j<i;++j)
            Require(!HudRadarOverlaps(parts[i],parts[j]),"no label or graph overlap");
    }
}

GarageComparisonTextMeasurements Measures(const GarageComparisonPanelLayout& p)
{
    GarageComparisonTextMeasurements m;
    const auto fit=[](const DesktopMenuMouseRect& r)
    {return HudRaceStatusTextSize{r.Width*0.75,r.Height*0.75};};
    m.Title=fit(p.Title);m.Context=fit(p.Context);
    for(std::size_t i=0;i<p.Metrics.size();++i)
    {
        m.Labels[i]=fit(p.Metrics[i].Label);
        m.Deltas[i]=fit(p.Metrics[i].Delta);
    }
    return m;
}

void DesktopLayoutAndFallback()
{
    for(const auto& size:std::array<std::array<double,2>,6>{{
        {900,600},{1120,600},{1280,720},{1366,768},{1920,1080},{3440,1440}}})
    {
        const MenuView v(size[0],size[1]);
        CheckLayout(v,v.Plan(),2);
        CheckLayout(v,v.Plan(1),1);
    }
    const MenuView v;
    Require(!v.Plan(2,false).Valid && !v.Plan(2,true,true).Valid &&
        !MenuView(640,480).Plan().Valid && !MenuView(1280,450).Plan().Valid,
        "unrelated pages and compact layouts retain textual garage");
    auto bad=v;bad.SafeWidth=100;
    Require(!bad.Plan().Valid,"narrow text lane falls back");
    bad=v;bad.Vertical.MaxDetailsRows=12;
    Require(!bad.Plan().Valid,"do not consume the last available detail rows");
    bad=v;bad.Readable.Valid=false;
    Require(!bad.Plan().Valid,"no detached comparison card");
    Require(!CalculateGarageComparisonPanelLayout(v.Hud,v.Vertical,v.Readable,
        v.SafeWidth,v.Hud.Top,2,true,false).Valid,"stale pinned baseline rejected");
}

void MeasuredTextFallback()
{
    const auto p=MenuView().Plan();
    const auto baseline=Measures(p);
    Require(CanDrawGarageComparisonPanel(p,baseline),"actual text sizes fit all lanes");
    auto m=baseline;m.Title.Width=p.Title.Width+1;
    Require(!CanDrawGarageComparisonPanel(p,m),"long title does not overdraw");
    m=baseline;m.Context.Height=p.Context.Height+1;
    Require(!CanDrawGarageComparisonPanel(p,m),"ownership context cannot disappear");
    for(std::size_t i=0;i<p.Metrics.size();++i)
    {
        m=baseline;m.Labels[i].Width=0;
        Require(!CanDrawGarageComparisonPanel(p,m),"unmeasured label refused");
        m=baseline;m.Deltas[i].Width=p.Metrics[i].Delta.Width+1;
        Require(!CanDrawGarageComparisonPanel(p,m),"exact signed number must fit");
        m=baseline;m.Deltas[i].Height=std::numeric_limits<double>::quiet_NaN();
        Require(!CanDrawGarageComparisonPanel(p,m),"nonfinite measure refused");
    }
    Require(!CanDrawGarageComparisonPanel({},baseline),"no geometry no draw");
}

void SignedProgressBars()
{
    const DesktopMenuMouseRect track{30,40,250,5};
    for(double axis:{25.0,50.0,100.0,500.0})
    {
        for(int n=-100;n<=100;++n)
        {
            const double percent=axis*n/100.0;
            const auto bar=BuildGarageComparisonBar(track,percent,axis);
            Require(bar.Valid && Near(bar.ZeroX,155),"common neutral midpoint");
            if(n==0)
            {
                Require(bar.Direction==GarageComparisonDirection::Equal &&
                    bar.Fill.Width==0,"zero difference has no false fill");
            }
            else
            {
                Require(HudRadarContains(track,bar.Fill) &&
                    Near(bar.Fill.Width,125.0*std::abs(n)/100.0),"proportional signed fill");
                if(n<0)
                    Require(bar.Direction==GarageComparisonDirection::Lower &&
                        Near(bar.Fill.X+bar.Fill.Width,bar.ZeroX),"negative points left");
                else
                    Require(bar.Direction==GarageComparisonDirection::Higher &&
                        Near(bar.Fill.X,bar.ZeroX),"positive points right");
            }
        }
    }
    Require(BuildGarageComparisonBar(track,-0.01,25).Direction==
        GarageComparisonDirection::Equal,"no rounded negative zero");
    Require(!BuildGarageComparisonBar(track,26,25).Valid &&
        !BuildGarageComparisonBar(track,1,0).Valid &&
        !BuildGarageComparisonBar(track,1,501).Valid &&
        !BuildGarageComparisonBar(track,std::numeric_limits<double>::infinity(),25).Valid,
        "bad scale and out of range differences refused");
}

void RandomViewportAndGarageState()
{
    std::mt19937 rng(10593);
    int visible=0;
    for(int i=0;i<10000;++i)
    {
        const MenuView view(240+rng()%3900,240+rng()%2000);
        const auto layout=view.Plan();
        const int selected=static_cast<int>(rng()%VehicleCount);
        const int viewed=static_cast<int>(rng()%VehicleCount);
        const int mask=static_cast<int>(rng()%KnownVehicleMask)|(1<<selected)|1;
        const auto data=ReadGarageComparisonPanel(Catalog(selected,mask,
            static_cast<int>(rng()%10000),static_cast<int>(rng()%4),
            static_cast<int>(rng()%4)),viewed);
        Require(data.Valid,"sanitized legal random catalogue");
        if(!layout.Valid)continue;
        ++visible;
        CheckLayout(view,layout,2);
        Require(CanDrawGarageComparisonPanel(layout,Measures(layout)),"complete measures fit");
        for(std::size_t m=0;m<data.Metrics.size();++m)
            Require(BuildGarageComparisonBar(layout.Metrics[m].Track,
                data.Metrics[m].DeltaPercent,data.AxisPercent).Valid,
                "all metrics rendered within same scale");
    }
    Require(visible>4000,"responsive garage has useful broad desktop coverage");
}
} // namespace

int main()
{
    ConfiguredCarDifferences();
    AllCarsAndSharedUpgrades();
    RejectMixedCatalogue();
    DesktopLayoutAndFallback();
    MeasuredTextFallback();
    SignedProgressBars();
    RandomViewportAndGarageState();
    std::cout<<"Garage comparison configuration, layout and font tests passed\n";
}
