#include "UI/GaragePaletteNavigation.h"
#include "UI/GarageAppearanceGlyph.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok)
    {
        std::cerr<<"GarageLookPresets: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
constexpr auto Layer=GarageLookLayer::Preset;

void ExhaustEveryAppearanceAndPreset()
{
    for(int id=0;id<VehicleCount;++id)
        for(int p=0;p<GaragePaintOptions;++p)
            for(int s=0;s<GarageStripeOptions;++s)
                for(int w=0;w<GarageWheelOptions;++w)
                {
                    const GarageLook before{static_cast<std::uint8_t>(p),
                        static_cast<std::uint8_t>(s),static_cast<std::uint8_t>(w)};
                    GarageAppearanceState initial;
                    Require(initial.Choose(id,GarageLookLayer::Paint,p) &&
                        initial.Choose(id,GarageLookLayer::Stripe,s) &&
                        initial.Choose(id,GarageLookLayer::Wheels,w),"canonical starting look");
                    for(int i=0;i<GaragePresetOptions;++i)
                    {
                        const auto& preset=GarageLookPresets[static_cast<std::size_t>(i)];
                        const auto d=PreviewGarageDirectAppearance(id,before,{id,before,Layer,i});
                        const bool same=SameGarageLook(before,preset.Look);
                        Require(d.Status==(same?GarageDirectAppearanceStatus::Unchanged:
                            GarageDirectAppearanceStatus::Ready) &&
                            SameGarageLook(d.Result,preset.Look),"exact atomic preset preview");
                        Require(GarageLookChoice(d.Result,Layer)==i &&
                            GarageLookPresetIndex(d.Result)==i,"preset round trip");
                        auto state=initial;
                        Require(state.Choose(id,Layer,i) &&
                            SameGarageLook(state.Get(id),preset.Look),"one complete appearance update");
                        for(int other=0;other<VehicleCount;++other)
                            if(other!=id)Require(SameGarageLook(state.Get(other),initial.Get(other)),
                                "other cars unchanged");
                        const auto raw=state.Serialize();
                        GarageAppearanceState restored;
                        Require(restored.Load(raw.data(),raw.size()) &&
                            restored.Serialize()==raw,"existing save schema round trip");
                        const auto choice=DescribeGaragePaletteChoice(id,before,Layer,i);
                        Require(choice.Index==i && choice.Selected==same &&
                            std::string_view(choice.Label)==preset.Name &&
                            std::string_view(choice.ShortLabel)==preset.ShortName,
                            "visible chooser matches exact preset");
                        const auto rgb=PaintForGarageLook(id,preset.Look).Body;
                        Require(choice.Swatch.R==rgb.R && choice.Swatch.G==rgb.G &&
                            choice.Swatch.B==rgb.B,"real paint recipe");
                    }
                    Require(SameGarageLook(initial.Get(id),before),"preview never changes original");
                }
}
void InvalidAndStaleRequests()
{
    const auto before=FactoryGarageLook(0);
    for(int invalid:{-1,GaragePresetOptions,255})
    {
        Require(PreviewGarageDirectAppearance(0,before,{0,before,Layer,invalid}).Status==
            GarageDirectAppearanceStatus::Invalid,"invalid preset rejected");
        GarageAppearanceState state;
        const auto saved=state.Serialize();
        Require(!state.Choose(0,Layer,invalid) && state.Serialize()==saved,
            "invalid choice cannot alter serialization");
    }
    Require(PreviewGarageDirectAppearance(0,before,{1,before,Layer,0}).Status==
        GarageDirectAppearanceStatus::Stale,"different viewed vehicle");
    Require(PreviewGarageDirectAppearance(0,before,{0,{1,0,0},Layer,0}).Status==
        GarageDirectAppearanceStatus::Stale,"different current look");
    Require(GarageLookPresetIndex(before)==-1,"factory is not a fabricated preset");
    GarageAppearanceState state;
    for(int i=0;i<GaragePresetOptions*2;++i)
    {
        Require(state.Cycle(0,Layer) && GarageLookPresetIndex(state.Get(0))==i%GaragePresetOptions,
            "whole look cycle from custom through all presets");
    }
    Require(state.ResetVehicle(0) && SameGarageLook(state.Get(0),before),
        "factory reset remains vehicle-specific");
}
void VisiblePresetGlyphsAndNavigation()
{
    for(int id=0;id<VehicleCount;++id)
        for(int i=0;i<GaragePresetOptions;++i)
        {
            const auto look=GarageLookPresets[static_cast<std::size_t>(i)].Look;
            const auto glyph=CalculateGarageAppearanceGlyph({0,0,48,48},id,look,Layer);
            Require(glyph.Valid && glyph.StripeCount==look.Stripe &&
                SameGarageLook(glyph.Look,look),"roof glyph shows actual paint and stripes");
        }
    for(const auto& dims:std::array<std::array<double,2>,3>{{
        {1280,720},{1920,1080},{2560,1440}}})
    {
        const auto hud=CalculateHudLayout(dims[0],dims[1]);
        const auto lane=CalculateGarageShowroomLane(dims[0],dims[1],hud,false,true,MenuPage::Garage);
        const auto layout=CalculateGaragePaletteLayout(lane,dims[0],dims[1],hud,Layer);
        Require(ValidGaragePaletteLayout(layout) && layout.Count==GaragePresetOptions,
            "full preset picker fits showroom");
        GaragePaletteSession session{true,0,FactoryGarageLook(0),Layer};
        GaragePresentedPalette shown;
        shown.Session=session;shown.Layout=layout;shown.ChoiceVisible.fill(true);
        Require(InitialGaragePaletteFocus(session)==0,"custom look starts at first visible preset");
        for(int i=0;i<GaragePresetOptions;++i)
        {
            const auto choice=GaragePaletteChoiceAction(i);
            Require(CanClickPresentedGaragePalette(shown,session,layout,choice),
                "visible preset can be confirmed");
            const auto result=NavigateGaragePalette(session,layout,shown,i,i,
                GaragePaletteNavigationInput::Confirm);
            Require(result.Effect==GaragePaletteNavigationEffect::Confirm &&
                result.Choice==i,"keyboard confirm uses drawn focus");
            Require(NavigateGaragePalette(session,layout,shown,i,-1,
                GaragePaletteNavigationInput::Confirm).Effect==GaragePaletteNavigationEffect::Ignore,
                "invisible focus never saves");
        }
        auto stale=session;stale.Look.Paint=1;
        Require(!CanClickPresentedGaragePalette(shown,stale,layout,GaragePaletteChoiceAction(0)),
            "preset receipt stale after appearance change");
    }
}
} // namespace
int main()
{
    ExhaustEveryAppearanceAndPreset();InvalidAndStaleRequests();VisiblePresetGlyphsAndNavigation();
    std::cout<<"Complete garage presets, persistence and visible selection passed\n";
}
