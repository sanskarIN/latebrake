#include "UI/GaragePaletteNavigation.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
using Input=GaragePaletteNavigationInput;
using Effect=GaragePaletteNavigationEffect;

void Require(bool ok,const char* label)
{
    if(!ok)
    {
        std::cerr<<label<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

GaragePaletteLayout LayoutFor(GarageLookLayer layer)
{
    const auto hud=CalculateHudLayout(1920,1080);
    const auto lane=CalculateGarageShowroomLane(1920,1080,hud,false,true,MenuPage::Garage);
    return CalculateGaragePaletteLayout(lane,1920,1080,hud,layer);
}

void ExhaustVisibleMasks()
{
    for(auto layer:{GarageLookLayer::Paint,GarageLookLayer::Stripe,GarageLookLayer::Wheels})
    {
        const auto layout=LayoutFor(layer);
        Require(layout.Valid,"desktop navigation layout available");
        const GaragePaletteSession session{true,1,{1,1,1},layer};
        GaragePresentedPalette shown;
        shown.Session=session;
        shown.Layout=layout;
        const int count=layout.Count;
        for(unsigned mask=0;mask<(1u<<count);++mask)
        {
            for(int i=0;i<count;++i)
                shown.ChoiceVisible[static_cast<std::size_t>(i)]=(mask&(1u<<i))!=0;
            for(int focused=-1;focused<=count;++focused)
            {
                for(auto input:{Input::Previous,Input::Next})
                {
                    const int step=input==Input::Next?1:-1;
                    int expected=-1;
                    const int origin=focused>=0&&focused<count?focused:(step>0?count-1:0);
                    for(int i=1;i<=count;++i)
                    {
                        const int k=(origin+step*i+count)%count;
                        if((mask&(1u<<k))!=0){expected=k;break;}
                    }
                    const auto moved=NavigateGaragePalette(session,layout,shown,focused,-1,input);
                    Require(moved.Effect==(expected<0?Effect::Ignore:Effect::Move)&&
                        moved.Choice==expected,"movement visits only visible choices with wraparound");
                }
                for(int drawn=-1;drawn<=count;++drawn)
                {
                    const bool visible=focused>=0&&focused<count&&(mask&(1u<<focused))!=0;
                    const bool confirm=visible&&focused==drawn;
                    const auto d=NavigateGaragePalette(session,layout,shown,focused,drawn,Input::Confirm);
                    Require(d.Effect==(confirm?Effect::Confirm:Effect::Ignore)&&
                        d.Choice==(confirm?focused:-1),"confirmation requires last actually drawn focus");
                }
            }
        }
    }
}

void OpeningAndLayerIdentity()
{
    for(int id=0;id<VehicleCount;++id)
        for(int p=0;p<GaragePaintOptions;++p)
            for(int s=0;s<GarageStripeOptions;++s)
                for(int w=0;w<GarageWheelOptions;++w)
                {
                    const GarageLook look{static_cast<std::uint8_t>(p),
                        static_cast<std::uint8_t>(s),static_cast<std::uint8_t>(w)};
                    for(auto layer:{GarageLookLayer::Paint,GarageLookLayer::Stripe,GarageLookLayer::Wheels})
                    {
                        const GaragePaletteSession session{true,id,look,layer};
                        Require(InitialGaragePaletteFocus(session)==GarageLookChoice(look,layer),
                            "opening focuses currently applied choice");
                        auto closed=session;
                        closed.Open=false;
                        Require(InitialGaragePaletteFocus(closed)==-1,"closed panel owns no focus");
                        auto invalid=session;
                        invalid.VehicleId=-1;
                        Require(InitialGaragePaletteFocus(invalid)==-1,"invalid car owns no focus");
                        const auto next=NextGaragePaletteLayer(layer);
                        Require(next!=layer&&NextGaragePaletteLayer(NextGaragePaletteLayer(next))==layer,
                            "paint stripe rims cycle exactly once");
                    }
                }
}

void StaleAndInvalidPresentation()
{
    const GaragePaletteSession session{true,0,{0,0,0},GarageLookLayer::Paint};
    const auto layout=LayoutFor(session.Layer);
    GaragePresentedPalette shown;
    shown.Session=session;
    shown.Layout=layout;
    shown.ChoiceVisible.fill(true);
    const auto expectIgnore=[&](GaragePaletteSession s,GaragePaletteLayout l,
        GaragePresentedPalette p,const char* label)
    {
        for(auto input:{Input::Next,Input::Previous,Input::Confirm})
            Require(NavigateGaragePalette(s,l,p,0,0,input).Effect==Effect::Ignore,label);
    };
    expectIgnore(session,layout,{},"before first draw no selection");
    auto stale=shown;
    stale.Session.VehicleId=1;
    expectIgnore(session,layout,stale,"different displayed vehicle");
    stale=shown;
    stale.Session.Look.Paint=2;
    expectIgnore(session,layout,stale,"different displayed appearance");
    stale=shown;
    stale.Session.Layer=GarageLookLayer::Stripe;
    expectIgnore(session,layout,stale,"different displayed layer");
    auto moved=layout;
    moved.Choices[7].Bounds.X+=1;
    expectIgnore(session,moved,shown,"even unrelated button changed after resize");
    moved=layout;
    moved.Panel.Y+=1;
    expectIgnore(session,moved,shown,"panel moved");
    moved=layout;
    moved.Close.Bounds.Height+=1;
    expectIgnore(session,moved,shown,"close geometry changed");
    moved=layout;
    moved.Count=std::numeric_limits<int>::max();
    expectIgnore(session,moved,shown,"invalid count");
    auto closed=session;closed.Open=false;
    expectIgnore(closed,layout,shown,"closed palette never commits");
    Require(NavigateGaragePalette(session,{}, {},-1,-1,Input::Cancel).Effect==Effect::Cancel,
        "cancel works even before drawing");
    Require(NavigateGaragePalette(closed,{}, {},-1,-1,Input::Cancel).Effect==Effect::Ignore,
        "closed palette does not consume cancel");
    for(int bad:{std::numeric_limits<int>::min(),std::numeric_limits<int>::max()})
    {
        Require(NavigateGaragePalette(session,layout,shown,bad,bad,Input::Confirm).Effect==Effect::Ignore,
            "extreme index never selects");
        Require(NavigateGaragePalette(session,layout,shown,bad,-1,Input::Next).Choice==0,
            "extreme index safely restores first focus");
    }
    Require(NavigateGaragePalette(session,layout,shown,0,0,static_cast<Input>(255)).Effect==Effect::Ignore,
        "unknown input ignored");
}
}
int main()
{
    ExhaustVisibleMasks();
    OpeningAndLayerIdentity();
    StaleAndInvalidPresentation();
    std::cout<<"Garage palette focus navigation passed.\n";
    return EXIT_SUCCESS;
}
