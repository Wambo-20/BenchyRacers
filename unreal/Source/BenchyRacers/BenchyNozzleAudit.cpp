#include "BenchyNozzleAudit.h"
#include "BenchyGameMode.h"
#include "BenchyMenu.h"
#include "BenchyPawn.h"
#include "BenchyRace.h"
#include "BenchyWorld.h"
#include "CourseNavigation.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
namespace {
bool Flag(const TCHAR* Name){return FParse::Param(FCommandLine::Get(),Name);}
void Report(const TCHAR* Name,bool Pass,const FString& Fields){
    auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);
    FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,%s}"),Pass?TEXT("true"):TEXT("false"),*Fields),*(Dir/(FString(Name)+TEXT(".json"))));
    UE_LOG(LogTemp,Display,TEXT("BENCHY_NOZZLE_VALIDATION %s passed=%d %s"),Name,Pass,*Fields);
}
void Shot(const TCHAR* Name){auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/(FString(Name)+TEXT(".png")),false,false);}
void Place(ABenchyPawn* B,float Index,int Passed){
    FVector P=ABenchyWorld::RoutePoint(Index);auto D=BenchyCourse::Direction(ABenchyWorld::ActiveTrack,Index);
    P.Z=ABenchyWorld::WaterHeight(P,B->GetWorld()->GetTimeSeconds())+70;
    B->AuditPose(P,FVector(D.x,D.y,0).Rotation(),Passed);B->IsBot=false;B->RespawnUntil=0;B->StunUntil=0;B->ShieldUntil=0;B->Item=0;
}
ABenchyPawn* Boat(UWorld* W,const TCHAR* Name,float Index,int Passed){
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto B=W->SpawnActor<ABenchyPawn>(ABenchyWorld::RoutePoint(Index)+FVector(0,0,70),FRotator::ZeroRotator,Params);
    B->RacerName=Name;Place(B,Index,Passed);B->Coins=10;return B;
}
int Projectiles(UWorld* W){int N=0;for(TActorIterator<ABenchyProjectile> It(W);It;++It)N++;return N;}
bool Locked(UWorld* W,ABenchyPawn* B){for(TActorIterator<ABenchyProjectile> It(W);It;++It)if(It->Guided&&It->Target==B)return true;return false;}
UMaterialInstanceDynamic* Filament(ABenchyPawn* B){auto C=B->FindComponentByClass<UStaticMeshComponent>();return C?Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)):nullptr;}
}
void BenchyValidateNozzles(ABenchyGameMode* Mode){
    auto W=Mode->GetWorld();auto S=W->GetGameState<ABenchyRaceState>();if(!S||!W->GetFirstPlayerController())return;
    const float T=W->GetTimeSeconds();auto B=Cast<ABenchyPawn>(W->GetFirstPlayerController()->GetPawn());if(!B)return;
    if(Flag(TEXT("BenchyNozzleAudit"))){
        static int Stage=0;static float Next=0;static ABenchyPawn* Victim=nullptr;static ABenchyPawn* Last=nullptr;static AActor* Wall=nullptr;
        static bool Moving=false,Rear=false,Kept=false,Shield=false,Straight=false,BackStraight=false,Blocked=false,Lost=false;
        static bool ForwardLocked=false,RearLocked=false;
        auto StraightSetup=[&](bool Backward){Place(B,230,11);Place(Victim,230,11);auto At=B->GetActorLocation()+B->GetActorForwardVector()*(Backward?-1600:1600);
            At.Z=ABenchyWorld::WaterHeight(At,T)+70;Victim->AuditPose(At,B->GetActorRotation(),11);Victim->Coins=10;B->Item=1;B->ItemFeedback(0);};
        if(Stage==0&&T>4){S->Phase=2;S->StartTime=T;Place(B,70,3);B->ServerColor_Implementation(4);
            Victim=Boat(W,TEXT("Curve Captain"),95,4);Victim->IsBot=true;Victim->ServerColor_Implementation(2);B->Item=4;B->ItemFeedback(0);Stage=1;Next=T+.6f;}
        if(Stage==1&&T>Next){Shot(TEXT("Lenknozzle-Ready"));Stage=20;Next=T+.22f;}
        if(Stage==20&&T>Next){B->ServerUseItem_Implementation(false);ForwardLocked=Locked(W,Victim);Stage=2;Next=T+.12f;}
        if(Stage==2&&T>Next){Shot(TEXT("Lenknozzle-Flight"));Stage=3;}
        if(Stage==3&&Victim->Coins==0&&Victim->StunUntil>T){Moving=ForwardLocked;Stage=4;Next=T+2.5f;}
        if(Stage==4&&T>Next){Place(B,235,11);Place(Victim,231,11);Victim->Coins=10;Last=Boat(W,TEXT("Last Place"),221,11);
            B->Item=4;B->ServerUseItem_Implementation(false);Kept=B->Item==4&&Projectiles(W)==0&&Victim->Coins==10&&Last->Coins==10;
            Stage=21;Next=T+.8f;}
        if(Stage==21&&T>Next){Shot(TEXT("Lenknozzle-No-Front-Target"));Stage=22;Next=T+.22f;}
        if(Stage==22&&T>Next){B->ServerUseItem_Implementation(true);RearLocked=Locked(W,Victim);Stage=5;Next=T+.13f;}
        if(Stage==5&&T>Next){Shot(TEXT("Lenknozzle-Rear"));Stage=6;}
        if(Stage==6&&Victim->Coins==0&&Victim->StunUntil>T){Rear=RearLocked&&Last->Coins==10&&B->LastShotBackward;Stage=7;Next=T+2.5f;}
        if(Stage==7&&T>Next){Place(B,235,11);Place(Victim,231,11);Victim->Coins=7;Victim->ShieldUntil=T+8;B->Item=4;B->ServerUseItem_Implementation(true);Stage=8;}
        if(Stage==8&&Victim->ShieldUntil==0&&Projectiles(W)==0){Shield=Victim->Coins==7&&Victim->StunUntil<=T;StraightSetup(false);B->ServerUseItem_Implementation(false);Stage=9;}
        if(Stage==9&&Victim->Coins==0){Straight=true;StraightSetup(true);B->ServerUseItem_Implementation(true);Stage=10;}
        if(Stage==10&&Victim->Coins==0){BackStraight=true;StraightSetup(false);
            Wall=W->SpawnActor<AActor>(B->GetActorLocation()+B->GetActorForwardVector()*800,FRotator::ZeroRotator);
            auto Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Box->SetBoxExtent(FVector(100,600,500));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();
            Box->SetWorldLocation(B->GetActorLocation()+B->GetActorForwardVector()*800);B->ServerUseItem_Implementation(false);Stage=11;Next=T+1;}
        if(Stage==11&&T>Next){Blocked=Projectiles(W)==0&&Victim->Coins==10;Wall->Destroy();Place(B,70,3);Place(Victim,100,5);B->Item=4;B->ServerUseItem_Implementation(false);
            bool Had=Locked(W,Victim);Victim->Destroy();Lost=Had;Stage=12;Next=T+.5f;}
        if(Stage==12&&T>Next){Lost&=Projectiles(W)==0&&Last->Coins==10;
            Report(TEXT("Nozzles"),Moving&&Rear&&Kept&&Shield&&Straight&&BackStraight&&Blocked&&Lost,FString::Printf(TEXT("\"moving_target_through_curve\":%s,\"rear_hits_second_only\":%s,\"first_keeps_item_without_front_target\":%s,\"shield_blocks\":%s,\"standard_forward\":%s,\"standard_backward\":%s,\"obstacle_blocks\":%s,\"lost_target_no_retarget\":%s"),Moving?TEXT("true"):TEXT("false"),Rear?TEXT("true"):TEXT("false"),Kept?TEXT("true"):TEXT("false"),Shield?TEXT("true"):TEXT("false"),Straight?TEXT("true"):TEXT("false"),BackStraight?TEXT("true"):TEXT("false"),Blocked?TEXT("true"):TEXT("false"),Lost?TEXT("true"):TEXT("false")));FPlatformMisc::RequestExit(false);Stage=13;}
        if(T>55&&Stage!=13){Report(TEXT("Nozzles"),false,FString::Printf(TEXT("\"timeout_stage\":%d,\"projectiles\":%d,\"victim_coins\":%d"),Stage,Projectiles(W),IsValid(Victim)?Victim->Coins:-1));FPlatformMisc::RequestExit(false);}
    }
    if(Flag(TEXT("BenchyFilamentAudit"))){
        static int Stage=0;static float Next=0;static ACameraActor* Camera=nullptr;static bool Printed=false,Galaxy=false,Glow=false;
        if(Stage==0&&T>4){S->Phase=2;S->StartTime=T;Place(B,230,11);B->ServerColor_Implementation(1);
            FVector At=B->GetActorLocation()-B->GetActorForwardVector()*720+B->GetActorRightVector()*540+FVector(0,0,310);
            Camera=W->SpawnActor<ACameraActor>(At,(B->GetActorLocation()+FVector(0,0,100)-At).Rotation());auto C=Camera->GetCameraComponent();C->SetFieldOfView(55);
            C->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;C->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;
            C->PostProcessSettings.AutoExposureMinBrightness=12;C->PostProcessSettings.AutoExposureMaxBrightness=12;W->GetFirstPlayerController()->SetViewTarget(Camera);Stage=1;}
        if(Stage==1&&T>6){auto M=Filament(B);Printed=M&&M->K2_GetScalarParameterValue(TEXT("Printed"))==1&&M->K2_GetScalarParameterValue(TEXT("LayerPitch"))>0;Shot(TEXT("Printed-Orange"));Stage=2;}
        if(Stage==2&&T>7){B->ServerColor_Implementation(0);Stage=3;}
        if(Stage==3&&T>8){Shot(TEXT("Printed-Black"));B->ServerColor_Implementation(4);Stage=4;}
        if(Stage==4&&T>10){auto M=Filament(B);Galaxy=M&&M->K2_GetScalarParameterValue(TEXT("Galaxy"))==1&&B->FilamentName().Contains(TEXT("Prusament"));Shot(TEXT("Prusament-Galaxy"));Stage=5;}
        if(Stage==5&&T>10.6f){Camera->AddActorWorldOffset(B->GetActorRightVector()*65);Camera->SetActorRotation((B->GetActorLocation()+FVector(0,0,100)-Camera->GetActorLocation()).Rotation());Stage=6;}
        if(Stage==6&&T>11.4f){Shot(TEXT("Prusament-Galaxy-Glint"));Stage=61;Next=T+.4f;}
        // Screenshot requests are captured on the next render frame. Keep Galaxy
        // active until that frame has completed, then switch to Glow.
        if(Stage==61&&T>Next){B->ServerColor_Implementation(5);Stage=7;}
        if(Stage==7&&T>13){auto M=Filament(B);Glow=M&&M->K2_GetScalarParameterValue(TEXT("Glow"))==1&&B->FilamentName().Contains(TEXT("Glow in the Dark"));Shot(TEXT("Glow-Day"));Stage=8;}
        if(Stage==8&&T>14){auto C=Camera->GetCameraComponent();C->PostProcessSettings.AutoExposureMinBrightness=10;C->PostProcessSettings.AutoExposureMaxBrightness=10;
            for(TActorIterator<ADirectionalLight> It(W);It;++It)It->GetLightComponent()->SetIntensity(.03f);
            for(TActorIterator<ASkyLight> It(W);It;++It)It->GetLightComponent()->SetIntensity(.004f);Stage=9;}
        if(Stage==9&&T>16){Shot(TEXT("Glow-Dark"));Stage=10;}
        if(Stage==10&&T>18){Report(TEXT("Filaments"),Printed&&Galaxy&&Glow,FString::Printf(TEXT("\"printed_layers_enabled\":%s,\"prusament_galaxy_enabled\":%s,\"glow_filament_enabled\":%s,\"color_count\":6"),Printed?TEXT("true"):TEXT("false"),Galaxy?TEXT("true"):TEXT("false"),Glow?TEXT("true"):TEXT("false")));FPlatformMisc::RequestExit(false);}
    }
    if(Flag(TEXT("BenchyNozzleNetworkAudit"))){
        static int Stage=0;static ABenchyPawn* Front=nullptr;static ABenchyPawn* Rear=nullptr;static bool Forward=false;
        ABenchyPawn* Remote=nullptr;for(TActorIterator<ABenchyPawn> It(W);It;++It)if(!It->IsBot&&!It->IsLocallyControlled()&&It->GetController())Remote=*It;
        if(Stage==0&&Remote&&T>8){Mode->StartRace();Stage=1;}
        if(Stage==1&&S->Phase==2&&Remote){for(TActorIterator<ABenchyPawn> It(W);It;++It)if(It->IsBot)It->Destroy();
            Place(Remote,230,11);Front=Boat(W,TEXT("Network Ahead"),235,11);Rear=Boat(W,TEXT("Network Behind"),225,11);Remote->Item=4;Remote->ForceNetUpdate();Stage=2;}
        if(Stage==2&&Front->Coins==0){Forward=true;Place(Front,221,11);Remote->Item=4;Remote->ForceNetUpdate();Stage=3;}
        if(Stage==3&&Rear->Coins==0&&S->Clock()>7){Report(TEXT("NozzleNetworkHost"),Forward&&Remote->LastShotBackward&&Remote->LastShotTarget==TEXT("Network Behind")&&Remote->ColorIndex==5,TEXT("\"guided_forward_rpc\":true,\"guided_rear_rpc\":true,\"glow_color_selected\":true"));FPlatformMisc::RequestExit(false);Stage=4;}
        if(T>70&&Stage<4){Report(TEXT("NozzleNetworkHost"),false,FString::Printf(TEXT("\"timeout_stage\":%d"),Stage));FPlatformMisc::RequestExit(false);}
    }
}
void BenchyValidateNozzleClient(ABenchyController* PC){
    if(!Flag(TEXT("BenchyNozzleNetworkAudit"))||PC->HasAuthority())return;
    auto W=PC->GetWorld();auto S=W->GetGameState<ABenchyRaceState>();auto B=Cast<ABenchyPawn>(PC->GetPawn());if(!S||!B)return;
    static int Stage=0;static bool SawFront=false,SawRear=false,Glow=false;
    if(S->Phase==2){
        for(TActorIterator<ABenchyProjectile> It(W);It;++It)if(It->Guided&&It->Target){SawFront|=It->Target->RacerName==TEXT("Network Ahead");SawRear|=It->Target->RacerName==TEXT("Network Behind");}
        if(Stage==0&&B->Item==4){B->ServerColor(5);B->ServerUseItem(false);Stage=1;}
        if(Stage==1&&B->Item==4&&B->LastShotTarget==TEXT("Network Ahead")){B->ServerUseItem(true);Stage=2;}
        if(auto M=Filament(B))Glow|=B->ColorIndex==5&&M->K2_GetScalarParameterValue(TEXT("Glow"))==1;
        if(Stage==2&&S->Clock()>6){Report(TEXT("NozzleNetworkClient"),SawFront&&SawRear&&Glow&&B->LastShotBackward&&B->LastShotTarget==TEXT("Network Behind"),FString::Printf(TEXT("\"front_target_replicated\":%s,\"rear_target_replicated\":%s,\"glow_material_replicated\":%s,\"rear_shot_direction_replicated\":%s"),SawFront?TEXT("true"):TEXT("false"),SawRear?TEXT("true"):TEXT("false"),Glow?TEXT("true"):TEXT("false"),B->LastShotBackward?TEXT("true"):TEXT("false")));Stage=3;}
    }
}
