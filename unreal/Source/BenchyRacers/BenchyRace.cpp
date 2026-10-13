#include "BenchyRace.h"
#include "BenchyItemFX.h"
#include "BenchyPawn.h"
#include "BenchyWorld.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
ABenchyRaceState::ABenchyRaceState(){ bReplicates=true; }
void ABenchyRaceState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ABenchyRaceState,Phase); DOREPLIFETIME(ABenchyRaceState,StartTime); DOREPLIFETIME(ABenchyRaceState,FirstFinishTime);DOREPLIFETIME(ABenchyRaceState,Results);DOREPLIFETIME(ABenchyRaceState,LobbyChat);
}
float ABenchyRaceState::Clock() const{return GetServerWorldTimeSeconds()-StartTime;}
void ABenchyRaceState::RegisterRacers(){
    if(!HasAuthority())return;Results.Reset();
    for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(!It->IsActorBeingDestroyed()){
        FBenchyResult R;R.EntryId=It->RaceEntryId;R.Name=It->RacerName;R.Boat=*It;Results.Add(R);
    }ForceNetUpdate();
}
int32 ABenchyRaceState::RecordFinish(ABenchyPawn* Boat){
    if(!HasAuthority()||!Boat)return 0;
    auto R=Results.FindByPredicate([Boat](const FBenchyResult& E){return E.EntryId==Boat->RaceEntryId&&E.Boat==Boat;});
    if(!R)return 0;if(R->Place)return R->Place;
    int32 Place=1;for(const auto& E:Results)if(E.Place>0)Place++;
    R->Place=Place;R->Time=Boat->RaceTime;R->Progress=1000-Place;
    if(FirstFinishTime==0)FirstFinishTime=GetServerWorldTimeSeconds();ForceNetUpdate();return Place;
}
void ABenchyRaceState::UpdateResults(){
    if(!HasAuthority())return;bool Changed=false;
    for(auto& R:Results){
        if(IsValid(R.Boat)&&!R.Boat->IsActorBeingDestroyed()){
            if(R.Name!=R.Boat->RacerName){R.Name=R.Boat->RacerName;Changed=true;}
            if(!R.Place)R.Progress=R.Boat->Progress();
        }else if(!R.Disconnected){R.Disconnected=true;Changed=true;}
    }if(Changed)ForceNetUpdate();
}
bool ABenchyRaceState::EveryoneDone() const {
    if(Results.IsEmpty())return false;
    for(const auto& R:Results)if(!R.Place&&!R.Disconnected)return false;return true;
}
TArray<FBenchyResult> ABenchyRaceState::OrderedResults() const {
    auto Order=Results;Order.Sort([](const FBenchyResult& A,const FBenchyResult& B){
        if(A.Place||B.Place){if(!A.Place)return false;if(!B.Place)return true;return A.Place<B.Place;}
        if(A.Disconnected!=B.Disconnected)return !A.Disconnected;
        return A.Progress==B.Progress?A.EntryId<B.EntryId:A.Progress>B.Progress;
    });return Order;
}
FString ABenchyRaceState::FormatTime(float Seconds){
    int32 MS=FMath::Max(0,FMath::RoundToInt(Seconds*1000));
    return FString::Printf(TEXT("%d:%02d.%03d"),MS/60000,(MS/1000)%60,MS%1000);
}
ABenchyPickup::ABenchyPickup(){bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));}
void ABenchyPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ABenchyPickup,Kind);DOREPLIFETIME(ABenchyPickup,Available);
}
void ABenchyPickup::BeginPlay(){
    Super::BeginPlay(); Anchor=GetActorLocation();
    auto Part=[&](FVector Scale,FVector Offset,FLinearColor Color){
        auto C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        C->SetRelativeScale3D(Scale);C->SetRelativeLocation(Offset);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();
        auto M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Filament.M_Filament")),this);
        M->SetVectorParameterValue(TEXT("FilamentColor"),Color);C->SetMaterial(0,M);
    };
    if(Kind==0){Part(FVector(.95,.95,.13),FVector::ZeroVector,FLinearColor(1,.59,.035));Part(FVector(.60,.60,.16),FVector::ZeroVector,FLinearColor(.2,.095,.01));
        for(float Face:{-1.f,1.f}){auto Mark=NewObject<UTextRenderComponent>(this);Mark->SetupAttachment(RootComponent);Mark->SetText(FText::FromString(TEXT("M")));Mark->SetWorldSize(43);Mark->SetHorizontalAlignment(EHTA_Center);Mark->SetVerticalAlignment(EVRTA_TextCenter);Mark->SetTextRenderColor(FColor(255,224,95));Mark->SetRelativeLocation(FVector(0,0,Face*9));Mark->SetRelativeRotation(FRotator(Face*90,0,0));Mark->RegisterComponent();}
    }
    else {Part(FVector(.86,.86,.7),FVector::ZeroVector,FLinearColor(.03,.75,.65));for(float Z:{-40.f,40.f})Part(FVector(1.25,1.25,.10),FVector(0,0,Z),FLinearColor(.08,.10,.12));}
    OnRep_Available();
}
void ABenchyPickup::OnRep_Available(){CollectedAt=GetWorld()->GetTimeSeconds();SetActorHiddenInGame(false);if(Available)SetActorScale3D(FVector(1));}
void ABenchyPickup::Tick(float DT){
    Super::Tick(DT);float T=GetWorld()->GetTimeSeconds();
    SetActorLocation(Anchor+FVector(0,0,25*FMath::Sin(T*2+Anchor.X*.001)));SetActorRotation(FRotator(Kind?65:90,T*65,0));
    if(!Available){float Age=T-CollectedAt;SetActorScale3D(FVector(FMath::Max(.001f,1-Age/.4f)));AddActorWorldOffset(FVector(0,0,Age*350));AddActorWorldRotation(FRotator(0,Age*720,0));SetActorHiddenInGame(Age>=.4f);}
    else SetActorScale3D(FVector(1));
    if(!HasAuthority())return;
    if(!Available){if(T>ReadyAt){Available=true;OnRep_Available();}return;}
    const auto S=GetWorld()->GetGameState<ABenchyRaceState>();if(!S||S->Phase!=2)return;
    for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(!It->Finished&&FVector::Dist2D(It->GetActorLocation(),Anchor)<280){
        if(It->Collect(Kind)){Available=false;ReadyAt=T+(Kind?9.f:6.f);OnRep_Available();ForceNetUpdate();break;}
    }
}

void ABenchyRaceState::AddLobbyLine(const FString& Line){if(!HasAuthority())return;LobbyChat.Add(Line.Left(260));while(LobbyChat.Num()>30)LobbyChat.RemoveAt(0);ForceNetUpdate();}
