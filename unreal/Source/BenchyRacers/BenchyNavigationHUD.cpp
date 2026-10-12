#include "BenchyGameMode.h"
#include "BenchyPawn.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Styling/CoreStyle.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

void ABenchyHUD::DrawNavigation(ABenchyPawn* B){
    const float Scale=FMath::Clamp(Canvas->SizeY/900.f,.75f,1.8f);
    const float X=Canvas->SizeX*.5f-170*Scale,Y=24*Scale;
    const FVector Toward=B->NavigationTarget()-B->GetActorLocation();
    const float CameraYaw=PlayerOwner->PlayerCameraManager?PlayerOwner->PlayerCameraManager->GetCameraRotation().Yaw:B->GetActorRotation().Yaw;
    const float Angle=FMath::DegreesToRadians(FMath::FindDeltaAngleDegrees(CameraYaw,Toward.Rotation().Yaw));
    const bool WrongWay=!B->MissedCheckpoint&&B->SpeedKmh()>10&&FVector::DotProduct(B->GetActorForwardVector(),Toward.GetSafeNormal2D())<-.55f;
    const bool Warning=B->MissedCheckpoint||WrongWay;
    const FLinearColor Accent=Warning?FLinearColor(1,.36,.12):FLinearColor(.3,.9,.88);
    DrawRect(FLinearColor(.008,.019,.029,.92),X,Y,340*Scale,84*Scale);
    DrawRect(Accent,X,Y+81*Scale,340*Scale,3*Scale);
    auto Text=[&](const FString& Value,float A,float D,int Size,bool Bold){
        FCanvasTextItem Label(FVector2D(X+A*Scale,Y+D*Scale),FText::FromString(Value),
            FCoreStyle::GetDefaultFontStyle(Bold?TEXT("Bold"):TEXT("Regular"),FMath::RoundToInt(Size*Scale)),Bold?Accent:FLinearColor::White);
        Label.Font=ItemTypeface;Canvas->DrawItem(Label);
    };
    FVector2D F(FMath::Sin(Angle),-FMath::Cos(Angle)),Right(-F.Y,F.X),Center(X+39*Scale,Y+42*Scale);
    auto Line=[&](FVector2D A,FVector2D D){DrawLine(A.X,A.Y,D.X,D.Y,Accent,4*Scale);};
    Line(Center-F*20*Scale,Center+F*22*Scale);Line(Center+F*22*Scale,Center+F*6*Scale+Right*14*Scale);Line(Center+F*22*Scale,Center+F*6*Scale-Right*14*Scale);
    Text(B->MissedCheckpoint?TEXT("PUNKT VERPASST"):WrongWay?TEXT("FALSCHE RICHTUNG"):TEXT("STRECKE FOLGEN"),75,14,16,true);
    Text(B->MissedCheckpoint?TEXT("R: zurück zum Kontrollpunkt"):WrongWay?TEXT("Wenden und dem Pfeil folgen"):TEXT("Blau links · Orange rechts"),75,47,12,false);
}
