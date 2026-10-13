#include "BenchyMenu.h"
#include "BenchyPawn.h"
#include "BenchyRace.h"
#include "BenchyLogo.h"
#include "BenchyDisplay.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Misc/SecureHash.h"

namespace {
FString LogoHash(const TArray<uint8>& Bytes){uint8 Hash[20];FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash);return BytesToHex(Hash,20);}
bool GoodHash(const FString& S){if(S.Len()!=40)return false;for(TCHAR C:S)if(!FChar::IsHexDigit(C))return false;return true;}
bool CleanPose(FBenchyLogoSpec& S){if(!FMath::IsFinite(S.X)||!FMath::IsFinite(S.Y)||!FMath::IsFinite(S.Size)||!FMath::IsFinite(S.Angle))return false;S.Side=FMath::Clamp(S.Side,0,5);S.X=FMath::Clamp(S.X,0.f,1.f);S.Y=FMath::Clamp(S.Y,0.f,1.f);S.Size=FMath::Clamp(S.Size,.08f,.9f);S.Angle=FMath::Clamp(S.Angle,-180.f,180.f);return S.Hash.IsEmpty()||GoodHash(S.Hash);}
void ApplySpec(UBenchyInstance* I,const FBenchyLogoSpec& S,UStaticMeshComponent* Mesh){if(!I||!Mesh)return;auto L=I->NetworkLogos.FindRef(S.Hash);if(L){L->Enabled=true;L->Side=S.Side;L->X=S.X;L->Y=S.Y;L->Size=S.Size;L->Angle=S.Angle;L->Apply(Mesh);}else Mesh->SetScalarParameterValueOnMaterials(TEXT("LogoEnabled"),0);}
bool CacheLogo(UBenchyInstance* I,const FString& Hash,const TArray<uint8>& Bytes){if(!I||LogoHash(Bytes)!=Hash)return false;if(I->NetworkLogos.Contains(Hash))return true;if(I->NetworkLogos.Num()>=32){TSet<FString> Used;for(TActorIterator<ABenchyPawn> It(I->GetWorld());It;++It)Used.Add(It->LogoSpec.Hash);FString Expired;for(const auto& Pair:I->NetworkLogos)if(!Used.Contains(Pair.Key)){Expired=Pair.Key;break;}if(Expired.IsEmpty())return false;I->NetworkLogos.Remove(Expired);I->LogoPNGs.Remove(Expired);}auto L=NewObject<UBenchyLogo>(I);if(!L->ImportNetwork(Bytes))return false;I->NetworkLogos.Add(Hash,L);I->LogoPNGs.Add(Hash,Bytes);return true;}
}
void ABenchyController::ServerChat_Implementation(const FString& Text){
    auto B=Cast<ABenchyPawn>(GetPawn());auto S=GetWorld()->GetGameState<ABenchyRaceState>();float Now=GetWorld()->GetTimeSeconds();if(!B||!S||Now-LastChatTime<1)return;
    FString Line=Text.Left(200);for(TCHAR& C:Line)if(C<' '||C==127)C=' ';Line.TrimStartAndEndInline();if(Line.IsEmpty())return;LastChatTime=Now;S->AddLobbyLine(B->RacerName+TEXT(": ")+Line);
}
void ABenchyController::SendChat(){if(!ChatBox)return;StoreFields();ServerChat(ChatBox->GetText().ToString());ChatBox->SetText(FText::GetEmpty());}
TSharedRef<SWidget> ABenchyController::BuildChat(){
    return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.015,.035,.04,.93)).Padding(14)[SNew(SVerticalBox)
      +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,9)[SNew(STextBlock).Text_Lambda([this](){int32 N=0;for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(!It->IsBot)++N;return FText::FromString(FString::Printf(TEXT("LOBBY-CHAT  ·  %d / 8 Fahrer"),N));}).Font(FCoreStyle::GetDefaultFontStyle("Bold",16)).ColorAndOpacity(FLinearColor::White)]
      +SVerticalBox::Slot().FillHeight(1)[SAssignNew(ChatScroll,SScrollBox)+SScrollBox::Slot()[SNew(STextBlock).Text_Lambda([this](){auto S=GetWorld()->GetGameState<ABenchyRaceState>();return FText::FromString(S&&S->LobbyChat.Num()?FString::Join(S->LobbyChat,TEXT("\n")):TEXT("Hier könnt ihr euch vor dem Rennen absprechen."));}).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(FLinearColor(.87,.96,.97))]]
      +SVerticalBox::Slot().AutoHeight().Padding(0,9,0,0)[SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1)[SAssignNew(ChatBox,SEditableTextBox).HintText(FText::FromString(TEXT("Nachricht …"))).OnTextCommitted_Lambda([this](const FText&,ETextCommit::Type T){if(T==ETextCommit::OnEnter)SendChat();})]+SHorizontalBox::Slot().AutoWidth().Padding(7,0)[SNew(SButton).Text(FText::FromString(TEXT("Senden"))).OnClicked_Lambda([this](){SendChat();return FReply::Handled();})]]];
}
void ABenchyController::ClearGallery(){for(auto P:Portraits)if(P)P->Destroy();Portraits.Empty();PortraitOwners.Empty();GalleryCanvas.Reset();GallerySignature.Empty();}
TSharedRef<SWidget> ABenchyController::BuildLobbyGallery(){SAssignNew(GalleryCanvas,SConstraintCanvas).Visibility(EVisibility::HitTestInvisible);RefreshGallery();return GalleryCanvas.ToSharedRef();}
void ABenchyController::RefreshGallery(){
    if(!GalleryCanvas||!Preview)return;TArray<ABenchyPawn*> Players;FString Signature;
    for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(!It->IsBot&&*It!=GetPawn())Players.Add(*It);
    Players.Sort([](const ABenchyPawn& A,const ABenchyPawn& B){return A.GetName()<B.GetName();});if(Players.Num()>7)Players.SetNum(7);
    for(auto B:Players)Signature+=B->GetName()+B->RacerName+FString::FromInt(B->ColorIndex)+FString::FromInt(B->WaitingForRace)+FString::FromInt(B->LobbyHost);
    if(Signature==GallerySignature&&Portraits.Num()==Players.Num())return;GallerySignature=Signature;
    for(auto P:Portraits)if(P)P->Destroy();Portraits.Empty();PortraitOwners.Empty();GalleryCanvas->ClearChildren();
    for(int32 N=0;N<Players.Num();++N){auto B=Players[N];auto P=GetWorld()->SpawnActor<ABenchyDisplay>();P->SetPortrait();P->SelectFilament(B->ColorIndex,false);P->SetActorScale3D(FVector(.58));Portraits.Add(P);PortraitOwners.Add(B);
        float X=.33f+(N%4)*.175f,Y=(N<4?.57f:.83f)-.12f;FString Name=B->RacerName+(B->LobbyHost?TEXT(" · Host"):TEXT(""))+(B->WaitingForRace?TEXT(" · wartet"):TEXT(""));
        GalleryCanvas->AddSlot().Anchors(FAnchors(X,Y)).Alignment(FVector2D(.5,1)).AutoSize(true)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.01,.025,.035,.8)).Padding(FMargin(10,5))[SNew(STextBlock).Text(FText::FromString(Name)).Font(FCoreStyle::GetDefaultFontStyle("Bold",14)).ColorAndOpacity(FLinearColor::White)]];
    }
}
void ABenchyController::SyncLogo(){
    auto I=GetGameInstance<UBenchyInstance>();auto B=Cast<ABenchyPawn>(GetPawn());if(!I||!I->Logo||!B)return;auto L=I->Logo;FBenchyLogoSpec S;S.Side=L->Side;S.X=L->X;S.Y=L->Y;S.Size=L->Size;S.Angle=L->Angle;
    auto Bytes=L->Enabled?L->NetworkPNG():TArray<uint8>();if(Bytes.Num())S.Hash=LogoHash(Bytes);
    if(HasAuthority()){if(S.Hash.IsEmpty()||CacheLogo(I,S.Hash,Bytes)){B->LogoSpec=S;B->ForceNetUpdate();}return;}
    if(S.Hash.IsEmpty()||B->LogoSpec.Hash==S.Hash){ServerLogoPose(S);return;}
    CacheLogo(I,S.Hash,Bytes);UploadLogo=FBenchyLogoTransfer();UploadLogo.Bytes=MoveTemp(Bytes);UploadLogo.Spec=S;UploadActive=true;ServerLogoStart(S,UploadLogo.Bytes.Num());
}
void ABenchyController::ServerLogoStart_Implementation(const FBenchyLogoSpec& Pose,int32 Total){float Now=GetWorld()->GetTimeSeconds();auto S=Pose;if(Now-LastUploadTime<2||Total<8||Total>65536||!CleanPose(S)||S.Hash.IsEmpty())return;LastUploadTime=Now;UploadLogo=FBenchyLogoTransfer();UploadLogo.Spec=S;UploadLogo.Hash=S.Hash;UploadLogo.Total=Total;UploadLogo.Started=Now;}
void ABenchyController::ServerLogoPart_Implementation(int32 Offset,const TArray<uint8>& Bytes){
    if(UploadLogo.Total<8||GetWorld()->GetTimeSeconds()-UploadLogo.Started>60||Bytes.IsEmpty()||Bytes.Num()>1024||Offset!=UploadLogo.Bytes.Num()||Offset+Bytes.Num()>UploadLogo.Total)return;UploadLogo.Bytes.Append(Bytes);
    if(UploadLogo.Bytes.Num()==UploadLogo.Total){auto I=GetGameInstance<UBenchyInstance>();if(CacheLogo(I,UploadLogo.Hash,UploadLogo.Bytes))if(auto B=Cast<ABenchyPawn>(GetPawn())){B->LogoSpec=UploadLogo.Spec;B->ForceNetUpdate();}UploadLogo=FBenchyLogoTransfer();}
}
void ABenchyController::ServerLogoPose_Implementation(const FBenchyLogoSpec& Pose){auto S=Pose;auto I=GetGameInstance<UBenchyInstance>();if(!CleanPose(S)||!I||(!S.Hash.IsEmpty()&&!I->NetworkLogos.Contains(S.Hash)))return;if(auto B=Cast<ABenchyPawn>(GetPawn())){B->LogoSpec=S;B->ForceNetUpdate();}}
void ABenchyController::ServerLogoRequest_Implementation(const FString& Hash){
    if(!GoodHash(Hash)||SendingLogos.Num()>=8)return;bool Used=false;for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)Used|=It->LogoSpec.Hash==Hash;if(!Used)return;for(const auto& T:SendingLogos)if(T.Hash==Hash)return;
    auto I=GetGameInstance<UBenchyInstance>();auto Bytes=I?I->LogoPNGs.Find(Hash):nullptr;if(!Bytes)return;FBenchyLogoTransfer T;T.Hash=Hash;T.Bytes=*Bytes;T.Total=Bytes->Num();SendingLogos.Add(MoveTemp(T));
}
void ABenchyController::ClientLogoPart_Implementation(const FString& Hash,int32 Total,int32 Offset,const TArray<uint8>& Bytes){
    if(!GoodHash(Hash)||Total<8||Total>65536||Bytes.IsEmpty()||Bytes.Num()>1024||Offset<0||Offset+Bytes.Num()>Total)return;
    if(Offset==0){if(ReceivingLogos.Num()>=8)return;FBenchyLogoTransfer T;T.Hash=Hash;T.Total=Total;T.Started=GetWorld()->GetTimeSeconds();ReceivingLogos.Add(Hash,MoveTemp(T));}
    auto T=ReceivingLogos.Find(Hash);if(!T||T->Total!=Total||T->Bytes.Num()!=Offset)return;T->Bytes.Append(Bytes);
    if(T->Bytes.Num()==Total){CacheLogo(GetGameInstance<UBenchyInstance>(),Hash,T->Bytes);ReceivingLogos.Remove(Hash);RequestedLogos.Remove(Hash);}
}
void ABenchyController::TickSocial(float DT){
    TransferClock+=DT;if(TransferClock>=.05f){TransferClock=0;
        if(!HasAuthority()&&UploadActive){int32 Count=FMath::Min(1024,UploadLogo.Bytes.Num()-UploadLogo.Offset);if(Count>0){TArray<uint8> Chunk;Chunk.Append(UploadLogo.Bytes.GetData()+UploadLogo.Offset,Count);ServerLogoPart(UploadLogo.Offset,Chunk);UploadLogo.Offset+=Count;}if(UploadLogo.Offset>=UploadLogo.Bytes.Num())UploadActive=false;}
        if(HasAuthority()&&SendingLogos.Num()){auto& T=SendingLogos[0];int32 Count=FMath::Min(1024,T.Bytes.Num()-T.Offset);TArray<uint8> Chunk;Chunk.Append(T.Bytes.GetData()+T.Offset,Count);ClientLogoPart(T.Hash,T.Total,T.Offset,Chunk);T.Offset+=Count;if(T.Offset>=T.Bytes.Num())SendingLogos.RemoveAt(0);}
    }
    if(!IsLocalController())return;auto I=GetGameInstance<UBenchyInstance>();if(!I)return;
    for(auto It=ReceivingLogos.CreateIterator();It;++It)if(GetWorld()->GetTimeSeconds()-It.Value().Started>60){RequestedLogos.Remove(It.Key());It.RemoveCurrent();}
    for(auto It=LogoRequestTimes.CreateIterator();It;++It)if(GetWorld()->GetTimeSeconds()-It.Value()>60){RequestedLogos.Remove(It.Key());It.RemoveCurrent();}
    for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It){auto B=*It;if(B==GetPawn())continue;const auto& Hash=B->LogoSpec.Hash;if(!Hash.IsEmpty()&&!I->NetworkLogos.Contains(Hash)&&!RequestedLogos.Contains(Hash)){RequestedLogos.Add(Hash);LogoRequestTimes.Add(Hash,GetWorld()->GetTimeSeconds());ServerLogoRequest(Hash);}ApplySpec(I,B->LogoSpec,B->FindComponentByClass<UStaticMeshComponent>());}
    auto S=GetWorld()->GetGameState<ABenchyRaceState>();if(S){FString Text=FString::Join(S->LobbyChat,TEXT("\n"));if(Text!=LastChat){LastChat=Text;if(ChatScroll)ChatScroll->ScrollToEnd();}}
    GalleryClock+=DT;if(GalleryClock>.3){GalleryClock=0;RefreshGallery();}
    if(!Preview||!GalleryCanvas)return;auto C=Preview->DisplayCamera();int32 W=1600,H=900;GetViewportSize(W,H);float Aspect=H>0?float(W)/H:1.777f;float Half=3200*FMath::Tan(FMath::DegreesToRadians(C->FieldOfView*.5f));
    for(int32 N=0;N<Portraits.Num();++N){auto P=Portraits[N];auto B=PortraitOwners[N].Get();if(!P||!B)continue;float X=.33f+(N%4)*.175f,Y=N<4?.57f:.83f;FVector Center=C->GetComponentLocation()+C->GetForwardVector()*3200+C->GetRightVector()*(2*(X-.5f)*Half)+C->GetUpVector()*(2*(.5f-Y)*Half/Aspect);P->SetActorRotation(Preview->GetActorRotation());P->SetActorLocation(Center-P->GetActorQuat().RotateVector(FVector(0,0,P->DisplayHeight()*.45f*.58f)));if(P->SelectedFilament()!=B->ColorIndex)P->SelectFilament(B->ColorIndex,false);ApplySpec(I,B->LogoSpec,P->LogoMesh());}
}
