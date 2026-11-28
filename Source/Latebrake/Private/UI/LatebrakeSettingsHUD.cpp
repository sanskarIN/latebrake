#include "UI/LatebrakeRaceHUD.h"

#include "Game/LatebrakeRaceGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/HudButtonCaptionFit.h"

bool ALatebrakeRaceHUD::TryHandleSettingsPointer(double X,double Y)
{
#if PLATFORM_ANDROID
    return false;
#else
    auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!Canvas || !PlayerOwner || !PlayerOwner->IsLocalController() || !Mode ||
        !Mode->IsMenuOpen() || Mode->GetMenuPage()!=latebrake::MenuPage::Settings)
        return false;
    const auto Hit=latebrake::HitPresentedSettingsPanel(PresentedSettings,
        Mode->GetSettingsMenuSnapshot(),Canvas->SizeX,Canvas->SizeY,X,Y);
    if(!Hit.Consumed)return false;
    // Revoke before a single synchronous mutation. Another click cannot
    // act on old captions or on an earlier settings selection.
    PresentedSettings={};
    if(latebrake::ValidSettingsControl(Hit.Action))
        Mode->ActivateSettingsControl(Hit.Action);
    return true;
#endif
}

void ALatebrakeRaceHUD::DrawSettingsPanel(const ALatebrakeRaceGameMode* Mode,
    latebrake::HudLayout Hud,const latebrake::DesktopMenuMouseLayout& Dock,
    const latebrake::HudMenuReadablePanel& Text,bool bTouch)
{
    PresentedSettings={};
    if(!Mode || !Canvas || !GEngine || !GEngine->GetSmallFont() ||
        !Mode->IsMenuOpen())return;
    const auto Layout=latebrake::CalculateSettingsPanelLayout(Canvas->SizeX,Canvas->SizeY,
        Hud,Dock,Text,bTouch,Mode->GetMenuPage());
    if(!Layout.Valid)return;
    const auto Snapshot=Mode->GetSettingsMenuSnapshot();
    const float S=static_cast<float>(Layout.Scale);
    const FLinearColor White(0.94f,0.96f,1.f),Gold(1.f,0.73f,0.17f);
    const FLinearColor Muted(0.58f,0.65f,0.73f),Surface(0.04f,0.07f,0.11f,0.98f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,const FLinearColor& Ink)
    {
        if(!latebrake::ValidSettingsRect(R))return;
        DrawRect(Ink,static_cast<float>(R.X),static_cast<float>(R.Y),
            static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    // Never advertise a clickable card unless the actual font draws its
    // complete label, current value and persistence/availability status.
    const auto Label=[&](const FString& Value,const FString& Short,
        const latebrake::DesktopMenuMouseRect& R,const FLinearColor& Ink,float Preferred)
    {
        UFont* Font=GEngine->GetSmallFont();
        const auto Try=[&](const FString& Caption)
        {
            if(Caption.IsEmpty())return false;
            for(int i=0;i<=4;++i)
            {
                const float Scale=Preferred*(1.0f-0.08f*i);
                float W=0.f,H=0.f;
                GetTextSize(Caption,W,H,Font,Scale);
                if(!FMath::IsFinite(W) || !FMath::IsFinite(H) || W<=0 || H<=0 ||
                    W>R.Width-4.0 || H>R.Height)continue;
                DrawText(Caption,Ink,static_cast<float>(R.X+(R.Width-W)*0.5),
                    static_cast<float>(R.Y+(R.Height-H)*0.5),Font,Scale);
                return true;
            }
            return false;
        };
        return Try(Value) || Try(Short);
    };
    Rect(Layout.Panel,Surface);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2.0*S},Gold);
    Label(TEXT("DISPLAY SETTINGS"),TEXT("SETTINGS"),Layout.Header,Gold,0.84f*S);
    float MouseX=0.f,MouseY=0.f;
    const bool HasCursor=PlayerOwner && PlayerOwner->GetMousePosition(MouseX,MouseY);
    latebrake::SettingsPanelPresentation Presentation;
    Presentation.Valid=true;
    Presentation.Layout=Layout;
    Presentation.Snapshot=Snapshot;
    for(std::size_t i=0;i<Layout.Rows.size();++i)
    {
        const auto& Row=Layout.Rows[i];
        const auto Desc=latebrake::DescribeSettingsControl(Snapshot,Row.Control);
        const bool Hover=HasCursor && Desc.Enabled && Row.Bounds.Contains(MouseX,MouseY);
        const FLinearColor Ink=Desc.Enabled?(Desc.Selected?Gold:White):Muted;
        Rect(Row.Bounds,!Desc.Enabled?FLinearColor(0.08f,0.11f,0.15f):
            Desc.Selected?FLinearColor(0.27f,0.21f,0.11f):
            Hover?FLinearColor(0.18f,0.28f,0.39f):FLinearColor(0.12f,0.19f,0.27f));
        if(Desc.Selected)Rect({Row.Bounds.X,Row.Bounds.Y,3.0*S,Row.Bounds.Height},Gold);
        const bool Title=Label(UTF8_TO_TCHAR(Desc.Title),UTF8_TO_TCHAR(Desc.ShortTitle),
            Row.Title,Ink,0.76f*S);
        const bool Value=Label(UTF8_TO_TCHAR(Desc.Value),FString(),Row.Value,Ink,1.02f*S);
        const bool Status=Label(UTF8_TO_TCHAR(Desc.Status),UTF8_TO_TCHAR(Desc.ShortStatus),
            Row.Status,Muted,0.68f*S);
        Presentation.Rendered[i]=Title && Value && Status;
    }
    PresentedSettings=Presentation;
}
