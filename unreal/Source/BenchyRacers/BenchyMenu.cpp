#include "BenchyMenu.h"
#include "BenchyFilaments.h"
#include "BenchyAudit.h"
#include "BenchyDisplay.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "BenchyPawn.h"
#include "BenchyWorld.h"
#include "BenchyRace.h"
#include "BenchyGameMode.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Styling/CoreStyle.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Interfaces/IHttpRequest.h"

void UBenchyInstance::Init(){
    Super::Init();GConfig->GetString(TEXT("Benchy"),TEXT("Name"),Nick,GGameUserSettingsIni);GConfig->GetString(TEXT("Benchy"),TEXT("Address"),Address,GGameUserSettingsIni);
    if(Address==TEXT("127.0.0.1"))Address.Reset();
    GConfig->GetFloat(TEXT("Benchy"),TEXT("Music"),MusicVolume,GGameUserSettingsIni);GConfig->GetInt(TEXT("Benchy"),TEXT("Color"),SelectedColor,GGameUserSettingsIni);
    GConfig->GetInt(TEXT("Benchy"),TEXT("Track"),SelectedTrack,GGameUserSettingsIni);SelectedTrack=FMath::Clamp(SelectedTrack,0,2);SelectedColor=FMath::Clamp(SelectedColor,0,BenchyFilaments::Count-1);
    GEngine->OnNetworkFailure().AddUObject(this,&UBenchyInstance::NetworkFailure);GEngine->OnTravelFailure().AddUObject(this,&UBenchyInstance::TravelFailure);
}
void UBenchyInstance::Shutdown(){if(PublicAddressRequest.IsValid()){PublicAddressRequest->OnProcessRequestComplete().Unbind();PublicAddressRequest->CancelRequest();PublicAddressRequest.Reset();}GEngine->OnNetworkFailure().RemoveAll(this);GEngine->OnTravelFailure().RemoveAll(this);Super::Shutdown();}
void UBenchyInstance::SavePreferences(){GConfig->SetString(TEXT("Benchy"),TEXT("Name"),*Nick,GGameUserSettingsIni);GConfig->SetString(TEXT("Benchy"),TEXT("Address"),*Address,GGameUserSettingsIni);GConfig->SetFloat(TEXT("Benchy"),TEXT("Music"),MusicVolume,GGameUserSettingsIni);GConfig->SetInt(TEXT("Benchy"),TEXT("Color"),SelectedColor,GGameUserSettingsIni);GConfig->SetInt(TEXT("Benchy"),TEXT("Track"),SelectedTrack,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);}
void UBenchyInstance::NetworkFailure(UWorld* World,UNetDriver*,ENetworkFailure::Type,const FString& Error){if((FParse::Param(FCommandLine::Get(),TEXT("BenchyNetworkAudit"))||FParse::Param(FCommandLine::Get(),TEXT("BenchyTravelAudit"))||FParse::Param(FCommandLine::Get(),TEXT("BenchyNozzleNetworkAudit")))){FPlatformMisc::RequestExit(false);return;}Notice=TEXT("Verbindung getrennt oder nicht erreichbar: ")+Error.Left(160);if(World)UGameplayStatics::OpenLevel(World,TEXT("/Game/Maps/Sonnendeck"));}
void UBenchyInstance::TravelFailure(UWorld*,ETravelFailure::Type,const FString& Error){Notice=TEXT("Strecke konnte nicht geladen werden: ")+Error.Left(160);}
void ABenchyController::BeginPlay(){
    Super::BeginPlay();PrimaryActorTick.bCanEverTick=true;
    if(!IsLocalController())return;
    auto I=GetGameInstance<UBenchyInstance>();
    if(auto Sound=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/WakeRunner.WakeRunner")))Music=UGameplayStatics::SpawnSound2D(this,Sound,I?I->MusicVolume:.35f);
    FInputModeGameOnly Mode;SetInputMode(Mode);
}
void ABenchyController::SetupInputComponent(){Super::SetupInputComponent();InputComponent->BindKey(EKeys::Escape,IE_Pressed,this,&ABenchyController::ToggleMenu).bExecuteWhenPaused=true;}
void ABenchyController::EndPlay(const EEndPlayReason::Type Reason){HideMenu();Super::EndPlay(Reason);}
void ABenchyController::Tick(float DT){
    Super::Tick(DT);if(IsLocalController())AuditLobby();if(IsLocalController())BenchyValidateClient(this);if(!IsLocalController()||FParse::Param(FCommandLine::Get(),TEXT("BenchyAudit")))return;
    auto S=GetWorld()->GetGameState<ABenchyRaceState>();if(!S)return;
    if(FParse::Param(FCommandLine::Get(),TEXT("BenchyShowroomAudit"))){
        static int Stage=0,Color=0;static float Next=0;static bool Printing=true,Materials=true,Travel=true,Restored=false,Intro=false;
        float T=GetWorld()->GetTimeSeconds();auto I=GetGameInstance<UBenchyInstance>();
        auto Shot=[&](const TCHAR* Name){auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/(FString(Name)+TEXT(".png")),true,false);};
        if(Stage==0&&T>6){ShowMenu();SelectColor(1);Stage=1;Next=T+1.4f;}
        else if(Stage==1&&T>Next){Printing&=Preview&&Preview->PrintProgress()>.2f&&Preview->PrintProgress()<.8f;Shot(TEXT("Menu-Printing"));Stage=2;Next=T+2.2f;}
        else if(Stage==2&&T>Next){Printing&=Preview&&Preview->PrintProgress()==1;Shot(TEXT("Menu"));Stage=3;Next=T+.4f;}
        else if(Stage==3&&T>Next){SelectColor(Color);Stage=4;Next=T+3.2f;}
        else if(Stage==4&&T>Next){auto C=Preview?Preview->FindComponentByClass<UStaticMeshComponent>():nullptr;auto M=C?Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)):nullptr;
            Materials&=Preview&&Preview->SelectedFilament()==Color&&Preview->PrintProgress()==1&&M&&M->K2_GetScalarParameterValue(TEXT("PrintProgress"))==1&&M->K2_GetScalarParameterValue(TEXT("Printed"))==1;
            if(Color==3){auto Tint=M?M->K2_GetVectorParameterValue(TEXT("FilamentColor")):FLinearColor::Black;Materials&=M&&Tint.R<.85&&Tint.G>.4&&Tint.G<.5&&M->K2_GetScalarParameterValue(TEXT("Glow"))==0;Shot(TEXT("Menu-Goldgelb"));}
            if(Color==4){Materials&=M&&M->K2_GetScalarParameterValue(TEXT("Galaxy"))==1;Shot(TEXT("Menu-Galaxy"));}
            if(Color==5){Materials&=M&&M->K2_GetScalarParameterValue(TEXT("Glow"))==1;Shot(TEXT("Menu-Glow"));}
            if(Color==6){auto Tint=M?M->K2_GetVectorParameterValue(TEXT("GlowColor")):FLinearColor::Black;Materials&=M&&M->K2_GetScalarParameterValue(TEXT("Glow"))==1&&Tint.R>.9&&Tint.G>.7&&Tint.B<.05;Shot(TEXT("Menu-NeonGelb"));}
            Color++;Stage=Color<BenchyFilaments::Count?3:5;Next=T+.4f;}
        else if(Stage==5&&T>Next){ShowMenu(1);Stage=6;Next=T+.8f;}
        else if(Stage==6&&T>Next){Shot(TEXT("Graphics"));Stage=7;Next=T+.4f;}
        else if(Stage==7&&T>Next){Stage=8;SelectTrack(1);}
        else if(Stage==8&&T>6){Travel&=ABenchyWorld::ActiveTrack==1&&I->SelectedTrack==1&&Preview;Shot(TEXT("Menu-Harbor"));Stage=9;Next=T+.5f;}
        else if(Stage==9&&T>Next){Stage=10;SelectTrack(2);}
        else if(Stage==10&&T>6){Travel&=ABenchyWorld::ActiveTrack==2&&I->SelectedTrack==2&&Preview;Shot(TEXT("Menu-River"));Stage=11;Next=T+.5f;}
        else if(Stage==11&&T>Next){HideMenu();Restored=GetViewTarget()==GetPawn()&&!Preview;ShowMenu();ServerStartRace();Stage=12;}
        else if(Stage==12&&S->Phase==1&&S->Clock()>-3.8f){auto B=Cast<ABenchyPawn>(GetPawn());auto C=B?B->FindComponentByClass<UStaticMeshComponent>():nullptr;auto M=C?Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)):nullptr;
            Intro=M&&M->K2_GetScalarParameterValue(TEXT("PrintProgress"))>0&&M->K2_GetScalarParameterValue(TEXT("PrintProgress"))<1;Shot(TEXT("Race-Printing"));Stage=13;}
        else if(Stage==13&&S->Phase==2&&S->Clock()>1){auto B=Cast<ABenchyPawn>(GetPawn());auto C=B?B->FindComponentByClass<UStaticMeshComponent>():nullptr;auto M=C?Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)):nullptr;
            Intro&=M&&M->K2_GetScalarParameterValue(TEXT("PrintProgress"))==1&&GetViewTarget()==GetPawn()&&!IntroCamera&&!MenuVisible();
            auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"preview_print_animation\":%s,\"seven_filaments\":%s,\"actual_map_background_travel\":%s,\"camera_restored\":%s,\"race_print_intro\":%s}"),Printing&&Materials&&Travel&&Restored&&Intro?TEXT("true"):TEXT("false"),Printing?TEXT("true"):TEXT("false"),Materials?TEXT("true"):TEXT("false"),Travel?TEXT("true"):TEXT("false"),Restored?TEXT("true"):TEXT("false"),Intro?TEXT("true"):TEXT("false")),*(Dir/TEXT("Showroom.json")));Stage=14;FPlatformMisc::RequestExit(false);}
        if(T>90&&Stage!=14){FPlatformMisc::RequestExit(false);}
    }
    if(S->Phase==1){int Count=FMath::CeilToInt(-S->Clock());if(Count!=LastCountdown){LastCountdown=Count;if(auto B=Cast<ABenchyPawn>(GetPawn()))B->PlayCue_Implementation(4);}}
    if(GetPawn() && NamedPawn.Get()!=GetPawn()){NamedPawn=GetPawn();if(auto I=GetGameInstance<UBenchyInstance>()){I->SelectedTrack=ABenchyWorld::ActiveTrack;ServerIdentity(I->Nick,I->SelectedColor);}}
    if(FParse::Param(FCommandLine::Get(),TEXT("BenchyRaceAudit"))||FParse::Param(FCommandLine::Get(),TEXT("BenchyGameplayAudit"))||FParse::Param(FCommandLine::Get(),TEXT("BenchyItemAudit"))||FParse::Param(FCommandLine::Get(),TEXT("BenchyNavigationAudit"))||FParse::Param(FCommandLine::Get(),TEXT("BenchyNozzleAudit"))||FParse::Param(FCommandLine::Get(),TEXT("BenchyFilamentAudit"))||FParse::Param(FCommandLine::Get(),TEXT("BenchyNozzleNetworkAudit")))return;
    if(LastPhase!=S->Phase){LastPhase=S->Phase;if(S->Phase==0)ShowMenu();else HideMenu();}
    if(auto B=Cast<ABenchyPawn>(GetPawn())){
        if(B->Finished&&!MenuVisible())ShowResults();else if(!B->Finished)HideResults();
    }
    if(S->Phase==1&&!MenuVisible()&&GetPawn()){
        if(!IntroCamera){IntroCamera=GetWorld()->SpawnActor<ACameraActor>();IntroCamera->GetCameraComponent()->SetFieldOfView(48);IntroCamera->GetCameraComponent()->PostProcessSettings.bOverride_MotionBlurAmount=true;IntroCamera->GetCameraComponent()->PostProcessSettings.MotionBlurAmount=0;IntroReturning=false;SetViewTargetWithBlend(IntroCamera,.2f);}
        auto B=GetPawn();FVector At=B->GetActorLocation()-B->GetActorForwardVector()*650+B->GetActorRightVector()*500+FVector(0,0,340);
        IntroCamera->SetActorLocationAndRotation(At,(B->GetActorLocation()+FVector(0,0,50)-At).Rotation());
        if(S->Clock()>-1.5f&&!IntroReturning){SetViewTargetWithBlend(B,.7f);IntroReturning=true;}
    }else if(IntroCamera){if(GetPawn())SetViewTarget(GetPawn());IntroCamera->Destroy();IntroCamera=nullptr;}

}
void ABenchyController::HideMenu(){HideResults();if(Preview){if(GetPawn())SetViewTarget(GetPawn());Preview->Destroy();Preview=nullptr;}if(IsLocalController()&&GetNetMode()==NM_Standalone)UGameplayStatics::SetGamePaused(this,false);if(Panel.IsValid()&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Panel.ToSharedRef());Panel.Reset();AddressBox.Reset();NameBox.Reset();if(IsLocalController()){bShowMouseCursor=false;SetInputMode(FInputModeGameOnly());}}
void ABenchyController::ToggleMenu(){if(Panel.IsValid())HideMenu();else ShowMenu();}
void ABenchyController::ServerStartRace_Implementation(){if(IsLocalController())if(auto M=GetWorld()->GetAuthGameMode<ABenchyGameMode>())M->StartRace();}
void ABenchyController::ServerNewTrack_Implementation(int32 Track){auto S=GetWorld()->GetGameState<ABenchyRaceState>();if(IsLocalController()&&S&&(S->Phase==0||S->Phase==3)&&Track>=0&&Track<3)GetWorld()->ServerTravel(FString::Printf(TEXT("/Game/Maps/Sonnendeck?Track=%d?listen"),Track));}
void ABenchyController::StoreFields(){auto I=GetGameInstance<UBenchyInstance>();if(!I)return;if(NameBox.IsValid()){I->Nick=NameBox->GetText().ToString().Left(24);for(TCHAR& C:I->Nick)if(!FChar::IsAlnum(C)&&C!=' '&&C!='_')C='_';if(I->Nick.IsEmpty())I->Nick=TEXT("Maker");}if(AddressBox.IsValid())I->Address=AddressBox->GetText().ToString().TrimStartAndEnd();I->SavePreferences();}
void ABenchyController::ShowMenu(int32 Page){
    if(!IsLocalController()||!GEngine->GameViewport)return;
    auto ExistingPreview=Preview;Preview=nullptr;HideMenu();Preview=ExistingPreview;
    if(auto B=Cast<ABenchyPawn>(GetPawn()))B->ServerControls(0,0,false);
    auto I=GetGameInstance<UBenchyInstance>();auto Settings=UGameUserSettings::GetGameUserSettings();if(!I)return;
    TSharedRef<SVerticalBox> List=SNew(SVerticalBox);
    auto Label=[&](const FString& T,int32 Size=16){List->AddSlot().AutoHeight().Padding(0,5)[SNew(STextBlock).Text(FText::FromString(T)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).ColorAndOpacity(FLinearColor(.85,.94,.95)).WrapTextAt(365)];};
    auto Button=[&](const FString& T,TFunction<void()> Fn){List->AddSlot().AutoHeight().Padding(0,4)[SNew(SButton).ContentPadding(FMargin(14,8)).ButtonColorAndOpacity(FLinearColor(.055,.19,.20)).OnClicked_Lambda([Fn](){Fn();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(T)).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Bold",15)).ColorAndOpacity(FLinearColor::White)]];};
    if(Page==0&&GetNetMode()==NM_ListenServer){Label(TEXT("BENCHY RACERS"),25);Label(TEXT("Dein Boot. Dein Rennen."),14);}else{Label(TEXT("BENCHY"),36);Label(TEXT("RACERS"),24);Label(Page==1?TEXT("Grafik & Ton"):TEXT("Dein Boot. Dein Rennen."),16);}
    if(Page==1){
        static const TCHAR* Q[]={TEXT("Niedrig"),TEXT("Mittel"),TEXT("Hoch"),TEXT("Ultra")};int Quality=FMath::Clamp(Settings->GetOverallScalabilityLevel(),0,3);
        Button(FString(TEXT("Grafik: "))+(Settings->GetOverallScalabilityLevel()<0?TEXT("Benutzerdefiniert"):Q[Quality]),[this,Settings,Quality](){Settings->SetOverallScalabilityLevel((Quality+1)%4);Settings->ApplySettings(false);ShowMenu(1);});
        FIntPoint Size=Settings->GetScreenResolution();Button(FString::Printf(TEXT("Aufloesung: %d x %d"),Size.X,Size.Y),[this,Settings,Size](){FIntPoint Next=Size.X<1600?FIntPoint(1600,900):Size.X<1920?FIntPoint(1920,1080):Size.X<2560?FIntPoint(2560,1440):FIntPoint(1280,720);Settings->SetScreenResolution(Next);Settings->ApplySettings(false);ShowMenu(1);});
        Button(Settings->GetFullscreenMode()==EWindowMode::Windowed?TEXT("Anzeige: Fenster"):TEXT("Anzeige: Randloses Vollbild"),[this,Settings](){Settings->SetFullscreenMode(Settings->GetFullscreenMode()==EWindowMode::Windowed?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);Settings->ApplySettings(false);ShowMenu(1);});
        Button(Settings->IsVSyncEnabled()?TEXT("VSync: An"):TEXT("VSync: Aus"),[this,Settings](){Settings->SetVSyncEnabled(!Settings->IsVSyncEnabled());Settings->ApplySettings(false);ShowMenu(1);});
        Button(FString::Printf(TEXT("Bildlimit: %.0f FPS (0 = offen)"),Settings->GetFrameRateLimit()),[this,Settings](){float V=Settings->GetFrameRateLimit();Settings->SetFrameRateLimit(V<1?60:V<61?120:0);Settings->ApplySettings(false);ShowMenu(1);});
        Label(TEXT("Musiklautstaerke"));List->AddSlot().AutoHeight().Padding(0,10)[SNew(SSlider).Value(I->MusicVolume).OnValueChanged_Lambda([this,I](float V){I->MusicVolume=V;if(Music)Music->SetVolumeMultiplier(V);I->SavePreferences();})];
        Button(TEXT("Zurueck"),[this](){ShowMenu();});
    }else{
        if(!I->Notice.IsEmpty())Label(I->Notice,14);
        if(GetNetMode()==NM_ListenServer){I->RefreshLobbyAddresses(GetWorld());List->AddSlot().AutoHeight().Padding(0,5)[BuildLobbyInvite()];}
        Label(TEXT("Fahrername"),14);List->AddSlot().AutoHeight()[SAssignNew(NameBox,SEditableTextBox).Text(FText::FromString(I->Nick))];
        Label(TEXT("WELT WÄHLEN"),12);
        for(int Track=0;Track<3;Track++)Button((I->SelectedTrack==Track?TEXT("●  "):TEXT("○  "))+ABenchyWorld::TrackName(Track),[this,Track](){SelectTrack(Track);});
        Label(TEXT("FILAMENT WÄHLEN"),12);
        const FLinearColor Swatches[]={FLinearColor(.06,.08,.09),FLinearColor(1,.3,.025),FLinearColor(.01,.65,.83),FLinearColor(.8,.43,.025),FLinearColor(.22,.2,.31),FLinearColor(.54,.85,.38),FLinearColor(1,.95,.015)};
        TSharedRef<SHorizontalBox> Palette=SNew(SHorizontalBox);
        for(int Color=0;Color<BenchyFilaments::Count;Color++)Palette->AddSlot().FillWidth(1).Padding(2)[SNew(SButton).ButtonColorAndOpacity(Swatches[Color]).ContentPadding(FMargin(1,12))
            .ToolTipText(FText::FromString(BenchyFilaments::Name(Color)))
            .OnClicked_Lambda([this,Color](){SelectColor(Color);return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(I->SelectedColor==Color?TEXT("✓"):Color==4?TEXT("✦"):BenchyFilaments::IsGlow(Color)?TEXT("G"):TEXT("●"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",19)).ColorAndOpacity(FLinearColor::White)]];
        List->AddSlot().AutoHeight()[Palette];
        if(auto B=Cast<ABenchyPawn>(GetPawn()))Label(B->FilamentName(),15);
        auto State=GetWorld()->GetGameState<ABenchyRaceState>();bool Active=State&&State->Phase>0&&State->Phase<3;
        if(Active){Button(TEXT("Weiterfahren"),[this](){StoreFields();HideMenu();});Button(TEXT("Zum letzten Kontrollpunkt"),[this](){if(auto B=Cast<ABenchyPawn>(GetPawn()))B->ResetBoat();HideMenu();});}
        if(GetNetMode()==NM_Standalone) {
            Button(TEXT("Einzelrennen mit Gegnern starten"),[this,I](){StoreFields();UGameplayStatics::OpenLevel(this,TEXT("/Game/Maps/Sonnendeck"),true,FString::Printf(TEXT("Track=%d?AutoStart=1?Name=%s"),I->SelectedTrack,*I->Nick));});
            Button(TEXT("Online-Lobby erstellen"),[this,I](){StoreFields();UGameplayStatics::OpenLevel(this,TEXT("/Game/Maps/Sonnendeck"),true,FString::Printf(TEXT("listen?Track=%d?Name=%s"),I->SelectedTrack,*I->Nick));});
            Label(TEXT("Beitreten: Adresse vom Gastgeber"),14);List->AddSlot().AutoHeight()[SAssignNew(AddressBox,SEditableTextBox).Text(FText::FromString(I->Address)).HintText(FText::FromString(TEXT("IP-Adresse oder Hostname:7777")))];
            Button(TEXT("Mit Freunden verbinden"),[this,I](){StoreFields();bool Valid=I->Address.Len()>0&&I->Address.Len()<254;for(TCHAR C:I->Address)Valid&=FChar::IsAlnum(C)||C=='.'||C=='-'||C==':';if(!Valid){I->Notice=TEXT("Bitte eine IP-Adresse oder einen Hostnamen eingeben.");ShowMenu();return;}I->Notice=TEXT("Verbindung wird aufgebaut ...");ClientTravel(I->Address+TEXT("?Name=")+I->Nick,TRAVEL_Absolute);});
        }else{
            List->AddSlot().AutoHeight().Padding(0,5)[SNew(STextBlock).Text_Lambda([this](){TArray<FString> Names;for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(!It->IsBot)Names.Add(It->RacerName);return FText::FromString(TEXT("Fahrer: ")+FString::Join(Names,TEXT(", ")));}).WrapTextAt(365).ColorAndOpacity(FLinearColor(.5,.9,.8))];
            Label(GetNetMode()==NM_ListenServer?TEXT("Lobby offen · Bis zu 8 Freunde"):TEXT("Verbunden. Der Host startet das Rennen."),15);
            if(HasAuthority()&&!Active){
                Button(State&&State->Phase==3?TEXT("Neues Rennen starten"):TEXT("Rennen gemeinsam starten"),[this](){StoreFields();ServerStartRace();});
                Button(TEXT("Karte für alle wechseln"),[this,I](){StoreFields();ServerNewTrack(I->SelectedTrack);});
            }
            Button(TEXT("Lobby verlassen"),[this](){UGameplayStatics::OpenLevel(this,TEXT("/Game/Maps/Sonnendeck"));});
        }
        Button(TEXT("Grafik & Ton"),[this](){StoreFields();ShowMenu(1);});
        Button(TEXT("Spiel beenden"),[this](){StoreFields();UKismetSystemLibrary::QuitGame(this,this,EQuitPreference::Quit,false);});
        Label(TEXT("WASD fahren · Leertaste Item · R zurücksetzen"),12);
    }
    auto State=GetWorld()->GetGameState<ABenchyRaceState>();
    const bool Showroom=State&&(State->Phase==0||State->Phase==3);
    if(Showroom&&!Preview){FVector At(27000,-34000,21500);
        Preview=GetWorld()->SpawnActor<ABenchyDisplay>(At,FRotator(0,77,0));Preview->SelectFilament(I->SelectedColor);
        auto C=Preview->FindComponentByClass<UCameraComponent>();if(C)Preview->SetActorRotation(FRotator(0,(-At).Rotation().Yaw-C->GetRelativeRotation().Yaw,0));}
    if(Preview)SetViewTarget(Preview);
    FVector2D View(1600,900);GEngine->GameViewport->GetViewportSize(View);float Width=FMath::Clamp(View.X*.28f,330.f,420.f);
    TSharedRef<SWidget> Detail=SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("DEIN BENCHY"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",24)).ColorAndOpacity(FLinearColor::White)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,6)[SNew(STextBlock).Text_Lambda([this](){auto B=Cast<ABenchyPawn>(GetPawn());return FText::FromString(B?B->FilamentName():TEXT(""));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",18)).ColorAndOpacity(FLinearColor(.65,.96,.89))]
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(Preview&&Preview->PrintProgress()<1?FString::Printf(TEXT("Wird gedruckt · %.0f %%"),Preview->PrintProgress()*100):TEXT("Bereit fürs Wasser"));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",15)).ColorAndOpacity(FLinearColor(.85,.9,.94))];
    Panel=SNew(SOverlay)
        +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill).Padding(22)[SNew(SBox).WidthOverride(Width)[SNew(SBorder).Padding(22).BorderBackgroundColor(FLinearColor(.012,.025,.035,.96))[SNew(SScrollBox)+SScrollBox::Slot()[List]]]]
        +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(35,30)
        [SNew(STextBlock).Visibility(Showroom?EVisibility::HitTestInvisible:EVisibility::Collapsed).Text(FText::FromString(TEXT("KARTENÜBERBLICK · ")+ABenchyWorld::TrackName(ABenchyWorld::ActiveTrack))).Font(FCoreStyle::GetDefaultFontStyle("Bold",19)).ColorAndOpacity(FLinearColor::White)]
        +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(35,25)
        [SNew(SBox).WidthOverride(370.f)[SNew(SBorder).Visibility(Showroom?EVisibility::HitTestInvisible:EVisibility::Collapsed).Padding(20).BorderBackgroundColor(FLinearColor(.012,.025,.035,.94))[Detail]]];
    if(GetNetMode()==NM_Standalone){if(State&&State->Phase==2)if(auto B=Cast<ABenchyPawn>(GetPawn()))if(!B->Finished)UGameplayStatics::SetGamePaused(this,true);}
    GEngine->GameViewport->AddViewportWidgetContent(Panel.ToSharedRef(),100);bShowMouseCursor=true;FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(Panel);Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);
}

void ABenchyController::ServerIdentity_Implementation(const FString& Name,int32 Color){if(auto B=Cast<ABenchyPawn>(GetPawn())){FString Safe=Name.Left(24);for(TCHAR& C:Safe)if(!FChar::IsAlnum(C)&&C!=' '&&C!='_')C='_';B->RacerName=Safe.IsEmpty()?TEXT("Maker"):Safe;B->ServerColor_Implementation(Color);}}

void ABenchyController::SelectColor(int32 Color){
    StoreFields();auto I=GetGameInstance<UBenchyInstance>();if(!I)return;I->SelectedColor=FMath::Clamp(Color,0,BenchyFilaments::Count-1);I->SavePreferences();
    if(auto B=Cast<ABenchyPawn>(GetPawn())){B->ColorIndex=I->SelectedColor;B->OnRep_Color();B->ServerColor(I->SelectedColor);}
    if(Preview)Preview->SelectFilament(I->SelectedColor);ShowMenu();
}
void ABenchyController::SelectTrack(int32 Track){
    StoreFields();auto I=GetGameInstance<UBenchyInstance>();if(!I)return;
    if(GetNetMode()==NM_Client){I->Notice=TEXT("In der Online-Lobby wählt der Host die Karte.");I->SelectedTrack=ABenchyWorld::ActiveTrack;ShowMenu();return;}
    I->SelectedTrack=FMath::Clamp(Track,0,2);I->SavePreferences();
    auto S=GetWorld()->GetGameState<ABenchyRaceState>();
    if(GetNetMode()==NM_Standalone&&S&&(S->Phase==0||S->Phase==3)&&ABenchyWorld::ActiveTrack!=I->SelectedTrack)
        UGameplayStatics::OpenLevel(this,TEXT("/Game/Maps/Sonnendeck"),true,FString::Printf(TEXT("Track=%d?Name=%s"),I->SelectedTrack,*I->Nick));
    else if(GetNetMode()==NM_ListenServer&&S&&(S->Phase==0||S->Phase==3)&&ABenchyWorld::ActiveTrack!=I->SelectedTrack)ServerNewTrack(I->SelectedTrack);
    else ShowMenu();
}
