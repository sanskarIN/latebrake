#include "UI/HudRaceStatusPanel.h"

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
        std::cerr<<"HudRaceStatusPanel: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a,double b,double tolerance=1e-7)
{
    return std::abs(a-b)<=tolerance;
}

struct View final
{
    double Width=1280;
    double Height=720;
    HudLayout Hud{};
    HudVerticalPlan Vertical{};
    HudTextBounds Text{};
    HudInstrumentCluster Instruments{};
    HudRaceRadarLayout Radar{};
    double SafeWidth=0.0;

    View(double w=1280,double h=720,bool touch=false):Width(w),Height(h)
    {
        Hud=CalculateHudLayout(w,h);
        Vertical=CalculateHudVerticalPlan(w,h,Hud,touch);
        Text=CalculateHudTextBounds(w,h,Hud,touch);
        Instruments=CalculateHudInstrumentCluster(w,h,Hud,touch,false,false,
            120,0.7,0.2,0.1);
        Radar=CalculateHudRaceRadarLayout(w,h,Hud,Text,Instruments,touch,false,false);
        SafeWidth=Text.MaxWidthPixels;
        if(Instruments.Visible)
            SafeWidth=std::min(SafeWidth,Instruments.Panel.X-Text.Left-16*Instruments.Scale);
        if(Radar.Valid)
            SafeWidth=std::min(SafeWidth,Radar.Panel.X-Text.Left-12*Hud.Scale);
    }

    HudRaceStatusLayout Plan(bool touch=false,bool menu=false,
        bool finished=false,bool running=true) const
    {
        return CalculateHudRaceStatusLayout(Width,Height,Hud,Vertical,Text,
            SafeWidth,touch,menu,finished,running);
    }
};

HudRaceStatusSource Sample()
{
    return {1,3,2,4,76.259,137.7,false,false,true,true};
}

void ReadAuthoritativeValues()
{
    const auto data=ReadHudRaceStatus(Sample());
    Require(data.Valid && data.CurrentLap==2 && data.RequiredLaps==3 &&
        data.NextGate==3 && data.GateCount==4,"exact current lap and next checkpoint");
    Require(data.Clock.Minutes==1 && data.Clock.Seconds==16 &&
        data.Clock.Hundredths==26,"whole clock split");
    Require(data.DisplaySpeed==138 && !data.MilesPerHour,"metric rounding");
    Require(data.Progress.Valid && data.Progress.CreditedGates==6 &&
        data.Progress.TotalGates==12 && Near(data.Progress.Fraction,0.5),
        "only actual accepted gates count");
    auto input=Sample();
    input.MilesPerHour=true;
    const auto mph=ReadHudRaceStatus(input);
    Require(mph.Valid && mph.MilesPerHour && mph.DisplaySpeed==86,
        "same measured speed converted to selected units");
    input=Sample();
    input.CompletedLaps=0;input.NextCheckpoint=0;
    const auto start=ReadHudRaceStatus(input);
    Require(start.Valid && start.CurrentLap==1 && start.NextGate==1 &&
        start.Progress.CreditedGates==0 && start.Progress.Fraction==0,
        "first lap without invented checkpoint");
    input.CompletedLaps=2;input.NextCheckpoint=3;
    const auto last=ReadHudRaceStatus(input);
    Require(last.Valid && last.CurrentLap==3 && last.NextGate==4 &&
        last.Progress.CreditedGates==11 && last.Progress.Fraction<1,
        "last pending checkpoint is not already credited");
}

void ClockBoundaries()
{
    for(int minute:{0,1,59,60,999,59998})
    {
        const double base=minute*60.0;
        const auto at=SplitHudRaceStatusClock(base);
        Require(at.Minutes==minute && at.Seconds==0 && at.Hundredths==0,
            "exact minute");
        const auto almost=SplitHudRaceStatusClock(base+59.994);
        Require(almost.Minutes==minute && almost.Seconds==59 &&
            almost.Hundredths==99,"last displayable hundredth");
        if(base+59.999<=HudRaceStatusMaximumElapsed)
        {
            const auto carry=SplitHudRaceStatusClock(base+59.999);
            Require(carry.Minutes==minute+1 && carry.Seconds==0 &&
                carry.Hundredths==0,"no sixty-second display");
        }
    }
    const auto tick=SplitHudRaceStatusClock(0.009);
    Require(tick.Hundredths==1,"nearest hundredth");
    const auto invalid=SplitHudRaceStatusClock(-1);
    Require(invalid.Minutes==0 && invalid.Seconds==0 && invalid.Hundredths==0,
        "invalid helper clock is empty");
}

void RejectUnavailableOrContradictoryData()
{
    const double nan=std::numeric_limits<double>::quiet_NaN();
    for(int fault=0;fault<19;++fault)
    {
        auto input=Sample();
        switch(fault)
        {
        case 0:input.Running=false;break;
        case 1:input.Finished=true;break;
        case 2:input.SpeedObserved=false;break;
        case 3:input.ElapsedSeconds=nan;break;
        case 4:input.ElapsedSeconds=-1;break;
        case 5:input.ElapsedSeconds=HudRaceStatusMaximumElapsed+1;break;
        case 6:input.SpeedKmh=nan;break;
        case 7:input.SpeedKmh=-1;break;
        case 8:input.SpeedKmh=HudRaceStatusMaximumSpeedKmh+1;break;
        case 9:input.CompletedLaps=-1;break;
        case 10:input.CompletedLaps=3;break;
        case 11:input.RequiredLaps=0;break;
        case 12:input.NextCheckpoint=-1;break;
        case 13:input.NextCheckpoint=4;break;
        case 14:input.CheckpointCount=0;break;
        case 15:input.CheckpointCount=257;break;
        case 16:input.RequiredLaps=std::numeric_limits<int>::max();break;
        case 17:input.SpeedKmh=std::numeric_limits<double>::infinity();break;
        case 18:input.ElapsedSeconds=std::numeric_limits<double>::infinity();break;
        }
        Require(!ReadHudRaceStatus(input).Valid,"bad or unavailable source falls back");
    }
}

void CheckLayout(const View& v,const HudRaceStatusLayout& panel)
{
    Require(panel.Valid && HudRadarContains({0,0,v.Width,v.Height},panel.Panel),
        "viewport containment");
    Require(panel.Panel.X==v.Text.Left && panel.Panel.Y==v.Hud.Top &&
        panel.Panel.Width<=v.SafeWidth+1e-7,"uses protected horizontal lane");
    Require(panel.ReservedRows+panel.RemainingRows==v.Vertical.MaxTelemetryLines &&
        panel.RemainingRows>=HudRaceStatusMinimumRemainingRows,
        "integral row reservation retains telemetry budget");
    Require(Near(panel.TelemetryTop,v.Hud.Top+panel.ReservedRows*v.Hud.LineHeight) &&
        panel.TelemetryTop>=panel.Panel.Y+panel.Panel.Height,
        "telemetry starts below card");
    Require(panel.TelemetryTop+(panel.RemainingRows-1)*v.Hud.LineHeight<=
        v.Vertical.SaveWarningY-1.25*v.Hud.LineHeight+1e-7,
        "save and pause baselines unaffected");
    if(v.Instruments.Visible)
        Require(!HudRadarOverlaps(panel.Panel,{v.Instruments.Panel.X,v.Instruments.Panel.Y,
            v.Instruments.Panel.Width,v.Instruments.Panel.Height}),"instrument separation");
    if(v.Radar.Valid)
        Require(!HudRadarOverlaps(panel.Panel,v.Radar.Panel),"radar separation");
    for(std::size_t i=0;i<panel.Tiles.size();++i)
    {
        const auto& tile=panel.Tiles[i];
        Require(HudRadarContains(panel.Panel,tile.Background) &&
            HudRadarContains(tile.Background,tile.Label) &&
            HudRadarContains(tile.Background,tile.Value) &&
            !HudRadarOverlaps(tile.Label,tile.Value),"independent measured text lanes");
        for(std::size_t j=0;j<i;++j)
            Require(!HudRadarOverlaps(tile.Background,panel.Tiles[j].Background),
                "tile backgrounds never overlap");
        Require(!HudRadarOverlaps(tile.Background,panel.ProgressLabel) &&
            !HudRadarOverlaps(tile.Background,panel.ProgressTrack),"progress separate");
    }
}

void ResponsiveDesktopAndFallback()
{
    for(const auto& size:std::array<std::array<double,2>,7>{{
        {900,600},{1120,600},{1280,720},{1366,768},{1920,1080},{2560,1440},{3440,1440}}})
    {
        const View v(size[0],size[1]);
        CheckLayout(v,v.Plan());
    }
    const View v;
    Require(!v.Plan(true).Valid && !v.Plan(false,true).Valid &&
        !v.Plan(false,false,true).Valid && !v.Plan(false,false,false,false).Valid,
        "touch, menu, finish and blocked states keep legacy display");
    Require(!View(640,480).Plan().Valid && !View(1280,480).Plan().Valid &&
        !View(0,0).Plan().Valid,"compact and resizing fallback");
    auto narrow=v;narrow.SafeWidth=200;
    Require(!narrow.Plan().Valid,"protected lane too narrow");
    auto shortView=v;shortView.Vertical.MaxTelemetryLines=8;
    Require(!shortView.Plan().Valid,"preserve remaining urgent rows");
    auto bad=v;bad.Text.Right=std::numeric_limits<double>::quiet_NaN();
    Require(!bad.Plan().Valid,"invalid text bounds");
    bad=v;bad.Hud.LineHeight=0;
    Require(!bad.Plan().Valid,"invalid line height");
}

HudRaceStatusMeasurements FitMeasures(const HudRaceStatusLayout& panel)
{
    HudRaceStatusMeasurements m;
    for(std::size_t i=0;i<panel.Tiles.size();++i)
    {
        m.Labels[i]={panel.Tiles[i].Label.Width*0.8,panel.Tiles[i].Label.Height*0.8};
        m.Values[i]={panel.Tiles[i].Value.Width*0.8,panel.Tiles[i].Value.Height*0.8};
    }
    m.Progress={panel.ProgressLabel.Width*0.8,panel.ProgressLabel.Height*0.8};
    return m;
}

void TransactionalFontAdmission()
{
    const auto panel=View().Plan();
    const auto good=FitMeasures(panel);
    Require(CanDrawHudRaceStatus(panel,good),"all required fields fit");
    for(std::size_t i=0;i<panel.Tiles.size();++i)
    {
        auto m=good;m.Labels[i].Width=panel.Tiles[i].Label.Width+1;
        Require(!CanDrawHudRaceStatus(panel,m),"no partial label suppression");
        m=good;m.Values[i].Height=panel.Tiles[i].Value.Height+1;
        Require(!CanDrawHudRaceStatus(panel,m),"no clipped essential number");
        m=good;m.Values[i].Width=std::numeric_limits<double>::quiet_NaN();
        Require(!CanDrawHudRaceStatus(panel,m),"nonfinite font size");
        m=good;m.Values[i].Height=0;
        Require(!CanDrawHudRaceStatus(panel,m),"empty font measurement");
    }
    auto m=good;m.Progress.Width=panel.ProgressLabel.Width+1;
    Require(!CanDrawHudRaceStatus(panel,m),"progress text also mandatory");
    m=good;
    Require(!CanDrawHudRaceStatus({},m),"invisible geometry cannot suppress legacy rows");
}

void GateProgressEveryBoundary()
{
    const auto panel=View().Plan();
    for(int total=1;total<=48;++total)
    {
        for(int credited=0;credited<=total;++credited)
        {
            const auto fraction=static_cast<double>(credited)/total;
            const auto bar=BuildHudRaceStatusProgress(panel,{true,credited,total,fraction});
            Require(bar.Valid && bar.Segmented==(total<=HudRaceStatusMaximumSegments) &&
                bar.Count==(bar.Segmented?total:1),"bounded segment mode");
            int filled=0;
            double trackWidth=0,fillWidth=0;
            for(int i=0;i<bar.Count;++i)
            {
                const auto& segment=bar.Segments[static_cast<std::size_t>(i)];
                Require(HudRadarContains(panel.ProgressTrack,segment.Track),"inside progress lane");
                if(segment.Fill.Width>0)
                    Require(HudRadarContains(segment.Track,segment.Fill),"fill inside segment");
                if(segment.Credited)++filled;
                trackWidth+=segment.Track.Width;
                fillWidth+=segment.Fill.Width;
                if(i)
                    Require(!HudRadarOverlaps(segment.Track,
                        bar.Segments[static_cast<std::size_t>(i-1)].Track),"segments separate");
            }
            Require(Near(fillWidth/trackWidth,fraction),"exact fraction no interpolation");
            if(bar.Segmented)Require(filled==credited,"each accepted checkpoint earns one segment");
        }
    }
    Require(!BuildHudRaceStatusProgress(panel,{true,2,4,0.75}).Valid,
        "inconsistent fraction rejected");
    Require(!BuildHudRaceStatusProgress(panel,{true,5,4,1}).Valid &&
        !BuildHudRaceStatusProgress(panel,{true,-1,4,0}).Valid &&
        !BuildHudRaceStatusProgress(panel,{true,0,0,0}).Valid &&
        !BuildHudRaceStatusProgress(panel,{false,0,4,0}).Valid,
        "malformed checkpoint counts rejected");
}

void RandomViewportsAndClocks()
{
    std::mt19937 random(92171);
    int visible=0;
    for(int i=0;i<12000;++i)
    {
        const View v(240+random()%3900,240+random()%2000);
        const auto panel=v.Plan();
        if(panel.Valid)
        {
            ++visible;
            CheckLayout(v,panel);
            Require(CanDrawHudRaceStatus(panel,FitMeasures(panel)),"font budget bounded");
        }
        auto sample=Sample();
        sample.RequiredLaps=1+static_cast<int>(random()%100);
        sample.CompletedLaps=static_cast<int>(random()%static_cast<unsigned>(sample.RequiredLaps));
        sample.NextCheckpoint=static_cast<int>(random()%4);
        sample.ElapsedSeconds=static_cast<double>(random()%100000000)/100.0;
        const auto read=ReadHudRaceStatus(sample);
        Require(read.Valid && read.Clock.Seconds>=0 && read.Clock.Seconds<60 &&
            read.Clock.Hundredths>=0 && read.Clock.Hundredths<100,
            "display clock normalization for long events");
        const auto reconstructed=60.0*read.Clock.Minutes+read.Clock.Seconds+
            read.Clock.Hundredths/100.0;
        Require(Near(reconstructed,sample.ElapsedSeconds),"clock precision conservation");
        if(panel.Valid)
            Require(BuildHudRaceStatusProgress(panel,read.Progress).Valid,"random authoritative progress");
    }
    Require(visible>5000,"desktop layout useful across broad viewport range");
}
} // namespace

int main()
{
    ReadAuthoritativeValues();
    ClockBoundaries();
    RejectUnavailableOrContradictoryData();
    ResponsiveDesktopAndFallback();
    TransactionalFontAdmission();
    GateProgressEveryBoundary();
    RandomViewportsAndClocks();
    std::cout<<"Race status layout, clock, measured text and gate progress tests passed\n";
}
