#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Actor.h"
#include "BenchyRace.generated.h"
class ABenchyPawn;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
USTRUCT()
struct FBenchyResult {
    GENERATED_BODY()
    UPROPERTY() int32 EntryId=-1;
    UPROPERTY() FString Name;
    UPROPERTY() TObjectPtr<ABenchyPawn> Boat;
    UPROPERTY() int32 Place=0;
    UPROPERTY() float Time=0;
    UPROPERTY() float Progress=0;
    UPROPERTY() bool Disconnected=false;
};
UCLASS()
class BENCHYRACERS_API ABenchyRaceState : public AGameStateBase {
    GENERATED_BODY()
public:
    ABenchyRaceState();
    UPROPERTY(Replicated) int32 Phase=0;
    UPROPERTY(Replicated) float StartTime=0;
    UPROPERTY(Replicated) float FirstFinishTime=0;
    UPROPERTY(Replicated) TArray<FBenchyResult> Results;
    void RegisterRacers();
    int32 RecordFinish(ABenchyPawn* Boat);
    void UpdateResults();
    bool EveryoneDone() const;
    TArray<FBenchyResult> OrderedResults() const;
    static FString FormatTime(float Seconds);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    float Clock() const;
};
UCLASS()
class BENCHYRACERS_API ABenchyPickup : public AActor {
    GENERATED_BODY()
public:
    ABenchyPickup();
    UPROPERTY(Replicated) int32 Kind=0;
    UPROPERTY(ReplicatedUsing=OnRep_Available) bool Available=true;
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    UFUNCTION() void OnRep_Available();
    float ReadyAt=0;
    float CollectedAt=-100;
    FVector Anchor;
};
UCLASS()
class BENCHYRACERS_API ABenchyProjectile : public AActor {
    GENERATED_BODY()
public:
    ABenchyProjectile();
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    UFUNCTION(NetMulticast,Unreliable) void Impact(FVector Position);
    UPROPERTY(ReplicatedUsing=OnRep_Guided) bool Guided=false;
    UPROPERTY(Replicated) TObjectPtr<ABenchyPawn> Target;
    bool Backward=false;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UFUNCTION() void OnRep_Guided();
private:
    FVector Direction;
    FVector DepartureOffset;
    float RouteCursor=0,FlightAge=0;
    float ObstacleFraction(FVector A,FVector B,FVector& At) const;
    bool ClearWaterPath(FVector A,FVector B) const;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Nozzle;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ProjectileMaterial;
};
