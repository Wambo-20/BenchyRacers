#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "BenchyLogoSpec.h"
#include "BenchyMenu.generated.h"
class SWidget;
class SEditableTextBox;
class UAudioComponent;
class ABenchyDisplay;
class ABenchyPawn;
class UBenchyOnline;
class ACameraActor;
class IHttpRequest;
class UBenchyLogo;
class SVerticalBox;
class SScrollBox;
class SConstraintCanvas;
struct FSlateBrush;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
UCLASS()
class BENCHYRACERS_API UBenchyInstance : public UGameInstance {
    GENERATED_BODY()
public:
    virtual void Init() override;
    virtual void Shutdown() override;
    FString Nick=TEXT("Maker"), Address, Notice;
    int32 SelectedTrack=0;
    int32 SelectedColor=0;
    float MusicVolume=.35f;
    UPROPERTY() TObjectPtr<UBenchyLogo> Logo;
    UPROPERTY() TObjectPtr<UBenchyOnline> Online;
    UPROPERTY() TMap<FString,TObjectPtr<UBenchyLogo>> NetworkLogos;
    TMap<FString,TArray<uint8>> LogoPNGs;
    TArray<FString> CarriedChat;
    void SavePreferences();
    void RefreshLobbyAddresses(UWorld* World,bool Force=false);
    FString LobbyAddress(bool Internet) const;
    FString InternetAddressStatus() const;
    FString LobbyShareNotice;
    TArray<FString> LocalIPv4s;
    int32 LocalAddressIndex=0,LobbyPort=7777;
    bool PublicAddressPending=false;
    FString PublicIPv4;
private:
    double LastPublicAddressCheck=-1;
    TSharedPtr<IHttpRequest,ESPMode::ThreadSafe> PublicAddressRequest;
    void NetworkFailure(UWorld* World,UNetDriver* Driver,ENetworkFailure::Type Type,const FString& Error);
    void TravelFailure(UWorld* World,ETravelFailure::Type Type,const FString& Error);
};
UCLASS()
class BENCHYRACERS_API ABenchyController : public APlayerController {
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DT) override;
    void ShowMenu(int32 Page=0);
    void HideMenu();
    void ToggleMenu();
    void ShowResults();
    void HideResults();
    bool ResultsVisible() const {return ResultPanel.IsValid();}
    void SelectColor(int32 Color);
    void SelectTrack(int32 Track);
    void CopyLobbyAddress(bool Internet);
    TSharedRef<SWidget> BuildLobbyInvite();
    void AuditLobby();
    void AuditLogo();
    void TickSocial(float DT);
    void AuditSocial();
    void SyncLogo();
    void SendChat();
    TSharedRef<SWidget> BuildChat();
    TSharedRef<SWidget> BuildLobbyGallery();
    void RefreshGallery();
    void ClearGallery();
    UFUNCTION(Server,Reliable) void ServerChat(const FString& Text);
    UFUNCTION(Server,Reliable) void ServerLogoStart(const FBenchyLogoSpec& Spec,int32 Total);
    UFUNCTION(Server,Reliable) void ServerLogoPart(int32 Offset,const TArray<uint8>& Bytes);
    UFUNCTION(Server,Reliable) void ServerLogoPose(const FBenchyLogoSpec& Spec);
    UFUNCTION(Server,Reliable) void ServerLogoRequest(const FString& Hash);
    UFUNCTION(Client,Reliable) void ClientLogoPart(const FString& Hash,int32 Total,int32 Offset,const TArray<uint8>& Bytes);
    void ImportLogo();
    void ApplyLogo(bool Save=false);
    TSharedRef<SWidget> BuildLogoEditor();
    ABenchyDisplay* Display() const {return Preview;}
    bool MenuVisible() const {return Panel.IsValid();}
    UFUNCTION(Server,Reliable) void ServerStartRace();
    UFUNCTION(Server,Reliable) void ServerNewTrack(int32 Track);
    UFUNCTION(Server,Reliable) void ServerIdentity(const FString& Name,int32 Color);
private:
    TSharedPtr<SWidget> Panel;
    TSharedPtr<SWidget> ResultPanel;
    TSharedPtr<SEditableTextBox> AddressBox,NameBox;
    TSharedPtr<SEditableTextBox> ChatBox;
    TSharedPtr<SScrollBox> ChatScroll;
    TSharedPtr<SConstraintCanvas> GalleryCanvas;
    TArray<TSharedPtr<FSlateBrush>> PortraitBrushes;
    UPROPERTY() TArray<TObjectPtr<ABenchyDisplay>> Portraits;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> PortraitMaterials;
    TArray<TWeakObjectPtr<ABenchyPawn>> PortraitOwners;
    FString GallerySignature,LastChat;
    TMap<FString,FBenchyLogoTransfer> ReceivingLogos;
    TArray<FBenchyLogoTransfer> SendingLogos;
    TSet<FString> RequestedLogos;
    TMap<FString,float> LogoRequestTimes;
    FBenchyLogoTransfer UploadLogo;
    bool UploadActive=false,JoinedAnnounced=false;
    float TransferClock=0,GalleryClock=0,LastChatTime=-100,LastUploadTime=-100;
    int32 MenuPage=0;
    UPROPERTY() TObjectPtr<UAudioComponent> Music;
    UPROPERTY() TObjectPtr<ABenchyDisplay> Preview;
    UPROPERTY() TObjectPtr<ACameraActor> IntroCamera;
    bool IntroReturning=false;
    int32 LastPhase=-1;
    int32 LastCountdown=-1;
    TWeakObjectPtr<APawn> NamedPawn;
    void StoreFields();
};
