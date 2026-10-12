#include "BenchyRace.h"
#include "BenchyPawn.h"
#include "BenchyWorld.h"
#include "BenchyItemFX.h"
#include "CourseNavigation.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

ABenchyProjectile::ABenchyProjectile(){
    bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(true);SetNetUpdateFrequency(45);PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Nozzle=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Nozzle"));Nozzle->SetupAttachment(RootComponent);Nozzle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto FX=CreateDefaultSubobject<UBenchyItemFX>(TEXT("ProjectileTrail"));FX->SetupAttachment(RootComponent);
}
void ABenchyProjectile::BeginPlay(){
    Super::BeginPlay();Nozzle->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cone.Cone")));
    Nozzle->SetRelativeScale3D(FVector(.65,.65,1.3));Nozzle->SetRelativeRotation(FRotator(-90,0,0));
    ProjectileMaterial=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Filament.M_Filament")),this);
    Nozzle->SetMaterial(0,ProjectileMaterial);OnRep_Guided();Direction=GetActorForwardVector();
    if(auto B=Cast<ABenchyPawn>(GetOwner())){auto P=B->GetActorLocation();RouteCursor=BenchyCourse::ProjectSector(ABenchyWorld::ActiveTrack,B->Passed,{P.X,P.Y}).Index;}
    DepartureOffset=GetActorLocation()-ABenchyWorld::RoutePoint(RouteCursor);DepartureOffset.Z=0;
    SetLifeSpan(Guided?30:6);
}
void ABenchyProjectile::OnRep_Guided(){if(ProjectileMaterial)ProjectileMaterial->SetVectorParameterValue(TEXT("FilamentColor"),Guided?FLinearColor(.035,.65,.85):FLinearColor(1,.23,.015));}
void ABenchyProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ABenchyProjectile,Guided);DOREPLIFETIME(ABenchyProjectile,Target);
}
float ABenchyProjectile::ObstacleFraction(FVector A,FVector B,FVector& At) const {
    float Fraction=2;FHitResult Hit;FCollisionQueryParams Q;Q.AddIgnoredActor(this);
    // Boat interception is swept separately and ordered against the world hit.
    for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)Q.AddIgnoredActor(*It);
    if(GetWorld()->LineTraceSingleByChannel(Hit,A,B,ECC_Visibility,Q)){Fraction=Hit.Time;At=Hit.ImpactPoint;}
    const int Count=FMath::Max(1,FMath::CeilToInt(FVector::Dist(A,B)/120.f));
    for(int I=1;I<=Count;I++){
        const float U=float(I)/Count;if(U>=Fraction)break;const FVector P=FMath::Lerp(A,B,U);
        if(ABenchyWorld::GroundHeight(FVector2D(P))>=P.Z-12){Fraction=U;At=P;break;}
    }
    return Fraction;
}
bool ABenchyProjectile::ClearWaterPath(FVector A,FVector B) const {FVector At;return ObstacleFraction(A,B,At)>1;}
void ABenchyProjectile::Tick(float DT){
    Super::Tick(DT);if(!HasAuthority())return;
    auto State=GetWorld()->GetGameState<ABenchyRaceState>();
    if(!State||State->Phase!=2||(Guided&&(!IsValid(Target)||Target->Finished||Target->IsActorBeingDestroyed()))){Destroy();return;}
    // Small swept steps keep both guidance and collision reliable during hitches.
    const int Steps=FMath::Max(1,FMath::CeilToInt(DT/.02f));const float Step=DT/Steps;
    for(int N=0;N<Steps;N++){
        FlightAge+=Step;const float Speed=Guided?7000.f:5500.f;const FVector A=GetActorLocation();FVector B;
        if(Guided){
            const auto TP=Target->GetActorLocation();
            auto Projection=BenchyCourse::ProjectSector(ABenchyWorld::ActiveTrack,Target->Passed,{TP.X,TP.Y});
            const float Arc=FMath::Fmod((Backward?RouteCursor-Projection.Index:Projection.Index-RouteCursor)+640.f,320.f);
            FVector Aim=TP+Target->GetVelocity()*FMath::Min(.22f,float(FVector::Dist2D(A,TP))/Speed);
            Aim.Z=ABenchyWorld::WaterHeight(Aim,GetWorld()->GetTimeSeconds())+80;
            if(Arc<12&&FVector::Dist2D(A,TP)<2200&&ClearWaterPath(A,Aim)){
                Direction=(Aim-A).GetSafeNormal2D();B=A+Direction*Speed*Step;
            }else{
                const float Units=(ABenchyWorld::RoutePoint(RouteCursor+.25f)-ABenchyWorld::RoutePoint(RouteCursor-.25f)).Size2D()*2;
                RouteCursor+= (Backward?-1.f:1.f)*Speed*Step/FMath::Max(Units,100.f);
                RouteCursor=FMath::Fmod(RouteCursor+640.f,320.f);
                B=ABenchyWorld::RoutePoint(RouteCursor)+DepartureOffset*FMath::Exp(-FlightAge*3);
                Direction=(B-A).GetSafeNormal2D();
            }
        }else B=A+Direction*Speed*Step;
        B.Z=ABenchyWorld::WaterHeight(B,GetWorld()->GetTimeSeconds())+80;
        FVector WorldAt;float First=ObstacleFraction(A,B,WorldAt);ABenchyPawn* Victim=nullptr;
        const FVector Segment=B-A;const double Length2=FMath::Max(1.,Segment.SizeSquared());
        for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It){
            if(*It==GetOwner()||It->Finished||(Guided&&*It!=Target))continue;
            const float U=FMath::Clamp(float(FVector::DotProduct(It->GetActorLocation()-A,Segment)/Length2),0.f,1.f);
            if(U<First&&FVector::Dist(It->GetActorLocation(),A+Segment*U)<200){First=U;Victim=*It;}
        }
        if(Victim){Victim->HitByItem();Destroy();return;}
        if(First<=1){Impact(WorldAt);Destroy();return;}
        SetActorLocationAndRotation(B,Direction.Rotation());
    }
}
void ABenchyProjectile::Impact_Implementation(FVector Position){ABenchyItemBurst::Spawn(GetWorld(),Position,Guided?6:4);}
