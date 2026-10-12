#include "BenchyMenu.h"
#include "LobbyAddress.h"
#include "Engine/World.h"
#include "Engine/NetDriver.h"
#include "HTTPModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "BenchyPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

void UBenchyInstance::RefreshLobbyAddresses(UWorld* World,bool Force) {
    if(!World||World->GetNetMode()!=NM_ListenServer)return;
    LobbyPort=World->URL.Port;
    if(auto Driver=World->GetNetDriver())if(auto Bound=Driver->GetLocalAddr())if(Bound->GetPort()>0)LobbyPort=Bound->GetPort();
    FString Previous=LocalIPv4s.IsValidIndex(LocalAddressIndex)?LocalIPv4s[LocalAddressIndex]:FString();
    LocalIPv4s.Reset();TArray<TSharedPtr<FInternetAddr>> Addresses;
    if(auto Sockets=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))Sockets->GetLocalAdapterAddresses(Addresses);
    for(auto& IP:Addresses)if(IP.IsValid()) {
        FString Text=IP->ToString(false);
        if(BenchyLobby::ShareableIPv4(TCHAR_TO_UTF8(*Text),false))LocalIPv4s.AddUnique(Text);
    }
    LocalIPv4s.Sort([](const FString& A,const FString& B){
        auto Rank=[](const FString& IP){return IP.StartsWith(TEXT("192.168."))?0:IP.StartsWith(TEXT("10."))?1:2;};
        return Rank(A)==Rank(B)?A<B:Rank(A)<Rank(B);
    });
    LocalAddressIndex=FMath::Max(0,LocalIPv4s.IndexOfByKey(Previous));
    // Existing offline audits never contact an external service. The dedicated
    // lobby audit explicitly opts in, or uses its loopback-only HTTP fixture.
    bool Audit=FString(FCommandLine::Get()).Contains(TEXT("Audit"));
    if(Audit&&!FParse::Param(FCommandLine::Get(),TEXT("BenchyLobbyAudit")))return;
    double Now=FPlatformTime::Seconds();
    if(PublicAddressPending||(!Force&&LastPublicAddressCheck>=0&&Now-LastPublicAddressCheck<300))return;
    LastPublicAddressCheck=Now;PublicAddressPending=true;PublicIPv4.Reset();LobbyShareNotice.Reset();
    FString URL=TEXT("https://api.ipify.org");
    if(FParse::Param(FCommandLine::Get(),TEXT("BenchyLobbyAudit"))) {
        FString Fixture;
        if(FParse::Value(FCommandLine::Get(),TEXT("BenchyLobbyFixture="),Fixture)&&Fixture.StartsWith(TEXT("http://127.0.0.1:")))URL=Fixture;
    }
    PublicAddressRequest=FHttpModule::Get().CreateRequest();
    PublicAddressRequest->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);PublicAddressRequest->SetURL(URL);PublicAddressRequest->SetVerb(TEXT("GET"));
    PublicAddressRequest->SetHeader(TEXT("Accept"),TEXT("text/plain"));PublicAddressRequest->SetTimeout(8);
    PublicAddressRequest->OnProcessRequestComplete().BindWeakLambda(this,[this](FHttpRequestPtr,FHttpResponsePtr Response,bool Success){
        PublicAddressPending=false;PublicIPv4.Reset();
        if(Success&&Response.IsValid()&&Response->GetResponseCode()==200&&Response->GetContent().Num()<=64) {
            FString Text=Response->GetContentAsString().TrimStartAndEnd();
            if(BenchyLobby::ShareableIPv4(TCHAR_TO_UTF8(*Text),true))PublicIPv4=Text;
        }
        PublicAddressRequest.Reset();
    });
    if(!PublicAddressRequest->ProcessRequest()){PublicAddressPending=false;PublicAddressRequest.Reset();}
}
FString UBenchyInstance::LobbyAddress(bool Internet) const {
    FString IP=Internet?PublicIPv4:(LocalIPv4s.IsValidIndex(LocalAddressIndex)?LocalIPv4s[LocalAddressIndex]:FString());
    if(Internet&&!BenchyLobby::ShareableIPv4(TCHAR_TO_UTF8(*IP),true))return FString();
    return UTF8_TO_TCHAR(BenchyLobby::Endpoint(TCHAR_TO_UTF8(*IP),LobbyPort).c_str());
}
FString UBenchyInstance::InternetAddressStatus() const {
    FString Endpoint=LobbyAddress(true);if(!Endpoint.IsEmpty())return Endpoint;
    return PublicAddressPending?TEXT("Adresse wird ermittelt …"):TEXT("Nicht ermittelt – erneut prüfen");
}
void ABenchyController::CopyLobbyAddress(bool Internet) {
    auto I=GetGameInstance<UBenchyInstance>();if(!I||GetNetMode()!=NM_ListenServer)return;
    FString Endpoint=I->LobbyAddress(Internet);if(Endpoint.IsEmpty())return;
    FPlatformApplicationMisc::ClipboardCopy(*Endpoint);
    I->LobbyShareNotice=Internet?TEXT("Internet-Adresse kopiert. An Freunde senden."):TEXT("Heimnetz-Adresse kopiert. Im selben Netz verwenden.");
}
TSharedRef<SWidget> ABenchyController::BuildLobbyInvite() {
    auto I=GetGameInstance<UBenchyInstance>();
    TSharedRef<SVerticalBox> Box=SNew(SVerticalBox);
    auto Label=[&](const FString& Text,int Size){Box->AddSlot().AutoHeight().Padding(0,2)[SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.85,.94,.95))];};
    auto Button=[&](const FString& Text,TFunction<void()> Action,TFunction<bool()> Enabled){Box->AddSlot().AutoHeight().Padding(0,2)[SNew(SButton).IsEnabled_Lambda([Enabled](){return Enabled();}).ContentPadding(FMargin(8,3)).ButtonColorAndOpacity(FLinearColor(.055,.19,.20)).OnClicked_Lambda([Action](){Action();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(Text)).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Bold",11)).ColorAndOpacity(FLinearColor::White)]];};
    auto AddressRow=[&](bool Internet){Box->AddSlot().AutoHeight().Padding(0,1)[SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([I,Internet](){FString A=Internet?I->InternetAddressStatus():I->LobbyAddress(false);return FText::FromString(A.IsEmpty()?TEXT("Keine Netzwerkadresse gefunden"):A);}).Font(FCoreStyle::GetDefaultFontStyle("Bold",14)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.65,.96,.89))]
        +SHorizontalBox::Slot().AutoWidth().Padding(5,0)[SNew(SButton).IsEnabled_Lambda([I,Internet](){return !I->LobbyAddress(Internet).IsEmpty();}).ContentPadding(FMargin(6,4)).ButtonColorAndOpacity(FLinearColor(.055,.19,.20)).ToolTipText(FText::FromString(Internet?TEXT("Internet-Adresse an Freunde senden. Die Erreichbarkeit ist nicht geprüft."):TEXT("Adresse im selben Heimnetz oder privaten VPN verwenden."))).OnClicked_Lambda([this,Internet](){CopyLobbyAddress(Internet);return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("Kopieren"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",11)).ColorAndOpacity(FLinearColor::White)]]];};
    Label(TEXT("FREUNDE EINLADEN"),14);
    Label(TEXT("Internet-Adresse"),11);AddressRow(true);
    Label(TEXT("Heimnetz / privates VPN"),11);AddressRow(false);
    if(I->LocalIPv4s.Num()>1)Button(TEXT("Andere Netzwerkadresse"),[this,I](){I->LocalAddressIndex=(I->LocalAddressIndex+1)%I->LocalIPv4s.Num();ShowMenu();},[](){return true;});
    Button(TEXT("Adressen erneut prüfen"),[this,I](){I->RefreshLobbyAddresses(GetWorld(),true);ShowMenu();},[I](){return !I->PublicAddressPending;});
    Label(FString::Printf(TEXT("Internet: UDP %d freigeben (Router / Firewall)."),I->LobbyPort),11);
    Box->AddSlot().AutoHeight().Padding(0,2)[SNew(STextBlock).Text_Lambda([I](){return FText::FromString(I->LobbyShareNotice);}).Font(FCoreStyle::GetDefaultFontStyle("Regular",11)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.65,.96,.89))];
    return Box;
}
void ABenchyController::AuditLobby() {
    if(!FParse::Param(FCommandLine::Get(),TEXT("BenchyLobbyAudit")))return;
    auto I=GetGameInstance<UBenchyInstance>();auto B=Cast<ABenchyPawn>(GetPawn());if(!I||!B)return;
    const float T=GetWorld()->GetTimeSeconds();static int Stage=0;static float Next=0;
    static bool Addresses=false,Copied=false,Rules=false,Public=false,Peer=false;
    auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");
    IFileManager::Get().MakeDirectory(*Dir,true);
    auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Dir/(FString(Name)+TEXT(".png")),true,false);};
    auto Save=[&](bool Passed,const TCHAR* Name){FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"usable_addresses\":%s,\"clipboard_exact_endpoint\":%s,\"address_validation\":%s,\"public_lookup_expected_result\":%s,\"peer_neon_replicated\":%s,\"port\":%d}"),Passed?TEXT("true"):TEXT("false"),Addresses?TEXT("true"):TEXT("false"),Copied?TEXT("true"):TEXT("false"),Rules?TEXT("true"):TEXT("false"),Public?TEXT("true"):TEXT("false"),Peer?TEXT("true"):TEXT("false"),I->LobbyPort),*(Dir/(FString(Name)+TEXT(".json"))));Stage=99;FPlatformMisc::RequestExit(false);};
    if(GetNetMode()==NM_Client){
        if(Stage==0&&T>3){SelectColor(6);Stage=1;Next=T+4;}
        if(Stage==1&&T>Next){for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(*It!=B&&!It->IsBot&&It->ColorIndex==6)Peer=true;
            auto C=B->FindComponentByClass<UStaticMeshComponent>();auto M=C?Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)):nullptr;auto Tint=M?M->K2_GetVectorParameterValue(TEXT("GlowColor")):FLinearColor::Black;
            if(Peer&&M&&M->K2_GetScalarParameterValue(TEXT("Glow"))==1&&Tint.R>.9&&Tint.G>.7&&Tint.B<.05){Shot(TEXT("Lobby-Client-Neon"));Stage=2;Next=T+.7;}}
        if(Stage==2&&T>Next){Addresses=Copied=Rules=Public=true;Save(Peer,TEXT("LobbyClient"));}
        if(T>65&&Stage!=99)Save(false,TEXT("LobbyClient"));return;
    }
    if(GetNetMode()!=NM_ListenServer)return;
    if(Stage==0&&T>5&&!I->PublicAddressPending){
        FString LAN=I->LobbyAddress(false),WAN=I->LobbyAddress(true);
        Addresses=!LAN.IsEmpty()&&!LAN.StartsWith(TEXT("127."))&&LAN.EndsWith(FString::Printf(TEXT(":%d"),I->LobbyPort));
        FFileHelper::SaveStringToFile(LAN,*(Dir/TEXT("LocalEndpoint.txt")));
        bool Failure=FParse::Param(FCommandLine::Get(),TEXT("BenchyLobbyExpectFailure"));Public=Failure?WAN.IsEmpty():!WAN.IsEmpty();
        Rules=!BenchyLobby::ShareableIPv4("127.0.0.1",false)&&!BenchyLobby::ShareableIPv4("192.168.1.5",true)&&!BenchyLobby::ShareableIPv4("100.64.0.1",true)&&!BenchyLobby::ShareableIPv4("169.254.1.2",false)&&!BenchyLobby::ShareableIPv4("203.0.113.12",true)&&!BenchyLobby::ShareableIPv4("300.1.2.3",true)&&!BenchyLobby::ShareableIPv4("1.2.3.4:7777",true)&&!BenchyLobby::ShareableIPv4("1.2.3.4<script>",true)&&BenchyLobby::ShareableIPv4("93.184.216.34",true)&&BenchyLobby::Endpoint("192.168.1.5",17816)=="192.168.1.5:17816"&&BenchyLobby::Endpoint("127.0.0.1",7777).empty()&&BenchyLobby::Endpoint("192.168.1.5",65536).empty();
        FString Previous,Check;FPlatformApplicationMisc::ClipboardPaste(Previous);CopyLobbyAddress(false);FPlatformApplicationMisc::ClipboardPaste(Check);Copied=Check==LAN;
        if(!WAN.IsEmpty()){CopyLobbyAddress(true);FPlatformApplicationMisc::ClipboardPaste(Check);Copied&=Check==WAN;}
        else {CopyLobbyAddress(true);FPlatformApplicationMisc::ClipboardPaste(Check);Copied&=Check==LAN;}
        FPlatformApplicationMisc::ClipboardCopy(*Previous);SelectColor(6);Stage=1;Next=T+3.5;
    }
    if(Stage==1&&T>Next){ShowMenu();Shot(TEXT("Lobby-Share"));Stage=2;Next=T+2;}
    if(Stage==2&&T>Next){for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(*It!=B&&!It->IsBot&&!It->IsLocallyControlled()&&It->ColorIndex==6)Peer=true;
        if(FParse::Param(FCommandLine::Get(),TEXT("BenchyLobbySolo")))Peer=true;
        if(Peer){Stage=3;Next=T+10;}}
    if(Stage==3&&T>Next)Save(Addresses&&Copied&&Rules&&Public&&Peer,TEXT("LobbyHost"));
    if(T>75&&Stage!=99)Save(false,TEXT("LobbyHost"));
}
