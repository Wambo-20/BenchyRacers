#include "BenchyItemFX.h"
#include "BenchyPawn.h"
#include "BenchyRace.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

namespace {
struct FEffectMesh {
    TArray<FVector> V,N; TArray<int32> I; TArray<FVector2D> UV;
    TArray<FLinearColor> C; TArray<FProcMeshTangent> Tangents;
    void Strip(FVector A,FVector B,FVector Side,float Width,FLinearColor Color,float EndAlpha=-1) {
        const int Start=V.Num();
        for(int J=0;J<2;J++)for(int K=0;K<2;K++) {
            V.Add((J?B:A)+Side*((K?1:-1)*Width)); N.Add(FVector::UpVector);
            UV.Add(FVector2D(K,J));FLinearColor Tint=Color;if(J&&EndAlpha>=0)Tint.A=EndAlpha;C.Add(Tint);
            Tangents.Add(FProcMeshTangent(1,0,0));
        }
        for(int K:{0,1,2,2,1,3})I.Add(Start+K);
    }
    void Ring(FVector Center,FVector X,FVector Y,float Radius,float Width,FLinearColor Color,float Phase=0,float Arc=2*PI) {
        const int Count=48;
        for(int J=0;J<Count;J++) {
            float A=Phase+Arc*J/Count,B=Phase+Arc*(J+1)/Count;
            FVector R=X*FMath::Cos(A)+Y*FMath::Sin(A),S=X*FMath::Cos(B)+Y*FMath::Sin(B);
            Strip(Center+R*Radius,Center+S*Radius,(R+S).GetSafeNormal(),Width,Color);
        }
    }
    void Apply(UProceduralMeshComponent* Mesh) {
        if(V.IsEmpty()){Mesh->ClearAllMeshSections();return;}
        Mesh->CreateMeshSection_LinearColor(0,V,I,N,UV,C,Tangents,false);
    }
};
FLinearColor Tint(int Kind,float A=1) {
    if(Kind==6)return FLinearColor(.04,.8,1,A);
    if(Kind==2)return FLinearColor(1,.30,.025,A);
    if(Kind==3||Kind==5)return FLinearColor(.025,.68,1,A);
    if(Kind==4)return FLinearColor(1,.12,.015,A);
    return FLinearColor(.25,1,.55,A);
}
void Setup(UProceduralMeshComponent* M) {
    M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCastShadow(false);
    M->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_ItemGlow.M_ItemGlow")));
}
}
UBenchyItemFX::UBenchyItemFX(const FObjectInitializer& Initializer):Super(Initializer){
    PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostPhysics;
    SetAbsolute(true,true,true);SetCollisionEnabled(ECollisionEnabled::NoCollision);SetCastShadow(false);
}
void UBenchyItemFX::BeginPlay(){Super::BeginPlay();Setup(this);if(GetNetMode()==NM_DedicatedServer)SetComponentTickEnabled(false);}
void UBenchyItemFX::TickComponent(float DT,ELevelTick Type,FActorComponentTickFunction* Fn){
    Super::TickComponent(DT,Type,Fn);SetWorldTransform(FTransform::Identity);
    auto B=Cast<ABenchyPawn>(GetOwner());auto P=Cast<ABenchyProjectile>(GetOwner());if(!B&&!P)return;
    auto State=GetWorld()->GetGameState<ABenchyRaceState>();const float T=State?State->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();
    const FVector Pos=GetOwner()->GetActorLocation(),F=GetOwner()->GetActorForwardVector(),R=GetOwner()->GetActorRightVector(),U=FVector::UpVector;
    FEffectMesh M;
    if(B&&B->ShieldUntil>T){
        const float Fade=FMath::Min(1.f,(B->ShieldUntil-T)*2),Pulse=.75f+.25f*FMath::Sin(T*5);
        const FVector Center=Pos+U*45;
        M.Ring(Center,F,R,235,5,Tint(3,.8f*Fade),T,PI*1.7f);
        // Moving meridians wrap the hull without a solid sphere obscuring it.
        for(int K=0;K<3;K++) {
            float Angle=T*1.5f+K*PI/3;FVector X=F*FMath::Cos(Angle)+R*FMath::Sin(Angle);
            M.Ring(Center,X,U,225,3.5f,Tint(3,Fade*Pulse*.55f),T*.7f+K,PI*1.65f);
        }
    }
    const bool Turbo=B&&B->ItemBoostUntil>T;
    for(auto& Point:Trail)Point.Age+=DT;
    while(Trail.Num()&&Trail.Last().Age>(P?.28f:.55f))Trail.Pop();
    if(Trail.Num()&&FVector::Dist(Pos,Trail[0].Position)>1600)Trail.Reset();
    SampleTime+=DT;
    if((Turbo||P)&&SampleTime>.025f){Trail.Insert({Pos-F*(P?45:185)+U*(P?0:-20),0},0);SampleTime=0;}
    for(int J=0;J+1<Trail.Num();J++) {
        float Life=P?.28f:.55f,Alpha=FMath::Square(1-Trail[J].Age/Life);
        FVector Side=FVector::CrossProduct((Trail[J].Position-Trail[J+1].Position).GetSafeNormal(),U).GetSafeNormal();
        auto Color=Tint(P&&P->Guided?6:2,Alpha*.55f);
        M.Strip(Trail[J].Position,Trail[J+1].Position,Side,(P?17:45)*(1-Trail[J].Age/Life),Color);
        M.Strip(Trail[J].Position,Trail[J+1].Position,U,P?9:15,Color);
    }
    if(Turbo||P){
        float Length=(P?170:330)*(1+.12f*FMath::Sin(T*43));
        for(int K=0;K<(P?1:2);K++){
            FVector Start=Pos-F*(P?55:165)+R*(P?0:(K?45:-45))+U*(P?0:-18);
            for(int J=0;J<6;J++){
                float A=J*PI/3+T*9;FVector Side=R*FMath::Cos(A)+U*FMath::Sin(A);
                M.Strip(Start,Start-F*Length,Side,P?13:18,P&&P->Guided?FLinearColor(.08,.85,1,.85):FLinearColor(1,.55,.08,.85),0);
            }
        }
    }
    M.Apply(this);
}
ABenchyItemBurst::ABenchyItemBurst(){
    PrimaryActorTick.bCanEverTick=true;Mesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Burst"));RootComponent=Mesh;
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCastShadow(false);
}
void ABenchyItemBurst::Spawn(UWorld* W,FVector Position,int32 Kind){
    if(!W||W->GetNetMode()==NM_DedicatedServer)return;
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if(auto B=W->SpawnActor<ABenchyItemBurst>(Position,FRotator::ZeroRotator,Params))B->Effect=Kind;
}
void ABenchyItemBurst::BeginPlay(){Super::BeginPlay();Setup(Mesh);SetLifeSpan(.85f);}
void ABenchyItemBurst::Tick(float DT){
    Super::Tick(DT);Age+=DT;float A=FMath::Clamp(1-Age/.85f,0.f,1.f);FEffectMesh M;
    M.Ring(FVector(0,0,45),FVector::ForwardVector,FVector::RightVector,90+Age*360,7*A,Tint(Effect,A*.75f));
    for(int J=0;J<18;J++){
        float Angle=J*2*PI/18;FVector D(FMath::Cos(Angle),FMath::Sin(Angle),.6f+FMath::Sin(J*2.7f)*.5f);
        FVector P=D*(80+Age*(Effect==4?650:330))+FVector(0,0,60-Age*Age*150);
        M.Strip(P,P-D*(16+35*A),FVector(-D.Y,D.X,0),4*A,Tint(Effect,A));
    }
    M.Apply(Mesh);
}
