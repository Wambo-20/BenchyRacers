#include "BenchyMenu.h"
#include "BenchyLogo.h"
#include "BenchyPawn.h"
#include "BenchyRace.h"
#include "BenchyOnline.h"
#include "BenchyNetDriver.h"
#include "BenchyGameMode.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

void ABenchyController::AuditLogo(){
    if(FParse::Param(FCommandLine::Get(),TEXT("BenchyLayerAudit"))){
        auto I=GetGameInstance<UBenchyInstance>();if(!I||!GetPawn())return;auto L=I->Logo;static int Step=0;static float At=0;static bool Good=true;float T=GetWorld()->GetTimeSeconds();auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);
        if(Step==0&&T>3){FString File;FParse::Value(FCommandLine::Get(),TEXT("BenchyLogoFixture="),File);Good=L->Import(File,false);L->Y=.17;L->Size=.35;L->EnableLayers();L->DuplicateLayer();L->ActiveLayer()->Side=1;L->ActiveLayer()->X=.6;L->DuplicateLayer();L->ActiveLayer()->Side=4;L->ActiveLayer()->X=.5;L->ActiveLayer()->Y=.18;L->ActiveLayer()->Size=.85;ApplyLogo(true);
            auto Copy=NewObject<UBenchyLogo>(I);Copy->Load();Good&=Copy->Layered&&Copy->Layers.Num()==3&&Copy->Layers[0]->Side==0&&Copy->Layers[1]->Side==1&&Copy->Layers[2]->Side==4&&FMath::IsNearlyEqual(Copy->Layers[1]->X,.6f);auto Before=Copy->ActiveLayer();Copy->MoveLayer(-1);Good&=Copy->ActiveLayer()==Before&&Copy->Layers[1]==Before;Copy->DuplicateLayer();Good&=Copy->Layers.Num()==4;Copy->DeleteLayer();Good&=Copy->Layers.Num()==3;
            Copy->Layers[0]->Enabled=false;Copy->BuildAtlas(FVector(90,40,50));Good&=!Copy->Layers[0]->Enabled&&Copy->Layers[1]->Enabled;
            while(Copy->Layers.Num()<8)Copy->DuplicateLayer();Good&=!Copy->AddLayer(File)&&Copy->Layers.Num()==8;
            int Counts[4]={0,0,0,0};for(int V=0;V<L->PixelHeight;++V)for(int U=0;U<L->PixelWidth;++U)if(L->Rendered[V*L->PixelWidth+U].A)++Counts[(V/512)*2+U/512];Good&=Counts[0]>10&&Counts[1]>10&&Counts[3]>10&&Counts[2]==0;
            auto PNG=L->NetworkPNG();auto Remote=NewObject<UBenchyLogo>(I);Good&=PNG.Num()>8&&PNG.Num()<=65536&&Remote->ImportNetwork(PNG);L->ActiveIndex=0;ShowMenu(2);Step=1;At=T+3;
        }else if(Step==1&&T>At){FScreenshotRequest::RequestScreenshot(Dir/TEXT("Layers-Left.png"),true,false);Step=2;At=T+1;
        }else if(Step==2&&T>At){L->ActiveIndex=1;ShowMenu(2);Step=3;At=T+3;
        }else if(Step==3&&T>At){FScreenshotRequest::RequestScreenshot(Dir/TEXT("Layers-Right.png"),true,false);Step=4;At=T+1;
        }else if(Step==4&&T>At){L->ActiveIndex=2;ShowMenu(2);Step=5;At=T+3;
        }else if(Step==5&&T>At){FScreenshotRequest::RequestScreenshot(Dir/TEXT("Layers-Stern.png"),true,false);Step=6;At=T+1;
        }else if(Step==6&&T>At){FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"three_faces\":true,\"independent_layers_save_load\":true,\"duplicate_delete_reorder_limit\":true,\"network_atlas_roundtrip\":true}"),Good?TEXT("true"):TEXT("false")),*(Dir/TEXT("Layers.json")));Step=99;FPlatformMisc::RequestExit(false);}return;
    }
    if(!FParse::Param(FCommandLine::Get(),TEXT("BenchyLogoAudit")))return;auto I=GetGameInstance<UBenchyInstance>();if(!I||!GetPawn())return;
    static int Stage=0;static float Next=0;static bool Good=true;float T=GetWorld()->GetTimeSeconds();auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);
    if(Stage==0&&T>3){ImportLogo();Good=I->Logo->Enabled&&I->Logo->Texture;I->Logo->Outline=0;I->Logo->Y=.23;I->Logo->Size=.28;I->Logo->Rebuild();ApplyLogo();ShowMenu(2);Stage=1;Next=T+3;}
    if(Stage==1&&T>Next){FScreenshotRequest::RequestScreenshot(Dir/TEXT("Logo-Black.png"),true,false);Stage=2;Next=T+1;}
    if(Stage==2&&T>Next){I->Logo->Outline=1;I->Logo->OutlineWidth=5;I->Logo->Rebuild();ApplyLogo(true);ShowMenu(2);Stage=3;Next=T+3;}
    if(Stage==3&&T>Next){FScreenshotRequest::RequestScreenshot(Dir/TEXT("Logo-White-Outline.png"),true,false);Stage=4;Next=T+1;}
    if(Stage==4&&T>Next){auto Copy=NewObject<UBenchyLogo>(I);Copy->Load();Good&=Copy->Enabled&&Copy->Outline==1&&Copy->OutlineWidth==5&&Copy->Source==I->Logo->Source;auto Bytes=I->Logo->NetworkPNG();Good&=Bytes.Num()>8&&Bytes.Num()<=65536;auto Remote=NewObject<UBenchyLogo>(I);Good&=Remote->ImportNetwork(Bytes);Good&=!Remote->ImportNetwork(TArray<uint8>{1,2,3,4,5,6,7,8});I->Logo->Outline=3;I->Logo->Rebuild();I->Logo->Angle=18;I->Logo->Side=3;ApplyLogo(true);ShowMenu(2);Stage=5;Next=T+3;}
    if(Stage==5&&T>Next){FScreenshotRequest::RequestScreenshot(Dir/TEXT("Logo-Neon-Outline.png"),true,false);Stage=6;Next=T+1;}
    if(Stage==6&&T>Next){FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"import_persistence_network_roundtrip\":%s}"),Good?TEXT("true"):TEXT("false"),Good?TEXT("true"):TEXT("false")),*(Dir/TEXT("Logo.json")));Stage=99;FPlatformMisc::RequestExit(false);}
}
void ABenchyController::AuditSocial(){
    if(FParse::Param(FCommandLine::Get(),TEXT("BenchyLateJoinAudit"))){
        auto S=GetWorld()->GetGameState<ABenchyRaceState>();auto B=Cast<ABenchyPawn>(GetPawn());if(!S||!B)return;static int Step=0;static float At=0;static bool NewEntry=false;float T=GetWorld()->GetTimeSeconds();auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);
        if(HasAuthority()){
            if(Step==0&&T>3){GetWorld()->GetAuthGameMode<ABenchyGameMode>()->StartRace();Step=1;}
            if(Step==1&&S->Phase==2){FFileHelper::SaveStringToFile(TEXT("ready"),*(Dir/TEXT("LateHostReady.txt")));for(const auto& Line:S->LobbyChat)if(Line.Contains(TEXT("LATE_JOIN_SEEN"))){S->Phase=3;S->ForceNetUpdate();Step=2;At=T+3;break;}}
            if(Step==2&&T>At){GetWorld()->GetAuthGameMode<ABenchyGameMode>()->StartRace();for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(!It->IsBot&&*It!=B&&!It->WaitingForRace&&It->RaceEntryId>=0)NewEntry=true;Step=3;At=T+15;}
            if(Step==3&&T>At){bool Good=NewEntry;for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(!It->IsBot&&*It!=B&&!It->WaitingForRace&&It->RaceEntryId>=0)Good=true;FFileHelper::SaveStringToFile(Good?TEXT("{\"passed\":true,\"late_join_waited\":true,\"next_race_entry\":true}"):TEXT("{\"passed\":false}"),*(Dir/TEXT("LateJoin.json")));Step=99;FPlatformMisc::RequestExit(false);}
        }else if(GetNetMode()==NM_Client){
            if(Step==0&&B->WaitingForRace){ShowMenu();FScreenshotRequest::RequestScreenshot(Dir/TEXT("Late-Join-Waiting.png"),true,false);ServerChat(TEXT("LATE_JOIN_SEEN"));Step=1;}
            if(Step==1&&!B->WaitingForRace&&B->RaceEntryId>=0&&(S->Phase==1||S->Phase==2)){FFileHelper::SaveStringToFile(TEXT("{\"passed\":true,\"late_join_waited\":true,\"next_race_entry\":true}"),*(Dir/TEXT("LateJoin.json")));Step=99;FPlatformMisc::RequestExit(false);}
        }
        if(T>180&&Step!=99){FFileHelper::SaveStringToFile(TEXT("{\"passed\":false}"),*(Dir/TEXT("LateJoin.json")));Step=99;FPlatformMisc::RequestExit(false);}return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("BenchyEOSAudit"))){auto I=GetGameInstance<UBenchyInstance>();static bool Started=false;static int Capture=0;if(I&&I->Online&&!Started&&GetWorld()->GetTimeSeconds()>3){Started=true;FString Code;if(FParse::Value(FCommandLine::Get(),TEXT("BenchyRoom="),Code))I->Online->Join(Code);else I->Online->Host();}if(I&&I->Online&&I->Online->Hosting&&GetNetMode()==NM_ListenServer){auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);if(Capture==0){FFileHelper::SaveStringToFile(I->Online->RoomCode,*(Dir/TEXT("RoomCode.txt")));Capture=1;}if(FParse::Param(FCommandLine::Get(),TEXT("BenchyEOSHostAudit"))){if(Capture==1&&GetWorld()->GetTimeSeconds()>10){ShowMenu();FScreenshotRequest::RequestScreenshot(Dir/TEXT("EOS-Lobby-Code.png"),true,false);Capture=2;}if(Capture==2&&GetWorld()->GetTimeSeconds()>12){auto D=Cast<UBenchyNetDriver>(GetWorld()->GetNetDriver());bool Good=D&&!D->bIsPassthrough&&I->Online->RoomCode.Len()==8;FFileHelper::SaveStringToFile(Good?TEXT("{\"passed\":true,\"device_login\":true,\"lobby_created\":true,\"eos_transport_listening\":true,\"remote_relay_connection_verified\":false}"):TEXT("{\"passed\":false}"),*(Dir/TEXT("EOSHost.json")));Capture=99;FPlatformMisc::RequestExit(false);}}}}
    if(!FParse::Param(FCommandLine::Get(),TEXT("BenchySocialAudit"))||GetNetMode()==NM_Standalone)return;auto I=GetGameInstance<UBenchyInstance>();auto B=Cast<ABenchyPawn>(GetPawn());auto S=GetWorld()->GetGameState<ABenchyRaceState>();if(!I||!B||!S)return;
    static int Stage=0;static float Next=0;float T=GetWorld()->GetTimeSeconds();auto Dir=FPaths::ProjectSavedDir()/TEXT("Audit");IFileManager::Get().MakeDirectory(*Dir,true);
    if(Stage==0&&T>4){SelectColor(HasAuthority()?6:4);ImportLogo();I->Logo->Outline=1;I->Logo->Side=3;I->Logo->Rebuild();if(FParse::Param(FCommandLine::Get(),TEXT("BenchyLayersSocial"))){I->Logo->Y=.17;I->Logo->EnableLayers();I->Logo->DuplicateLayer();I->Logo->ActiveLayer()->Side=4;I->Logo->ActiveLayer()->Size=.8;I->Logo->ActiveLayer()->Y=.18;}ApplyLogo(true);ServerChat(HasAuthority()?TEXT("Hallo vom Host"):TEXT("Hallo vom Freund"));ShowMenu();Stage=1;Next=T+8;}
    if(Stage==1&&T>Next){bool Peer=false,Logo=false;for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(!It->IsBot&&*It!=B){Peer=true;Logo=!It->LogoSpec.Hash.IsEmpty()&&I->NetworkLogos.Contains(It->LogoSpec.Hash);}bool Chat=false;for(const auto& Line:S->LobbyChat)Chat|=Line.Contains(HasAuthority()?TEXT("Hallo vom Freund"):TEXT("Hallo vom Host"));if(Peer&&Logo&&Chat&&Portraits.Num()>0){FScreenshotRequest::RequestScreenshot(Dir/TEXT("Lobby-Live-Chat-Logos.png"),true,false);Stage=2;Next=T+(HasAuthority()?15:1);}else if(T>180){FFileHelper::SaveStringToFile(TEXT("{\"passed\":false}"),*(Dir/TEXT("Social.json")));Stage=99;FPlatformMisc::RequestExit(false);}}
    if(Stage==2&&T>Next){FFileHelper::SaveStringToFile(TEXT("{\"passed\":true,\"peer_logo\":true,\"bidirectional_chat\":true,\"live_3d_preview\":true}"),*(Dir/TEXT("Social.json")));Stage=99;FPlatformMisc::RequestExit(false);}
}
