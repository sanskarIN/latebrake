#include "Garage/GaragePaletteLegibility.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Check(bool ok,const char* message)
{
    if(!ok)
    {
        std::cerr << "GaragePaletteLegibility: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1e-7)
{
    return std::abs(a-b)<=tolerance;
}
void StandardSrgbContrastReferences()
{
    const VehiclePaintRgb white{1,1,1};
    const VehiclePaintRgb black{0,0,0};
    const VehiclePaintRgb middle{0.5,0.5,0.5};
    Check(Near(GarageSrgbLinearize(0.0),0.0) &&
        Near(GarageSrgbLinearize(1.0),1.0) &&
        Near(GaragePaletteLuminance(white),1.0) &&
        Near(GaragePaletteLuminance(black),0.0),
        "sRGB transfer and luminance have standard endpoints");
    Check(Near(GaragePaletteContrastRatio(white,black),21.0) &&
        Near(GaragePaletteContrastRatio(white,white),1.0) &&
        Near(GaragePaletteContrastRatio(black,black),1.0),
        "luminance contrast has exact black/white bounds");
    const auto color=GaragePaletteContrastRatio(white,middle);
    Check(color>3.95 && color<4.0 &&
        Near(GaragePaletteContrastRatio(white,middle),
            GaragePaletteContrastRatio(middle,white)),
        "mid-gray contrast is symmetric with correct gamma");
    Check(!ValidGaragePaintColor({-0.01,0,0}) &&
        !ValidGaragePaintColor({0,1.001,0}) &&
        !ValidGaragePaintColor({0,0,std::numeric_limits<double>::infinity()}),
        "out-of-gamut and nonfinite color channels fail closed");
    Check(GaragePaletteContrastRatio(white,{-0.1,0,0})==0.0,
        "invalid sRGB colors cannot produce visual contrast advice");
}
void EveryFactoryAndSelectableLookIsFinite()
{
    for(int vehicle=0;vehicle<6;++vehicle)
    for(int paint=0;paint<GaragePaintOptions;++paint)
    for(int stripe=0;stripe<GarageStripeOptions;++stripe)
    for(int wheel=0;wheel<GarageWheelOptions;++wheel)
    {
        const GarageLook look{static_cast<std::uint8_t>(paint),
            static_cast<std::uint8_t>(stripe),
            static_cast<std::uint8_t>(wheel)};
        const auto review=EvaluateGaragePaletteLegibility(vehicle,look);
        Check(ValidGaragePaletteLegibility(review) &&
            review.Valid && review.VehicleId==vehicle &&
            SameGarageLook(review.Look,look) &&
            review.BodyStripeContrast>=1.0 &&
            review.BodyLabelContrast>=GarageContrastIllustrativeSmallText &&
            !review.ChangesAppearance && !review.ChangesGarageEconomy &&
            !review.IsOfficialAccessibilityAudit &&
            !review.UsesCalibratedDisplayEvidence,
            "all actual saved cosmetic combinations have bounded read-only advice");
        if(stripe==0)
            Check(review.Visibility==GaragePaletteVisibility::NoStripes &&
                !review.StripeVisible &&
                !review.StripesMeetIllustrativeContrast,
                "no stripe selection never pretends to show a contrast score");
        else
            Check(review.StripeVisible &&
                review.Visibility!=GaragePaletteVisibility::NoStripes,
                "stripe visibility tracks chosen visual layer");
    }
}
void SuggestionUsesCurrentAuthoredLookNotHypotheticalCost()
{
    const auto dark=EvaluateGaragePaletteLegibility(0,{6,2,2});
    const auto pearl=EvaluateGaragePaletteLegibility(5,{7,1,0});
    Check(ValidGaragePaletteLegibility(dark) &&
        ValidGaragePaletteLegibility(pearl) &&
        dark.Ink==GaragePaletteInk::Light &&
        pearl.Ink==GaragePaletteInk::Dark,
        "body brightness yields a deterministic readable schematic label ink");
    Check(dark.StripesMeetIllustrativeContrast &&
        pearl.StripesMeetIllustrativeContrast,
        "the original high-contrast paint recipes remain distinguishable");
    const auto old=FactoryGarageLook(2);
    const auto palette=CompareGaragePalettePaintOptions(2,old);
    Check(palette.Valid && palette.VehicleId==2 &&
        palette.RequiresPlayerConsent && palette.ReadOnly &&
        palette.StrongestStripeOption>=0 &&
        SameGarageLook(old,FactoryGarageLook(2)),
        "comparison never writes or modifies the existing saved appearance");
    for(int paint=0;paint<GaragePaintOptions;++paint)
    {
        const auto& choice=palette.Paints[static_cast<std::size_t>(paint)];
        Check(choice.PaintId==paint &&
            ValidGaragePaletteLegibility(choice.Legibility) &&
            choice.Legibility.Look.Paint==paint,
            "every palette option keeps its stable persisted paint ID");
    }
    Check(palette.StrongStripeOptionCount==0 &&
        palette.FirstIllustrativelyReadableOption==-1,
        "factory clean look cannot imply an invisible stripe is legible");
    auto visible=old;
    visible.Stripe=2;
    const auto stripes=CompareGaragePalettePaintOptions(2,visible);
    Check(stripes.Valid && stripes.StrongStripeOptionCount>0 &&
        stripes.FirstIllustrativelyReadableOption>=0,
        "paint palette can find visually distinct choices without selection");
}
void InvalidAndFalsifiedContrastIsRejected()
{
    const GarageLook known{0,1,2};
    Check(!EvaluateGaragePaletteLegibility(-1,known).Valid &&
        !EvaluateGaragePaletteLegibility(6,known).Valid &&
        !EvaluateGaragePaletteLegibility(0,{99,0,0}).Valid &&
        !EvaluateGaragePaletteLegibility(0,{0,255,0}).Valid &&
        !CompareGaragePalettePaintOptions(6,known).Valid,
        "unrecognized vehicle and appearance IDs are not silently normalized");
    auto forged=EvaluateGaragePaletteLegibility(2,{4,2,1});
    Check(ValidGaragePaletteLegibility(forged),"baseline authentic recipe");
    forged.IsOfficialAccessibilityAudit=true;
    Check(!ValidGaragePaletteLegibility(forged),
        "fake formal accessibility certification cannot pass validation");
    forged=EvaluateGaragePaletteLegibility(2,{4,2,1});
    forged.ChangesGarageEconomy=true;
    Check(!ValidGaragePaletteLegibility(forged),
        "color analysis is not a garage purchase or unlock authority");
    forged=EvaluateGaragePaletteLegibility(2,{4,2,1});
    forged.BodyStripeContrast+=1.0;
    Check(!ValidGaragePaletteLegibility(forged),
        "fabricated visual contrast cannot change the actual authored recipe");
    forged=EvaluateGaragePaletteLegibility(2,{4,2,1});
    forged.LabelMeetsIllustrativeContrast=false;
    Check(!ValidGaragePaletteLegibility(forged),
        "inconsistent contrasting-ink disclosures rejected");
}
void RandomizedGammaAndContrastOracle()
{
    std::mt19937 generator{0x7293A12};
    std::uniform_real_distribution<double> unit(0.0,1.0);
    for(int n=0;n<40000;++n)
    {
        const VehiclePaintRgb first{unit(generator),unit(generator),unit(generator)};
        const VehiclePaintRgb second{unit(generator),unit(generator),unit(generator)};
        const double lhs=GaragePaletteContrastRatio(first,second);
        const double rhs=GaragePaletteContrastRatio(second,first);
        Check(std::isfinite(lhs) && lhs>=1.0 && lhs<=21.0 &&
            Near(lhs,rhs,1e-12) &&
            Near(GaragePaletteContrastRatio(first,first),1.0,1e-12),
            "randomized valid RGB colors preserve contrast symmetry/bounds");
        const double lum=GaragePaletteLuminance(first);
        const double bright=GaragePaletteContrastRatio(first,{1,1,1});
        const double dark=GaragePaletteContrastRatio(first,{0,0,0});
        Check(Near(lum,0.2126*GarageSrgbLinearize(first.R)+
            0.7152*GarageSrgbLinearize(first.G)+
            0.0722*GarageSrgbLinearize(first.B),1e-12) &&
            Near(bright*dark,21.0,1e-10),
            "independent luminance oracle and black/white ratio product");
    }
}
}
int main()
{
    StandardSrgbContrastReferences();
    EveryFactoryAndSelectableLookIsFinite();
    SuggestionUsesCurrentAuthoredLookNotHypotheticalCost();
    InvalidAndFalsifiedContrastIsRejected();
    RandomizedGammaAndContrastOracle();
    std::cout << "Garage authored paint contrast advisor tests passed.\n";
    return EXIT_SUCCESS;
}
