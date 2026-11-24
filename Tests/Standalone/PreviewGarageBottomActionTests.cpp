#include "PreviewGarageBottomAction.h"
#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool ok,const char* reason)
{
    if(!ok){std::cerr<<"Garage action partition failure: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);}
}
void ButtonCases()
{
    using namespace latebrake_preview;
    const unsigned owned=1u|(1u<<5);
    Check(PreviewGarageButtonAction(477,520,5,0,owned)==
        PreviewGarageBottomAction::EquipOnly,"left owned action EQUIP only");
    Check(PreviewGarageButtonAction(661,553,5,0,owned)==
        PreviewGarageBottomAction::EquipOnly,"left inclusive final pixel");
    Check(PreviewGarageButtonAction(662,534,5,0,owned)==
        PreviewGarageBottomAction::None,"split gap is inert");
    Check(PreviewGarageButtonAction(671,534,5,0,owned)==
        PreviewGarageBottomAction::None,"split gap stays inert");
    Check(PreviewGarageButtonAction(672,520,5,0,owned)==
        PreviewGarageBottomAction::Race,"right target races after equip");
    Check(PreviewGarageButtonAction(869,553,5,0,owned)==
        PreviewGarageBottomAction::Race,"last right pixel is active");
    Check(PreviewGarageButtonAction(477,520,0,0,owned)==
        PreviewGarageBottomAction::Race,"already equipped is full-width race");
    Check(PreviewGarageButtonAction(500,534,5,0,1u)==
        PreviewGarageBottomAction::Unlock,"locked always requests consent");
    Check(PreviewGarageButtonAction(730,534,5,0,1u)==
        PreviewGarageBottomAction::Unlock,"locked right side cannot launch");
    for(int id=-2;id<=14;id++){
        const auto expected=ValidPreviewCar(id);
        Check((PreviewGarageButtonAction(500,534,id,0,owned)!=
            PreviewGarageBottomAction::None)==expected,
            "invalid car IDs have no hitbox action");
    }
    for(const int x : {476,870,-4000,1000000})
        Check(PreviewGarageButtonAction(x,534,5,0,owned)==
            PreviewGarageBottomAction::None,"outside horizontal dock ignored");
    for(const int y : {519,554,-4000,1000000})
        Check(PreviewGarageButtonAction(500,y,5,0,owned)==
            PreviewGarageBottomAction::None,"outside vertical dock ignored");
}
void ExactPixelSweep()
{
    using namespace latebrake_preview;
    const unsigned owned=1u|(1u<<5);
    for(int y=514;y<=560;y++){
        for(int x=460;x<=888;x++){
            const bool inside=x>=477&&x<870&&y>=520&&y<554;
            auto action=PreviewGarageButtonAction(x,y,5,0,owned);
            auto locked=PreviewGarageButtonAction(x,y,5,0,1u);
            auto selected=PreviewGarageButtonAction(x,y,0,0,owned);
            const auto expected=!inside?PreviewGarageBottomAction::None:
                x<662?PreviewGarageBottomAction::EquipOnly:
                x<672?PreviewGarageBottomAction::None:
                PreviewGarageBottomAction::Race;
            Check(action==expected,"all 20k dock pixels have exact semantics");
            Check(locked==(inside?PreviewGarageBottomAction::Unlock:
                PreviewGarageBottomAction::None),
                "locked vehicle never has an equip or race target");
            Check(selected==(inside?PreviewGarageBottomAction::Race:
                PreviewGarageBottomAction::None),
                "already equipped uses the existing full-width race target");
        }
    }
}
} // namespace
int main()
{
    ButtonCases();
    ExactPixelSweep();
    std::cout<<"2D showroom split-button action policy passed.\n";
    return EXIT_SUCCESS;
}
