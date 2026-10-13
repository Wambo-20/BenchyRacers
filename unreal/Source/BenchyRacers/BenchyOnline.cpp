#include "BenchyOnline.h"
#include "BenchyMenu.h"
#include "BenchyWorld.h"
#include "IEOSSDKManager.h"
#include "SocketSubsystemEOS.h"
#include "SocketSubsystemEOSUtils.h"
#include "InternetAddrEOS.h"
#include "eos_connect.h"
#include "eos_sdk.h"
#include "eos_lobby.h"
#include "eos_p2p.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"

struct FBenchyEOSUtils : ISocketSubsystemEOSUtils {
    EOS_ProductUserId User=nullptr;
    virtual EOS_ProductUserId GetLocalUserId() override{return User;}
    virtual FString GetSessionId() override{return TEXT("BenchyRacers");}
    virtual FName GetSubsystemInstanceName() override{return TEXT("BenchyRacers");}
    virtual bool IsLoggedIn() override{return User!=nullptr;}
};
struct FBenchyEOS {
    IEOSPlatformHandlePtr Platform;
    TSharedPtr<FBenchyEOSUtils,ESPMode::ThreadSafe> Utils;
    TUniquePtr<FSocketSubsystemEOS> Sockets;
    EOS_HConnect Connect=nullptr;
    EOS_HLobby Lobby=nullptr;
    EOS_HLobbySearch Search=nullptr;
    EOS_HLobbyDetails Details=nullptr;
    EOS_HLobbyModification Modification=nullptr;
    EOS_ProductUserId User=nullptr,Owner=nullptr;
    FString LobbyId;
    EOS_NotificationId Expiry=EOS_INVALID_NOTIFICATIONID;
    TWeakObjectPtr<UBenchyOnline> OwnerObject;
    ~FBenchyEOS(){if(Expiry!=EOS_INVALID_NOTIFICATIONID)EOS_Connect_RemoveNotifyAuthExpiration(Connect,Expiry);if(Search)EOS_LobbySearch_Release(Search);if(Details)EOS_LobbyDetails_Release(Details);if(Modification)EOS_LobbyModification_Release(Modification);if(Sockets)Sockets->Shutdown();Sockets.Reset();Platform.Reset();}
};
namespace {
using FContext=TWeakObjectPtr<UBenchyOnline>;
FContext* Context(UBenchyOnline* O){return new FContext(O);}
UBenchyOnline* Resolve(void* Data){TUniquePtr<FContext> C(static_cast<FContext*>(Data));return C->Get();}
UBenchyInstance* Instance(UBenchyOnline* O){return Cast<UBenchyInstance>(O->GetOuter());}
const char* Bucket="BenchyRacers-1.6.1";
}
UBenchyOnline::UBenchyOnline(){}
UBenchyOnline::~UBenchyOnline(){}
void UBenchyOnline::Initialize(){
    FModuleManager::LoadModuleChecked<IModuleInterface>(TEXT("EOSShared"));FModuleManager::LoadModuleChecked<IModuleInterface>(TEXT("SocketSubsystemEOS"));
    FString File=FPaths::ProjectConfigDir()/TEXT("BenchyEOS.ini");FParse::Value(FCommandLine::Get(),TEXT("BenchyEOSConfig="),File);FConfigFile Config;if(!FPaths::FileExists(File)){Status=TEXT("Online-Dienst noch nicht eingerichtet.");return;}Config.Read(File);
    FEOSSDKPlatformConfig C;C.Name=TEXT("BenchyRacers");Config.GetString(TEXT("BenchyEOS"),TEXT("ProductId"),C.ProductId);Config.GetString(TEXT("BenchyEOS"),TEXT("SandboxId"),C.SandboxId);Config.GetString(TEXT("BenchyEOS"),TEXT("DeploymentId"),C.DeploymentId);Config.GetString(TEXT("BenchyEOS"),TEXT("ClientId"),C.ClientId);Config.GetString(TEXT("BenchyEOS"),TEXT("ClientSecret"),C.ClientSecret);
    if(C.ProductId.IsEmpty()||C.SandboxId.IsEmpty()||C.DeploymentId.IsEmpty()||C.ClientId.IsEmpty()||C.ClientSecret.IsEmpty()){Status=TEXT("Online-Konfiguration unvollständig.");return;}
    C.bDisableOverlay=true;C.bDisableSocialOverlay=true;C.bEnableRTC=false;C.CacheDirectory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("EOS"));IFileManager::Get().MakeDirectory(*C.CacheDirectory,true);
    auto M=IEOSSDKManager::Get();if(!M){Status=TEXT("Online-Dienst nicht verfügbar.");return;}M->AddPlatformConfig(C,true);EOS=MakeShared<FBenchyEOS>();EOS->Platform=M->CreatePlatform(C.Name,TEXT("BenchyRacers"));
    if(!EOS->Platform){EOS.Reset();Status=TEXT("Online-Dienst konnte nicht gestartet werden.");return;}EOS->Connect=EOS_Platform_GetConnectInterface(*EOS->Platform);EOS->Lobby=EOS_Platform_GetLobbyInterface(*EOS->Platform);Configured=true;Status=TEXT("Online bereit · ohne Portfreigabe");
    EOS_P2P_SetRelayControlOptions Relay={};Relay.ApiVersion=EOS_P2P_SETRELAYCONTROL_API_LATEST;Relay.RelayControl=FParse::Param(FCommandLine::Get(),TEXT("BenchyForceRelay"))?EOS_ERelayControl::EOS_RC_ForceRelays:EOS_ERelayControl::EOS_RC_AllowRelays;EOS_P2P_SetRelayControl(EOS_Platform_GetP2PInterface(*EOS->Platform),&Relay);
}
void UBenchyOnline::Fail(const TCHAR* Action,int32 Result){Busy=false;Status=FString(Action)+TEXT(": ")+UTF8_TO_TCHAR(EOS_EResult_ToString(static_cast<EOS_EResult>(Result)));if(auto I=Instance(this))I->Notice=Status;}
void UBenchyOnline::Stop(){Configured=false;Busy=false;EOS.Reset();}
void UBenchyOnline::Host(){if(Busy)return;if(!Configured){Instance(this)->Notice=Status;return;}Leave();PendingHost=true;Busy=true;RoomCode=FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8).ToUpper();Status=TEXT("Online-Lobby wird erstellt …");if(EOS->User)CreateRoom();else Login();}
void UBenchyOnline::Join(FString Code){Code.TrimStartAndEndInline();Code=Code.Replace(TEXT("-"),TEXT("")).ToUpper();if(Code.Len()!=8){Instance(this)->Notice=TEXT("Bitte den 8-stelligen Lobby-Code eingeben.");return;}for(TCHAR C:Code)if(!FChar::IsHexDigit(C)){Instance(this)->Notice=TEXT("Ungültiger Lobby-Code.");return;}if(Busy)return;if(!Configured){Instance(this)->Notice=Status;return;}Leave();RoomCode=Code;PendingHost=false;Busy=true;Status=TEXT("Lobby wird gesucht …");if(EOS->User)FindRoom();else Login();}
void UBenchyOnline::Login(){
    EOS_Connect_CreateDeviceIdOptions O={};O.ApiVersion=EOS_CONNECT_CREATEDEVICEID_API_LATEST;O.DeviceModel="Benchy Racers Windows";
    EOS_Connect_CreateDeviceId(EOS->Connect,&O,Context(this),[](const EOS_Connect_CreateDeviceIdCallbackInfo* R){auto Self=Resolve(R->ClientData);if(!Self||!Self->EOS)return;if(R->ResultCode!=EOS_EResult::EOS_Success&&R->ResultCode!=EOS_EResult::EOS_DuplicateNotAllowed){Self->Fail(TEXT("Geräteanmeldung"),int32(R->ResultCode));return;}
        EOS_Connect_Credentials Credentials={};Credentials.ApiVersion=EOS_CONNECT_CREDENTIALS_API_LATEST;Credentials.Type=EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;
        FTCHARToUTF8 Nick(*Instance(Self)->Nick);EOS_Connect_UserLoginInfo Info={};Info.ApiVersion=EOS_CONNECT_USERLOGININFO_API_LATEST;Info.DisplayName=Nick.Get();EOS_Connect_LoginOptions Login={};Login.ApiVersion=EOS_CONNECT_LOGIN_API_LATEST;Login.Credentials=&Credentials;Login.UserLoginInfo=&Info;
        EOS_Connect_Login(Self->EOS->Connect,&Login,Context(Self),[](const EOS_Connect_LoginCallbackInfo* L){auto S=Resolve(L->ClientData);if(!S||!S->EOS)return;if(L->ResultCode==EOS_EResult::EOS_Success){S->Ready(L->LocalUserId);return;}if(L->ResultCode!=EOS_EResult::EOS_InvalidUser){S->Fail(TEXT("Online-Anmeldung"),int32(L->ResultCode));return;}
            EOS_Connect_CreateUserOptions U={};U.ApiVersion=EOS_CONNECT_CREATEUSER_API_LATEST;U.ContinuanceToken=L->ContinuanceToken;EOS_Connect_CreateUser(S->EOS->Connect,&U,Context(S),[](const EOS_Connect_CreateUserCallbackInfo* R){auto S=Resolve(R->ClientData);if(!S||!S->EOS)return;if(R->ResultCode==EOS_EResult::EOS_Success)S->Ready(R->LocalUserId);else S->Fail(TEXT("Spieler anlegen"),int32(R->ResultCode));});
        });
    });
}
void UBenchyOnline::Ready(void* User){
    EOS->User=static_cast<EOS_ProductUserId>(User);if(EOS->Expiry==EOS_INVALID_NOTIFICATIONID){EOS->OwnerObject=this;EOS_Connect_AddNotifyAuthExpirationOptions N={};N.ApiVersion=EOS_CONNECT_ADDNOTIFYAUTHEXPIRATION_API_LATEST;EOS->Expiry=EOS_Connect_AddNotifyAuthExpiration(EOS->Connect,&N,&EOS->OwnerObject,[](const EOS_Connect_AuthExpirationCallbackInfo* R){auto W=static_cast<TWeakObjectPtr<UBenchyOnline>*>(R->ClientData);if(auto S=W->Get())if(S->EOS)S->Login();});}if(!EOS->Sockets){EOS->Utils=MakeShared<FBenchyEOSUtils,ESPMode::ThreadSafe>();EOS->Utils->User=EOS->User;EOS->Sockets=MakeUnique<FSocketSubsystemEOS>(EOS->Platform,EOS->Utils);FString Error;if(!EOS->Sockets->Init(Error)){Busy=false;Status=TEXT("Online-Verbindung konnte nicht gestartet werden: ")+Error;return;}}
    if(!Busy)return;
    if(PendingHost)CreateRoom();else FindRoom();
}
void UBenchyOnline::CreateRoom(){
    EOS_Lobby_CreateLobbyOptions O={};O.ApiVersion=EOS_LOBBY_CREATELOBBY_API_LATEST;O.LocalUserId=EOS->User;O.MaxLobbyMembers=8;O.PermissionLevel=EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;O.BucketId=Bucket;O.bDisableHostMigration=EOS_TRUE;
    EOS_Lobby_CreateLobby(EOS->Lobby,&O,Context(this),[](const EOS_Lobby_CreateLobbyCallbackInfo* R){auto S=Resolve(R->ClientData);if(!S||!S->EOS)return;if(R->ResultCode!=EOS_EResult::EOS_Success){S->Fail(TEXT("Lobby erstellen"),int32(R->ResultCode));return;}S->EOS->LobbyId=UTF8_TO_TCHAR(R->LobbyId);S->Hosting=true;S->PublishRoom();});
}
void UBenchyOnline::PublishRoom(){
    FTCHARToUTF8 Id(*EOS->LobbyId),Code(*RoomCode);EOS_Lobby_UpdateLobbyModificationOptions O={};O.ApiVersion=EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;O.LocalUserId=EOS->User;O.LobbyId=Id.Get();auto Result=EOS_Lobby_UpdateLobbyModification(EOS->Lobby,&O,&EOS->Modification);if(Result!=EOS_EResult::EOS_Success){Fail(TEXT("Lobby konfigurieren"),int32(Result));return;}
    EOS_Lobby_AttributeData Data={};Data.ApiVersion=EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;Data.Key="ROOM_CODE";Data.ValueType=EOS_ELobbyAttributeType::EOS_AT_STRING;Data.Value.AsUtf8=Code.Get();EOS_LobbyModification_AddAttributeOptions A={};A.ApiVersion=EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;A.Attribute=&Data;A.Visibility=EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;Result=EOS_LobbyModification_AddAttribute(EOS->Modification,&A);if(Result!=EOS_EResult::EOS_Success){Fail(TEXT("Lobby-Code setzen"),int32(Result));return;}
    Data.Key="BUILD";Data.Value.AsUtf8=Bucket;EOS_LobbyModification_AddAttribute(EOS->Modification,&A);EOS_Lobby_UpdateLobbyOptions U={};U.ApiVersion=EOS_LOBBY_UPDATELOBBY_API_LATEST;U.LobbyModificationHandle=EOS->Modification;
    EOS_Lobby_UpdateLobby(EOS->Lobby,&U,Context(this),[](const EOS_Lobby_UpdateLobbyCallbackInfo* R){auto S=Resolve(R->ClientData);if(!S||!S->EOS)return;EOS_LobbyModification_Release(S->EOS->Modification);S->EOS->Modification=nullptr;if(R->ResultCode!=EOS_EResult::EOS_Success){S->Fail(TEXT("Lobby veröffentlichen"),int32(R->ResultCode));return;}S->Busy=false;S->Status=TEXT("Lobby bereit · ohne Portfreigabe");auto I=Instance(S);I->Notice.Reset();const TCHAR* Maps[]={TEXT("/Game/Maps/Sonnendeck"),TEXT("/Game/Maps/Kupferhafen"),TEXT("/Game/Maps/Frostfjord")};UGameplayStatics::OpenLevel(I,TEXT("/Game/Maps/Sonnendeck"),true,FString::Printf(TEXT("listen?Track=%d?RoomCode=%s"),I->SelectedTrack,*S->RoomCode));});
}
void UBenchyOnline::FindRoom(){
    if(EOS->Search){EOS_LobbySearch_Release(EOS->Search);EOS->Search=nullptr;}EOS_Lobby_CreateLobbySearchOptions O={};O.ApiVersion=EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;O.MaxResults=10;auto R=EOS_Lobby_CreateLobbySearch(EOS->Lobby,&O,&EOS->Search);if(R!=EOS_EResult::EOS_Success){Fail(TEXT("Lobby suchen"),int32(R));return;}
    FTCHARToUTF8 Code(*RoomCode);EOS_Lobby_AttributeData D={};D.ApiVersion=EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;D.Key="ROOM_CODE";D.ValueType=EOS_ELobbyAttributeType::EOS_AT_STRING;D.Value.AsUtf8=Code.Get();EOS_LobbySearch_SetParameterOptions P={};P.ApiVersion=EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;P.Parameter=&D;P.ComparisonOp=EOS_EComparisonOp::EOS_CO_EQUAL;EOS_LobbySearch_SetParameter(EOS->Search,&P);D.Key="BUILD";D.Value.AsUtf8=Bucket;EOS_LobbySearch_SetParameter(EOS->Search,&P);
    EOS_LobbySearch_FindOptions F={};F.ApiVersion=EOS_LOBBYSEARCH_FIND_API_LATEST;F.LocalUserId=EOS->User;EOS_LobbySearch_Find(EOS->Search,&F,Context(this),[](const EOS_LobbySearch_FindCallbackInfo* R){auto S=Resolve(R->ClientData);if(!S||!S->EOS)return;if(R->ResultCode!=EOS_EResult::EOS_Success){S->Fail(TEXT("Lobby suchen"),int32(R->ResultCode));return;}
        EOS_LobbySearch_GetSearchResultCountOptions C={};C.ApiVersion=EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;if(!EOS_LobbySearch_GetSearchResultCount(S->EOS->Search,&C)){S->Busy=false;S->Status=TEXT("Lobby nicht gefunden. Code und Spielversion prüfen.");Instance(S)->Notice=S->Status;return;}
        if(S->EOS->Details)EOS_LobbyDetails_Release(S->EOS->Details);EOS_LobbySearch_CopySearchResultByIndexOptions Copy={};Copy.ApiVersion=EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;auto Result=EOS_LobbySearch_CopySearchResultByIndex(S->EOS->Search,&Copy,&S->EOS->Details);if(Result!=EOS_EResult::EOS_Success){S->Fail(TEXT("Lobby laden"),int32(Result));return;}
        EOS_LobbyDetails_GetLobbyOwnerOptions Owner={};Owner.ApiVersion=EOS_LOBBYDETAILS_GETLOBBYOWNER_API_LATEST;S->EOS->Owner=EOS_LobbyDetails_GetLobbyOwner(S->EOS->Details,&Owner);if(S->EOS->Owner==S->EOS->User){S->Busy=false;S->Status=TEXT("Diese Lobby gehört deinem Gerät. Ein Freund muss von seinem PC aus beitreten.");Instance(S)->Notice=S->Status;return;}EOS_Lobby_JoinLobbyOptions Join={};Join.ApiVersion=EOS_LOBBY_JOINLOBBY_API_LATEST;Join.LocalUserId=S->EOS->User;Join.LobbyDetailsHandle=S->EOS->Details;
        EOS_Lobby_JoinLobby(S->EOS->Lobby,&Join,Context(S),[](const EOS_Lobby_JoinLobbyCallbackInfo* R){auto S=Resolve(R->ClientData);if(!S||!S->EOS)return;if(R->ResultCode!=EOS_EResult::EOS_Success){S->Fail(TEXT("Lobby beitreten"),int32(R->ResultCode));return;}S->EOS->LobbyId=UTF8_TO_TCHAR(R->LobbyId);S->Hosting=false;S->TravelToHost();});
    });
}
void UBenchyOnline::TravelToHost(){Busy=false;Status=TEXT("Verbindung zum Gastgeber wird aufgebaut …");FInternetAddrEOS Address(EOS->Owner);auto I=Instance(this);I->Notice=Status;if(auto C=I->GetFirstLocalPlayerController())C->ClientTravel(TEXT("[")+Address.ToString(false)+TEXT("]?RoomCode=")+RoomCode,TRAVEL_Absolute);}
void UBenchyOnline::Leave(){
    RoomCode.Reset();if(!EOS||EOS->LobbyId.IsEmpty())return;FTCHARToUTF8 Id(*EOS->LobbyId);if(Hosting){EOS_Lobby_DestroyLobbyOptions O={};O.ApiVersion=EOS_LOBBY_DESTROYLOBBY_API_LATEST;O.LocalUserId=EOS->User;O.LobbyId=Id.Get();EOS_Lobby_DestroyLobby(EOS->Lobby,&O,nullptr,[](const EOS_Lobby_DestroyLobbyCallbackInfo*){});}else{EOS_Lobby_LeaveLobbyOptions O={};O.ApiVersion=EOS_LOBBY_LEAVELOBBY_API_LATEST;O.LocalUserId=EOS->User;O.LobbyId=Id.Get();EOS_Lobby_LeaveLobby(EOS->Lobby,&O,nullptr,[](const EOS_Lobby_LeaveLobbyCallbackInfo*){});}EOS->LobbyId.Reset();Hosting=false;
}
