#include "BenchyGameMode.h"
#include "BenchyFilaments.h"
#include "BenchyPawn.h"
#include "BenchyWorld.h"
#include "BenchyRace.h"
#include "BenchyMenu.h"
#include "BenchyOnline.h"
#include "BenchyAudit.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
ABenchyGameMode::ABenchyGameMode(){
    DefaultPawnClass=nullptr;HUDClass=ABenchyHUD::StaticClass();PlayerControllerClass=ABenchyController::StaticClass();GameStateClass=ABenchyRaceState::StaticClass();PrimaryActorTick.bCanEverTick=true;
}
void ABenchyGameMode::InitGame(const FString& Map,const FString& Options,FString& Error){
    Super::InitGame(Map,Options,Error);int Track=UGameplayStatics::GetIntOption(Options,TEXT("Track"),GetGameInstance<UBenchyInstance>()?GetGameInstance<UBenchyInstance>()->SelectedTrack:0);FParse::Value(FCommandLine::Get(),TEXT("BenchyTrack="),Track);ABenchyWorld::ActiveTrack=FMath::Clamp(Track,0,2);
}
void ABenchyGameMode::PreLogin(const FString& Options,const FString& Address,const FUniqueNetIdRepl& Id,FString& Error){
    Super::PreLogin(Options,Address,Id,Error);auto S=GetGameState<ABenchyRaceState>();if(GetNumPlayers()>=8)Error=TEXT("Die Lobby ist voll (8 Fahrer).");
    auto I=GetGameInstance<UBenchyInstance>();if(I&&I->Online&&I->Online->Hosting&&!I->Online->RoomCode.IsEmpty()&&UGameplayStatics::ParseOption(Options,TEXT("RoomCode"))!=I->Online->RoomCode)Error=TEXT("Der Lobby-Code stimmt nicht überein.");
}
void ABenchyGameMode::RestartPlayer(AController* Player){
    if(!Player||Player->GetPawn())return;int32 Slot=0;for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)Slot++;
    float Index=FParse::Param(FCommandLine::Get(),TEXT("BenchyWaterfallAudit"))?60:0;
    FVector P=ABenchyWorld::RoutePoint(Index),Dir=(ABenchyWorld::RoutePoint(Index+1)-P).GetSafeNormal();
    P+=FVector(-Dir.Y,Dir.X,0)*((Slot%2)*2-1)*320-Dir*(Slot/2)*470;
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto B=GetWorld()->SpawnActor<ABenchyPawn>(P+FVector(0,0,150),Dir.Rotation(),Params);Player->Possess(B);
    auto Race=GetGameState<ABenchyRaceState>();B->WaitingForRace=Race&&(Race->Phase==1||Race->Phase==2);B->LobbyHost=GetNetMode()==NM_ListenServer&&Player->IsLocalController();
    B->RacerName=Player->PlayerState?Player->PlayerState->GetPlayerName():TEXT("Maker");if(Index>0){B->Passed=3;B->NextGate=4;}
}
void ABenchyGameMode::Logout(AController* Player){if(Player&&Player->GetPawn()){if(auto State=GetGameState<ABenchyRaceState>())if(auto Boat=Cast<ABenchyPawn>(Player->GetPawn()))State->AddLobbyLine(Boat->RacerName+TEXT(" hat die Lobby verlassen."));Player->GetPawn()->Destroy();}Super::Logout(Player);}
void ABenchyGameMode::BeginPlay(){
    Super::BeginPlay();if(auto Instance=GetGameInstance<UBenchyInstance>())if(auto State=GetGameState<ABenchyRaceState>())State->LobbyChat=Instance->CarriedChat;GetWorld()->SpawnActor<ABenchyWorld>();
    for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It)RestartPlayer(It->Get());
    const bool Audit=FParse::Param(FCommandLine::Get(),TEXT("BenchyAudit"));
    if(Audit){auto S=GetGameState<ABenchyRaceState>();S->Phase=2;S->StartTime=0;}
    else {
        for(int32 R=10;R<320;R+=10){FVector P=ABenchyWorld::RoutePoint(R);auto Item=GetWorld()->SpawnActorDeferred<ABenchyPickup>(ABenchyPickup::StaticClass(),FTransform(P+FVector(0,0,150)));Item->Kind=(R%40==10)?1:0;UGameplayStatics::FinishSpawningActor(Item,FTransform(P+FVector(0,0,150)));}
        if(UGameplayStatics::GetIntOption(OptionsString,TEXT("AutoStart"),0)){FTimerHandle H;GetWorld()->GetTimerManager().SetTimer(H,this,&ABenchyGameMode::StartRace,2.f,false);}
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("BenchySceneryAudit")))if(auto PC=GetWorld()->GetFirstPlayerController()){
        FVector Position(27000,-34000,21500);auto Overview=GetWorld()->SpawnActor<ACameraActor>(Position,(-Position).Rotation());auto C=Overview->GetCameraComponent();C->SetFieldOfView(65);C->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;C->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;C->PostProcessSettings.AutoExposureMinBrightness=10;C->PostProcessSettings.AutoExposureMaxBrightness=14;PC->SetViewTarget(Overview);if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
    }
    if(GetNetMode()!=NM_DedicatedServer){auto S=UGameUserSettings::GetGameUserSettings();if(Audit)S->SetFullscreenMode(EWindowMode::Windowed);S->ApplySettings(true);}
}
void ABenchyGameMode::StartRace(){
    auto S=GetGameState<ABenchyRaceState>();if(!S||S->Phase==1||S->Phase==2)return;
    S->Results.Reset();
    for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(It->IsBot)It->Destroy();
    for(TActorIterator<ABenchyProjectile> It(GetWorld());It;++It)It->Destroy();
    int Slot=0;for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(!It->IsActorBeingDestroyed())It->BeginRace(Slot++);
    static const TCHAR* Names[]={TEXT("Layer Larry"),TEXT("Nozzle Nova"),TEXT("Captain PETG"),TEXT("Speedy Spool"),TEXT("Aqua ASA")};
    for(int I=0;Slot<6;I++,Slot++){
        FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto B=GetWorld()->SpawnActor<ABenchyPawn>(ABenchyWorld::RoutePoint(0)+FVector(0,0,150),FRotator::ZeroRotator,P);B->IsBot=true;B->RacerName=Names[I%5];B->ServerColor_Implementation((I+1)%BenchyFilaments::Count);B->BeginRace(Slot);
    }
    for(TActorIterator<ABenchyPickup> It(GetWorld());It;++It){It->Available=true;It->SetActorHiddenInGame(false);It->ForceNetUpdate();}
    S->RegisterRacers();S->Phase=1;S->StartTime=GetWorld()->GetTimeSeconds()+5;S->FirstFinishTime=0;S->ForceNetUpdate();
}
void ABenchyGameMode::Tick(float DT){
    Super::Tick(DT);BenchyValidateGame(this);auto S=GetGameState<ABenchyRaceState>();if(!S)return;
    if(S->Phase==1&&S->Clock()>=0)S->Phase=2;
    if(S->Phase==2&&!FParse::Param(FCommandLine::Get(),TEXT("BenchyAudit"))){
        S->UpdateResults();
        // Every participant gets to finish. A first finish never starts a cutoff timer.
        if(S->EveryoneDone()){S->Phase=3;S->ForceNetUpdate();}
    }
}
void ABenchyHUD::DrawHUD(){
    if(FParse::Param(FCommandLine::Get(),TEXT("BenchyWorld15Audit")))return;
    Super::DrawHUD();if(!Canvas||!PlayerOwner)return;if(auto PC=Cast<ABenchyController>(PlayerOwner))if(PC->MenuVisible())return;auto B=Cast<ABenchyPawn>(PlayerOwner->GetPawn());if(!B)return;
    auto S=GetWorld()->GetGameState<ABenchyRaceState>();float T=S?S->GetServerWorldTimeSeconds():0;
    if(B->Finished)return; // The local live result window takes over after the finish.
    if(S&&S->Phase==1){
        if(S->Clock()<-2.2f){DrawRect(FLinearColor(.01,.025,.035,.8),Canvas->SizeX*.28,Canvas->SizeY*.12,Canvas->SizeX*.44,70);DrawText(TEXT("DEIN BENCHY WIRD GEDRUCKT"),FLinearColor(.5,1,.85),Canvas->SizeX*.31,Canvas->SizeY*.14,nullptr,1.8);}
        else DrawText(FString::Printf(TEXT("%d"),FMath::CeilToInt(-S->Clock())),FLinearColor(1,.8,.15),Canvas->SizeX*.49,Canvas->SizeY*.38,nullptr,6);
        return;
    }
    TArray<ABenchyPawn*> Racers;for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)Racers.Add(*It);Racers.Sort([](const ABenchyPawn& A,const ABenchyPawn& Other){return A.Progress()>Other.Progress();});
    DrawRect(FLinearColor(.012,.03,.04,.85),24,24,440,139);
    DrawText(TEXT("BENCHY RACERS | ")+ABenchyWorld::TrackName(ABenchyWorld::ActiveTrack),FLinearColor::White,40,38,nullptr,1.25);
    DrawText(FString::Printf(TEXT("Runde %d / 3     Platz %d / %d     %.1f s"),B->Lap,Racers.IndexOfByKey(B)+1,Racers.Num(),B->RaceTime),FLinearColor(.4,.9,.9),40,70);
    DrawText(FString::Printf(TEXT("Makercoins %d / 10"),B->Coins),FLinearColor(1,.78,.25),40,96);
    DrawText(B->FilamentName(),FLinearColor::White,40,126);
    DrawText(FString::Printf(TEXT("%.0f km/h"),B->SpeedKmh()),FLinearColor::White,Canvas->SizeX-190,Canvas->SizeY-100,nullptr,2);
    DrawText(TEXT("WASD fahren | SHIFT Boost | LEERTASTE Item | R letzter Kontrollpunkt | G Grafik | ESC Menue"),FLinearColor::White,24,Canvas->SizeY-30);
    if(S&&S->Phase==1&&S->Clock()<-2.2f)DrawText(TEXT("DEIN BENCHY WIRD GEDRUCKT"),FLinearColor(.5,1,.85),Canvas->SizeX*.31,Canvas->SizeY*.2,nullptr,1.8);
    if(S&&S->Phase==1)DrawText(FString::Printf(TEXT("%d"),FMath::CeilToInt(-S->Clock())),FLinearColor(1,.8,.15),Canvas->SizeX*.49,Canvas->SizeY*.38,nullptr,6);
    if(T<B->RespawnUntil)DrawText(TEXT("Zurueck auf der Strecke"),FLinearColor(.3,1,.8),Canvas->SizeX*.39,Canvas->SizeY*.75,nullptr,1.5);
    else if(T<B->StunUntil)DrawText(TEXT("Treffer!"),FLinearColor(1,.4,.2),Canvas->SizeX*.46,Canvas->SizeY*.70,nullptr,2);
    DrawItemPanel(B,T);
    if(S&&S->Phase>0&&S->Phase<3&&!B->Finished)DrawNavigation(B);
    // A compact course map supplements the physical signs on open water.
    const FVector2D MapCenter(Canvas->SizeX-125,125);const float MapScale=.003;
    DrawRect(FLinearColor(.012,.03,.04,.72),Canvas->SizeX-238,24,214,214);
    for(int K=0;K<320;K+=4){auto A=ABenchyWorld::RoutePoint(K),D=ABenchyWorld::RoutePoint(K+4);DrawLine(MapCenter.X+A.X*MapScale,MapCenter.Y-A.Y*MapScale,MapCenter.X+D.X*MapScale,MapCenter.Y-D.Y*MapScale,FLinearColor(.35,.7,.68),2);}
    for(auto R:Racers){auto P=R->GetActorLocation();DrawRect(R==B?FLinearColor(1,.8,.1):FLinearColor(.9,.95,.95),MapCenter.X+P.X*MapScale-3,MapCenter.Y-P.Y*MapScale-3,6,6);}
    if(!B->Finished){auto P=ABenchyWorld::RoutePoint(B->NextGate*20.f);float X=MapCenter.X+P.X*MapScale,Y=MapCenter.Y-P.Y*MapScale;
        for(int I=0;I<24;I++){float A=I*PI/12,D=(I+1)*PI/12;DrawLine(X+FMath::Cos(A)*7,Y+FMath::Sin(A)*7,X+FMath::Cos(D)*7,Y+FMath::Sin(D)*7,B->MissedCheckpoint?FLinearColor(1,.25,.08):FLinearColor(1,.85,.25),2);}}
    if(B->Finished)DrawText(FString::Printf(TEXT("ZIEL! Platz %d  |  %.1f s"),B->FinishPlace,B->RaceTime),FLinearColor(1,.8,.2),Canvas->SizeX*.32,Canvas->SizeY*.29,nullptr,2.5);
    if(S&&S->Phase==3){float Y=190;for(auto R:Racers){DrawRect(FLinearColor(.01,.025,.035,.85),24,Y-3,400,29);DrawText(FString::Printf(TEXT("%d. %s   %s"),Racers.IndexOfByKey(R)+1,*R->RacerName,R->Finished?*FString::Printf(TEXT("%.1f s"),R->RaceTime):TEXT("nicht im Ziel")),FLinearColor::White,40,Y);Y+=30;}}
}
