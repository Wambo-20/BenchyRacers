#include "BenchyMenu.h"
#include "BenchyPawn.h"
#include "BenchyRace.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

void ABenchyController::HideResults(){
    if(ResultPanel.IsValid()&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(ResultPanel.ToSharedRef());
    ResultPanel.Reset();
}
void ABenchyController::ShowResults(){
    if(ResultPanel.IsValid()||!IsLocalController()||!GEngine||!GEngine->GameViewport)return;
    auto State=GetWorld()->GetGameState<ABenchyRaceState>();auto Boat=Cast<ABenchyPawn>(GetPawn());if(!State||!Boat||!Boat->Finished)return;
    const int32 OwnId=Boat->RaceEntryId;
    auto Rows=SNew(SVerticalBox);
    Rows->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("IM ZIEL"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",30)).ColorAndOpacity(FLinearColor(.55,1,.86))];
    Rows->AddSlot().AutoHeight().Padding(0,10)[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Platz %d   ·   %s"),Boat->FinishPlace,*ABenchyRaceState::FormatTime(Boat->RaceTime)))).Font(FCoreStyle::GetDefaultFontStyle("Bold",25)).ColorAndOpacity(FLinearColor::White)];
    Rows->AddSlot().AutoHeight().Padding(0,0,0,18)[SNew(STextBlock).Text(FText::FromString(TEXT("Dein Benchy folgt automatisch der Strecke."))).Font(FCoreStyle::GetDefaultFontStyle("Regular",15)).WrapTextAt(475).ColorAndOpacity(FLinearColor(.7,.8,.85))];
    Rows->AddSlot().AutoHeight().Padding(0,0,0,8)[SNew(STextBlock).Text_Lambda([State](){return FText::FromString(State->Phase==3?TEXT("RENNEN ABGESCHLOSSEN"):TEXT("LIVE-ERGEBNISSE · RENNEN LÄUFT WEITER"));}).Font(FCoreStyle::GetDefaultFontStyle("Bold",15)).ColorAndOpacity(FLinearColor(.9,.75,.4))];
    for(int32 I=0;I<State->Results.Num();I++){
        auto Entry=[State,I](){auto Order=State->OrderedResults();return Order.IsValidIndex(I)?Order[I]:FBenchyResult();};
        auto Tint=[Entry,OwnId](){return FSlateColor(Entry().EntryId==OwnId?FLinearColor(.55,1,.86):FLinearColor(.9,.93,.95));};
        Rows->AddSlot().AutoHeight().Padding(0,3)[SNew(SBorder).Padding(FMargin(10,12)).BorderBackgroundColor_Lambda([Entry,OwnId](){return FSlateColor(Entry().EntryId==OwnId?FLinearColor(.025,.12,.13,.95):FLinearColor(.018,.035,.045,.8));})
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(36)[SNew(STextBlock).Text_Lambda([Entry](){auto E=Entry();return FText::FromString(E.Place?FString::Printf(TEXT("%d."),E.Place):TEXT("–"));}).Font(FCoreStyle::GetDefaultFontStyle("Bold",19)).ColorAndOpacity_Lambda(Tint)]]
                +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).OverflowPolicy(ETextOverflowPolicy::Ellipsis).Text_Lambda([Entry,OwnId](){auto E=Entry();return FText::FromString(E.Name+(E.EntryId==OwnId?TEXT(" · DU"):TEXT("")));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",18)).ColorAndOpacity_Lambda(Tint)]
                +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(160)[SNew(STextBlock).Justification(ETextJustify::Right).Text_Lambda([Entry](){auto E=Entry();return FText::FromString(E.Place?ABenchyRaceState::FormatTime(E.Time):E.Disconnected?TEXT("ausgeschieden"):TEXT("läuft noch..."));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",18)).ColorAndOpacity_Lambda(Tint)]]]];
    }
    Rows->AddSlot().AutoHeight().Padding(0,16,0,5)[SNew(SButton).ContentPadding(12).ButtonColorAndOpacity(FLinearColor(.05,.18,.20)).OnClicked_Lambda([this](){ShowMenu();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("Menü öffnen"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",18))]];
    if(HasAuthority())Rows->AddSlot().AutoHeight().Padding(0,4)[SNew(SButton).IsEnabled_Lambda([State](){return State->Phase==3;}).ContentPadding(12).ButtonColorAndOpacity(FLinearColor(.05,.28,.24)).OnClicked_Lambda([this](){ServerStartRace();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("Nächstes Rennen starten"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",18))]];
    FVector2D View;GEngine->GameViewport->GetViewportSize(View);float Width=FMath::Clamp(View.X*.35f,440.f,560.f);
    ResultPanel=SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(24)[SNew(SBox).WidthOverride(Width).MaxDesiredHeight(View.Y*.9f)[SNew(SBorder).Padding(24).BorderBackgroundColor(FLinearColor(.008,.018,.03,.96))[SNew(SScrollBox)+SScrollBox::Slot()[Rows]]]];
    GEngine->GameViewport->AddViewportWidgetContent(ResultPanel.ToSharedRef(),90);
    bShowMouseCursor=true;FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(ResultPanel);Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);
}
