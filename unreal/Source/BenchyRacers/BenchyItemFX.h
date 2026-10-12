#pragma once
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/Actor.h"
#include "BenchyItemFX.generated.h"

// Cosmetic only: gameplay and effect lifetimes remain server authoritative.
UCLASS()
class BENCHYRACERS_API UBenchyItemFX : public UProceduralMeshComponent {
    GENERATED_BODY()
public:
    UBenchyItemFX(const FObjectInitializer& Initializer);
    virtual void BeginPlay() override;
    virtual void TickComponent(float DT,ELevelTick Type,FActorComponentTickFunction* Fn) override;
private:
    struct FTrail { FVector Position; float Age=0; };
    TArray<FTrail> Trail;
    float SampleTime=0;
};

UCLASS()
class BENCHYRACERS_API ABenchyItemBurst : public AActor {
    GENERATED_BODY()
public:
    ABenchyItemBurst();
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    int32 Effect=0;
    static void Spawn(UWorld* World,FVector Position,int32 Effect);
private:
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
    float Age=0;
};
