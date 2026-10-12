#include "BenchyWorld.h"
#include "CourseNavigation.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void ABenchyWorld::BuildBuoys(){
    auto Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Filament.M_Filament"));
    for(int Part=0;Part<5;Part++){
        const TCHAR* Shape=Part==3?TEXT("Cylinder"):TEXT("Sphere");
        auto M=UMaterialInstanceDynamic::Create(Base,this);
        const FLinearColor Color=Part==0?FLinearColor(.015,.26,1):Part==1?FLinearColor(1,.19,.012):Part==3?FLinearColor(.035,.045,.05):FLinearColor(.98,.98,.90);
        M->SetVectorParameterValue(TEXT("FilamentColor"),Color);
        auto C=NewObject<UInstancedStaticMeshComponent>(this);C->SetupAttachment(RootComponent);
        FString Path=FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Shape,Shape);
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));C->SetMaterial(0,M);C->SetMobility(EComponentMobility::Movable);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();BuoyMeshes.Add(C);
    }
    int Counts[2]={0,0};bool InWater=true;
    for(double Index:BenchyCourse::MarkerIndices(ActiveTrack)){
        const auto P=BenchyTracks::Route(ActiveTrack,Index),D=BenchyCourse::Direction(ActiveTrack,Index);
        for(int Side=0;Side<2;Side++){
            const double Offset=(Side==0?-1:1)*BenchyCourse::HalfWidth;
            FBuoy B;B.Anchor=FVector(P.x-D.y*Offset,P.y+D.x*Offset,0);B.Side=Side;B.Instance=Counts[Side]++;
            InWater&=TerrainSurfaceHeight(FVector2D(B.Anchor))<-25;Buoys.Add(B);
            for(int Part:{Side,2,3,4})BuoyMeshes[Part]->AddInstance(FTransform(B.Anchor));
        }
    }
    UpdateBuoys(0);
    FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"track\":%d,\"buoys\":%d,\"lane_width_cm\":%.0f,\"style\":\"rounded_float_short_mast\",\"placement\":\"checkpoints_open_water_and_bends\",\"all_in_water\":%s}"),
        ActiveTrack,Buoys.Num(),BenchyCourse::HalfWidth*2,InWater?TEXT("true"):TEXT("false")),*(FPaths::ProjectSavedDir()/TEXT("BuoyPlacement.json")));
}
void ABenchyWorld::UpdateBuoys(float Time){
    for(int I=0;I<Buoys.Num();I++){
        const auto& B=Buoys[I];FVector P=B.Anchor;P.Z=WaterHeight(P,Time)+4;
        const FQuat Rotation=FRotationMatrix::MakeFromZ((FVector::UpVector*.65f+WaterNormal(P,Time)*.35f).GetSafeNormal()).ToQuat();
        BuoyMeshes[B.Side]->UpdateInstanceTransform(B.Instance,FTransform(Rotation,P+Rotation.RotateVector(FVector(0,0,28)),FVector(1.65,1.65,1.10)),false,false,true);
        BuoyMeshes[2]->UpdateInstanceTransform(I,FTransform(Rotation,P+Rotation.RotateVector(FVector(0,0,23)),FVector(1.67,1.67,.24)),false,false,true);
        BuoyMeshes[3]->UpdateInstanceTransform(I,FTransform(Rotation,P+Rotation.RotateVector(FVector(0,0,105)),FVector(.10,.10,.66)),false,false,true);
        BuoyMeshes[4]->UpdateInstanceTransform(I,FTransform(Rotation,P+Rotation.RotateVector(FVector(0,0,146)),FVector(.24,.24,.24)),false,false,true);
    }
    for(auto C:BuoyMeshes)C->MarkRenderStateDirty();
}
