#include "BenchyAudit.h"
#include "BenchyNozzleAudit.h"
#include "BenchyFinishAudit.h"
#include "BenchyGameMode.h"
#include "BenchyMenu.h"
#include "BenchyPawn.h"
#include "BenchyRace.h"
#include "BenchyWorld.h"
#include "BenchyItemFX.h"
#include "CourseNavigation.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
namespace {
bool Flag(const TCHAR* Name){return FParse::Param(FCommandLine::Get(),Name);}
void Report(const TCHAR* Name,bool Pass,const FString& Detail){
 const FString Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);
 FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,%s}\n"),Pass?TEXT("true"):TEXT("false"),*Detail),*(Dir/(FString(Name)+TEXT(".json"))));
 UE_LOG(LogTemp,Display,TEXT("BENCHY_VALIDATION %s passed=%d %s"),Name,Pass,*Detail);
}
void Shot(const TCHAR* Name){const FString Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/(FString(Name)+TEXT(".png")),FString(Name)==TEXT("Menu")||FString(Name)==TEXT("Graphics"),false);}
}
void BenchyValidateGame(ABenchyGameMode* M){
 BenchyValidateNozzles(M);
 BenchyValidateFinish(M);
 auto W=M->GetWorld();auto S=W->GetGameState<ABenchyRaceState>();if(!S)return;float T=W->GetTimeSeconds();
 if(Flag(TEXT("BenchyNavigationAudit"))){
  static int Stage=0;static bool WideCrossing=false,Ranking=false,Missed=false,Recovery=false;
  static ABenchyPawn* Peer=nullptr;auto B=Cast<ABenchyPawn>(W->GetFirstPlayerController()->GetPawn());if(!B)return;
  auto Place=[&](int Gate,float Along,float Side){auto D=BenchyCourse::Direction(ABenchyWorld::ActiveTrack,Gate*20);
   FVector P=ABenchyWorld::RoutePoint(Gate*20)+FVector(D.x*Along-D.y*Side,D.y*Along+D.x*Side,0);P.Z=ABenchyWorld::WaterHeight(P,T)+70;
   B->SetActorLocationAndRotation(P,FVector(D.x,D.y,0).Rotation(),false,nullptr,ETeleportType::TeleportPhysics);
   auto Hull=Cast<UPrimitiveComponent>(B->GetRootComponent());Hull->SetPhysicsLinearVelocity(FVector::ZeroVector);Hull->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);};
  if(Stage==0&&T>4){S->Phase=2;S->StartTime=T;Place(1,-700,1000);Stage=1;}
  if(Stage==1&&T>4.6f){Place(1,700,1000);Stage=2;}
  if(Stage==2&&T>5){WideCrossing=B->Passed==1&&B->NextGate==2&&!B->MissedCheckpoint;
   B->Passed=10;B->NextGate=11;B->SetActorLocation(ABenchyWorld::RoutePoint(210)+FVector(0,0,70),false,nullptr,ETeleportType::TeleportPhysics);
   FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   Peer=W->SpawnActor<ABenchyPawn>(ABenchyWorld::RoutePoint(205)+FVector(0,0,70),FRotator::ZeroRotator,Params);Peer->Passed=10;Peer->NextGate=11;Peer->RacerName=TEXT("Ranking Test");Ranking=B->Progress()>Peer->Progress();Stage=3;}
  if(Stage==3&&T>6){Shot(TEXT("Navigation"));Stage=4;}
  if(Stage==4&&T>7){Place(11,-600,1800);Stage=5;}
  if(Stage==5&&T>7.4f){Place(11,600,1800);Stage=6;}
  if(Stage==6&&T>8){Missed=B->Passed==10&&B->NextGate==11&&B->MissedCheckpoint;Shot(TEXT("MissedCheckpoint"));Stage=7;}
  if(Stage==7&&T>10){B->ResetBoat();Recovery=!B->MissedCheckpoint&&B->Passed==10&&B->NextGate==11&&FVector::Dist2D(B->GetActorLocation(),ABenchyWorld::RoutePoint(200))<1;Stage=8;}
  if(Stage==8&&T>11){Shot(TEXT("CheckpointRecovery"));Stage=9;}
  if(Stage==9&&T>13){Report(TEXT("Navigation"),WideCrossing&&Ranking&&Missed&&Recovery,FString::Printf(TEXT("\"wide_lane_crossing\":%s,\"front_boat_ranks_first\":%s,\"missed_checkpoint_detected\":%s,\"recovery_keeps_lap\":%s"),WideCrossing?TEXT("true"):TEXT("false"),Ranking?TEXT("true"):TEXT("false"),Missed?TEXT("true"):TEXT("false"),Recovery?TEXT("true"):TEXT("false")));FPlatformMisc::RequestExit(false);}
 }
 if(Flag(TEXT("BenchyItemAudit"))){
  static int Stage=0;static bool Good=true,ShieldGeometry=false,TurboGeometry=false,ProjectileGeometry=false;
  static ABenchyPickup* AuditPickup=nullptr;
  static float PreviousShieldVertex=0;static bool ShieldAnimated=false;
  auto B=Cast<ABenchyPawn>(W->GetFirstPlayerController()->GetPawn());if(!B)return;
  auto FX=B->FindComponentByClass<UBenchyItemFX>();
  auto Place=[&](){FVector P=ABenchyWorld::RoutePoint(230)+FVector(0,0,100);FRotator R=(ABenchyWorld::RoutePoint(231)-P).Rotation();R.Pitch=0;
   B->SetActorLocationAndRotation(P,R,false,nullptr,ETeleportType::TeleportPhysics);Cast<UPrimitiveComponent>(B->GetRootComponent())->SetPhysicsLinearVelocity(FVector::ZeroVector);B->AuditInput(0,0);};
  if(Stage==0&&T>4){S->Phase=2;S->StartTime=T;Place();B->RespawnUntil=0;B->Item=0;
   AuditPickup=W->SpawnActorDeferred<ABenchyPickup>(ABenchyPickup::StaticClass(),FTransform(B->GetActorLocation()));AuditPickup->Kind=1;AuditPickup->FinishSpawning(FTransform(B->GetActorLocation()));Stage=1;}
  if(Stage==1&&T>5){Good&=B->Item>0&&B->ItemEvent==0&&B->ItemEventTime>4;AuditPickup->Destroy();B->Item=3;B->ItemFeedback(0);Stage=2;}
  if(Stage==2&&T>7){Shot(TEXT("Item-Ready"));Stage=3;}
  if(Stage==3&&T>8){B->ServerUseItem_Implementation(false);Good&=B->Item==0&&B->ItemEvent==3&&B->ShieldUntil>T;Stage=4;}
  if(Stage==4&&T>9){auto Mesh=FX?FX->GetProcMeshSection(0):nullptr;ShieldGeometry=Mesh&&Mesh->ProcVertexBuffer.Num()>100;
   if(ShieldGeometry)PreviousShieldVertex=Mesh->ProcVertexBuffer[0].Position.X;Shot(TEXT("Item-Shield"));Stage=5;}
  if(Stage==5&&T>9.4f){auto Mesh=FX?FX->GetProcMeshSection(0):nullptr;ShieldAnimated=Mesh&&FMath::Abs(Mesh->ProcVertexBuffer[0].Position.X-PreviousShieldVertex)>1;Shot(TEXT("Item-Shield-Moving"));Stage=6;}
  if(Stage==6&&T>11){B->Coins=7;B->HitByItem();Good&=B->ShieldUntil==0&&B->Coins==7&&B->ItemEvent==5;Stage=7;}
  if(Stage==7&&T>11.15f){Shot(TEXT("Item-Shield-Break"));Stage=8;}
  if(Stage==8&&T>12){Place();B->Item=2;B->ItemFeedback(0);Stage=9;}
  if(Stage==9&&T>13){Shot(TEXT("Item-Turbo-Ready"));Stage=10;}
  if(Stage==10&&T>14){B->ServerUseItem_Implementation(false);B->AuditInput(.7f,0);Good&=B->Item==0&&B->ItemEvent==2;Stage=11;}
  if(T>14&&T<18)B->AuditInput(.7f,0);
  if(Stage==11&&T>15){auto Mesh=FX?FX->GetProcMeshSection(0):nullptr;TurboGeometry=Mesh&&Mesh->ProcVertexBuffer.Num()>20;Shot(TEXT("Item-Turbo"));Stage=12;}
  if(Stage==12&&T>19){B->AuditInput(0,0);Place();B->Item=1;B->ItemFeedback(0);Good&=B->ItemBoostUntil<T;Stage=13;}
  if(Stage==13&&T>20){Shot(TEXT("Item-Nozzle-Ready"));Stage=14;}
  if(Stage==14&&T>21){B->ServerUseItem_Implementation(false);Good&=B->Item==0&&B->ItemEvent==1;Stage=15;}
  if(Stage==15&&T>21.13f){for(TActorIterator<ABenchyProjectile> It(W);It;++It){auto Trail=It->FindComponentByClass<UBenchyItemFX>();ProjectileGeometry|=Trail&&Trail->GetNumSections()>0;}Shot(TEXT("Item-Nozzle"));Stage=16;}
  if(Stage==16&&T>23){B->Coins=10;B->HitByItem();Good&=B->ItemEvent==4&&B->Coins==0;Stage=17;}
  if(Stage==17&&T>23.15f){Shot(TEXT("Item-Hit"));Stage=18;}
  if(Stage==18&&T>25){Good&=ShieldGeometry&&ShieldAnimated&&TurboGeometry&&ProjectileGeometry;
   Report(TEXT("Items"),Good,FString::Printf(TEXT("\"real_pickup\":true,\"shield_geometry\":%s,\"shield_animated\":%s,\"turbo_geometry\":%s,\"projectile_trail\":%s,\"shield_block_and_coin_loss\":true"),ShieldGeometry?TEXT("true"):TEXT("false"),ShieldAnimated?TEXT("true"):TEXT("false"),TurboGeometry?TEXT("true"):TEXT("false"),ProjectileGeometry?TEXT("true"):TEXT("false")));FPlatformMisc::RequestExit(false);}
 }
 if(Flag(TEXT("BenchyGameplayAudit"))){
  static int Stage=0;static bool Good=true;static ABenchyPawn* Victim=nullptr;auto B=Cast<ABenchyPawn>(W->GetFirstPlayerController()->GetPawn());if(!B)return;
  if(Stage==0&&T>5){
   S->Phase=2;S->StartTime=T;B->Passed=5;B->NextGate=6;B->Lap=1;B->SetActorLocationAndRotation(FVector(0,0,2200),FRotator(80,75,100),false,nullptr,ETeleportType::TeleportPhysics);
   auto Hull=Cast<UPrimitiveComponent>(B->GetRootComponent());Hull->SetPhysicsLinearVelocity(FVector(1000,1000,1000));B->ResetBoat();
   Good&=FVector::Dist2D(B->GetActorLocation(),ABenchyWorld::RoutePoint(100))<1&&B->NextGate==6&&B->Passed==5&&Hull->GetPhysicsLinearVelocity().Size()<1&&B->GetActorUpVector().Z>.99;
   B->RespawnUntil=0;for(int I=0;I<12;I++)B->Collect(0);Good&=B->Coins==10;
   B->HitByItem();Good&=B->Coins==0&&B->StunUntil>T;B->StunUntil=0;B->Collect(0);B->ShieldUntil=T+8;B->HitByItem();Good&=B->Coins==1&&B->ShieldUntil==0;
   B->Item=2;B->ServerUseItem_Implementation(false);Good&=B->Item==0&&B->ItemBoostUntil>T+3;
   FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   FVector At=ABenchyWorld::RoutePoint(230)+FVector(0,0,150);FRotator R=(ABenchyWorld::RoutePoint(231)-At).Rotation();R.Pitch=0;B->SetActorLocationAndRotation(At,R,false,nullptr,ETeleportType::TeleportPhysics);
   Victim=W->SpawnActor<ABenchyPawn>(At+R.Vector()*950,R,P);Victim->Coins=10;Victim->RespawnUntil=0;B->Item=1;B->ServerUseItem_Implementation(false);Good&=B->Item==0;Stage=1;
  }
  if(Stage==1&&T>7){Good&=Victim&&Victim->Coins==0&&Victim->StunUntil>T;Report(TEXT("Gameplay"),Good,TEXT("\"reset_last_checkpoint\":true,\"reset_clears_velocity\":true,\"coin_cap\":10,\"hit_loses_coins\":true,\"shield_and_turbo\":true,\"projectile_collision\":true"));Stage=2;FPlatformMisc::RequestExit(false);}
 }
 if(Flag(TEXT("BenchyRaceAudit"))){
  static float NextProgress=0;if(T>NextProgress){NextProgress=T+30;for(TActorIterator<ABenchyPawn> It(W);It;++It)if(!It->IsBot)UE_LOG(LogTemp,Display,TEXT("RACE_PROGRESS track=%d phase=%d passed=%d speed=%.1f position=%s"),ABenchyWorld::ActiveTrack,S->Phase,It->Passed,It->SpeedKmh(),*It->GetActorLocation().ToString());}
  static bool Started=false,ShotDone=false;if(!Started&&T>3){M->StartRace();Started=true;}
  if(S->Phase==2){if(auto B=Cast<ABenchyPawn>(W->GetFirstPlayerController()->GetPawn()))B->DriveAI();if(!ShotDone&&S->Clock()>16){Shot(TEXT("Race"));ShotDone=true;}}
  if(S->Phase==3||T>700){int Finished=0,HumansFinished=0,Count=0;for(TActorIterator<ABenchyPawn> It(W);It;++It){Count++;if(It->Finished){Finished++;if(!It->IsBot)HumansFinished++;}}
   Report(TEXT("Race"),HumansFinished>0&&Count>=6&&Finished==Count&&S->Phase==3,FString::Printf(TEXT("\"racers\":%d,\"finishers\":%d,\"human_finished\":%d,\"track\":%d,\"seconds\":%.1f"),Count,Finished,HumansFinished,ABenchyWorld::ActiveTrack,S->Clock()));FPlatformMisc::RequestExit(false);}
  if(Flag(TEXT("BenchyPreviewAudit"))&&T>30)FPlatformMisc::RequestExit(false);
 }
 if(Flag(TEXT("BenchyTravelAudit"))){
  static bool TravelRequested=false;int Humans=0;for(TActorIterator<ABenchyPawn> It(W);It;++It)if(!It->IsBot)Humans++;
  if(!TravelRequested&&Humans>=2&&T>10){TravelRequested=true;W->ServerTravel(TEXT("/Game/Maps/Sonnendeck?Track=2?listen"));}
  if(TravelRequested&&ABenchyWorld::ActiveTrack==2&&Humans>=2&&T>10){Report(TEXT("TravelHost"),true,TEXT("\"track\":2,\"players_reconnected\":true"));FPlatformMisc::RequestExit(false);}
  if(T>90){Report(TEXT("TravelHost"),false,TEXT("\"timeout\":true"));FPlatformMisc::RequestExit(false);}
 }
 if(Flag(TEXT("BenchyNetworkAudit"))){
  static bool Started=false,Granted=false,NavigationGranted=false;ABenchyPawn* Remote=nullptr;for(TActorIterator<ABenchyPawn> It(W);It;++It)if(!It->IsBot&&!It->IsLocallyControlled()&&It->GetController())Remote=*It;
  if(!Started&&Remote&&T>8){M->StartRace();Started=true;}
  if(Started&&S->Phase==2&&Remote){if(!Granted){Remote->Coins=7;Remote->Item=3;Remote->ForceNetUpdate();Granted=true;}
   if(Flag(TEXT("BenchyNavigationNetworkAudit"))&&!NavigationGranted&&S->Clock()>2){Remote->MissedCheckpoint=true;Remote->ForceNetUpdate();NavigationGranted=true;}
   if(S->Clock()>9){bool Good=Remote->ColorIndex==4&&Remote->ShieldUntil>0;Report(TEXT("NetworkHost"),Good,FString::Printf(TEXT("\"remote_color\":%d,\"remote_coins\":%d,\"remote_shield\":%.1f"),Remote->ColorIndex,Remote->Coins,Remote->ShieldUntil));FPlatformMisc::RequestExit(false);}}
  if(T>80){Report(TEXT("NetworkHost"),false,TEXT("\"timeout\":true"));FPlatformMisc::RequestExit(false);}
 }
}
void BenchyValidateClient(ABenchyController* PC){
 BenchyValidateFinishClient(PC);
 BenchyValidateNozzleClient(PC);
 auto W=PC->GetWorld();float T=W->GetTimeSeconds();
 if(Flag(TEXT("BenchyMenuAudit"))){static int Stage=0;if(Stage==0&&T>6){PC->ShowMenu();Shot(TEXT("Menu"));Stage=1;}if(Stage==1&&T>9){PC->ShowMenu(1);Stage=2;}if(Stage==2&&T>11){Shot(TEXT("Graphics"));Stage=3;}if(T>14)FPlatformMisc::RequestExit(false);}
 if(Flag(TEXT("BenchyTravelAudit"))&&!PC->HasAuthority()){
  static bool SawHarbor=false,Reported=false;SawHarbor|=ABenchyWorld::ActiveTrack==1;
  if(!Reported&&SawHarbor&&ABenchyWorld::ActiveTrack==2&&PC->GetPawn()&&T>3){Report(TEXT("TravelClient"),true,TEXT("\"track\":2,\"pawn_after_travel\":true"));Reported=true;}
 }
 if(!Flag(TEXT("BenchyNetworkAudit"))||PC->HasAuthority())return;
 static int Stage=0;static bool Coins=false,Shield=false,Moved=false,Reset=false,ItemEffect=false,MissedReplicated=false;static FVector Start;
 auto S=W->GetGameState<ABenchyRaceState>();auto B=Cast<ABenchyPawn>(PC->GetPawn());if(!S||!B)return;
 if(S->Phase==2){float C=S->Clock();
  if(Stage==0){Start=B->GetActorLocation();B->ServerColor(4);Stage=1;}
  Coins|=B->Coins==7;if(C>1&&B->Item==3)B->ServerUseItem();Shield|=B->ShieldUntil>S->GetServerWorldTimeSeconds();
  MissedReplicated|=B->MissedCheckpoint;
  if(auto FX=B->FindComponentByClass<UBenchyItemFX>())ItemEffect|=B->ItemEvent==3&&FX->GetNumSections()>0;
  if(C<4)B->AuditInput(.6,0);else B->AuditInput(0,0);
  Moved|=FVector::Dist2D(Start,B->GetActorLocation())>500;
  if(C>4&&Stage==1){B->ServerReset();Stage=2;}
  Reset|=Stage==2&&!B->MissedCheckpoint&&B->RespawnUntil>S->GetServerWorldTimeSeconds()&&FVector::Dist2D(B->GetActorLocation(),ABenchyWorld::RoutePoint(0))<600;
  if(C>7&&Stage==2){int Peers=0;for(TActorIterator<ABenchyPawn> It(W);It;++It)Peers++;
   Report(TEXT("NetworkClient"),Coins&&Shield&&ItemEffect&&Moved&&Reset&&(!Flag(TEXT("BenchyNavigationNetworkAudit"))||MissedReplicated)&&B->ColorIndex==4&&Peers>=6&&ABenchyWorld::ActiveTrack==1,FString::Printf(TEXT("\"coins_replicated\":%s,\"item_rpc\":%s,\"item_effect_replicated\":%s,\"movement\":%s,\"reset_rpc\":%s,\"missed_checkpoint_replicated\":%s,\"peers\":%d,\"track\":%d"),Coins?TEXT("true"):TEXT("false"),Shield?TEXT("true"):TEXT("false"),ItemEffect?TEXT("true"):TEXT("false"),Moved?TEXT("true"):TEXT("false"),Reset?TEXT("true"):TEXT("false"),MissedReplicated?TEXT("true"):TEXT("false"),Peers,ABenchyWorld::ActiveTrack));Stage=3;}
 }
 if(T>70){Report(TEXT("NetworkClient"),false,TEXT("\"timeout\":true"));FPlatformMisc::RequestExit(false);}
}
