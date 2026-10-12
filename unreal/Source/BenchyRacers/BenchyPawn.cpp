#include "BenchyPawn.h"
#include "BenchyItemFX.h"
#include "BenchyPrintPath.h"
#include "BenchyWorld.h"
#include "WaveMath.h"
#include "RaceRules.h"
#include "CourseNavigation.h"
#include "BenchyRace.h"
#include "BenchyMenu.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "InputCoreTypes.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Engine/GameViewportClient.h"

ABenchyPawn::ABenchyPawn() {
    bReplicates=true; bAlwaysRelevant=true; SetReplicateMovement(false); SetNetUpdateFrequency(30);
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PrePhysics;
    Hull=CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicalHull"));
    SetRootComponent(Hull);
    Hull->SetBoxExtent(FVector(170,65,75));
    Hull->SetCollisionProfileName(TEXT("PhysicsActor"));
    Hull->SetSimulatePhysics(true);
    Hull->SetLinearDamping(0.12f);
    Hull->SetAngularDamping(2.0f);
    Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OriginalBenchy"));
    Visual->SetupAttachment(Hull);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Arm=CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseCamera"));
    Arm->SetupAttachment(Hull);
    Arm->TargetArmLength=850;
    Arm->SetRelativeLocation(FVector(0,0,130));
    Arm->SetRelativeRotation(FRotator(-17,8,0));
    Arm->bInheritPitch=false;
    Arm->bInheritRoll=false;
    Arm->bEnableCameraLag=true;
    Arm->CameraLagSpeed=7;
    Arm->bEnableCameraRotationLag=true;
    Arm->CameraRotationLagSpeed=9;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Arm,USpringArmComponent::SocketName);
    Camera->SetFieldOfView(84);
    Camera->PostProcessSettings.bOverride_MotionBlurAmount=true;
    Camera->PostProcessSettings.MotionBlurAmount=0;
    Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;
    Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;
    Camera->PostProcessSettings.AutoExposureMinBrightness=10;
    Camera->PostProcessSettings.AutoExposureMaxBrightness=14;
    Wake=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WorldSpaceWake"));
    Wake->SetupAttachment(Hull);
    Wake->SetAbsolute(true,true,true);
    Wake->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Wake->SetCastShadow(false);
    ItemFX=CreateDefaultSubobject<UBenchyItemFX>(TEXT("ItemEffects"));
    ItemFX->SetupAttachment(Hull);
    GlowLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowFilamentLight"));GlowLight->SetupAttachment(Hull);
    GlowLight->SetRelativeLocation(FVector(0,0,45));GlowLight->SetAttenuationRadius(480);GlowLight->SetCastShadows(false);
    GlowLight->SetLightColor(FLinearColor(.16,1,.35));GlowLight->SetIntensity(0);
    PrintHead=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartingNozzle"));PrintHead->SetupAttachment(Hull);PrintHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);PrintHead->SetVisibility(false);
}

void ABenchyPawn::BeginPlay() {
    Super::BeginPlay();
    if(!HasAuthority()) Hull->SetSimulatePhysics(false);
    Hull->SetMassOverrideInKg(NAME_None,320,true);
    // Ballast below the waterline gives the hull a restoring roll moment.
    Hull->SetCenterOfMass(FVector(0,0,-55));
    UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Benchy/SM_Benchy.SM_Benchy"));
    if (Mesh) {
        Visual->SetStaticMesh(Mesh);
        // FBX has been converted to centimetres with its bow facing +X.
        const FBox Bounds=Mesh->GetBoundingBox();
        const float Scale=360.0f/FMath::Max(Bounds.GetSize().X,1.0f);
        Visual->SetRelativeScale3D(FVector(Scale));
        Visual->SetRelativeLocation(FVector(-Bounds.GetCenter().X*Scale,
            -Bounds.GetCenter().Y*Scale,-85-Bounds.Min.Z*Scale));
    } else {
        UE_LOG(LogTemp,Error,TEXT("Benchy asset missing. Run Prepare-Unreal.ps1 first."));
    }
    UMaterialInterface* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Filament.M_Filament"));
    if (M) {
        Filament=UMaterialInstanceDynamic::Create(M,this);
        Filament->SetScalarParameterValue(TEXT("Printed"),1);
        Filament->SetScalarParameterValue(TEXT("LayerPitch"),1.1f/FMath::Max(.001f,float(Visual->GetRelativeScale3D().X)));
        if(Mesh){Filament->SetScalarParameterValue(TEXT("PrintMin"),Mesh->GetBoundingBox().Min.Z);Filament->SetScalarParameterValue(TEXT("PrintMax"),Mesh->GetBoundingBox().Max.Z);}
        for(int32 I=0;I<Visual->GetNumMaterials();I++) Visual->SetMaterial(I,Filament);
        OnRep_Color();
    }
    Wake->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Wake.M_Wake")));
    PrintHead->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cone.Cone")));PrintHead->SetRelativeScale3D(FVector(.35,.35,.5));PrintHead->SetRelativeRotation(FRotator(180,0,0));
    if(M){auto Brass=UMaterialInstanceDynamic::Create(M,this);Brass->SetVectorParameterValue(TEXT("FilamentColor"),FLinearColor(.8,.4,.06));PrintHead->SetMaterial(0,Brass);}
    ResetPosition=GetActorLocation(); ResetRotation=GetActorRotation();
    Audit=FParse::Param(FCommandLine::Get(),TEXT("BenchyAudit"));
    AuditStart=GetActorLocation();
    AuditLastPosition=AuditStart;
    AuditWaterfall=FParse::Param(FCommandLine::Get(),TEXT("BenchyWaterfallAudit"));
    AuditScenery=FParse::Param(FCommandLine::Get(),TEXT("BenchySceneryAudit"));
}

void ABenchyPawn::SetupPlayerInputComponent(UInputComponent* Input) {
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis("Throttle",this,&ABenchyPawn::SetThrottle);
    Input->BindAxis("Steer",this,&ABenchyPawn::SetSteering);
    Input->BindAction("Boost",IE_Pressed,this,&ABenchyPawn::StartBoost);
    Input->BindAction("Boost",IE_Released,this,&ABenchyPawn::StopBoost);
    Input->BindAction("Item",IE_Pressed,this,&ABenchyPawn::UseItem);
    Input->BindAction("Reset",IE_Pressed,this,&ABenchyPawn::ResetBoat);
    Input->BindAction("Color",IE_Pressed,this,&ABenchyPawn::NextColor);
    Input->BindAction("Quality",IE_Pressed,this,&ABenchyPawn::NextQuality);
    Input->BindAction("Fullscreen",IE_Pressed,this,&ABenchyPawn::ToggleFullscreen);
    Input->BindAction("Quit",IE_Pressed,this,&ABenchyPawn::Quit);
}

void ABenchyPawn::Tick(float DT) {
    Super::Tick(DT);
    const auto SharedClock=GetWorld()->GetGameState<ABenchyRaceState>();
    const float Time=SharedClock?SharedClock->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();
    const float Print=SharedClock&&SharedClock->Phase==1?FMath::Clamp((SharedClock->Clock()+5.f)/2.8f,0.f,1.f):1.f;
    if(Filament)Filament->SetScalarParameterValue(TEXT("PrintProgress"),Print);
    if(PrintHead){PrintHead->SetVisibility(Print<1);if(Print<1&&Visual->GetStaticMesh()){float Age=SharedClock->Clock()+5;float H=Visual->GetStaticMesh()->GetBoundingBox().GetSize().Z*Visual->GetRelativeScale3D().Z;
        auto XY=BenchyPrint::Nozzle(Print,Age,360);PrintHead->SetRelativeLocation(FVector(XY.X,XY.Y,-85+H*Print+25));}}
    if(IsLocallyControlled())Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView,Time<ItemBoostUntil?94.f:84.f,DT,3.f));
    if(Audit) TickAudit(Time);
    if(!HasAuthority()) {
        if(IsLocallyControlled()) {if(auto PC=Cast<ABenchyController>(Controller))if(PC->MenuVisible()){Throttle=0;Steering=0;Boost=false;}InputClock+=DT;if(InputClock>.05f){ServerControls(Throttle,Steering,Boost);InputClock=0;}}
        if(!NetworkPosition.IsNearlyZero()) {
            const bool Snap=!NetworkInitialized || FVector::DistSquared(GetActorLocation(),NetworkPosition)>25000000;
            SetActorLocationAndRotation(Snap?NetworkPosition:FMath::VInterpTo(GetActorLocation(),NetworkPosition,DT,18),
                Snap?NetworkRotation:FMath::RInterpTo(GetActorRotation(),NetworkRotation,DT,18));NetworkInitialized=true;
        }
        UpdateWake(DT,Time); return;
    }
    auto Race=GetWorld()->GetGameState<ABenchyRaceState>();
    const bool Racing=Audit || (Race && Race->Phase==2 && !Finished);
    const bool Cruising=Finished&&Race&&(Race->Phase==2||Race->Phase==3);
    if(Racing && (IsBot || (IsLocallyControlled() && FParse::Param(FCommandLine::Get(),TEXT("BenchyRaceAudit"))))) DriveAI();
    if(Cruising)DriveFinished(DT);
    if(IsLocallyControlled()&&FParse::Param(FCommandLine::Get(),TEXT("BenchyItemAudit")))Throttle=(Time>14&&Time<18)?.7f:0;
    if(!Cruising&&!IsBot && !IsLocallyControlled() && Time-LastInputTime>.5f){Throttle=0;Steering=0;Boost=false;}
    if(!Cruising)if(auto PC=Cast<ABenchyController>(Controller))if(PC->IsLocalController() && PC->MenuVisible()){Throttle=0;Steering=0;Boost=false;}
    if((!Racing&&!Cruising) || Time<StunUntil){Throttle=0;Steering=0;Boost=false;}
    const float Mass=Hull->GetMass();
    const FTransform Transform=Hull->GetComponentTransform();
    for(float X : {-130.f,130.f}) for(float Y : {-55.f,55.f}) {
        FVector Point=Transform.TransformPosition(FVector(X,Y,-85));
        float Fraction=BenchySea::BuoyancyFraction(Point.Z,ABenchyWorld::WaterHeight(Point,Time));
        float VerticalSpeed=Hull->GetPhysicsLinearVelocityAtPoint(Point).Z;
        float Force=FMath::Max(0.f,Mass*980.f*.25f*Fraction-Mass*.25f*VerticalSpeed*3.4f);
        FVector Direction=(FVector::UpVector*.92f+ABenchyWorld::WaterNormal(Point,Time)*.08f).GetSafeNormal();
        Hull->AddForceAtLocation(Direction*Force,Point);
    }
    const FVector Forward=GetActorForwardVector();
    const FVector Side=GetActorRightVector();
    const FVector V=Hull->GetPhysicsLinearVelocity();
    const float Along=FVector::DotProduct(V,Forward);
    const float Across=FVector::DotProduct(V,Side);
    const float MaxSpeed=(Boost || Time<ItemBoostUntil ? 4600.f : 3100.f)*BenchyRules::CoinSpeed(Coins);
    float Acceleration=(Throttle>=0 ? Throttle*1350.f : Throttle*2200.f)*BenchyRules::CoinSpeed(Coins);
    if((!Racing&&!Cruising) || Time<StunUntil) Acceleration=-Along*2.5f;
    if(Along>MaxSpeed && Acceleration>0) Acceleration=0;
    if(Along<-800 && Acceleration<0) Acceleration=0;
    Hull->AddForce((Forward*(Acceleration-Along*.10f)-Side*Across*3.2f)*Mass);
    // Water rudder acts at the stern, producing yaw through physical torque.
    const float Turning=FMath::Clamp(FMath::Abs(Along)/1200.f,.15f,1.f);
    Hull->AddForceAtLocation(-Side*Steering*Mass*340.f*Turning,
        Transform.TransformPosition(FVector(-145,0,-40)));
    if(Racing) {
        RaceTime=Race?FMath::Max(0.f,Race->Clock()):RaceTime+DT;
        const FVector Position=GetActorLocation();
        const auto Crossing=CoursePositionInitialized?BenchyCourse::CrossCheckpoint(ABenchyWorld::ActiveTrack,NextGate,
            {PreviousCoursePosition.X,PreviousCoursePosition.Y},{Position.X,Position.Y}):BenchyCourse::Crossing::None;
        if(Crossing!=BenchyCourse::Crossing::Valid&&!MissedCheckpoint&&BenchyCourse::BeyondPendingGate(ABenchyWorld::ActiveTrack,Passed,{Position.X,Position.Y})){
            MissedCheckpoint=true;ForceNetUpdate();
        }
        if(Crossing==BenchyCourse::Crossing::Valid && FMath::Abs(Position.Z-ABenchyWorld::WaterHeight(Position,Time))<450) {
            MissedCheckpoint=false;
            Passed++; NextGate=BenchyRules::Next(Passed); Lap=BenchyRules::Lap(Passed);
            ForceNetUpdate();
            if(BenchyRules::Finished(Passed)) {
                FinishRace();
            }
        }
        const bool Stranded=(FMath::Abs(Throttle)>.4f && SpeedKmh()<3 && ABenchyWorld::GroundHeight(FVector2D(GetActorLocation()))>-40) || GetActorUpVector().Z<.35f;
        StuckTime=Stranded?StuckTime+DT:0;
        if(StuckTime>4)ResetBoat();
    }
    PreviousCoursePosition=GetActorLocation();CoursePositionInitialized=true;
    PoseClock+=DT;if(PoseClock>.033f){NetworkPosition=GetActorLocation();NetworkRotation=GetActorRotation();NetworkSpeed=SpeedKmh();PoseClock=0;}

    UpdateWake(DT,Time);
    if(GetActorLocation().Z<-2000) ResetBoat();
}

void ABenchyPawn::UpdateWake(float DT,float Time) {
    Wake->SetWorldTransform(FTransform::Identity);
    for(auto& P:History) P.Age+=DT;
    while(History.Num()>0 && History.Last().Age>1.8f) History.Pop();
    WakeClock+=DT;
    FVector Velocity=HasAuthority()?Hull->GetPhysicsLinearVelocity():GetActorForwardVector()*NetworkSpeed/.036f;
    float Speed=Velocity.Size2D();
    // Sample the actual stern in world space. Ageing samples never rotate with
    // the boat, so turns bend the wake rather than dragging it sideways.
    FVector Stern=Hull->GetComponentTransform().TransformPosition(FVector(-175,0,-70));
    if(Speed>400 && WakeClock>.045f) {
        WakeClock=0;
        if(History.Num()>0 && FVector::Dist2D(History[0].Position,Stern)>800) History.Reset();
        FBenchyWakePoint P; P.Position=Stern; P.Side=GetActorRightVector();
        P.Side.Z=0; P.Side.Normalize(); P.Power=FMath::Clamp(Speed/3500.f,0.f,1.f);
        P.Speed=Speed;
        History.Insert(P,0);
    }
    if(History.Num()<3) { Wake->ClearAllMeshSections(); return; }
    TArray<FVector> Vertices,Normals;
    TArray<int32> Indices;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    for(int32 I=0;I<History.Num()-1;I++) for(int32 Ribbon=-1;Ribbon<=1;Ribbon++) {
        int32 Start=Vertices.Num();
        for(int32 J=0;J<2;J++) for(int32 Edge=-1;Edge<=1;Edge+=2) {
            const FBenchyWakePoint& P=History[I+J];
            float Spread=65+P.Age*P.Speed*.12f;
            float Width=Ribbon==0 ? 40+P.Age*48 : 14+P.Age*15;
            FVector Pos=P.Position+P.Side*(Ribbon*Spread+Edge*Width);
            Pos+=P.Side*(FMath::Sin(P.Position.X*.009f+P.Position.Y*.007f+Time*.8f)*P.Age*15);
            Pos.Z=ABenchyWorld::WaterHeight(Pos,Time)+8;
            Vertices.Add(Pos); Normals.Add(ABenchyWorld::WaterNormal(Pos,Time));
            UVs.Add(FVector2D((Edge+1)*.5f,P.Age/1.8f));
            float Alpha=FMath::Pow(FMath::Max(0.f,1-P.Age/1.8f),2.2f)*P.Power*(Ribbon==0?.48f:.28f);
            Colors.Add(FLinearColor(1,1,1,Alpha)); Tangents.Add(FProcMeshTangent(1,0,0));
        }
        for(int32 K:{0,2,1,1,2,3}) Indices.Add(Start+K);
    }
    Wake->CreateMeshSection_LinearColor(0,Vertices,Indices,Normals,UVs,Colors,Tangents,false);
}

float ABenchyPawn::SpeedKmh() const { return HasAuthority()?Hull->GetPhysicsLinearVelocity().Size2D()*.036f:NetworkSpeed; }
void ABenchyPawn::ResetBoat() {
    if(Finished)return;
    if(!HasAuthority()){ServerReset();return;}
    const float T=GetWorld()->GetTimeSeconds();if(T<NextResetTime)return;NextResetTime=T+2;
    if(Audit) AuditResets++;
    History.Reset(); Wake->ClearAllMeshSections(); StuckTime=0;Throttle=0;Steering=0;Boost=false;
    const float Index=BenchyRules::Last(Passed)*20.f;
    ResetPosition=ABenchyWorld::RoutePoint(Index);ResetPosition.Z=ABenchyWorld::WaterHeight(ResetPosition,T)+150;
    ResetRotation=(ABenchyWorld::RoutePoint(Index+2)-ResetPosition).Rotation();ResetRotation.Pitch=0;ResetRotation.Roll=0;
    // No momentum or roll survives a recovery. The next checkpoint remains unchanged.
    Hull->SetPhysicsLinearVelocity(FVector::ZeroVector);Hull->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    SetActorLocationAndRotation(ResetPosition,ResetRotation,false,nullptr,ETeleportType::TeleportPhysics);
    PreviousCoursePosition=ResetPosition;CoursePositionInitialized=true;MissedCheckpoint=false;
    RespawnUntil=T+2;NetworkPosition=ResetPosition;NetworkRotation=ResetRotation;NetworkSpeed=0;ForceNetUpdate();
}
void ABenchyPawn::ServerReset_Implementation(){ResetBoat();}
void ABenchyPawn::ServerControls_Implementation(float Gas,float Turn,bool Boosting){
    if(Finished||!FMath::IsFinite(Gas)||!FMath::IsFinite(Turn))return;
    Throttle=FMath::Clamp(Gas,-1.f,1.f);Steering=FMath::Clamp(Turn,-1.f,1.f);Boost=Boosting;LastInputTime=GetWorld()->GetTimeSeconds();
}
void ABenchyPawn::BeginRace(int32 Slot){
    RaceEntryId=Slot;PostFinishCursor=0;Hull->SetCollisionResponseToChannel(ECC_PhysicsBody,ECR_Block);
    ItemEventTime=-100;ItemEvent=0;ItemNoticeText.Empty();LastShotTarget.Empty();LastShotBackward=false;
    Passed=0;NextGate=1;Lap=1;Coins=0;Item=0;RaceTime=0;Finished=false;FinishPlace=0;ShieldUntil=0;StunUntil=0;ItemBoostUntil=0;NextResetTime=0;
    ResetBoat();FVector P=GetActorLocation();FVector F=GetActorForwardVector();P+=GetActorRightVector()*((Slot%2)*2-1)*320-F*(Slot/2)*470;
    SetActorLocation(P,false,nullptr,ETeleportType::TeleportPhysics);NetworkPosition=P;
}
bool ABenchyPawn::Collect(int32 Kind){
    if(!HasAuthority()||Finished)return false;
    if(Kind==0){if(Coins>=BenchyRules::MaxCoins)return false;Coins++;PlayCue(0);}
    else {if(Item!=0)return false;Item=FMath::RandRange(1,4);PlayCue(1);ItemFeedback(0);}ForceNetUpdate();return true;
}
void ABenchyPawn::HitByItem(){
    const float T=GetWorld()->GetTimeSeconds();if(!HasAuthority()||Finished||T<RespawnUntil||T<StunUntil)return;
    if(T<ShieldUntil){ShieldUntil=0;ItemFeedback(5);ForceNetUpdate();return;}Coins=0;StunUntil=T+2.2f;PlayCue(3);ItemFeedback(4);ForceNetUpdate();
    Hull->SetPhysicsLinearVelocity(Hull->GetPhysicsLinearVelocity()*.18f);Hull->AddAngularImpulseInDegrees(FVector(0,0,70),NAME_None,true);
}
bool ABenchyPawn::WantsBackwardShot() const {
    if(auto PC=Cast<APlayerController>(Controller))return PC->IsInputKeyDown(EKeys::S)||PC->IsInputKeyDown(EKeys::Down)||
        PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY)<-.35f||PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis)>.35f||Throttle<-.35f;
    return Throttle<-.35f;
}
ABenchyPawn* ABenchyPawn::NozzleTarget(bool Backward) const {
    TArray<ABenchyPawn*> Order;
    for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(!It->Finished&&!It->IsActorBeingDestroyed())Order.Add(*It);
    Order.Sort([](const ABenchyPawn& A,const ABenchyPawn& B){float AP=A.Progress(),BP=B.Progress();return AP==BP?A.GetUniqueID()<B.GetUniqueID():AP>BP;});
    const int Place=Order.IndexOfByKey(this),Index=Place+(Backward?1:-1);
    return Place!=INDEX_NONE&&Order.IsValidIndex(Index)?Order[Index]:nullptr;
}
void ABenchyPawn::UseItem(){ServerUseItem(WantsBackwardShot());}
void ABenchyPawn::ServerUseItem_Implementation(bool Backward){
    auto S=GetWorld()->GetGameState<ABenchyRaceState>();float T=GetWorld()->GetTimeSeconds();if(!S||S->Phase!=2||Finished||Item==0||T<StunUntil)return;
    if(Item==1||Item==4){
        ABenchyPawn* Locked=Item==4?NozzleTarget(Backward):nullptr;
        if(Item==4&&!Locked){ItemNotice(Backward?TEXT("Kein Ziel hinten. Item bleibt erhalten."):TEXT("Vorne kein Ziel. S + Leertaste: rückwärts."));return;}
        FVector Direction=FRotator(0,GetActorRotation().Yaw,0).Vector()*(Backward?-1:1);
        FVector At=GetActorLocation()+Direction*250;At.Z=ABenchyWorld::WaterHeight(At,T)+80;
        const FTransform Pose(Direction.Rotation(),At);
        auto P=GetWorld()->SpawnActorDeferred<ABenchyProjectile>(ABenchyProjectile::StaticClass(),Pose,this,this,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(!P)return;P->Guided=Item==4;P->Target=Locked;P->Backward=Backward;P->FinishSpawning(Pose);
        LastShotBackward=Backward;LastShotTarget=Locked?Locked->RacerName:TEXT("");
    }
    if(Item==2)ItemBoostUntil=T+4;
    if(Item==3)ShieldUntil=T+8;
    PlayCue(2);ItemFeedback(Item==4?6:Item);Item=0;ItemNoticeText.Empty();ForceNetUpdate();
}
void ABenchyPawn::ItemNotice_Implementation(const FString& Message){ItemNoticeText=Message;ItemNoticeTime=GetWorld()->GetTimeSeconds();}
void ABenchyPawn::ItemFeedback_Implementation(int32 Effect){
    ItemNoticeText.Empty();
    ItemEvent=Effect;ItemEventTime=GetWorld()->GetTimeSeconds();
    ABenchyItemBurst::Spawn(GetWorld(),GetActorLocation(),Effect);
}
FString ABenchyPawn::ItemName() const {static const TCHAR* N[]={TEXT("Kein Item"),TEXT("Standard Nozzle"),TEXT("Turbo"),TEXT("Schutzschild"),TEXT("Lenknozzle")};return N[FMath::Clamp(Item,0,4)];}
void ABenchyPawn::AuditPose(FVector Position,FRotator Rotation,int32 ProgressPassed){
    if(!HasAuthority())return;SetActorLocationAndRotation(Position,Rotation,false,nullptr,ETeleportType::TeleportPhysics);
    Hull->SetPhysicsLinearVelocity(FVector::ZeroVector);Hull->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    Passed=ProgressPassed;NextGate=BenchyRules::Next(Passed);Lap=BenchyRules::Lap(Passed);Finished=false;MissedCheckpoint=false;
    PreviousCoursePosition=Position;CoursePositionInitialized=true;NetworkPosition=Position;NetworkRotation=Rotation;ForceNetUpdate();
}
float ABenchyPawn::Progress() const {
    if(Finished)return 1000-FinishPlace;
    const auto P=GetActorLocation();
    return Passed+float(BenchyCourse::ProjectSector(ABenchyWorld::ActiveTrack,Passed,{P.X,P.Y}).Fraction);
}
FVector ABenchyPawn::NavigationTarget() const {
    if(MissedCheckpoint)return ABenchyWorld::RoutePoint(NextGate*20.f);
    const auto P=GetActorLocation();const auto Projection=BenchyCourse::ProjectSector(ABenchyWorld::ActiveTrack,Passed,{P.X,P.Y});
    return ABenchyWorld::RoutePoint(FMath::Min(float(Projection.Index)+5.f,float((Passed%16+1)*20)));
}
void ABenchyPawn::DriveAI(){
    if(MissedCheckpoint){ResetBoat();return;}
    int32 Nearest=0;double Best=1e30;for(int I=0;I<320;I++){double D=FVector::DistSquared2D(GetActorLocation(),ABenchyWorld::RoutePoint(I));if(D<Best){Best=D;Nearest=I;}}
    const FVector Ahead=ABenchyWorld::RoutePoint(Nearest+4),Tangent=(ABenchyWorld::RoutePoint(Nearest+5)-Ahead).GetSafeNormal();
    const uint32 Personality=GetTypeHash(RacerName);const float Lane=IsBot?(int32(Personality%5)-2)*190.f:0.f;
    const FVector Target=Ahead+FVector(-Tangent.Y,Tangent.X,0)*Lane-GetActorLocation();
    Steering=FMath::Clamp(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw,Target.Rotation().Yaw)/28.f,-1.f,1.f);
    const FVector A=ABenchyWorld::RoutePoint(Nearest+4)-ABenchyWorld::RoutePoint(Nearest),B=ABenchyWorld::RoutePoint(Nearest+12)-ABenchyWorld::RoutePoint(Nearest+4);
    float Bend=FMath::Abs(FMath::FindDeltaAngleDegrees(A.Rotation().Yaw,B.Rotation().Yaw));
    float Desired=FMath::Lerp(2700.f,950.f,FMath::Clamp(Bend/75.f,0.f,1.f))*(IsBot?.94f+(Personality%12)*.01f:1.f);
    Throttle=FMath::Clamp((Desired-Hull->GetPhysicsLinearVelocity().Size2D())/750.f+.17f,-.65f,.9f);
    if(Best>6500*6500)ResetBoat();
    if(Item && GetWorld()->GetTimeSeconds()>BotItemTime){ServerUseItem_Implementation(Item==4&&!NozzleTarget(false));BotItemTime=GetWorld()->GetTimeSeconds()+FMath::FRandRange(4.f,9.f);}
}
void ABenchyPawn::FinishRace(){
    if(!HasAuthority()||Finished)return;
    Finished=true;PostFinishCursor=Passed*20.f;MissedCheckpoint=false;
    Throttle=0;Steering=0;Boost=false;Item=0;ShieldUntil=0;StunUntil=0;ItemBoostUntil=0;
    Hull->SetCollisionResponseToChannel(ECC_PhysicsBody,ECR_Ignore);
    auto S=GetWorld()->GetGameState<ABenchyRaceState>();FinishPlace=S?S->RecordFinish(this):0;
    if(!FinishPlace){FinishPlace=1;for(TActorIterator<ABenchyPawn> It(GetWorld());It;++It)if(*It!=this&&It->Finished)FinishPlace++;}
    PlayCue(5);ForceNetUpdate();
}
void ABenchyPawn::DriveFinished(float DT){
    // Search only beside the current segment, so nearby channels cannot steal the boat.
    const FVector P=GetActorLocation();float Best=1e30f,Cursor=PostFinishCursor;
    for(float I=PostFinishCursor-2;I<=PostFinishCursor+7;I+=.25f){float D=FVector::DistSquared2D(P,ABenchyWorld::RoutePoint(I));if(D<Best){Best=D;Cursor=I;}}
    PostFinishCursor=FMath::Max(PostFinishCursor,Cursor);
    const FVector A=ABenchyWorld::RoutePoint(PostFinishCursor+4),D=(ABenchyWorld::RoutePoint(PostFinishCursor+5)-A).GetSafeNormal();
    FVector Target=A+FVector(-D.Y,D.X,0)*((RaceEntryId%3)-1)*140-P;
    Steering=FMath::Clamp(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw,Target.Rotation().Yaw)/28.f,-1.f,1.f);
    float Bend=FMath::Abs(FMath::FindDeltaAngleDegrees(D.Rotation().Yaw,(ABenchyWorld::RoutePoint(PostFinishCursor+12)-A).Rotation().Yaw));
    float Desired=FMath::Lerp(1850.f,950.f,FMath::Clamp(Bend/65.f,0.f,1.f));
    Throttle=FMath::Clamp((Desired-Hull->GetPhysicsLinearVelocity().Size2D())/750.f+.17f,-.65f,.85f);Boost=false;
    bool Stranded=(Best>1100*1100&&SpeedKmh()<5)||GetActorUpVector().Z<.35f||P.Z<-800;
    StuckTime=Stranded?StuckTime+DT:0;
    if(Best>2600*2600||StuckTime>2){
        FVector At=ABenchyWorld::RoutePoint(PostFinishCursor);At.Z=ABenchyWorld::WaterHeight(At,GetWorld()->GetTimeSeconds())+120;
        FRotator R=(ABenchyWorld::RoutePoint(PostFinishCursor+1)-At).Rotation();R.Pitch=0;R.Roll=0;
        Hull->SetPhysicsLinearVelocity(FVector::ZeroVector);Hull->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        SetActorLocationAndRotation(At,R,false,nullptr,ETeleportType::TeleportPhysics);History.Reset();StuckTime=0;
        NetworkPosition=At;NetworkRotation=R;ForceNetUpdate();
    }
}
void ABenchyPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABenchyPawn,NextGate);DOREPLIFETIME(ABenchyPawn,Lap);DOREPLIFETIME(ABenchyPawn,RaceTime);DOREPLIFETIME(ABenchyPawn,Finished);
    DOREPLIFETIME(ABenchyPawn,Passed);DOREPLIFETIME(ABenchyPawn,Coins);DOREPLIFETIME(ABenchyPawn,Item);DOREPLIFETIME(ABenchyPawn,FinishPlace);
    DOREPLIFETIME(ABenchyPawn,RaceEntryId);
    DOREPLIFETIME(ABenchyPawn,MissedCheckpoint);
    DOREPLIFETIME(ABenchyPawn,LastShotBackward);DOREPLIFETIME(ABenchyPawn,LastShotTarget);
    DOREPLIFETIME(ABenchyPawn,RacerName);DOREPLIFETIME(ABenchyPawn,IsBot);DOREPLIFETIME(ABenchyPawn,ShieldUntil);DOREPLIFETIME(ABenchyPawn,StunUntil);
    DOREPLIFETIME(ABenchyPawn,RespawnUntil);DOREPLIFETIME(ABenchyPawn,ItemBoostUntil);DOREPLIFETIME(ABenchyPawn,NetworkSpeed);
    DOREPLIFETIME(ABenchyPawn,NetworkPosition);DOREPLIFETIME(ABenchyPawn,NetworkRotation);DOREPLIFETIME(ABenchyPawn,ColorIndex);
}

void ABenchyPawn::TickAudit(float Time) {
    if(AuditScenery) {
        if(Time>7 && AuditShots==0) {
            const FString Directory=FPaths::ProjectSavedDir()/TEXT("Audit");
            IFileManager::Get().MakeDirectory(*Directory,true);
            FScreenshotRequest::RequestScreenshot(Directory/TEXT("Dschungel-Uebersicht.png"),false,false);AuditShots++;
        }
        if(Time>12) {Audit=false;FPlatformMisc::RequestExit(false);}
        return;
    }
    // Opt-in end-to-end drive: uses the same throttle, rudder and physics as a player.
    if(Time>8) {
        if(Time>35 && ColorIndex!=4) {ColorIndex=3;NextColor();}
        int32 Nearest=0; float Distance=1e30f;
        for(int32 I=0;I<320;I++) {
            const float D=FVector::DistSquared2D(GetActorLocation(),ABenchyWorld::RoutePoint(I));
            if(D<Distance) {Distance=D;Nearest=I;}
        }
        const FVector Target=ABenchyWorld::RoutePoint(Nearest+4)-GetActorLocation();
        const float Error=FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw,Target.Rotation().Yaw);
        Steering=FMath::Clamp(Error/28.f,-1.f,1.f);
        const FVector A=ABenchyWorld::RoutePoint(Nearest+4)-ABenchyWorld::RoutePoint(Nearest);
        const FVector B=ABenchyWorld::RoutePoint(Nearest+12)-ABenchyWorld::RoutePoint(Nearest+4);
        const float Bend=FMath::Abs(FMath::FindDeltaAngleDegrees(A.Rotation().Yaw,B.Rotation().Yaw));
        const float Desired=FMath::Lerp(Nearest<160?1650.f:2400.f,950.f,FMath::Clamp(Bend/75.f,0.f,1.f));
        Throttle=FMath::Clamp((Desired-Hull->GetPhysicsLinearVelocity().Size2D())/750.f+.17f,-.65f,.8f);
        const float RouteError=FMath::Sqrt(Distance);
        AuditMaximumRouteError=FMath::Max(AuditMaximumRouteError,RouteError);
        if(RouteError<1600) AuditVisitedSections|=1u<<(Nearest/20);
        AuditFrames++;
        const FVector Location=GetActorLocation();
        AuditPathLength+=FVector::Dist2D(Location,AuditLastPosition);AuditLastPosition=Location;
        if(Location.X>-6000 && Location.X<2000 && Location.Y>0 && Location.Y<6500 && Location.Z<500)
            AuditCrossedWaterfall=true;
        const FVector Bottom=Hull->GetComponentTransform().TransformPosition(FVector(0,0,-85));
        const float Depth=ABenchyWorld::WaterHeight(Bottom,Time)-Bottom.Z;
        AuditMinDepth=FMath::Min(AuditMinDepth,Depth);
        AuditMaxDepth=FMath::Max(AuditMaxDepth,Depth);
        AuditMaxSpeed=FMath::Max(AuditMaxSpeed,SpeedKmh());
        const float Tilt=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(GetActorUpVector().Z,-1.,1.)));
        AuditMaxTilt=FMath::Max(AuditMaxTilt,Tilt);
    }
    const float ShotTimes[]={7,AuditWaterfall?17.f:26.f,AuditWaterfall?29.f:47.f};
    if(AuditShots<3 && Time>ShotTimes[AuditShots]) {
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("Audit");
        IFileManager::Get().MakeDirectory(*Directory,true);
        FScreenshotRequest::RequestScreenshot(Directory/FString::Printf(TEXT("Sonnendeck-%d.png"),AuditShots+1),false,false);
        AuditShots++;
    }
    if(Time>105) {
        const float Travel=FVector::Dist2D(AuditStart,GetActorLocation());
        int32 RiverSections=0,CoastSections=0;
        for(int32 I=0;I<16;I++)if(AuditVisitedSections&(1u<<I)){if(I<8)RiverSections++;else CoastSections++;}
        const bool AuditPassed=RiverSections>=5 && CoastSections>=5 && AuditFrames>100 && AuditMaxSpeed>20 && AuditPathLength>2000 && AuditResets==0
            && AuditMinDepth>-100 && AuditMaxDepth<180 && AuditMaxTilt<65 && (!AuditWaterfall || AuditCrossedWaterfall);
        const FString Report=FString::Printf(TEXT("{\n  \"passed\": %s,\n  \"frames\": %d,\n  \"max_speed_kmh\": %.2f,\n  \"depth_min_cm\": %.2f,\n  \"depth_max_cm\": %.2f,\n  \"max_tilt_degrees\": %.2f,\n  \"displacement_cm\": %.2f,\n  \"path_length_cm\": %.2f,\n  \"crossed_river_bend\": %s,\n  \"resets\": %d,\n  \"next_gate\": %d\n}\n"),
            AuditPassed?TEXT("true"):TEXT("false"),AuditFrames,AuditMaxSpeed,AuditMinDepth,AuditMaxDepth,AuditMaxTilt,Travel,AuditPathLength,
            AuditCrossedWaterfall?TEXT("true"):TEXT("false"),AuditResets,NextGate);
        FString FullReport=Report.LeftChop(2)+FString::Printf(TEXT(",\n  \"river_sections_visited\": %d,\n  \"coast_sections_visited\": %d,\n  \"maximum_route_error_cm\": %.2f\n}\n"),RiverSections,CoastSections,AuditMaximumRouteError);
        FFileHelper::SaveStringToFile(FullReport,*(FPaths::ProjectSavedDir()/TEXT("Audit/Drive.json")));
        UE_LOG(LogTemp,Display,TEXT("BENCHY_DRIVE_AUDIT: %s"),*Report);
        Audit=false;
        FPlatformMisc::RequestExit(false);
    }
}
FString ABenchyPawn::FilamentName() const {
    static const TCHAR* Names[]={TEXT("Bambu Lab PLA Matte Schwarz"),TEXT("Bambu Lab PLA Matte Mandarin Orange"),
        TEXT("Bambu Lab PLA Basic Cyan"),TEXT("Bambu Lab PLA Basic Gelb"),TEXT("Prusament Galaxy Black"),TEXT("Prusament Glow in the Dark")};
    return Names[FMath::Clamp(ColorIndex,0,5)];
}
void ABenchyPawn::NextColor(){ServerColor((ColorIndex+1)%6);}
void ABenchyPawn::ServerColor_Implementation(int32 Index){ColorIndex=FMath::Clamp(Index,0,5);OnRep_Color();}
void ABenchyPawn::OnRep_Color() {
    static const FLinearColor Colors[]={FLinearColor(.018,.023,.028),FLinearColor(1,.30,.025),
        FLinearColor(.02,.65,.8),FLinearColor(1,.8,.04),FLinearColor(.015,.019,.025),FLinearColor(.54,.78,.38)};
    if(Filament) {
        Filament->SetVectorParameterValue(TEXT("FilamentColor"),Colors[FMath::Clamp(ColorIndex,0,5)]);
        Filament->SetScalarParameterValue(TEXT("Galaxy"),ColorIndex==4?1:0);
        Filament->SetScalarParameterValue(TEXT("Glow"),ColorIndex==5?1:0);
    }
    if(GlowLight)GlowLight->SetIntensity(ColorIndex==5?65:0);
}
void ABenchyPawn::NextQuality() { if(auto PC=Cast<ABenchyController>(Controller))PC->ShowMenu(1); }

void ABenchyPawn::ToggleFullscreen() {
    UGameUserSettings* S=UGameUserSettings::GetGameUserSettings();
    S->SetFullscreenMode(S->GetFullscreenMode()==EWindowMode::Windowed?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);
    S->ApplySettings(false);
}
void ABenchyPawn::Quit() {if(auto PC=Cast<ABenchyController>(Controller))PC->ToggleMenu();}


void ABenchyPawn::PlayCue_Implementation(int32 Cue){
    if(!IsLocallyControlled())return;
    static const TCHAR* Names[]={TEXT("Coin"),TEXT("Spool"),TEXT("Fire"),TEXT("Hit"),TEXT("Ready"),TEXT("Finish")};
    if(Cue<0||Cue>5)return;FString Path=FString::Printf(TEXT("/Game/Audio/Cues/%s.%s"),Names[Cue],Names[Cue]);
    if(auto Sound=LoadObject<USoundBase>(nullptr,*Path))UGameplayStatics::PlaySound2D(this,Sound,.55f);
}
