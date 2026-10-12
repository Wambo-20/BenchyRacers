#include "BenchyGameMode.h"
#include "BenchyPawn.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Styling/CoreStyle.h"

void ABenchyHUD::DrawItemPanel(ABenchyPawn* B,float ServerTime){
    // Canvas needs a UFont cache handle even when a Slate composite font is supplied.
    if(!ItemTypeface){ItemTypeface=NewObject<UFont>(this);ItemTypeface->FontCacheType=EFontCacheType::Runtime;}
    const float Scale=FMath::Clamp(Canvas->SizeY/900.f,.75f,1.8f);
    const float Now=GetWorld()->GetTimeSeconds(),Age=Now-B->ItemEventTime;
    const bool Used=Age<1.15f&&((B->ItemEvent>=1&&B->ItemEvent<=3)||B->ItemEvent==6);
    const bool Fresh=B->Item!=0&&Age<1.25f&&B->ItemEvent==0;
    int Kind=B->Item;
    if(!Kind){if(B->ShieldUntil>ServerTime)Kind=3;else if(B->ItemBoostUntil>ServerTime)Kind=2;else if(Used)Kind=B->ItemEvent==6?4:B->ItemEvent;}
    const bool NozzleSlot=Kind==1||Kind==4;
    const float X=24*Scale,Y=Canvas->SizeY-(NozzleSlot?236:200)*Scale,Height=NozzleSlot?182:146;
    const bool Active=!B->Item&&((Kind==3&&B->ShieldUntil>ServerTime)||(Kind==2&&B->ItemBoostUntil>ServerTime));
    const FLinearColor Accent=Kind==1?FLinearColor(1,.56,.19):Kind==2?FLinearColor(1,.77,.17):Kind==3||Kind==4?FLinearColor(.2,.8,1):FLinearColor(.48,.63,.67);
    auto Rect=[&](float A,float D,float Width,float Height,FLinearColor Color){DrawRect(Color,X+A*Scale,Y+D*Scale,Width*Scale,Height*Scale);};
    auto Text=[&](const FString& Value,float A,float D,float Size,FLinearColor Color){
        FCanvasTextItem Label(FVector2D(X+A*Scale,Y+D*Scale),FText::FromString(Value),
            FCoreStyle::GetDefaultFontStyle(Size>1.8f?TEXT("Bold"):TEXT("Regular"),FMath::RoundToInt(Size*12*Scale)),Color);
        Label.Font=ItemTypeface;
        Canvas->DrawItem(Label);
    };
    Rect(4,5,440,Height,FLinearColor(0,0,0,.28));
    Rect(0,0,440,Height,FLinearColor(.008,.019,.029,.94));
    Rect(0,0,5,Height,Accent);Rect(5,0,435,2,FLinearColor(Accent.R,Accent.G,Accent.B,.3));
    if(Fresh||Used){const float Flash=FMath::Max(0.f,1-Age/1.15f);Rect(5,0,435,Height,FLinearColor(Accent.R,Accent.G,Accent.B,Flash*.17f));}
    Rect(20,25,88,88,FLinearColor(Accent.R*.16,Accent.G*.16,Accent.B*.16,.9));
    // Icons are drawn at native resolution at every display size.
    const float Pop=Fresh?1+.2f*FMath::Exp(-Age*5)*FMath::Sin(Age*18):1;
    const FVector2D Center(X+64*Scale,Y+69*Scale);const float IconScale=Scale*Pop;
    auto Line=[&](float AX,float AY,float BX,float BY,float Thick=3.f){DrawLine(Center.X+AX*IconScale,Center.Y+AY*IconScale,Center.X+BX*IconScale,Center.Y+BY*IconScale,Accent,Thick*Scale);};
    auto Loop=[&](std::initializer_list<FVector2D> Points){TArray<FVector2D> P;for(auto V:Points)P.Add(V);for(int I=0;I<P.Num();I++){const auto A=P[I],D=P[(I+1)%P.Num()];Line(A.X,A.Y,D.X,D.Y);}};
    if(Kind==1||Kind==4){ // Nozzle and target reticle distinguish the guided variant.
        Loop({{-19,-25},{16,-25},{22,-15},{22,4},{9,24},{-9,24},{-22,4},{-22,-15}});
        Line(-20,-9,20,-9);Line(-15,-18,15,-18);Line(-5,24,-5,29);Line(5,24,5,29);Line(-5,29,5,29);
        Line(28,-5,35,-14,2);Line(-28,-5,-35,-14,2);
        if(Kind==4){for(int I=0;I<40;I++){float A=I*PI/20,D=(I+1)*PI/20;Line(FMath::Cos(A)*35,FMath::Sin(A)*35,FMath::Cos(D)*35,FMath::Sin(D)*35,2);}Line(0,-39,0,-29,2);Line(0,29,0,39,2);Line(-39,0,-29,0,2);Line(29,0,39,0,2);}
    }else if(Kind==2){
        Loop({{3,-31},{-24,5},{-4,5},{-9,31},{25,-8},{5,-8},{13,-31}});
        Line(-28,-16,-37,-16,2);Line(-30,16,-39,16,2);Line(27,18,35,18,2);
    }else if(Kind==3){
        Loop({{0,-31},{26,-21},{24,7},{16,23},{0,33},{-16,23},{-24,7},{-26,-21}});
        Line(-12,0,-2,11,4);Line(-2,11,15,-10,4);
    }else{ // Empty slot: a filament spool signals what to collect.
        Loop({{-24,-22},{24,-22},{24,-13},{-24,-13}});Loop({{-24,13},{24,13},{24,22},{-24,22}});
        Line(-16,-13,-16,13);Line(16,-13,16,13);for(int I=-8;I<=8;I+=8)Line(I,-12,I,12,2);
    }
    if(Active||Fresh){float Angle=Now*(Active?2:6);for(int I=0;I<22;I++){
        float A=Angle+I*.075f,D=A+.06f;Line(FMath::Cos(A)*39,FMath::Sin(A)*39,FMath::Cos(D)*39,FMath::Sin(D)*39,2);
    }}
    FString Heading=Fresh?TEXT("NEUES ITEM"):B->Item?TEXT("BEREIT"):Active?TEXT("AKTIV"):Used?TEXT("ABGEFEUERT"):TEXT("ITEM-SLOT");
    Text(Heading,126,16,1.10f,Accent);
    static const TCHAR* Names[]={TEXT("Kein Item"),TEXT("Standard Nozzle"),TEXT("Turbo"),TEXT("Schutzschild"),TEXT("Lenknozzle")};
    Text(Names[Kind],124,42,Kind==1?1.95f:2.15f,FLinearColor::White);
    if(B->Item){
        Rect(126,94,118,30,Accent);Text(TEXT("LEERTASTE"),137,100,1.12f,FLinearColor(.005,.015,.025));
        Text(TEXT("/ A  benutzen"),253,100,1.1f,FLinearColor(.79,.85,.88));
    }else if(Active){
        const float Remaining=Kind==3?B->ShieldUntil-ServerTime:B->ItemBoostUntil-ServerTime;
        Text(FString::Printf(TEXT("%.1f s verbleibend"),Remaining),126,90,1.25f,Accent);
        Rect(126,120,286,5,FLinearColor(.15,.23,.27));Rect(126,120,286*FMath::Clamp(Remaining/(Kind==3?8.f:4.f),0.f,1.f),5,Accent);
    }else Text(Used?TEXT("Nozzle unterwegs!"):TEXT("Filamentrolle einsammeln"),126,94,1.13f,FLinearColor(.72,.8,.82));
    if(NozzleSlot){
        const bool Backward=B->Item?B->WantsBackwardShot():B->LastShotBackward;
        FString Detail=Backward?TEXT("Schuss nach hinten"):TEXT("Schuss nach vorne");
        if(Kind==4){auto Target=B->Item?B->NozzleTarget(Backward):nullptr;const FString Name=B->Item?(Target?Target->RacerName:TEXT("Kein Ziel")):B->LastShotTarget;
            Detail=TEXT("Ziel: ")+Name.Left(20);}
        Text(Detail,126,131,1.05f,Accent);Text(TEXT("S + LEER / Stick zurück: rückwärts"),20,156,1.02f,FLinearColor(.77,.83,.85));
    }
    if(!B->ItemNoticeText.IsEmpty()&&Now-B->ItemNoticeTime<3.5f){Rect(0,-34,440,30,FLinearColor(.05,.025,.009,.96));Text(B->ItemNoticeText,12,-29,1.02f,FLinearColor(1,.76,.3));}
    // An active effect stays visible even when another item occupies the slot.
    if((B->ItemNoticeText.IsEmpty()||Now-B->ItemNoticeTime>=3.5f)&&((B->Item&&(B->ShieldUntil>ServerTime||B->ItemBoostUntil>ServerTime))||(B->ShieldUntil>ServerTime&&B->ItemBoostUntil>ServerTime))){
        Rect(0,-31,440,27,FLinearColor(.008,.019,.029,.9));
        FString Status;
        if(B->ShieldUntil>ServerTime)Status+=FString::Printf(TEXT("Schild %.1f s   "),B->ShieldUntil-ServerTime);
        if(B->ItemBoostUntil>ServerTime)Status+=FString::Printf(TEXT("Turbo %.1f s"),B->ItemBoostUntil-ServerTime);
        Text(Status,16,-26,1.15f,FLinearColor(.45,.88,1));
    }
}
