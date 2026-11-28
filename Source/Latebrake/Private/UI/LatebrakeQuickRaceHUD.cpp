#include "UI/LatebrakeRaceHUD.h"

#include "Game/LatebrakeRaceGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

bool ALatebrakeRaceHUD::TryHandleQuickRaceChoicePointer(double X,double Y)
{
#if PLATFORM_ANDROID
    return false;
#else
    auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!Mode || !Canvas || !PlayerOwner || !PlayerOwner->IsLocalController() ||
        !Mode->IsMenuOpen() || Mode->GetMenuPage()!=latebrake::MenuPage::QuickRace)
        return false;
    const auto Hit=latebrake::HitPresentedQuickRaceChoice(PresentedQuickRace,
        Mode->GetQuickRaceChoiceSnapshot(),Canvas->SizeX,Canvas->SizeY,X,Y);
    if(!Hit.Consumed)return false;
    PresentedQuickRace={};
    if(Hit.Index>=0)Mode->SelectQuickRaceChoice(Hit.Index);
    return true;
#endif
}

void ALatebrakeRaceHUD::DrawQuickRaceChoicePanel(const ALatebrakeRaceGameMode* Mode,
    latebrake::HudLayout Hud,const latebrake::DesktopMenuMouseLayout& Dock,
    const latebrake::HudMenuReadablePanel& Text,bool bTouch)
{
    PresentedQuickRace={};
    if(!Mode || !Canvas || !PlayerOwner || !GEngine || !GEngine->GetSmallFont() ||
        !Mode->IsMenuOpen())return;
    const auto Layout=latebrake::CalculateQuickRaceChoicePanel(Canvas->SizeX,Canvas->SizeY,
        Hud,Dock,Text,bTouch,Mode->GetMenuPage());
    if(!Layout.Valid)return;
    const auto Snapshot=Mode->GetQuickRaceChoiceSnapshot();
    const float S=static_cast<float>(Layout.Scale);
    const FLinearColor White(0.94f,0.96f,1.f),Gold(1.f,0.73f,0.17f);
    const FLinearColor Muted(0.58f,0.65f,0.73f),Surface(0.04f,0.07f,0.11f,0.98f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,const FLinearColor& Ink)
    {
        if(!latebrake::ValidSettingsRect(R))return;
        DrawRect(Ink,static_cast<float>(R.X),static_cast<float>(R.Y),
            static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    const auto Label=[&](const FString& Primary,const FString& Short,
        const latebrake::DesktopMenuMouseRect& R,const FLinearColor& Ink,float Preferred)
    {
        UFont* Font=GEngine->GetSmallFont();
        const auto Try=[&](const FString& Value)
        {
            if(Value.IsEmpty())return false;
            for(int I=0;I<=4;++I)
            {
                const float Scale=Preferred*(1.f-0.07f*I);
                float W=0.f,H=0.f;
                GetTextSize(Value,W,H,Font,Scale);
                if(!FMath::IsFinite(W) || !FMath::IsFinite(H) || W<=0 || H<=0 ||
                    W>R.Width-2.f || H>R.Height)continue;
                DrawText(Value,Ink,static_cast<float>(R.X+(R.Width-W)*0.5),
                    static_cast<float>(R.Y+(R.Height-H)*0.5),Font,Scale);
                return true;
            }
            return false;
        };
        return Try(Primary) || Try(Short);
    };
    Rect(Layout.Panel,Surface);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2.0*S},Gold);
    Label(TEXT("CHOOSE MODE | SELECT TO START"),TEXT("CHOOSE MODE"),Layout.Header,Gold,0.82f*S);
    float MouseX=0.f,MouseY=0.f;
    const bool HasCursor=PlayerOwner->GetMousePosition(MouseX,MouseY);
    latebrake::QuickRaceChoicePresentation Presentation;
    Presentation.Valid=true;
    Presentation.Layout=Layout;
    Presentation.Snapshot=Snapshot;
    for(std::size_t I=0;I<Layout.Cards.size();++I)
    {
        const auto& Card=Layout.Cards[I];
        const auto Desc=latebrake::DescribeQuickRaceChoice(Snapshot,Card.Index);
        if(!Desc.Valid)continue;
        const bool Hover=Desc.Enabled && HasCursor && Card.Bounds.Contains(MouseX,MouseY);
        const FLinearColor Ink=Desc.Enabled?(Desc.Selected?Gold:White):Muted;
        Rect(Card.Bounds,!Desc.Enabled?FLinearColor(0.08f,0.11f,0.15f):
            Desc.Selected?FLinearColor(0.27f,0.21f,0.11f):
            Hover?FLinearColor(0.18f,0.28f,0.39f):FLinearColor(0.12f,0.19f,0.27f));
        if(Desc.Selected)Rect({Card.Bounds.X,Card.Bounds.Y,3*S,Card.Bounds.Height},Gold);
        const FString Detail=Desc.RequestedRivals==0?
            FString::Printf(TEXT("%dL | SOLO"),Desc.Laps):
            FString::Printf(TEXT("%dL | %d RIVAL"),Desc.Laps,Desc.RequestedRivals);
        const bool Title=Label(UTF8_TO_TCHAR(Desc.Title),UTF8_TO_TCHAR(Desc.ShortTitle),
            Card.Title,Ink,0.78f*S);
        const bool DetailShown=Label(Detail,FString(),Card.Detail,Ink,0.75f*S);
        const bool Status=Label(UTF8_TO_TCHAR(Desc.Status),FString(),Card.Status,Muted,0.64f*S);
        Presentation.Rendered[I]=Title && DetailShown && Status;
    }
    PresentedQuickRace=Presentation;
}
