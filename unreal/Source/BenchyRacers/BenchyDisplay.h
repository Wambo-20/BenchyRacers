#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchyDisplay.generated.h"
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UCameraComponent;
class UPointLightComponent;
UCLASS()
class BENCHYRACERS_API ABenchyDisplay : public AActor {
    GENERATED_BODY()
public:
    ABenchyDisplay();
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    void SelectFilament(int32 Index,bool Reprint=true);
    float PrintProgress() const {return Progress;}
    int32 SelectedFilament() const {return Color;}
    UStaticMeshComponent* LogoMesh() const {return Boat;}
    void SetLogoEditor(bool Enabled,int32 Side);
    void RotateLogoView(float Degrees){Angle+=Degrees;}
    UCameraComponent* DisplayCamera() const {return Camera;}
    float DisplayHeight() const {return Height;}
    void SetLobbyLayout(bool Enabled,float Aspect);
    void SetPortrait();
    void SetEditorLayout(int32 Side,float Aspect);
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Boat;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Bed;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Head;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Tip;
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TObjectPtr<UPointLightComponent> Fill;
    UPROPERTY() TObjectPtr<UPointLightComponent> Glow;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Filament;
    float Progress=1,PrintAge=0,Angle=0,Height=300;
    FVector BoatOrigin;
    int32 Color=-1;
    bool LogoEditing=false;
    FVector DefaultCameraLocation;
    FRotator DefaultCameraRotation;
};
