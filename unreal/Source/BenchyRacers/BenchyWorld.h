#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchyWorld.generated.h"
class UProceduralMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class UMaterialInstanceDynamic;
class UInstancedStaticMeshComponent;

UCLASS()
class BENCHYRACERS_API ABenchyWorld : public AActor {
    GENERATED_BODY()
public:
    ABenchyWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(Replicated) int32 Track=0;
    static int32 ActiveTrack;
    static FString TrackName(int32 Index);
    static float WaterHeight(FVector P, float Time);
    static FVector WaterNormal(FVector P, float Time);
    static FVector RoutePoint(float Index);
    static float GroundHeight(FVector2D P);
    static float TerrainSurfaceHeight(FVector2D P);
private:
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Surfaces;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> OceanMaterial;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> WaterEffects;
    void BuildSurface(FVector2D Center, FVector2D Extent, int32 Resolution, bool Ocean);
    void BuildVegetation();
    void BuildFabScenery();
    void BuildCliffs();
    void BuildWaterfall();
    void BuildWayfinding();
    void BuildBuoys();
    void UpdateBuoys(float Time);
    struct FBuoy { FVector Anchor; int32 Side=0,Instance=0; };
    TArray<FBuoy> Buoys;
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> BuoyMeshes;
    float BuoyClock=0;
    void BuildLandmarks();
};
