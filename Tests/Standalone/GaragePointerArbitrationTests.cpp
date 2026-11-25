#include "UI/GaragePointerArbitration.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
using Action = DesktopMenuMouseAction;

void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "Garage pointer arbitration: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::array<Action, GaragePointerGroupCount> Representative{
    Action::Activate, Action::GaragePaint, Action::GarageQuickView0,
    Action::GarageOrbit0, Action::GarageUnlockOffer, Action::GarageLookChoice0};

void AllClaimsAndPermissions()
{
    // Every possible combination of the six physical regions and every
    // combination of permissions. A disabled hit still owns its region.
    for (unsigned physical = 0; physical < (1u << GaragePointerGroupCount); ++physical)
        for (unsigned allowed = 0; allowed < (1u << GaragePointerGroupCount); ++allowed)
        {
            GaragePointerClaims claims{};
            Action expected = Action::None;
            unsigned count = 0;
            for (std::size_t i = 0; i < claims.size(); ++i)
            {
                const unsigned bit = 1u << i;
                if ((physical & bit) == 0u) continue;
                ++count;
                claims[i] = {Representative[i], (allowed & bit) != 0u};
                if (claims[i].Authorized) expected = claims[i].Action;
            }
            if (count != 1u) expected = Action::None;
            Require(ResolveGaragePointerClaims(claims) == expected,
                "all physical/permission masks match independent oracle");
        }
}

void ExactGroupIdentity()
{
    constexpr std::array all{
        Action::PreviousPage, Action::NextPage, Action::DetailsUp,
        Action::DetailsDown, Action::Activate, Action::Close,
        Action::GaragePaint, Action::GarageStripe, Action::GarageWheels,
        Action::GarageReset, Action::GarageBrowsePrevious,
        Action::GarageBrowseNext, Action::GarageQuickView0,
        Action::GarageQuickView1, Action::GarageQuickView2,
        Action::GarageQuickView3, Action::GarageQuickView4,
        Action::GarageQuickView5, Action::GarageOrbit0, Action::GarageOrbit1,
        Action::GarageOrbit2, Action::GarageOrbit3,
        Action::GarageUnlockOffer, Action::GarageEngineOffer, Action::GarageGripOffer,
        Action::GarageLookChoice0, Action::GarageLookChoice1, Action::GarageLookChoice2,
        Action::GarageLookChoice3, Action::GarageLookChoice4, Action::GarageLookChoice5,
        Action::GarageLookChoice6, Action::GarageLookChoice7, Action::GaragePaletteClose};
    for (std::size_t i = 0; i < all.size(); ++i)
    {
        const std::size_t expectedGroup = i < 6u ? 0u :
            i < 12u ? 1u : i < 18u ? 2u : i < 22u ? 3u : i < 25u ? 4u : 5u;
        for (std::size_t group = 0; group < GaragePointerGroupCount; ++group)
        {
            GaragePointerClaims claims{};
            claims[group] = {all[i], true};
            Require(ResolveGaragePointerClaims(claims) ==
                (group == expectedGroup ? all[i] : Action::None),
                "an action may originate only in its designated region");
            claims[group].Authorized = false;
            Require(ResolveGaragePointerClaims(claims) == Action::None,
                "no disabled action is admitted");
        }
    }
    for (const int invalid : {std::numeric_limits<int>::min(), -1,
            std::numeric_limits<int>::max()})
    {
        GaragePointerClaims claims{};
        claims[0] = {Action::Close, true};
        claims[2] = {static_cast<Action>(invalid), false};
        Require(ResolveGaragePointerClaims(claims) == Action::None,
            "malformed even disabled claim cannot pass a valid action");
    }
    GaragePointerClaims claims{};
    claims[1].Authorized = true;
    Require(ResolveGaragePointerClaims(claims) == Action::None,
        "permission without a physical action is not an action");
}

void RealDesktopPlans()
{
    const GaragePointerPermissions full{{true,true,true},true,true,true,{true,true,true}};
    const GaragePointerPermissions readOnly{{false,false,false},true,false,false,{false,false,false}};
    for (const auto& screen : {std::array<int,2>{900,600},{1280,720},
         {1366,768},{1920,1080},{2560,1440},{3840,2160},{2560,1080}})
    {
        const double w=screen[0], h=screen[1];
        const auto hud=CalculateHudLayout(w,h);
        const auto nav=CalculateDesktopMenuMouseLayout(w,h,hud,false,true);
        const auto style=CalculateGarageStyleMouseLayout(w,h,hud,false,true,MenuPage::Garage);
        const auto lane=CalculateGarageShowroomLane(w,h,hud,false,true,MenuPage::Garage);
        const auto orbit=CalculateGarageOrbitButtons(lane,w,h,hud);
        const auto purchase=CalculateGaragePurchaseMouseLayout(lane,w,h,hud);
        Require(nav.Valid && style.Valid && orbit.Valid && purchase.Valid, "full desktop plans valid");
        for (int vehicle=0;vehicle<VehicleCount;++vehicle)
        {
            const auto diagram=CalculateGarageSchematic(w,h,hud,style,
                vehicle,FactoryGarageLook(vehicle));
            const auto quick=CalculateGarageQuickViewLayout(diagram);
            Require(quick.Valid, "quick-view plan valid for every car");
            const auto hit=[&](const DesktopMenuMouseRect& r, GaragePointerPermissions p)
            {
                return ResolveGaragePointerClaims(CollectGaragePointerClaims(
                    nav,style,quick,orbit,purchase,r.X+r.Width/2.0,r.Y+r.Height/2.0,p));
            };
            for (const auto& b:nav.Buttons)
            {
                Require(hit(b.Bounds,full)==b.Action,"normal nav center maps uniquely");
                const bool allowed=DesktopMenuMouseActionEnabled(b.Action,readOnly.Navigation);
                Require(hit(b.Bounds,readOnly)==(allowed?b.Action:Action::None),
                    "locked equipment does not disable page or close controls");
            }
            for (const auto& b:style.Buttons)
            {
                Require(hit(b.Bounds,full)==b.Action,"style center maps uniquely");
                Require(hit(b.Bounds,readOnly)==
                    (IsGarageBrowseAction(b.Action)?b.Action:Action::None),
                    "read-only player may browse but not style");
            }
            for (const auto& b:quick.Buttons)
            {
                Require(hit(b.Bounds,full)==b.Action,"six view-only chips map uniquely");
                Require(hit(b.Bounds,readOnly)==b.Action,"read-only still views any car");
            }
            for (const auto& b:orbit.Buttons)
            {
                Require(hit(b.Bounds,full)==b.Action,"camera angle maps uniquely");
                Require(hit(b.Bounds,readOnly)==Action::None,"disabled camera cannot dispatch");
            }
            for (const auto& b:purchase.Buttons)
            {
                Require(hit(b.Bounds,full)==b.Action,"purchase offer maps uniquely");
                Require(hit(b.Bounds,readOnly)==Action::None,"read-only cannot click a purchase");
                auto partial=full;
                partial.Purchase[static_cast<std::size_t>(GarageOfferIndex(b.Action))]=false;
                Require(hit(b.Bounds,partial)==Action::None,"disabled offer cannot dispatch");
            }
            auto overlappingPurchase=purchase;
            overlappingPurchase.Buttons[0].Bounds=nav.Buttons[4].Bounds;
            const auto& nr=nav.Buttons[4].Bounds;
            // A malformed or disabled purchase area must not grant spending.
            Require(ResolveGaragePointerClaims(CollectGaragePointerClaims(
                nav,style,quick,orbit,overlappingPurchase,
                nr.X+nr.Width/2,nr.Y+nr.Height/2,full))==Action::None,
                "purchase overlapping navigation cannot dispatch");
            // Inject cross-region overlap without corrupting either group
            // internally. A disabled style region must not expose navigation.
            auto overlapping=nav;
            overlapping.Buttons[4].Bounds=style.Buttons[0].Bounds;
            const auto& r=style.Buttons[0].Bounds;
            for (const auto p : {full,readOnly,GaragePointerPermissions{{true,true,true},true,false,true,{true,true,true}}})
                Require(ResolveGaragePointerClaims(CollectGaragePointerClaims(
                    overlapping,style,quick,orbit,purchase,r.X+r.Width/2.0,r.Y+r.Height/2.0,p))
                    ==Action::None,"overlap fails regardless of group authorization");
            const double nan=std::numeric_limits<double>::quiet_NaN();
            Require(ResolveGaragePointerClaims(CollectGaragePointerClaims(
                nav,style,quick,orbit,purchase,nan,0.0,full))==Action::None,
                "invalid pointer cannot dispatch");
        }
    }
}
void MalformedActiveGroupFailsClosed()
{
    const auto hud=CalculateHudLayout(1920,1080);
    const auto nav=CalculateDesktopMenuMouseLayout(1920,1080,hud,false,true);
    const auto style=CalculateGarageStyleMouseLayout(1920,1080,hud,false,true,MenuPage::Garage);
    const auto lane=CalculateGarageShowroomLane(1920,1080,hud,false,true,MenuPage::Garage);
    const auto orbit=CalculateGarageOrbitButtons(lane,1920,1080,hud);
    const auto purchase=CalculateGaragePurchaseMouseLayout(lane,1920,1080,hud);
    const auto quick=CalculateGarageQuickViewLayout(
        CalculateGarageSchematic(1920,1080,hud,style,0,FactoryGarageLook(0)));
    const GaragePointerPermissions allow{{true,true,true},true,true,true,{true,true,true}};
    Require(ValidGaragePointerPlans(nav,style,quick,orbit,purchase),"baseline surface valid");

    // Every corruption is tested against all other valid physical groups,
    // not just at the invalid button. In particular a failed local hit
    // test must never become permission for an unrelated paid action.
    const auto reject=[&](auto n,auto s,auto q,auto o,auto p)
    {
        Require(!ValidGaragePointerPlans(n,s,q,o,p),"malformed active group detected");
        const auto probe=[&](const auto& buttons)
        {
            for(const auto& button:buttons)
            {
                const auto& b=button.Bounds;
                Require(ResolveGaragePointerClaims(CollectGaragePointerClaims(
                    n,s,q,o,p,b.X+b.Width/2,b.Y+b.Height/2,allow))==Action::None,
                    "malformed group cannot leak any other group's click");
            }
        };
        probe(nav.Buttons);probe(style.Buttons);probe(quick.Buttons);
        probe(orbit.Buttons);probe(purchase.Buttons);
        Require(ResolveGaragePointerClaims(CollectGaragePointerClaims(
            n,s,q,o,p,0,0,allow,{Action::GarageLookChoice0,true}))==Action::None,
            "malformed group rejects palette claim as well");
    };
    const auto corrupt=[&](auto original,auto verify)
    {
        const double nan=std::numeric_limits<double>::quiet_NaN();
        const double inf=std::numeric_limits<double>::infinity();
        for(std::size_t i=0;i<original.Buttons.size();++i)
        {
            auto bad=original;
            bad.Buttons[i].Action=Action::None;
            verify(bad);
            bad=original;
            bad.Buttons[i].Bounds.X=nan;
            verify(bad);
            bad=original;
            bad.Buttons[i].Bounds.Y=-1.0;
            verify(bad);
            bad=original;
            bad.Buttons[i].Bounds.Width=0.0;
            verify(bad);
            bad=original;
            bad.Buttons[i].Bounds.Height=inf;
            verify(bad);
            bad=original;
            bad.Buttons[i].Bounds.X=1e308;
            verify(bad);
            bad=original;
            bad.Buttons[i].Bounds=original.Buttons[(i+1)%original.Buttons.size()].Bounds;
            verify(bad);
        }
    };
    corrupt(nav,[&](auto v){reject(v,style,quick,orbit,purchase);});
    corrupt(style,[&](auto v){reject(nav,v,quick,orbit,purchase);});
    corrupt(quick,[&](auto v){reject(nav,style,v,orbit,purchase);});
    corrupt(orbit,[&](auto v){reject(nav,style,quick,v,purchase);});
    corrupt(purchase,[&](auto v){reject(nav,style,quick,orbit,v);});

    // Unsupported compact layouts deliberately do not claim clicks.
    auto inactive=quick;
    inactive.Valid=false;
    inactive.Buttons[0].Action=Action::Close;
    Require(ValidGaragePointerPlans(nav,style,inactive,orbit,purchase),
        "inactive groups remain empty");
    const auto& close=nav.Buttons[5].Bounds;
    Require(ResolveGaragePointerClaims(CollectGaragePointerClaims(
        nav,style,inactive,orbit,purchase,close.X+1,close.Y+1,allow))==Action::Close,
        "inactive optional group cannot disable normal navigation");
}
}

int main()
{
    AllClaimsAndPermissions();
    ExactGroupIdentity();
    RealDesktopPlans();
    MalformedActiveGroupFailsClosed();
    std::cout << "Garage pointer claim arbitration passed.\n";
    return EXIT_SUCCESS;
}
