#include "UI/GarageAppearancePalette.h"
#include "UI/GaragePointerArbitration.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
using namespace latebrake;
void Require(bool pass,const char* label)
{
    if(!pass)
    {
        std::cerr<<label<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

void AllChoices()
{
    constexpr std::array layers{GarageLookLayer::Paint,GarageLookLayer::Stripe,GarageLookLayer::Wheels};
    for(int id=0;id<VehicleCount;++id)
        for(int paint=0;paint<GaragePaintOptions;++paint)
            for(int stripe=0;stripe<GarageStripeOptions;++stripe)
                for(int rims=0;rims<GarageWheelOptions;++rims)
                {
                    const GarageLook before{static_cast<std::uint8_t>(paint),
                        static_cast<std::uint8_t>(stripe),static_cast<std::uint8_t>(rims)};
                    for(auto layer:layers)
                        for(int c=0;c<GarageLookChoiceCount(layer);++c)
                        {
                            const GarageDirectAppearanceRequest request{id,before,layer,c};
                            const auto d=PreviewGarageDirectAppearance(id,before,request);
                            const bool same=GarageLookChoice(before,layer)==c;
                            Require(d.Status==(same?GarageDirectAppearanceStatus::Unchanged:
                                GarageDirectAppearanceStatus::Ready),"exact direct choice");
                            Require(ValidGarageLook(d.Result)&&GarageLookChoice(d.Result,layer)==c,
                                "result is a legal selected option");
                            for(auto other:layers)
                                if(other!=layer)
                                    Require(GarageLookChoice(before,other)==GarageLookChoice(d.Result,other),
                                        "all unrelated appearance fields remain unchanged");
                            const auto descriptor=DescribeGaragePaletteChoice(id,before,layer,c);
                            Require(descriptor.Index==c&&descriptor.Selected==same,"selected badge matches");
                            Require(descriptor.Factory==(GarageLookChoice(FactoryGarageLook(id),layer)==c),
                                "factory badge matches immutable factory choice");
                            Require(std::string_view(descriptor.Label)==GarageLookChoiceName(layer,c),
                                "labels are canonical");
                            Require(std::string_view(descriptor.ShortLabel).size()>0,"short label available");
                            const auto rgb=PaintForGarageLook(id,d.Result).Body;
                            Require(descriptor.Swatch.R==rgb.R&&descriptor.Swatch.G==rgb.G&&descriptor.Swatch.B==rgb.B,
                                "swatch uses actual vehicle color recipe");
                            Require(PreviewGarageDirectAppearance((id+1)%VehicleCount,before,request).Status
                                ==GarageDirectAppearanceStatus::Stale,"other car cannot be edited");
                            auto newLook=before;
                            newLook.Paint=static_cast<std::uint8_t>((paint+1)%GaragePaintOptions);
                            Require(PreviewGarageDirectAppearance(id,newLook,request).Status
                                ==GarageDirectAppearanceStatus::Stale,"changed look rejects stale click");
                            // The existing appearance state performs exactly the same choice.
                            GarageAppearanceState state;
                            state.Choose(id,GarageLookLayer::Paint,paint);
                            state.Choose(id,GarageLookLayer::Stripe,stripe);
                            state.Choose(id,GarageLookLayer::Wheels,rims);
                            const auto snapshot=state.Serialize();
                            if(!same) Require(state.Choose(id,layer,c),"existing state accepts validated option");
                            Require(SameGarageLook(state.Get(id),d.Result),"direct policy agrees with state");
                            GarageAppearanceState restored;
                            const auto packed=state.Serialize();
                            Require(restored.Load(packed.data(),packed.size()),"appearance persists roundtrip");
                            for(int other=0;other<VehicleCount;++other)
                                if(other!=id)
                                    Require(SameGarageLook(restored.Get(other),FactoryGarageLook(other)),
                                        "other vehicles remain unchanged");
                            Require(state.Load(snapshot.data(),snapshot.size())&&SameGarageLook(state.Get(id),before),
                                "rollback restores prior look");
                        }
                }
}

void InvalidRequests()
{
    const auto look=FactoryGarageLook(0);
    for(int bad:{-1,VehicleCount,std::numeric_limits<int>::min(),std::numeric_limits<int>::max()})
    {
        Require(PreviewGarageDirectAppearance(0,look,{bad,look,GarageLookLayer::Paint,1}).Status
            ==GarageDirectAppearanceStatus::Invalid,"invalid vehicle");
        Require(PreviewGarageDirectAppearance(0,look,{0,look,GarageLookLayer::Paint,bad}).Status
            !=GarageDirectAppearanceStatus::Ready || (bad>=0&&bad<GaragePaintOptions),
            "invalid choice cannot be applied");
    }
    Require(PreviewGarageDirectAppearance(0,look,{0,look,static_cast<GarageLookLayer>(255),0}).Status
        ==GarageDirectAppearanceStatus::Invalid,"unknown layer");
    Require(PreviewGarageDirectAppearance(0,{255,0,0},{0,look,GarageLookLayer::Paint,1}).Status
        ==GarageDirectAppearanceStatus::Invalid,"corrupt current look");
    Require(PreviewGarageDirectAppearance(0,look,{0,{0,255,0},GarageLookLayer::Paint,1}).Status
        ==GarageDirectAppearanceStatus::Invalid,"corrupt expected look");
    for(int bad:{std::numeric_limits<int>::min(),-1,8,std::numeric_limits<int>::max()})
        Require(GaragePaletteChoiceAction(bad)==DesktopMenuMouseAction::None,"invalid action index");
    Require(GaragePaletteChoiceIndex(static_cast<DesktopMenuMouseAction>(std::numeric_limits<int>::min()))==-1,
        "malformed action safe");
}

void FullPresentationAndModalGuards()
{
    using Action=DesktopMenuMouseAction;
    const auto hud=CalculateHudLayout(1920,1080);
    const auto lane=CalculateGarageShowroomLane(1920,1080,hud,false,true,MenuPage::Garage);
    for(auto layer:{GarageLookLayer::Paint,GarageLookLayer::Stripe,GarageLookLayer::Wheels})
    {
        const auto layout=CalculateGaragePaletteLayout(lane,1920,1080,hud,layer);
        const GaragePaletteSession session{true,0,FactoryGarageLook(0),layer};
        GaragePresentedPalette shown;
        shown.Session=session;
        shown.Layout=layout;
        shown.CloseVisible=true;
        shown.ChoiceVisible.fill(true);
        Require(ValidGaragePaletteLayout(layout),"baseline layout valid");
        const auto choice=layout.Choices[0].Action;
        const auto checkInvalid=[&](const GaragePaletteLayout& bad)
        {
            Require(!ValidGaragePaletteLayout(bad),"invalid surface detected");
            auto forged=shown;
            forged.Layout=bad;
            Require(!CanClickPresentedGaragePalette(forged,session,bad,choice),
                "identical malformed receipts cannot authorize choice");
            Require(!CanClickPresentedGaragePalette(forged,session,bad,Action::GaragePaletteClose),
                "malformed surface cannot authorize another visible button");
            Require(GaragePaletteBlocksUnderlyingPointer(true,bad,0,0,Action::None),
                "malformed open surface blocks underlying dispatch");
            Require(!GaragePaletteBlocksUnderlyingPointer(false,bad,0,0,Action::None),
                "inactive modal cannot intercept normal menu");
        };
        auto bad=layout;
        bad.Panel.Width=std::numeric_limits<double>::infinity();
        checkInvalid(bad);
        bad=layout;bad.Panel.X=-1.0;checkInvalid(bad);
        bad=layout;bad.Panel.Height=0.0;checkInvalid(bad);
        bad=layout;bad.Close.Action=Action::Activate;checkInvalid(bad);
        bad=layout;bad.Choices[1].Bounds=bad.Choices[0].Bounds;checkInvalid(bad);
        bad=layout;bad.Choices[1].Bounds.Width=0.0;checkInvalid(bad);
        bad=layout;bad.Choices[1].Action=Action::Close;checkInvalid(bad);
        bad=layout;bad.Count=std::numeric_limits<int>::max();checkInvalid(bad);
        bad=layout;bad.Choices[0].Bounds.X=std::numeric_limits<double>::quiet_NaN();
        checkInvalid(bad);

        // A different valid geometry elsewhere in the chooser invalidates
        // even a choice whose own rectangle still matches the last draw.
        auto moved=layout;
        moved.Panel.Height+=1.0;
        Require(ValidGaragePaletteLayout(moved),"expanded panel remains valid");
        Require(!CanClickPresentedGaragePalette(shown,session,moved,choice),
            "changed panel revokes untouched button");
        const double blankX=layout.Panel.X+1.0,blankY=layout.Panel.Y+1.0;
        Require(HitTestGaragePalette(layout,blankX,blankY)==Action::None,
            "panel padding not a choice");
        Require(GaragePaletteBlocksUnderlyingPointer(true,layout,blankX,blankY,Action::None),
            "panel padding cannot click behind overlay");
        Require(!GaragePaletteBlocksUnderlyingPointer(true,layout,-1,-1,Action::None),
            "outside panel remains normal navigation");
        const auto& bounds=layout.Choices[0].Bounds;
        const double x=bounds.X+bounds.Width/2.0,y=bounds.Y+bounds.Height/2.0;
        Require(!GaragePaletteBlocksUnderlyingPointer(true,layout,x,y,
            HitTestGaragePalette(layout,x,y)),"real choice goes to claim arbitration");
    }
}

void LayoutsAndPresentation()
{
    for(const auto& size:{std::array<int,2>{900,600},{1280,720},{1366,768},
        {1920,1080},{2560,1080},{3840,2160}})
    {
        const double w=size[0],h=size[1];
        const auto hud=CalculateHudLayout(w,h);
        const auto lane=CalculateGarageShowroomLane(w,h,hud,false,true,MenuPage::Garage);
        for(auto layer:{GarageLookLayer::Paint,GarageLookLayer::Stripe,GarageLookLayer::Wheels})
        {
            const auto layout=CalculateGaragePaletteLayout(lane,w,h,hud,layer);
            Require(layout.Valid,"desktop chooser fits");
            Require(layout.Count==GarageLookChoiceCount(layer),"exact number of valid choices");
            const auto& panel=layout.Panel;
            const auto& a=lane.Aperture;
            Require(panel.X>=a.X&&panel.Y>=a.Y&&panel.X+panel.Width<=a.X+a.Width&&
                panel.Y+panel.Height<=a.Y+a.Height,"chooser inside showroom only");
            const GaragePaletteSession session{true,2,FactoryGarageLook(2),layer};
            GaragePresentedPalette shown;
            shown.Session=session;
            shown.Layout=layout;
            shown.CloseVisible=true;
            const auto& close=layout.Close.Bounds;
            Require(HitTestGaragePalette(layout,close.X+1,close.Y+1)==layout.Close.Action,
                "close target exact");
            Require(CanClickPresentedGaragePalette(shown,session,layout,layout.Close.Action),
                "visible close available");
            for(int i=0;i<layout.Count;++i)
            {
                const auto index=static_cast<std::size_t>(i);
                const auto& b=layout.Choices[index];
                const auto& r=b.Bounds;
                Require(r.Width>=44&&r.Height>=44,"minimum target dimensions");
                Require(HitTestGaragePalette(layout,r.X+r.Width/2,r.Y+r.Height/2)==b.Action,
                    "exact physical action");
                Require(!CanClickPresentedGaragePalette(shown,session,layout,b.Action),
                    "unpainted choice has no click authority");
                shown.ChoiceVisible[index]=true;
                Require(CanClickPresentedGaragePalette(shown,session,layout,b.Action),
                    "visible choice has click authority");
                auto changed=session;
                changed.VehicleId=3;
                Require(!CanClickPresentedGaragePalette(shown,changed,layout,b.Action),
                    "changed vehicle rejects old palette");
                changed=session;
                changed.Look.Paint=7;
                Require(!CanClickPresentedGaragePalette(shown,changed,layout,b.Action),
                    "changed appearance invalidates palette");
                changed=session;
                changed.Open=false;
                Require(!CanClickPresentedGaragePalette(shown,changed,layout,b.Action),
                    "closed chooser has no click authority");
                auto moved=layout;
                moved.Choices[index].Bounds.X+=1;
                Require(!CanClickPresentedGaragePalette(shown,session,moved,b.Action),
                    "resized chooser rejects stale rectangle");
                auto corrupted=layout;
                corrupted.Choices[index].Action=DesktopMenuMouseAction::Activate;
                Require(HitTestGaragePalette(corrupted,r.X+1,r.Y+1)==DesktopMenuMouseAction::None,
                    "corrupt action cannot become equip");
                GaragePointerClaims claims{};
                claims[static_cast<std::size_t>(GaragePointerGroup::Palette)]={b.Action,true};
                Require(ResolveGaragePointerClaims(claims)==b.Action,"palette action resolves");
                claims[0]={DesktopMenuMouseAction::Close,false};
                Require(ResolveGaragePointerClaims(claims)==DesktopMenuMouseAction::None,
                    "disabled overlap cannot leak through");
            }
            auto overlap=layout;
            overlap.Choices[1].Bounds=overlap.Choices[0].Bounds;
            const auto& b=overlap.Choices[0].Bounds;
            Require(HitTestGaragePalette(overlap,b.X+1,b.Y+1)==DesktopMenuMouseAction::None,
                "overlap rejected");
            Require(!CalculateGaragePaletteLayout({},w,h,hud,layer).Valid,"no lane no chooser");
            Require(!session.Matches(2,session.Look,false),"read-only context closes chooser");
        }
    }
}
}
int main()
{
    AllChoices();
    InvalidRequests();
    LayoutsAndPresentation();
    FullPresentationAndModalGuards();
    std::cout<<"Garage direct appearance palette tests passed.\n";
    return EXIT_SUCCESS;
}
