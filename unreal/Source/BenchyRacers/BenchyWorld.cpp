#include "BenchyWorld.h"
#include "WaveMath.h"
#include "TrackLayout.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/TextRenderComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace {
constexpr int32 TerrainResolution=512;
const FVector2D TerrainExtent(35000,30000);
struct FIsland { FVector2D Center,Radius; float Height; };
const FIsland Islands[]={{{-1500,500},{17500,15000},1550},
    {{-26000,-17000},{5700,3000},320},{{27000,15000},{6200,4100},420},
    {{24000,-20000},{5300,2900},280},{{-24500,18000},{4700,5200},370}};
}

int32 ABenchyWorld::ActiveTrack=0;
FString ABenchyWorld::TrackName(int32 I){static const TCHAR* N[]={TEXT("Sonnendeck"),TEXT("Kupferhafen"),TEXT("Frostfjord")};return N[FMath::Clamp(I,0,2)];}
void ABenchyWorld::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ABenchyWorld,Track);}
ABenchyWorld::ABenchyWorld() {
    bReplicates=true;bAlwaysRelevant=true;
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PrePhysics;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
}
void ABenchyWorld::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);
    auto Clock=GetWorld()->GetGameState();const float Time=Clock?Clock->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();
    if(OceanMaterial) OceanMaterial->SetScalarParameterValue(TEXT("SeaTime"),Time);
    for(auto& Effect:WaterEffects) if(Effect) Effect->SetScalarParameterValue(TEXT("SeaTime"),Time);
    BuoyClock+=DeltaSeconds;if(BuoyClock>.05f){UpdateBuoys(Time);BuoyClock=0;}
}
float ABenchyWorld::WaterHeight(FVector P,float T) { return BenchySea::Height(P.X,P.Y,T); }
FVector ABenchyWorld::WaterNormal(FVector P,float T) {
    double X,Y; BenchySea::Gradient(P.X,P.Y,T,X,Y);
    return FVector(-X,-Y,1).GetSafeNormal();
}
FVector ABenchyWorld::RoutePoint(float Index) {
    auto P=BenchyTracks::Route(ActiveTrack,Index);return FVector(P.x,P.y,0);
}
float ABenchyWorld::GroundHeight(FVector2D P) {
    return BenchyTracks::Height(ActiveTrack,P.X,P.Y);
}
float ABenchyWorld::TerrainSurfaceHeight(FVector2D P) {
    // Match the actual triangulated surface, including interpolation between vertices.
    double GX=(P.X/TerrainExtent.X+1)*TerrainResolution*.5;
    double GY=(P.Y/TerrainExtent.Y+1)*TerrainResolution*.5;
    int32 X=FMath::Clamp(FMath::FloorToInt(GX),0,TerrainResolution-1);
    int32 Y=FMath::Clamp(FMath::FloorToInt(GY),0,TerrainResolution-1);
    float U=FMath::Clamp(float(GX-X),0.f,1.f),V=FMath::Clamp(float(GY-Y),0.f,1.f);
    auto Height=[&](int32 IX,int32 IY) {
        return GroundHeight(FVector2D((float(IX)/TerrainResolution*2-1)*TerrainExtent.X,
            (float(IY)/TerrainResolution*2-1)*TerrainExtent.Y));
    };
    if(U+V<=1) {
        float A=Height(X,Y);
        return A+(Height(X+1,Y)-A)*U+(Height(X,Y+1)-A)*V;
    }
    float D=Height(X+1,Y+1);
    return D+(Height(X,Y+1)-D)*(1-U)+(Height(X+1,Y)-D)*(1-V);
}
void ABenchyWorld::BeginPlay() {
    Super::BeginPlay();
    if(HasAuthority())Track=ActiveTrack;else ActiveTrack=Track;
    if(GetNetMode()==NM_DedicatedServer)return;
    BuildSurface(FVector2D::ZeroVector,FVector2D(60000,60000),384,true);
    // Far-water skirt: keep the dense animated grid around the course, extend
    // the sea to the horizon without drawing a second surface over that grid.
    BuildSurface(FVector2D(0,-280000),FVector2D(500000,220000),32,true);
    BuildSurface(FVector2D(0,280000),FVector2D(500000,220000),32,true);
    BuildSurface(FVector2D(-280000,0),FVector2D(220000,60000),32,true);
    BuildSurface(FVector2D(280000,0),FVector2D(220000,60000),32,true);
    BuildSurface(FVector2D::ZeroVector,TerrainExtent,TerrainResolution,false);
    BuildCliffs();
    BuildVegetation();
    BuildFabScenery();
    if(ActiveTrack==0)BuildWaterfall();
    BuildWayfinding();BuildBuoys();BuildLandmarks();
    if(ActiveTrack>0){
        for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It){
            It->SetActorRotation(ActiveTrack==1?FRotator(-18,-80,0):FRotator(-21,-155,0));
            auto L=Cast<UDirectionalLightComponent>(It->GetLightComponent());L->SetIntensity(ActiveTrack==1?19000:14500);L->SetLightColor(ActiveTrack==1?FLinearColor(1,.72,.48):FLinearColor(.75,.85,1));
        }
        for(TActorIterator<AExponentialHeightFog> It(GetWorld());It;++It){It->GetComponent()->SetFogDensity(ActiveTrack==1?.006f:.010f);It->GetComponent()->SetFogInscatteringColor(ActiveTrack==1?FLinearColor(.45,.32,.20):FLinearColor(.48,.62,.77));}
    }
}

void ABenchyWorld::BuildSurface(FVector2D Center,FVector2D Extent,int32 N,bool Ocean) {
    UProceduralMeshComponent* Mesh=NewObject<UProceduralMeshComponent>(this);
    Mesh->SetupAttachment(RootComponent);Mesh->bUseAsyncCooking=true;Mesh->RegisterComponent();
    Surfaces.Add(Mesh);
    TArray<FVector> V,Normals;TArray<int32> Tri;TArray<FVector2D> UV;
    TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
    V.Reserve((N+1)*(N+1));
    for(int32 Y=0;Y<=N;Y++) for(int32 X=0;X<=N;X++) {
        FVector2D P=Center+FVector2D((float(X)/N*2-1)*Extent.X,(float(Y)/N*2-1)*Extent.Y);
        float H=Ocean?0:GroundHeight(P);
        FVector Normal=FVector::UpVector;
        if(!Ocean) Normal=FVector(GroundHeight(P-FVector2D(30,0))-GroundHeight(P+FVector2D(30,0)),
            GroundHeight(P-FVector2D(0,30))-GroundHeight(P+FVector2D(0,30)),60).GetSafeNormal();
        float Rock=FMath::Clamp((.75f-Normal.Z)*3.f,0.f,1.f);
        float Grass=FMath::Clamp((H-55)/180,0.f,1.f)*(1-Rock);
        V.Add(FVector(P.X,P.Y,H));Normals.Add(Normal);UV.Add(P/500);
        Colors.Add(FLinearColor(1-Grass-Rock,Grass,Rock,1));
        const FVector Tangent=FVector(1,0,-Normal.X/FMath::Max(.001f,Normal.Z)).GetSafeNormal();
        Tangents.Add(FProcMeshTangent(Tangent,false));
    }
    for(int32 Y=0;Y<N;Y++) for(int32 X=0;X<N;X++) {
        int32 A=Y*(N+1)+X,B=A+1,C=A+N+1,D=C+1;
        // Unreal's visible upper face uses the opposite winding to the XY cross product.
        Tri.Append({A,C,B,B,C,D});
    }
    Mesh->CreateMeshSection_LinearColor(0,V,Tri,Normals,UV,Colors,Tangents,!Ocean);
    UMaterialInterface* SurfaceMaterial=LoadObject<UMaterialInterface>(nullptr,Ocean?
        TEXT("/Game/Materials/M_Ocean.M_Ocean"):ActiveTrack==2?TEXT("/Game/Materials/M_FrostTerrain.M_FrostTerrain"):TEXT("/Game/Materials/M_Terrain.M_Terrain"));
    if(Ocean && SurfaceMaterial) {
        if(!OceanMaterial){OceanMaterial=UMaterialInstanceDynamic::Create(SurfaceMaterial,this);OceanMaterial->SetScalarParameterValue(TEXT("FrostWater"),ActiveTrack==2?1:0);}
        Mesh->SetMaterial(0,OceanMaterial);
    } else Mesh->SetMaterial(0,SurfaceMaterial);
    Mesh->SetCastShadow(!Ocean);
}
void ABenchyWorld::BuildVegetation() {
    const TCHAR* Paths[]={TEXT("/Game/Megascans/3D_Plants/AlexandraPalm/SM_AlexandraPalm_01.SM_AlexandraPalm_01"),
        TEXT("/Game/Megascans/3D_Plants/EuropeanHornbeam/Geometry/SimpleWind/SM_EuropeanHornbeam_Forest_01.SM_EuropeanHornbeam_Forest_01"),
        TEXT("/Game/Megascans/3D_Plants/BostonFern/SM_BostonFern_01.SM_BostonFern_01"),
        TEXT("/Game/Megascans/3D_Assets/LichenedForestBoulder/SM_LichenedForestBoulder_01.SM_LichenedForestBoulder_01"),
        TEXT("/Game/Megascans/3D_Assets/TundraMossyBoulder/SM_TundraMossyBoulder_01.SM_TundraMossyBoulder_01")};
    const int32 Summer[]={500,1000,10500,170,35},Harbor[]={35,520,4800,150,40},Winter[]={0,330,900,95,35};
    const int32* Counts=ActiveTrack==2?Winter:ActiveTrack==1?Harbor:Summer;
    FRandomStream RNG(8811+ActiveTrack*719);
    int32 PlacedCounts[5]={};
    float WorstRockSupport=-1e9f;
    for(int32 Type=0;Type<5;Type++) {
        TArray<UStaticMesh*> Variants;
        auto AddAsset=[&](const TCHAR* Path) {
            if(UStaticMesh* Asset=LoadObject<UStaticMesh>(nullptr,Path)) Variants.Add(Asset);
            else UE_LOG(LogTemp,Error,TEXT("Missing sample vegetation: %s"),Path);
        };
        AddAsset(Paths[Type]);
        if(Type==0) {
            AddAsset(TEXT("/Game/Megascans/3D_Plants/AlexandraPalm/SM_AlexandraPalm_02.SM_AlexandraPalm_02"));
            AddAsset(TEXT("/Game/Megascans/3D_Plants/ArecaPalm/SM_ArecaPalm_01.SM_ArecaPalm_01"));
        }
        if(Type==1) {
            AddAsset(TEXT("/Game/Megascans/3D_Plants/EuropeanHornbeam/Geometry/SimpleWind/SM_EuropeanHornbeam_Forest_03.SM_EuropeanHornbeam_Forest_03"));
            AddAsset(TEXT("/Game/Megascans/3D_Plants/EuropeanHornbeam/Geometry/SimpleWind/SM_EuropeanHornbeam_Forest_06.SM_EuropeanHornbeam_Forest_06"));
        }
        if(Type==2) {
            AddAsset(TEXT("/Game/Megascans/3D_Plants/BeechFern/SM_BeechFern_03.SM_BeechFern_03"));
            if(ActiveTrack!=2){
            AddAsset(TEXT("/Game/Megascans/3D_Plants/CanePalm/SM_CanePalm_01.SM_CanePalm_01"));
            AddAsset(TEXT("/Game/Megascans/3D_Plants/BroadleafLadyPalm/SM_BroadleafLadyPalm_01.SM_BroadleafLadyPalm_01"));
            AddAsset(TEXT("/Game/Megascans/3D_Plants/ZamiaPalm/SM_ZamiaPalm_01.SM_ZamiaPalm_01"));
            }
        }
        if(Variants.IsEmpty()) continue;
        TArray<UHierarchicalInstancedStaticMeshComponent*> Batches;
        for(UStaticMesh* Asset:Variants) {
            auto Batch=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            Batch->SetupAttachment(RootComponent);Batch->SetStaticMesh(Asset);
            if(ActiveTrack==2)for(int32 Slot=0;Slot<Asset->GetStaticMaterials().Num();Slot++){
                auto Base=Asset->GetMaterial(Slot);if(!Base)continue;FString Name=TEXT("M_Winter_")+Base->GetName();
                auto Snow=LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/Materials/")+Name+TEXT(".")+Name));if(Snow)Batch->SetMaterial(Slot,Snow);
            }
            Batch->RegisterComponent();
            if(Type==2) Batch->SetCastShadow(false);
            Batch->SetCollisionEnabled(Type>=3?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
            Batch->SetCullDistances(Type==2?22000:100000,Type==2?35000:160000);
            Batches.Add(Batch);
        }
        TArray<FVector2D> Occupied;
        for(int32 I=0;I<Counts[Type]*90 && PlacedCounts[Type]<Counts[Type];I++) {
            const auto& Spec=BenchyTracks::Land(ActiveTrack)[RNG.FRand()<.79f?0:RNG.RandRange(1,4)];
            const FIsland Island{FVector2D(Spec.center.x,Spec.center.y),FVector2D(Spec.radius.x,Spec.radius.y),float(Spec.height)};
            float Angle=RNG.FRandRange(0,2*PI);
            float Radius=Type==0?RNG.FRandRange(.55f,1.03f):Type==1?RNG.FRandRange(.02f,.95f):
                Type==2?RNG.FRandRange(.02f,.95f):Type==3?RNG.FRandRange(.55f,1.05f):RNG.FRandRange(.83f,1.07f);
            FVector2D P=Island.Center+FVector2D(FMath::Cos(Angle)*Island.Radius.X,FMath::Sin(Angle)*Island.Radius.Y)*Radius;
            float H=TerrainSurfaceHeight(P);
            float RouteDistance=1e9f;
            for(int32 R=0;R<320;R+=4) RouteDistance=FMath::Min(RouteDistance,FVector2D::Distance(P,FVector2D(RoutePoint(R))));
            if(RouteDistance<(Type>=3?2050.f:1450.f)) continue;
            if(Type==0 && (H<55 || H>2100)) continue;
            if(Type==1 && (H<90 || H>(ActiveTrack==2?1900:4500))) continue;
            if(Type==2 && (H<55 || H>3500)) continue;
            if(Type==3 && (H<40 || H>6500)) continue;
            if(Type==4 && (H<-80 || H>650)) continue;
            FVector Normal=FVector(TerrainSurfaceHeight(P-FVector2D(100,0))-TerrainSurfaceHeight(P+FVector2D(100,0)),
                TerrainSurfaceHeight(P-FVector2D(0,100))-TerrainSurfaceHeight(P+FVector2D(0,100)),200).GetSafeNormal();
            if(Normal.Z<(Type==4?.94f:Type>=3?.72f:Type==2?.48f:.74f)) continue;
            float Patch=.55f+.25f*FMath::Sin(P.X*.0007f)*FMath::Cos(P.Y*.0008f)+.18f*FMath::Sin(P.X*.0015f+P.Y*.001f);
            if(Type<3 && RNG.FRand()>Patch) continue;
            float Spacing=Type==0?560:Type==1?550:Type==2?100:Type==3?450:1000;
            bool TooClose=false;
            for(FVector2D Other:Occupied) if(FVector2D::DistSquared(P,Other)<Spacing*Spacing) {TooClose=true;break;}
            if(TooClose) continue;
            const int32 Variant=RNG.RandRange(0,Variants.Num()-1);
            const FBox Bounds=Variants[Variant]->GetBoundingBox();
            float Scale=Type==0?RNG.FRandRange(350,650)/Bounds.GetSize().Z:Type==1?RNG.FRandRange(850,1550)/Bounds.GetSize().Z:
                Type==2?RNG.FRandRange(Variant<2?40.f:100.f,Variant<2?95.f:230.f)/Bounds.GetSize().Z:
                (Type==3?RNG.FRandRange(120,420):RNG.FRandRange(400,850))/FMath::Max(Bounds.GetSize().X,Bounds.GetSize().Y);
            float Yaw=RNG.FRandRange(0,360);
            FQuat Rotation=Type>=3?FRotationMatrix::MakeFromZX(Normal,FRotator(0,Yaw,0).Vector()).ToQuat():FRotator(0,Yaw,0).Quaternion();
            FVector Position(P.X,P.Y,H-Bounds.Min.Z*Scale);
            FTransform Placement(Rotation,Position,FVector(Scale));
            if(Type>=3) {
                // Lower the entire footprint into the rendered terrain. Large coastal slabs
                // get 25 support samples, rather than being balanced on one centre point.
                int32 Samples=Type==4?4:2;
                float Clearance=-1e9f;
                for(int32 SX=0;SX<=Samples;SX++) for(int32 SY=0;SY<=Samples;SY++) {
                    FVector Foot(FMath::Lerp(Bounds.Min.X,Bounds.Max.X,double(SX)/Samples),
                        FMath::Lerp(Bounds.Min.Y,Bounds.Max.Y,double(SY)/Samples),Bounds.Min.Z);
                    FVector WorldFoot=Placement.TransformPosition(Foot);
                    Clearance=FMath::Max(Clearance,float(WorldFoot.Z)-TerrainSurfaceHeight(FVector2D(WorldFoot)));
                }
                const float Embed=Type==4?22.f:FMath::Max(20.f,float(Bounds.GetSize().Z)*Scale*.14f);
                Position.Z-=FMath::Max(0.f,Clearance)+Embed;
                Placement.SetTranslation(Position);
                WorstRockSupport=FMath::Max(WorstRockSupport,Clearance-FMath::Max(0.f,Clearance)-Embed);
            } else {
                Position.Z-=Type==2?3.f:8.f;
                Placement.SetTranslation(Position);
            }
            Batches[Variant]->AddInstance(Placement);
            Occupied.Add(P);
            PlacedCounts[Type]++;
        }
    }
    FString Report=FString::Printf(TEXT("{\n  \"palms\": %d,\n  \"trees\": %d,\n  \"ferns\": %d,\n  \"boulders\": %d,\n  \"coastal_rocks\": %d,\n  \"maximum_rock_footprint_clearance_cm\": %.2f,\n  \"support_check_passed\": %s\n}\n"),
        PlacedCounts[0],PlacedCounts[1],PlacedCounts[2],PlacedCounts[3],PlacedCounts[4],WorstRockSupport,
        WorstRockSupport<=0 && PlacedCounts[3]>0?TEXT("true"):TEXT("false"));
    FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("WorldPlacement.json")));
    UE_LOG(LogTemp,Display,TEXT("BENCHY_WORLD_PLACEMENT: %s"),*Report);
}

void ABenchyWorld::BuildCliffs() {
    const TCHAR* Paths[]={
        TEXT("/Game/Megascans/3D_Assets/HugeNordicCoastalCliff/SM_HugeNordicCoastalCliff_01.SM_HugeNordicCoastalCliff_01"),
        TEXT("/Game/Megascans/3D_Assets/HugeNordicCoastalCliff/SM_HugeNordicCoastalCliff_02.SM_HugeNordicCoastalCliff_02"),
        TEXT("/Game/Megascans/3D_Assets/HugeSandstoneCliff/SM_HugeSandstoneCliff_01.SM_HugeSandstoneCliff_01")};
    UStaticMesh* Assets[3]={};
    for(int32 I=0;I<3;I++) {
        Assets[I]=LoadObject<UStaticMesh>(nullptr,Paths[I]);
        if(!Assets[I]) {UE_LOG(LogTemp,Error,TEXT("Missing scanned cliff %s"),Paths[I]);return;}
    }
    // Asymmetric landmarks at river bends and headlands, separated by planted banks.
    const FVector Sites[]={FVector(-7300,2600,1700),FVector(-9600,6000,1450),
        FVector(-10800,10100,1100),FVector(4300,12700,850),FVector(13300,4600,1050),
        FVector(14800,-7100,900),FVector(7500,-11600,700),FVector(-10500,-10200,750),
        FVector(-14000,1800,1000),FVector(8500,9300,650)};
    FRandomStream RNG(3719);int32 Placed=0;
    for(int32 I=0;I<UE_ARRAY_COUNT(Sites);I++) {
        FVector2D P(Sites[I]);
        if(ActiveTrack>0){P=FVector2D(RoutePoint(I*31+12));auto Q=RoutePoint(I*31+13)-RoutePoint(I*31+12);P+=FVector2D(-Q.Y,Q.X).GetSafeNormal()*(I%2?3100:-3400);}
        if(BenchyTracks::Distance(ActiveTrack,{P.X,P.Y})<2250 || TerrainSurfaceHeight(P)<30)continue;
        UStaticMesh* Asset=Assets[(I*7+2)%3];const FBox B=Asset->GetBoundingBox();
        const float Scale=Sites[I].Z/B.GetSize().Z;
        FRotator Rotation(RNG.FRandRange(-7,7),RNG.FRandRange(0,360),RNG.FRandRange(-6,6));
        float Floor=TerrainSurfaceHeight(P);
        for(int32 K=0;K<8;K++) {
            const float Angle=K*PI/4;
            Floor=FMath::Min(Floor,TerrainSurfaceHeight(P+FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Sites[I].Z*.45f));
        }
        FVector Center(P.X,P.Y,Floor+Sites[I].Z*.27f);
        UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(this);
        C->SetupAttachment(RootComponent);C->SetStaticMesh(Asset);
        C->SetRelativeScale3D(FVector(Scale));C->SetRelativeRotation(Rotation);
        C->SetRelativeLocation(Center-Rotation.RotateVector(B.GetCenter()*Scale));
        C->RegisterComponent();C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Placed++;
    }
    UE_LOG(LogTemp,Display,TEXT("BENCHY_SCANNED_CLIFFS sections=%d independent_matte_materials=true uniform_scale=true"),Placed);
}
void ABenchyWorld::BuildWaterfall() {
    UStaticMesh* Rock=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Megascans/3D_Assets/HugeNordicCoastalCliff/SM_HugeNordicCoastalCliff_01.SM_HugeNordicCoastalCliff_01"));
    UMaterialInterface* RockMaterial=Rock?Rock->GetMaterial(0):nullptr;
    auto Mass=[&](FVector Center,FVector Radius,FRotator Rotation) {
        if(!Rock || !RockMaterial) return;
        UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(this);
        C->SetupAttachment(RootComponent);C->SetStaticMesh(Rock);C->SetMaterial(0,RockMaterial);
        C->SetRelativeRotation(Rotation);
        FVector Extent=Rock->GetBoundingBox().GetExtent();
        const FVector Scale(Radius.X/Extent.X,Radius.Y/Extent.Y,Radius.Z/Extent.Z);
        C->SetRelativeScale3D(Scale);
        C->SetRelativeLocation(Center-Rotation.RotateVector(Rock->GetBoundingBox().GetCenter()*Scale));
        C->RegisterComponent();C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    };
    // A continuous rock overhang links the island cliff and an embedded outer pillar.
    // The navigable opening lies between them, over the original sea surface.
    Mass(FVector(-5200,1800,1150),FVector(2200,1100,400),FRotator(0,0,0));
    Mass(FVector(-7300,2000,500),FVector(1400,1700,1000),FRotator(0,21,0));
    auto Effect=[&](const TCHAR* Material,TArray<FVector>& V,TArray<int32>& Tri,TArray<FVector2D>& UV) {
        UProceduralMeshComponent* C=NewObject<UProceduralMeshComponent>(this);
        C->SetupAttachment(RootComponent);C->RegisterComponent();C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCastShadow(false);Surfaces.Add(C);
        TArray<FVector> N;TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
        const FVector Normal=FString(Material).Contains(TEXT("FallFoam"))?FVector::UpVector:FVector(0,-1,0);
        for(int32 I=0;I<V.Num();I++) {N.Add(Normal);Colors.Add(FLinearColor::White);Tangents.Add(FProcMeshTangent(1,0,0));}
        C->CreateMeshSection_LinearColor(0,V,Tri,N,UV,Colors,Tangents,false);
        if(UMaterialInterface* M=LoadObject<UMaterialInterface>(nullptr,Material)) {
            auto MID=UMaterialInstanceDynamic::Create(M,this);C->SetMaterial(0,MID);WaterEffects.Add(MID);
        } else UE_LOG(LogTemp,Error,TEXT("Missing waterfall material: %s"),Material);
    };
    TArray<FVector> V;TArray<int32> Tri;TArray<FVector2D> UV;
    const int32 Across=40,Down=64;
    for(int32 Z=0;Z<=Down;Z++) for(int32 X=0;X<=Across;X++) {
        float U=float(X)/Across,W=float(Z)/Down;
        V.Add(FVector(FMath::Lerp(-5900.f,-3400.f,U),2700+FMath::Sin(U*8)*60+W*W*100,1500*(1-W)+15));
        UV.Add(FVector2D(U,W));
    }
    for(int32 Z=0;Z<Down;Z++) for(int32 X=0;X<Across;X++) {
        int32 A=Z*(Across+1)+X;Tri.Append({A,A+Across+1,A+1,A+1,A+Across+1,A+Across+2});
    }
    Effect(TEXT("/Game/Materials/M_Waterfall.M_Waterfall"),V,Tri,UV);
    V.Reset();Tri.Reset();UV.Reset();
    for(int32 Y=0;Y<=24;Y++) for(int32 X=0;X<=48;X++) {
        float U=float(X)/48,W=float(Y)/24;
        V.Add(FVector(FMath::Lerp(-6200.f,-3100.f,U),FMath::Lerp(2400.f,3400.f,W),10));UV.Add(FVector2D(U,W));
    }
    for(int32 Y=0;Y<24;Y++) for(int32 X=0;X<48;X++) {
        int32 A=Y*49+X;Tri.Append({A,A+49,A+1,A+1,A+49,A+50});
    }
    Effect(TEXT("/Game/Materials/M_FallFoam.M_FallFoam"),V,Tri,UV);
    for(int32 I=0;I<3;I++) {
        V={FVector(-5800+I*650,2850,40),FVector(-5800+I*650,2850,480),
            FVector(-4600+I*650,2850,40),FVector(-4600+I*650,2850,480)};
        Tri={0,1,2,2,1,3};UV={FVector2D(0,1),FVector2D(0,0),FVector2D(1,1),FVector2D(1,0)};
        Effect(TEXT("/Game/Materials/M_FallMist.M_FallMist"),V,Tri,UV);
    }
    const FString Report=TEXT("{\"waterfall_present\":true,\"course_crosses_curtain\":false,\"curtain_y_cm\":2700,\"curtain_x_min_cm\":-5900,\"curtain_x_max_cm\":-3400,\"rock_roof_minimum_height_cm\":750,\"rock_material_opaque\":");
    FFileHelper::SaveStringToFile(Report+(RockMaterial && RockMaterial->GetBlendMode()==BLEND_Opaque?TEXT("true}"):TEXT("false}")),
        *(FPaths::ProjectSavedDir()/TEXT("Waterfall.json")));
}
void ABenchyWorld::BuildWayfinding(){
    auto Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto Wood=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Fab_Wood.M_Fab_Wood"));
    auto Rust=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Fab_Rust.M_Fab_Rust"));
    auto Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Filament.M_Filament"));
    auto Board=UMaterialInstanceDynamic::Create(Base,this);Board->SetVectorParameterValue(TEXT("FilamentColor"),FLinearColor(.025,.065,.065));
    for(int32 I=0;I<16;I+=2){
        FVector P=RoutePoint(I*20),F=(RoutePoint(I*20+2)-P).GetSafeNormal(),Side(-F.Y,F.X,0);
        float Bend=FMath::FindDeltaAngleDegrees(F.Rotation().Yaw,(RoutePoint(I*20+15)-RoutePoint(I*20+5)).Rotation().Yaw);
        float Sign=Bend>0?-1:1;FVector At=P+Side*Sign*1600;float Ground=TerrainSurfaceHeight(FVector2D(At));
        float Z=FMath::Max(240.f,Ground+180);FRotator Rot=F.Rotation();
        auto Part=[&](UStaticMesh* Mesh,UMaterialInterface* M,FVector Pos,FVector Scale){auto C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh);C->SetMaterial(0,M);C->SetRelativeLocation(Pos);C->SetRelativeScale3D(Scale);C->SetRelativeRotation(Rot);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();};
        Part(Cube,Board,FVector(At.X,At.Y,Z),FVector(.20,4.2,1.55));
        for(float K:{-1.f,1.f})Part(Cylinder,Wood,At+Side*K*150+FVector(0,0,(Z-100+Ground)/2),FVector(.15,.15,(Z-100-Ground)/100));
        auto Text=NewObject<UTextRenderComponent>(this);Text->SetupAttachment(RootComponent);Text->SetHorizontalAlignment(EHTA_Center);Text->SetVerticalAlignment(EVRTA_TextCenter);Text->SetWorldSize(110);
        Text->SetText(FText::FromString(I==0?TEXT("START / ZIEL"):FMath::Abs(Bend)<15?TEXT("^^^"):Bend>0?TEXT(">>>"):TEXT("<<<")));if(I==0)Text->SetWorldSize(45);
        Text->SetTextRenderColor(FColor(255,224,130));Text->SetRelativeLocation(FVector(At.X,At.Y,Z)-F*14);Text->SetRelativeRotation(FRotator(0,Rot.Yaw+180,0));Text->RegisterComponent();
    }
}
void ABenchyWorld::BuildLandmarks(){
    if(ActiveTrack==0)return;
    auto Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto Wood=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Fab_Wood.M_Fab_Wood"));
    auto Rust=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Fab_Rust.M_Fab_Rust"));
    auto Part=[&](FVector P,FVector Size,FRotator R,UMaterialInterface* M){auto C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(Cube);C->SetMaterial(0,M);C->SetRelativeLocation(P);C->SetRelativeScale3D(Size/100);C->SetRelativeRotation(R);C->RegisterComponent();C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);};
    if(ActiveTrack==1){
        // Small timber harbours with individual deck boards, driven piles and covered sheds.
        for(int Dock:{25,120,235}){
            FVector P=RoutePoint(Dock),F=(RoutePoint(Dock+1)-P).GetSafeNormal(),S(-F.Y,F.X,0);P+=S*2550;
            FRotator R=F.Rotation();
            for(int J=-12;J<=12;J++)Part(P+F*(J*62)+FVector(0,0,110),FVector(57,780,32),R,Wood);
            for(int J=-2;J<=2;J++)for(float Edge:{-1.f,1.f}){
                FVector Q=P+F*J*330+S*Edge*320;float H=TerrainSurfaceHeight(FVector2D(Q));
                Part(Q+FVector(0,0,(H+170)/2),FVector(42,42,FMath::Max(120.f,170-H)),R,Wood);
            }
            FVector Shed=P+S*580;
            for(int K=0;K<4;K++)Part(Shed+F*((K%2)*2-1)*300+S*((K/2)*2-1)*200+FVector(0,0,230),FVector(35,35,430),R,Wood);
            for(int J=-6;J<=6;J++)Part(Shed+F*(J*60)+FVector(0,0,465),FVector(58,570,25),R+FRotator(0,0,9),Rust);
            for(int J=-5;J<=5;J++)for(float Side:{-1.f,1.f})if(J<-1||J>1){
                Part(Shed+F*(J*58)+S*Side*210+FVector(0,0,230),FVector(52,25,390),R,Wood);
            }
            for(int J=-3;J<=3;J++)Part(Shed+F*330+S*(J*58)+FVector(0,0,230),FVector(25,52,390),R,Wood);
            for(int J=0;J<5;J++)Part(P+F*(J*155-350)+S*270+FVector(0,0,185),FVector(110,110,140),R+FRotator(0,J*17,0),Wood);
        }
    }else if(ActiveTrack!=2){
        // Forest cascades sit on embedded rock faces outside the navigable channel.
        for(int N:{55,105,178}){
            FVector P=RoutePoint(N),F=(RoutePoint(N+1)-P).GetSafeNormal(),Side(-F.Y,F.X,0);P+=Side*3000;
            float H=TerrainSurfaceHeight(FVector2D(P));if(H<20)continue;
            auto Rock=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Megascans/3D_Assets/HugeNordicCoastalCliff/SM_HugeNordicCoastalCliff_02.SM_HugeNordicCoastalCliff_02"));
            auto C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(Rock);auto Bounds=Rock->GetBoundingBox();float Scale=1500/Bounds.GetSize().Z;FRotator R=F.Rotation();
            C->SetRelativeScale3D(FVector(Scale));C->SetRelativeRotation(R);C->SetRelativeLocation(P+FVector(0,0,H+350)-R.RotateVector(Bounds.GetCenter()*Scale));C->RegisterComponent();C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            auto Fall=NewObject<UProceduralMeshComponent>(this);Fall->SetupAttachment(RootComponent);Fall->RegisterComponent();Fall->SetCollisionEnabled(ECollisionEnabled::NoCollision);Fall->SetCastShadow(false);Surfaces.Add(Fall);
            TArray<FVector> V,Normals;TArray<int32> Tri;TArray<FVector2D> UV;TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
            FVector Front=P-Side*620;
            for(int Y=0;Y<=32;Y++)for(int X=0;X<=12;X++){
                float U=X/12.f,W=Y/32.f;V.Add(Front+F*((U-.5)*900)-Side*(W*W*220)+FVector(0,0,FMath::Lerp(H+1000,30.f,W)));Normals.Add(-Side);UV.Add(FVector2D(U,W));Colors.Add(FLinearColor::White);Tangents.Add(FProcMeshTangent(F,false));
            }
            for(int Y=0;Y<32;Y++)for(int X=0;X<12;X++){int A=Y*13+X;Tri.Append({A,A+13,A+1,A+1,A+13,A+14});}
            Fall->CreateMeshSection_LinearColor(0,V,Tri,Normals,UV,Colors,Tangents,false);
            auto M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Waterfall.M_Waterfall")),this);Fall->SetMaterial(0,M);WaterEffects.Add(M);
        }
    }
}
