#include "BenchyWorld.h"
#include "TrackLayout.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void ABenchyWorld::BuildFabScenery(){
    FRandomStream RNG(70091+ActiveTrack*1103);
    int32 Rocks=0,Nordic=0,Ice=0;float ClearanceMax=-1e9f;bool Opaque=true;
    // Work against the actual triangulated bank and protect the complete race corridor.
    auto Place=[&](UStaticMesh* Mesh,FVector2D P,float Size,bool Cliff,FRotator Rotation,FTransform& Out){
        const FBox B=Mesh->GetBoundingBox();float Span=FMath::Max(B.GetSize().X,B.GetSize().Y);
        if(Cliff)Span=FMath::Max(Span,B.GetSize().Z);
        float Scale=Size/FMath::Max(1.,Span);
        float H=TerrainSurfaceHeight(P);if(H<(Cliff?20:35))return false;
        Out=FTransform(Rotation,FVector(P.X,P.Y,H-B.Min.Z*Scale),FVector(Scale));
        float Clearance=-1e9f;
        for(int X=0;X<=4;X++)for(int Y=0;Y<=4;Y++){
            FVector Foot=Out.TransformPosition(FVector(FMath::Lerp(B.Min.X,B.Max.X,X/4.),FMath::Lerp(B.Min.Y,B.Max.Y,Y/4.),B.Min.Z));
            if(BenchyTracks::Distance(ActiveTrack,{Foot.X,Foot.Y})<1900)return false;
            Clearance=FMath::Max(Clearance,float(Foot.Z)-TerrainSurfaceHeight(FVector2D(Foot)));
        }
        float Embed=FMath::Max(22.f,float(B.GetSize().Z)*Scale*(Cliff?.16f:.14f));
        FVector At=Out.GetTranslation();At.Z-=FMath::Max(0.f,Clearance)+Embed;Out.SetTranslation(At);
        ClearanceMax=FMath::Max(ClearanceMax,Clearance-FMath::Max(0.f,Clearance)-Embed);return true;
    };
    auto Batch=[&](UStaticMesh* Mesh){
        auto C=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh);
        C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);C->SetCullDistances(90000,160000);C->RegisterComponent();
        for(int S=0;S<Mesh->GetStaticMaterials().Num();S++)if(auto M=Mesh->GetMaterial(S)){
            Opaque&=M->GetBlendMode()==BLEND_Opaque;auto Snow=UMaterialInstanceDynamic::Create(M,this);Snow->SetScalarParameterValue(TEXT("SnowAmount"),ActiveTrack==2?1:0);C->SetMaterial(S,Snow);
        }return C;
    };
    TArray<UHierarchicalInstancedStaticMeshComponent*> RockBatches;
    for(int I=1;I<=7;I++){
        FString Path=FString::Printf(TEXT("/Game/Rock_Collection_04/Meshes/Rock_%02d/StaticMeshes/SM_Rock_%02d.SM_Rock_%02d"),I,I,I);
        auto Mesh=LoadObject<UStaticMesh>(nullptr,*Path);if(!Mesh){UE_LOG(LogTemp,Error,TEXT("Missing Fab rock %s"),*Path);return;}RockBatches.Add(Batch(Mesh));
    }
    const int32 RockCount=ActiveTrack==0?175:ActiveTrack==1?135:120;
    for(int Attempt=0;Attempt<3000&&Rocks<RockCount;Attempt++){
        auto& Land=BenchyTracks::Land(ActiveTrack)[RNG.FRand()<.80?0:RNG.RandRange(1,4)];float A=RNG.FRandRange(0,2*PI),R=RNG.FRandRange(.35,1.1);
        FVector2D P(Land.center.x+FMath::Cos(A)*Land.radius.x*R,Land.center.y+FMath::Sin(A)*Land.radius.y*R);
        int I=RNG.RandRange(0,6);FTransform T;
        if(Place(RockBatches[I]->GetStaticMesh(),P,RNG.FRandRange(330,1150),false,FRotator(RNG.FRandRange(-7,7),RNG.FRandRange(0,360),RNG.FRandRange(-5,5)),T)){RockBatches[I]->AddInstance(T);Rocks++;}
    }
    auto NordicMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/FabWorld/Meshes/SM_NordicCliff.SM_NordicCliff"));
    auto IceMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/FabWorld/Meshes/SM_IceCliff.SM_IceCliff"));
    if(!NordicMesh||!IceMesh){UE_LOG(LogTemp,Error,TEXT("Missing Fab Nordic/Ice cliff"));return;}
    auto NordicBatch=Batch(NordicMesh);auto IceBatch=ActiveTrack==2?Batch(IceMesh):nullptr;
    for(int Attempt=0;Attempt<900&&(Nordic<18||(ActiveTrack==2&&Ice<28));Attempt++){
        bool Frozen=IceBatch&&(Attempt%2==0)&&Ice<28;auto C=Frozen?IceBatch:NordicBatch;if(!Frozen&&Nordic>=18)continue;
        float Index=RNG.FRandRange(0,320);FVector P=RoutePoint(Index),D=(RoutePoint(Index+1)-P).GetSafeNormal2D(),Side(-D.Y,D.X,0);
        float Size=RNG.FRandRange(Frozen?2300:1500,Frozen?4300:3300);
        P+=Side*(RNG.FRand()<.5?-1:1)*(2500+Size*.55f+RNG.FRandRange(0,1300));FTransform T;
        if(Place(C->GetStaticMesh(),FVector2D(P),Size,true,FRotator(0,D.Rotation().Yaw+RNG.FRandRange(-28,28),0),T)){C->AddInstance(T);if(Frozen)Ice++;else Nordic++;}
    }
    FString Data=FString::Printf(TEXT("{\"track\":%d,\"new_rock_variants\":7,\"new_rocks\":%d,\"nordic_cliffs\":%d,\"ice_cliffs\":%d,\"opaque_materials\":%s,\"maximum_footprint_clearance_cm\":%.2f,\"support_check_passed\":%s}"),ActiveTrack,Rocks,Nordic,Ice,Opaque?TEXT("true"):TEXT("false"),ClearanceMax,ClearanceMax<=0&&Rocks>0&&Nordic>0&&(ActiveTrack!=2||Ice>0)?TEXT("true"):TEXT("false"));
    FFileHelper::SaveStringToFile(Data,*(FPaths::ProjectSavedDir()/TEXT("FabWorldPlacement.json")));UE_LOG(LogTemp,Display,TEXT("BENCHY_FAB_WORLD %s"),*Data);
}
