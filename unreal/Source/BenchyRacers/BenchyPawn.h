#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BenchyPawn.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UMaterialInstanceDynamic;
class UBenchyItemFX;
class UPointLightComponent;

struct FBenchyWakePoint {
    FVector Position;
    FVector Side;
    float Age=0;
    float Power=0;
    float Speed=0;
};

UCLASS()
class BENCHYRACERS_API ABenchyPawn : public APawn {
    GENERATED_BODY()
public:
    ABenchyPawn();
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    float SpeedKmh() const;
    FString FilamentName() const;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(Replicated) int32 NextGate=1;
    UPROPERTY(Replicated) int32 Lap=1;
    UPROPERTY(Replicated) float RaceTime=0;
    UPROPERTY(Replicated) bool Finished=false;
    UPROPERTY(Replicated) int32 Passed=0;
    UPROPERTY(Replicated) bool MissedCheckpoint=false;
    UPROPERTY(Replicated) int32 Coins=0;
    UPROPERTY(Replicated) int32 Item=0;
    UPROPERTY(Replicated) int32 FinishPlace=0;
    UPROPERTY(Replicated) int32 RaceEntryId=-1;
    UPROPERTY(Replicated) FString RacerName;
    UPROPERTY(Replicated) bool IsBot=false;
    UPROPERTY(Replicated) float ShieldUntil=0;
    UPROPERTY(Replicated) float StunUntil=0;
    UPROPERTY(Replicated) float RespawnUntil=0;
    UPROPERTY(Replicated) float ItemBoostUntil=0;
    UPROPERTY(Replicated) float NetworkSpeed=0;
    UPROPERTY(Replicated) FVector NetworkPosition;
    UPROPERTY(Replicated) FRotator NetworkRotation;
    UPROPERTY(ReplicatedUsing=OnRep_Color) int32 ColorIndex=0;
    UFUNCTION() void OnRep_Color();
    UFUNCTION(Server,Reliable) void ServerReset();
    UFUNCTION(Server,Reliable) void ServerUseItem(bool Backward=false);
    UFUNCTION(Client,Reliable) void ItemNotice(const FString& Message);
    UFUNCTION(Server,Reliable) void ServerColor(int32 Index);
    UFUNCTION(Server,Unreliable) void ServerControls(float Gas,float Turn,bool Boosting);
    UFUNCTION(NetMulticast,Unreliable) void PlayCue(int32 Cue);
    UFUNCTION(NetMulticast,Reliable) void ItemFeedback(int32 Effect);
    float ItemEventTime=-100;
    int32 ItemEvent=0;
    FString ItemNoticeText;
    float ItemNoticeTime=-100;
    UPROPERTY(Replicated) bool LastShotBackward=false;
    UPROPERTY(Replicated) FString LastShotTarget;
    void ResetBoat();
    void BeginRace(int32 Slot);
    bool Collect(int32 Kind);
    void HitByItem();
    void DriveAI();
    void FinishRace();
    void DriveFinished(float DT);
    float FinishRouteCursor() const {return PostFinishCursor;}
    void AuditInput(float Gas,float Turn) {Throttle=Gas;Steering=Turn;}
    float Progress() const;
    FVector NavigationTarget() const;
    FString ItemName() const;
    ABenchyPawn* NozzleTarget(bool Backward) const;
    bool WantsBackwardShot() const;
    void AuditPose(FVector Position,FRotator Rotation,int32 ProgressPassed);
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Hull;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Arm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Wake;
    UPROPERTY() TObjectPtr<UBenchyItemFX> ItemFX;
    UPROPERTY() TObjectPtr<UPointLightComponent> GlowLight;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> PrintHead;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Filament;
    TArray<FBenchyWakePoint> History;
    float Throttle=0, Steering=0, WakeClock=0;
    bool Boost=false;
    int32 Quality=2;
    float InputClock=0, PoseClock=0, LastInputTime=0, StuckTime=0, NextResetTime=0, BotItemTime=0;
    float PostFinishCursor=0;
    bool NetworkInitialized=false;
    FVector ResetPosition;
    FVector PreviousCoursePosition;
    bool CoursePositionInitialized=false;
    FRotator ResetRotation;
    bool Audit=false;
    int32 AuditFrames=0, AuditShots=0, AuditResets=0;
    float AuditMinDepth=1e9f, AuditMaxDepth=-1e9f, AuditMaxSpeed=0, AuditMaxTilt=0;
    FVector AuditStart;
    FVector AuditLastPosition;
    float AuditPathLength=0;
    bool AuditWaterfall=false, AuditCrossedWaterfall=false;
    bool AuditScenery=false;
    uint32 AuditVisitedSections=0;
    float AuditMaximumRouteError=0;
    void TickAudit(float Time);
    void SetThrottle(float V) { Throttle=V; }
    void SetSteering(float V) { Steering=V; }
    void StartBoost() { Boost=true; }
    void StopBoost() { Boost=false; }
    void UseItem();
    void NextColor();
    void NextQuality();
    void ToggleFullscreen();
    void Quit();
    void UpdateWake(float DT, float Time);
};
