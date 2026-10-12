#include "BenchyDisplay.h"
#include "BenchyWorld.h"
#include "BenchyPrintPath.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

ABenchyDisplay::ABenchyDisplay(){
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.bTickEvenWhenPaused=true;
    auto Root=CreateDefaultSubobject<USceneComponent>(TEXT("DisplayRoot"));SetRootComponent(Root);
    Boat=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FilamentPreview"));Boat->SetupAttachment(Root);
    Bed=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrintBed"));Bed->SetupAttachment(Root);
    Head=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrintHead"));Head->SetupAttachment(Root);
    Tip=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NozzleTip"));Tip->SetupAttachment(Head);
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("ShowroomCamera"));Camera->SetupAttachment(Root);
    Fill=CreateDefaultSubobject<UPointLightComponent>(TEXT("ShowroomFill"));Fill->SetupAttachment(Root);
    Glow=CreateDefaultSubobject<UPointLightComponent>(TEXT("PhosphorLight"));Glow->SetupAttachment(Root);
    for(auto C:{Boat.Get(),Bed.Get(),Head.Get(),Tip.Get()})C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Camera->SetFieldOfView(65);Camera->PostProcessSettings.bOverride_MotionBlurAmount=true;Camera->PostProcessSettings.MotionBlurAmount=0;
    Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;
    Camera->PostProcessSettings.AutoExposureMinBrightness=12.4;Camera->PostProcessSettings.AutoExposureMaxBrightness=12.4;
    Fill->SetIntensity(30000);Fill->SetAttenuationRadius(2200);Fill->SetCastShadows(false);Fill->SetRelativeLocation(FVector(-450,-600,550));
    Glow->SetLightColor(FLinearColor(.12,1,.3));Glow->SetAttenuationRadius(650);Glow->SetCastShadows(false);Glow->SetIntensity(0);
}
void ABenchyDisplay::BeginPlay(){
    Super::BeginPlay();auto M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Benchy/SM_Benchy.SM_Benchy"));
    auto Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Filament.M_Filament"));
    if(M&&Parent){auto B=M->GetBoundingBox();float Scale=900.f/B.GetSize().X;Height=B.GetSize().Z*Scale;
        Boat->SetStaticMesh(M);Boat->SetRelativeScale3D(FVector(Scale));BoatOrigin=FVector(-B.GetCenter().X*Scale,-B.GetCenter().Y*Scale,-B.Min.Z*Scale+12);Boat->SetRelativeLocation(BoatOrigin);
        Filament=UMaterialInstanceDynamic::Create(Parent,this);Filament->SetScalarParameterValue(TEXT("Printed"),1);Filament->SetScalarParameterValue(TEXT("LayerPitch"),1.1f/Scale);
        Filament->SetScalarParameterValue(TEXT("PrintMin"),B.Min.Z);Filament->SetScalarParameterValue(TEXT("PrintMax"),B.Max.Z);
        for(int K=0;K<Boat->GetNumMaterials();K++)Boat->SetMaterial(K,Filament);
    }
    auto Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));auto Cone=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cone.Cone"));
    Bed->SetStaticMesh(Cube);Bed->SetRelativeScale3D(FVector(11.4,7.3,.14));Bed->SetRelativeLocation(FVector(0,0,5));
    Head->SetStaticMesh(Cube);Head->SetRelativeScale3D(FVector(.7,.65,.55));
    Tip->SetStaticMesh(Cone);Tip->SetRelativeScale3D(FVector(.35,.35,.65));Tip->SetRelativeRotation(FRotator(180,0,0));Tip->SetRelativeLocation(FVector(0,0,-48));
    if(Parent){auto Dark=UMaterialInstanceDynamic::Create(Parent,this);Dark->SetVectorParameterValue(TEXT("FilamentColor"),FLinearColor(.025,.045,.055));
        Bed->SetMaterial(0,Dark);Head->SetMaterial(0,Dark);auto Brass=UMaterialInstanceDynamic::Create(Parent,this);Brass->SetVectorParameterValue(TEXT("FilamentColor"),FLinearColor(.8,.4,.065));Tip->SetMaterial(0,Brass);}
    FVector Target(0,0,Height*.45f);FVector At(-1050,-1270,Height*.45f+800);
    FVector Right=FRotationMatrix((Target-At).Rotation()).GetUnitAxis(EAxis::Y);
    Camera->SetRelativeLocation(At);Camera->SetRelativeRotation((Target-Right*620-At).Rotation());
}
void ABenchyDisplay::SelectFilament(int32 Index,bool Reprint){
    Color=FMath::Clamp(Index,0,5);const FLinearColor Colors[]={FLinearColor(.025,.031,.037),FLinearColor(1,.3,.025),FLinearColor(.01,.65,.83),FLinearColor(1,.78,.035),FLinearColor(.015,.019,.025),FLinearColor(.54,.78,.38)};
    if(Filament){Filament->SetVectorParameterValue(TEXT("FilamentColor"),Colors[Color]);Filament->SetScalarParameterValue(TEXT("Galaxy"),Color==4?1:0);Filament->SetScalarParameterValue(TEXT("Glow"),Color==5?1:0);}
    Glow->SetIntensity(Color==5?100:0);if(Reprint){PrintAge=0;Progress=0;}else Progress=1;
    if(Filament)Filament->SetScalarParameterValue(TEXT("PrintProgress"),Progress);
}
void ABenchyDisplay::Tick(float DT){
    Super::Tick(DT);PrintAge+=DT;Progress=FMath::Clamp(PrintAge/2.8f,0.f,1.f);
    if(Filament)Filament->SetScalarParameterValue(TEXT("PrintProgress"),Progress);
    bool Printing=Progress<1;Head->SetVisibility(Printing,true);
    // Keep the object stationary during deposition; rotate the complete print bed afterward.
    if(!Printing)Angle+=DT*7;
    FRotator R(0,Angle,0);Boat->SetRelativeRotation(R);Boat->SetRelativeLocation(R.RotateVector(BoatOrigin));Bed->SetRelativeRotation(R);
    if(Printing){auto XY=BenchyPrint::Nozzle(Progress,PrintAge,900);FVector P(XY.X,XY.Y,12+Progress*Height+44);Head->SetRelativeLocation(R.RotateVector(P));}
}
