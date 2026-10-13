#include "BenchyMenu.h"
#include "BenchyLogo.h"
#include "BenchyDisplay.h"
#include "BenchyPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <commdlg.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif
void ABenchyController::ApplyLogo(bool Save){
    auto I=GetGameInstance<UBenchyInstance>();if(!I||!I->Logo)return;
    if(Preview)I->Logo->Apply(Preview->LogoMesh());
    if(auto B=Cast<ABenchyPawn>(GetPawn()))I->Logo->Apply(B->FindComponentByClass<UStaticMeshComponent>());
    if(Save){I->Logo->Save();SyncLogo();}
}
void ABenchyController::ImportLogo(){
    if(FParse::Param(FCommandLine::Get(),TEXT("BenchyLogoAudit"))||FParse::Param(FCommandLine::Get(),TEXT("BenchySocialAudit"))){FString Path;if(FParse::Value(FCommandLine::Get(),TEXT("BenchyLogoFixture="),Path)){auto I=GetGameInstance<UBenchyInstance>();I->Logo->Import(Path);ApplyLogo();ShowMenu(2);}return;}
#if PLATFORM_WINDOWS
    wchar_t File[32768]={0};OPENFILENAMEW Dialog={};Dialog.lStructSize=sizeof(Dialog);
    Dialog.hwndOwner=(HWND)FSlateApplication::Get().FindBestParentWindowHandleForDialogs(Panel);
    Dialog.lpstrFilter=L"PNG-Logo (*.png)\0*.png\0\0";Dialog.lpstrFile=File;Dialog.nMaxFile=32768;Dialog.lpstrTitle=L"Dein PNG-Logo wählen";
    Dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER;
    if(GetOpenFileNameW(&Dialog)){auto I=GetGameInstance<UBenchyInstance>();if(I&&I->Logo){I->Logo->AddLayer(FString(File));ApplyLogo();ShowMenu(2);}}
#endif
}
TSharedRef<SWidget> ABenchyController::BuildLogoEditor(){
    auto I=GetGameInstance<UBenchyInstance>();auto Root=I->Logo;if(!FParse::Param(FCommandLine::Get(),TEXT("BenchyLogoAudit"))&&!FParse::Param(FCommandLine::Get(),TEXT("BenchySocialAudit")))Root->EnableLayers();auto D=Root->ActiveLayer();
    TSharedRef<SVerticalBox> Box=SNew(SVerticalBox);
    auto Label=[&](FString Text,int32 Size=13){Box->AddSlot().AutoHeight().Padding(0,3)[SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).WrapTextAt(280).ColorAndOpacity(FLinearColor(.85,.94,.95))];};
    auto Button=[&](FString Text,TFunction<void()> Action){Box->AddSlot().AutoHeight().Padding(0,3)[SNew(SButton).ContentPadding(FMargin(10,7)).ButtonColorAndOpacity(FLinearColor(.055,.19,.20)).OnClicked_Lambda([Action](){Action();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(Text)).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Bold",14)).ColorAndOpacity(FLinearColor::White)]];};
    auto Slider=[&](FString Text,float* Value,float Low,float High){Label(Text);Box->AddSlot().AutoHeight().Padding(0,4)[SNew(SSlider).Value_Lambda([Value,Low,High](){return (*Value-Low)/(High-Low);}).OnValueChanged_Lambda([this,Value,Low,High](float V){*Value=FMath::Lerp(Low,High,V);ApplyLogo();})];};
    Label(TEXT("LOGO-EBENEN"),18);
    Label(TEXT("Jedes Logo hat seine eigene Fläche, Position und Kontur. Spätere Ebenen liegen über früheren."),12);
    Button(TEXT("Neue PNG-Ebene hinzufügen …"),[this](){ImportLogo();});
    if(Root->Layered){auto Rows=SNew(SVerticalBox);for(int32 N=0;N<Root->Layers.Num();++N){auto Layer=Root->Layers[N];Rows->AddSlot().AutoHeight().Padding(0,2)[SNew(SButton).ContentPadding(FMargin(6,5)).OnClicked_Lambda([this,Root,N](){Root->ActiveIndex=N;ShowMenu(2);return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s %d · %s%s"),Root->ActiveIndex==N?TEXT("✓"):TEXT("○"),N+1,*Layer->Name,Layer->Enabled?TEXT(""):TEXT(" · verborgen")))).AutoWrapText(true).ColorAndOpacity(FLinearColor::White)]];}Box->AddSlot().AutoHeight()[SNew(SBox).MaxDesiredHeight(150)[SNew(SScrollBox)+SScrollBox::Slot()[Rows]]];}
    if(!D){Label(TEXT("Füge ein PNG hinzu, um dein Benchy zu gestalten."));Button(TEXT("Zurück"),[this](){ApplyLogo(true);ShowMenu();});return Box;}
    auto Pair=[&](FString A,TFunction<void()> FA,FString B,TFunction<void()> FB){Box->AddSlot().AutoHeight().Padding(0,3)[SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1).Padding(0,0,3,0)[SNew(SButton).Text(FText::FromString(A)).ContentPadding(FMargin(6,7)).OnClicked_Lambda([FA](){FA();return FReply::Handled();})]+SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(FText::FromString(B)).ContentPadding(FMargin(6,7)).OnClicked_Lambda([FB](){FB();return FReply::Handled();})]];};
    if(Root->Layered){Pair(TEXT("Duplizieren"),[this,Root](){Root->DuplicateLayer();ApplyLogo();ShowMenu(2);},TEXT("Löschen"),[this,Root](){Root->DeleteLayer();ApplyLogo();ShowMenu(2);});Pair(TEXT("Nach vorn"),[this,Root](){Root->MoveLayer(1);ApplyLogo();ShowMenu(2);},TEXT("Nach hinten"),[this,Root](){Root->MoveLayer(-1);ApplyLogo();ShowMenu(2);});}
    static const TCHAR* Sides[]={TEXT("Links / Backbord"),TEXT("Rechts / Steuerbord"),TEXT("Deck"),TEXT("Beide Seiten"),TEXT("Heck")};
    Button(FString(TEXT("Fläche: "))+Sides[D->Side],[this,D](){D->Side=(D->Side+1)%5;if(Preview)Preview->SetLogoEditor(true,D->Side);ApplyLogo();ShowMenu(2);});
    Button(TEXT("Auf dem Rumpf platzieren"),[this,D](){D->X=.5;D->Y=.17;D->Size=.28;D->Angle=0;if(D->Side==2)D->Side=0;ApplyLogo();ShowMenu(2);});
    Button(TEXT("Auf der Kabine platzieren"),[this,D](){D->X=.55;D->Y=.72;D->Size=.19;D->Angle=0;if(D->Side==2)D->Side=0;ApplyLogo();ShowMenu(2);});
    Slider(D->Side==4?TEXT("Position quer am Heck"):TEXT("Position längs"),&D->X,0,1);Slider(D->Side==2?TEXT("Querposition"):TEXT("Höhe"),&D->Y,0,1);
    Slider(TEXT("Größe"),&D->Size,.08,.9);Slider(TEXT("Drehung"),&D->Angle,-180,180);
    static const TCHAR* Contours[]={TEXT("Keine"),TEXT("Weiß"),TEXT("Schwarz"),TEXT("Neon Gelb")};
    Button(FString(TEXT("Kontur: "))+Contours[D->Outline],[this,D](){D->Outline=(D->Outline+1)%4;D->Rebuild();ApplyLogo();ShowMenu(2);});
    Label(TEXT("Konturdicke"));Box->AddSlot().AutoHeight().Padding(0,4)[SNew(SSlider).Value(float(D->OutlineWidth)/12).OnValueChanged_Lambda([D](float V){D->OutlineWidth=FMath::RoundToInt(V*12);}).OnMouseCaptureEnd_Lambda([this,D](){D->Rebuild();ApplyLogo();}).OnControllerCaptureEnd_Lambda([this,D](){D->Rebuild();ApplyLogo();})];
    Button(TEXT("Ansicht um 30° drehen"),[this](){if(Preview)Preview->RotateLogoView(30);});
    Button(D->Enabled?TEXT("Ebene ausblenden"):TEXT("Ebene einblenden"),[this,D](){D->Enabled=!D->Enabled;ApplyLogo();ShowMenu(2);});
    Button(TEXT("Speichern & zurück"),[this](){ApplyLogo(true);ShowMenu();});
    Box->AddSlot().AutoHeight().Padding(0,4)[SNew(STextBlock).Text_Lambda([D,Root](){return FText::FromString(Root->Notice.IsEmpty()?D->Notice:Root->Notice);}).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).WrapTextAt(280).ColorAndOpacity(FLinearColor(.65,.96,.89))];
    Label(TEXT("Dein Logo wird gespeichert und beim Lobby-Beitritt auf dein Benchy übertragen."),11);
    return Box;
}
