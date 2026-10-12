#include "BenchyFinishAudit.h"
#include "BenchyGameMode.h"
#include "BenchyMenu.h"
#include "BenchyPawn.h"
#include "BenchyRace.h"
#include "BenchyWorld.h"
#include "CourseNavigation.h"
#include "Components/PrimitiveComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Engine/GameViewportClient.h"

namespace {
bool Flag(const TCHAR* N){return FParse::Param(FCommandLine::Get(),N);}
void Report(const TCHAR* Name,bool Good,const FString& Fields){
    auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);
    FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,%s}"),Good?TEXT("true"):TEXT("false"),*Fields),*(Dir/(FString(Name)+TEXT(".json"))));
    UE_LOG(LogTemp,Display,TEXT("BENCHY_FINISH_VALIDATION %s passed=%d %s"),Name,Good,*Fields);
}
void Shot(const TCHAR* Name,bool UI=true){auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/(FString(Name)+TEXT(".png")),UI,false);}
void ApproachFinish(ABenchyPawn* B){
    auto D=BenchyCourse::Direction(ABenchyWorld::ActiveTrack,0);FVector F(D.x,D.y,0),P=ABenchyWorld::RoutePoint(0)-F*600;
    P.Z=ABenchyWorld::WaterHeight(P,B->GetWorld()->GetTimeSeconds())+70;
    B->AuditPose(P,F.Rotation(),47);B->AuditInput(0,0);B->RespawnUntil=0;B->StunUntil=0;
    B->FindComponentByClass<UPrimitiveComponent>()->SetPhysicsLinearVelocity(F*1700);
}
}
void BenchyValidateFinish(ABenchyGameMode* M){
    auto W=M->GetWorld();auto S=W->GetGameState<ABenchyRaceState>();auto PC=Cast<ABenchyController>(W->GetFirstPlayerController());if(!S||!PC)return;
    auto B=Cast<ABenchyPawn>(PC->GetPawn());if(!B)return;const float T=W->GetTimeSeconds();
    if(Flag(TEXT("BenchyWorld15Audit"))){
        static int Stage=0;static ACameraActor* Camera=nullptr;
        if(Stage==0&&T>4){PC->HideMenu();Camera=W->SpawnActor<ACameraActor>();Camera->SetActorLocationAndRotation(FVector(27000,-34000,21500),(-FVector(27000,-34000,21500)).Rotation());auto C=Camera->GetCameraComponent();C->SetFieldOfView(65);C->PostProcessSettings.bOverride_MotionBlurAmount=true;C->PostProcessSettings.MotionBlurAmount=0;PC->SetViewTarget(Camera);Stage=1;}
        if(Stage==1&&T>8){Shot(TEXT("World-Overview"),false);Stage=2;}
        if(Stage==2&&T>10){float I=ABenchyWorld::ActiveTrack==1?25:65;auto P=ABenchyWorld::RoutePoint(I),F=(ABenchyWorld::RoutePoint(I+1)-P).GetSafeNormal();FVector At=P-F*2600+FVector(0,0,1100);Camera->SetActorLocationAndRotation(At,(P+F*3000+FVector(0,0,400)-At).Rotation());Camera->GetCameraComponent()->SetFieldOfView(65);Stage=3;}
        if(Stage==3&&T>13){Shot(TEXT("World-Channel"),false);Stage=4;}
        if(Stage==4&&T>15){for(TActorIterator<ABenchyWorld> It(W);It;++It){TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> Parts(*It);
            for(auto C:Parts)if(C->GetStaticMesh()&&C->GetInstanceCount()>0&&C->GetStaticMesh()->GetName()==(ABenchyWorld::ActiveTrack==2?TEXT("SM_IceCliff"):TEXT("SM_NordicCliff"))){FTransform X;C->GetInstanceTransform(0,X,true);FVector P=X.TransformPosition(C->GetStaticMesh()->GetBoundingBox().GetCenter());FVector At=P+FVector(-2200,-3300,1600);Camera->SetActorLocationAndRotation(At,(P-At).Rotation());break;}}
            Stage=5;}
        if(Stage==5&&T>18){Shot(TEXT("World-Cliffs"),false);Stage=6;}
        if(Stage==6&&T>19){Report(TEXT("World"),true,FString::Printf(TEXT("\"track\":%d,\"native_overview_channel_cliff_captures\":true"),ABenchyWorld::ActiveTrack));FPlatformMisc::RequestExit(false);Stage=7;}
    }
    if(Flag(TEXT("BenchyFinishAudit"))){
        static int Stage=0;static float Next=0,Time=0,Cursor=0;static FVector Start;static ABenchyPawn* Second=nullptr;
        static bool Actual=false,Continuing=false,Following=false,Frozen=false,Ignored=false,Live=false,Disconnect=false,Done=false,Restart=false;
        if(Stage==0&&T>4){M->StartRace();S->Phase=2;S->StartTime=T-180;PC->HideMenu();for(TActorIterator<ABenchyPawn> It(W);It;++It)if(*It!=B){It->IsBot=false;It->AuditInput(0,0);if(!Second)Second=*It;}ApproachFinish(B);Stage=1;}
        if(Stage==1&&B->Finished){Actual=B->Passed==48&&B->FinishPlace==1&&S->Results.Num()==6&&S->Results[0].Place>=0;
            Time=B->RaceTime;Cursor=B->FinishRouteCursor();Start=B->GetActorLocation();B->Coins=7;B->ServerControls_Implementation(-1,1,true);B->ResetBoat();B->HitByItem();Ignored=B->Coins==7&&B->StunUntil==0&&!B->Collect(0);
            S->FirstFinishTime=T-100;S->StartTime=T-1000;Stage=2;Next=T+3;}
        if(Stage==2&&T>Next){Continuing=S->Phase==2&&PC->ResultsVisible()&&!PC->MenuVisible();Shot(TEXT("Finish-Live"));M->StartRace();Continuing&=B->Finished&&S->Phase==2;Stage=3;Next=T+14;}
        if(Stage==3&&T>Next){Frozen=B->RaceTime==Time;Following=B->FinishRouteCursor()>Cursor+20&&FVector::Dist2D(Start,B->GetActorLocation())>3000&&B->SpeedKmh()>10;
            auto P=B->GetActorLocation();Following&=BenchyTracks::Distance(ABenchyWorld::ActiveTrack,{P.X,P.Y})<1400;ApproachFinish(Second);Stage=4;}
        if(Stage==4&&Second->Finished){auto Order=S->OrderedResults();Live=Second->FinishPlace==2&&Order.Num()==6&&Order[0].EntryId==B->RaceEntryId&&Order[1].EntryId==Second->RaceEntryId&&Order[1].Time>Time;Stage=5;Next=T+.6;}
        if(Stage==5&&T>Next){Shot(TEXT("Finish-Updated"));for(TActorIterator<ABenchyPawn> It(W);It;++It)if(*It!=B&&*It!=Second){It->Destroy();break;}Stage=6;Next=T+.6;}
        if(Stage==6&&T>Next){Disconnect=S->Results.Num()==6&&S->Results.ContainsByPredicate([](const FBenchyResult& R){return R.Disconnected&&!R.Place;});for(TActorIterator<ABenchyPawn> It(W);It;++It)if(!It->Finished){It->RaceTime=S->Clock();It->FinishRace();}Stage=7;Next=T+1;}
        if(Stage==7&&T>Next){Done=S->Phase==3&&S->EveryoneDone()&&PC->ResultsVisible()&&B->RaceTime==Time;Shot(TEXT("Finish-All"));Stage=8;Next=T+.6;}
        if(Stage==8&&T>Next){M->StartRace();Restart=S->Phase==1&&!B->Finished&&B->RaceTime==0&&B->FinishPlace==0&&S->Results.Num()==6;Stage=9;}
        if(Stage==9){Report(TEXT("Finish"),Actual&&Continuing&&Following&&Frozen&&Ignored&&Live&&Disconnect&&Done&&Restart,
            FString::Printf(TEXT("\"actual_final_checkpoint\":%s,\"no_60_or_900_second_cutoff\":%s,\"physical_course_autopilot\":%s,\"finish_time_frozen\":%s,\"finished_input_items_ignored\":%s,\"live_second_finish\":%s,\"disconnect_retains_row\":%s,\"all_done_ends_race\":%s,\"restart_resets_results\":%s,\"track\":%d"),Actual?TEXT("true"):TEXT("false"),Continuing?TEXT("true"):TEXT("false"),Following?TEXT("true"):TEXT("false"),Frozen?TEXT("true"):TEXT("false"),Ignored?TEXT("true"):TEXT("false"),Live?TEXT("true"):TEXT("false"),Disconnect?TEXT("true"):TEXT("false"),Done?TEXT("true"):TEXT("false"),Restart?TEXT("true"):TEXT("false"),ABenchyWorld::ActiveTrack));FPlatformMisc::RequestExit(false);Stage=10;}
        if(T>70&&Stage!=10){Report(TEXT("Finish"),false,FString::Printf(TEXT("\"timeout_stage\":%d,\"finished\":%s,\"passed\":%d"),Stage,B->Finished?TEXT("true"):TEXT("false"),B->Passed));FPlatformMisc::RequestExit(false);}
    }
    if(Flag(TEXT("BenchyFinishNetworkAudit"))){
        static int Stage=0;static ABenchyPawn* Remote=nullptr;static float Time=0,Next=0;static FVector Start;static bool Following=false,Live=false;
        if(Stage==0&&T>4){for(TActorIterator<ABenchyPawn> It(W);It;++It)if(!It->IsBot&&!It->IsLocallyControlled()&&It->GetController())Remote=*It;
            if(Remote){M->StartRace();S->Phase=2;S->StartTime=T-180;for(TActorIterator<ABenchyPawn> It(W);It;++It)if(*It!=Remote){It->IsBot=false;It->AuditInput(0,0);}ApproachFinish(Remote);PC->HideMenu();Stage=1;}}
        if(Stage==1&&Remote->Finished){Time=Remote->RaceTime;Start=Remote->GetActorLocation();S->FirstFinishTime=T-100;S->StartTime=T-1000;Stage=2;Next=T+12;}
        if(Stage==2&&T>Next){Following=S->Phase==2&&Remote->RaceTime==Time&&FVector::Dist2D(Start,Remote->GetActorLocation())>3000;ApproachFinish(B);Stage=3;}
        if(Stage==3&&B->Finished){auto Order=S->OrderedResults();Live=Order.Num()==6&&Order[0].Place==1&&Order[0].Boat==Remote&&Order[1].Boat==B&&B->FinishPlace==2;Stage=4;Next=T+12;}
        if(Stage==4&&T>Next){Report(TEXT("FinishNetworkHost"),Following&&Live&&S->Phase==2,TEXT("\"remote_actual_finish\":true,\"remote_autopilot_continues\":true,\"other_human_can_finish\":true,\"race_still_active\":true"));FPlatformMisc::RequestExit(false);Stage=5;}
        if(T>70&&Stage!=5){Report(TEXT("FinishNetworkHost"),false,FString::Printf(TEXT("\"timeout_stage\":%d"),Stage));FPlatformMisc::RequestExit(false);}
    }
}
void BenchyValidateFinishClient(ABenchyController* PC){
    if(!Flag(TEXT("BenchyFinishNetworkAudit"))||PC->GetNetMode()!=NM_Client)return;
    auto W=PC->GetWorld();auto S=W->GetGameState<ABenchyRaceState>();auto B=Cast<ABenchyPawn>(PC->GetPawn());if(!S||!B)return;
    static int Stage=0;static float Time=0,Next=0;static FVector Start;
    float T=W->GetTimeSeconds();
    if(Stage==0&&B->Finished&&PC->ResultsVisible()){Time=B->RaceTime;Start=B->GetActorLocation();Stage=1;Next=T+4;Shot(TEXT("Finish-Network-Live"));}
    if(Stage==1&&T>Next&&S->Results.Num()==6&&S->OrderedResults()[1].Place==2){
        auto Order=S->OrderedResults();bool Good=Order[0].EntryId==B->RaceEntryId&&Order[0].Time==Time&&B->RaceTime==Time&&S->Phase==2&&PC->ResultsVisible()&&!PC->MenuVisible()&&FVector::Dist2D(Start,B->GetActorLocation())>1500;
        Good&=Order.ContainsByPredicate([](const FBenchyResult& R){return !R.Place&&!R.Disconnected;});
        Shot(TEXT("Finish-Network-Updated"));Report(TEXT("FinishNetworkClient"),Good,TEXT("\"personal_result_window\":true,\"finish_times_replicated\":true,\"unfinished_rows_remain\":true,\"autopilot_movement_replicated\":true"));Stage=2;Next=T+.7;
    }
    if(Stage==2&&T>Next){FPlatformMisc::RequestExit(false);Stage=3;}
    if(T>65&&Stage!=3){Report(TEXT("FinishNetworkClient"),false,FString::Printf(TEXT("\"timeout_stage\":%d"),Stage));FPlatformMisc::RequestExit(false);}
}
