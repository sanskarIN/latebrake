#include "Track/WorldScaleGuardrailScenePlan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool okay,const char* reason)
{
    if(!okay)
    {
        std::cerr<<"WorldScaleGuardrailScene FAIL: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

void VerifyAllRailElements()
{
    using namespace latebrake;
    const auto plan=BuildWorldScaleGuardrailScene();
    Require(plan.Valid &&
            plan.Fault==WorldScaleGuardrailFault::None &&
            plan.Sections>250u &&
            plan.Sections<=WorldScaleGuardrailMaximumSegments &&
            plan.Panels.size()==
                static_cast<std::size_t>(plan.Sections)*6u &&
            plan.Posts.size()==
                static_cast<std::size_t>(plan.Sections)*2u &&
            plan.PhysicalSectionMidpointsChecked==plan.Sections*2u &&
            !plan.IsCollisionBearing &&
            plan.OriginalPhysicalRoadUnchanged &&
            plan.OriginalRaceRulesUnchanged,
            "bounded three-level roadside render inventory has source provenance");
    Require(AuditWorldScaleGuardrailScene(plan),
            "every proposed guardrail and post remains off physical roadway");

    std::array<std::array<unsigned,3>,2> bySide{};
    for(const auto& panel:plan.Panels)
    {
        const auto side=panel.Side>0 ? 1u : 0u;
        ++bySide[side][panel.RailLayer];
        Require(panel.VisualOnly &&
                panel.SourceSection<plan.Sections &&
                panel.EndStationCm>panel.StartStationCm &&
                panel.RenderToken!=0 &&
                panel.NormalizedYawDegrees>=0.0 &&
                panel.NormalizedYawDegrees<360.0 &&
                panel.Corners[0].Z<panel.Corners[3].Z &&
                panel.Corners[1].Z<panel.Corners[2].Z,
                "physical roadside proposals have consistent visual rail faces");
        Require(!IsOnWorldScaleRoad({
                    panel.Corners[0].X,panel.Corners[0].Y}) &&
                !IsOnWorldScaleRoad({
                    panel.Corners[1].X,panel.Corners[1].Y}),
                "neither visual rail endpoint may add collision-backed asphalt");
    }
    for(unsigned side=0;side<2u;++side)
    {
        for(unsigned layer=0;layer<3u;++layer)
        {
            Require(bySide[side][layer]==plan.Sections,
                    "the full circuit has three guardrail levels on both sides");
        }
    }

    for(unsigned station=0;station<plan.Sections;++station)
    {
        const auto left=plan.Posts[2u*station];
        const auto right=plan.Posts[2u*station+1u];
        Require(left.SourceSection==station &&
                right.SourceSection==station &&
                left.Side==-1 && right.Side==1 &&
                left.Top.Z>left.Base.Z &&
                right.Top.Z>right.Base.Z &&
                left.WidthCm>0.0 && right.WidthCm>0.0 &&
                left.RenderToken!=right.RenderToken,
                "paired nonphysical guardrail posts follow source station order");
    }

    const auto next=BuildWorldScaleGuardrailScene();
    Require(next.Valid && next.Sections==plan.Sections &&
            next.Panels.size()==plan.Panels.size() &&
            next.Posts.size()==plan.Posts.size() &&
            next.MaximumPhysicalPanelChordCm==
                plan.MaximumPhysicalPanelChordCm,
            "rebuilt guardrail source geometry is deterministic");
    for(unsigned section=0;section<plan.Sections;++section)
    {
        const auto& now=plan.Posts[2u*section];
        const auto& again=next.Posts[2u*section];
        Require(now.Base.X==again.Base.X &&
                now.Base.Y==again.Base.Y &&
                now.RenderToken==again.RenderToken,
                "source post positions do not rely on random materials/assets");
    }
}

void InvalidSceneRequests()
{
    using namespace latebrake;
    auto settings=WorldScaleGuardrailSettings{};
    settings.RequestedPanelSpacingCm=0.0;
    Require(!BuildWorldScaleGuardrailScene(settings).Valid,
            "zero spacing cannot create infinite safety barriers");
    settings={};
    settings.RequestedPanelSpacingCm=100.0;
    Require(!BuildWorldScaleGuardrailScene(settings).Valid,
            "too-dense roadside placement is rejected at the source gate");
    settings={};
    settings.LateralDistanceFromCenterCm=300.0;
    Require(!BuildWorldScaleGuardrailScene(settings).Valid,
            "rail face cannot be placed in an authentic lane");
    settings={};
    settings.LateralDistanceFromCenterCm=
        std::numeric_limits<double>::quiet_NaN();
    Require(!BuildWorldScaleGuardrailScene(settings).Valid,
            "nonfinite offsets are rejected before mesh allocation");
    settings={};
    settings.MustKeepRoadCollisionUnchanged=false;
    Require(!BuildWorldScaleGuardrailScene(settings).Valid,
            "visual proposal never promotes independent Chaos collision");
    settings={};
    settings.MustNotModifyRaceControl=false;
    Require(!BuildWorldScaleGuardrailScene(settings).Valid,
            "trackside decoration never changes official lap/penalty rules");
    settings={};
    settings.ExplicitlyVisualOnly=false;
    Require(!BuildWorldScaleGuardrailScene(settings).Valid,
            "render-only scenario cannot silently become a roadside collision wall");
    settings={};
    settings.RailBaseCm=95.0;
    Require(!BuildWorldScaleGuardrailScene(settings).Valid,
            "unrealistic rail/post vertical intersection fails geometry contract");
    settings={};
    settings.PostHeightCm=80.0;
    Require(!BuildWorldScaleGuardrailScene(settings).Valid,
            "too-short physical reference posts cannot hold proposed rails");
}

void SeamAndPanelSpacing()
{
    using namespace latebrake;
    WorldScaleGuardrailSettings settings{};
    settings.RequestedPanelSpacingCm=375.0;
    const auto plan=BuildWorldScaleGuardrailScene(settings);
    Require(plan.Valid && AuditWorldScaleGuardrailScene(plan),
            "alternate 375cm section plan remains physically disjoint from pavement");
    Require(plan.Panels.front().StartStationCm==0.0 &&
            plan.Panels.back().EndStationCm==WorldScaleCircuitLapCm,
            "last future roadside scene section closes to authored lap exactly");
    const auto& first=plan.Posts.front().Base;
    const auto& final=plan.Panels.back().Corners[1];
    // The last back-facing panel corner is elevated but in the same
    // XY as the first source station's corresponding side.
    Require(std::isfinite(first.X) && std::isfinite(final.X) &&
            !IsOnWorldScaleRoad({first.X,first.Y}) &&
            !IsOnWorldScaleRoad({final.X,final.Y}),
            "render-only safety ribbons never close over drivable road");
}
} // namespace

int main()
{
    VerifyAllRailElements();
    InvalidSceneRequests();
    SeamAndPanelSpacing();
    std::cout<<"WorldScaleGuardrailScene PASS: three rail levels, "
                "both nonphysical road sides, dense posts, "
                "original seam, deterministic source and safety rejection "
                "(no Unreal collisions or installed driving)\n";
    return EXIT_SUCCESS;
}
