#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "BenchyGameMode.generated.h"
UCLASS()
class BENCHYRACERS_API ABenchyHUD : public AHUD {
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    void DrawItemPanel(class ABenchyPawn* Boat,float ServerTime);
    void DrawNavigation(class ABenchyPawn* Boat);
    UPROPERTY() TObjectPtr<class UFont> ItemTypeface;
};
UCLASS()
class BENCHYRACERS_API ABenchyGameMode : public AGameModeBase {
    GENERATED_BODY()
public:
    ABenchyGameMode();
    virtual void InitGame(const FString& MapName,const FString& Options,FString& Error) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    virtual void RestartPlayer(AController* Player) override;
    virtual void PreLogin(const FString& Options,const FString& Address,const FUniqueNetIdRepl& Id,FString& Error) override;
    virtual void Logout(AController* Player) override;
    void StartRace();
};
