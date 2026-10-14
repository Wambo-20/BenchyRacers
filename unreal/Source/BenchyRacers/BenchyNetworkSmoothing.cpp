#include "BenchyNetworkSmoothing.h"
#include "BenchyRace.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

void ABenchyPawn::PublishNetworkPose(bool Teleport){
    if(!HasAuthority())return;
    NetworkPosition=GetActorLocation();NetworkRotation=GetActorRotation();NetworkSpeed=SpeedKmh();
    NetworkPose.Position=NetworkPosition;NetworkPose.Rotation=NetworkRotation;
    NetworkPose.Velocity=Hull->GetPhysicsLinearVelocity();
    auto Clock=GetWorld()->GetGameState<ABenchyRaceState>();
    NetworkPose.Time=Clock?Clock->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();
    if(Teleport)++NetworkPose.Generation;
}
void ABenchyPawn::OnRep_NetworkPose(){
    if(NetworkPose.Time<0||NetworkPose.Position.ContainsNaN()||NetworkPose.Velocity.ContainsNaN())return;
    if(NetworkSamples.Num()&&NetworkPose.Generation!=NetworkSamples.Last().Generation){
        NetworkSamples.Reset();NetworkInitialized=false;History.Reset();
    }
    if(NetworkSamples.Num()&&NetworkPose.Time<=NetworkSamples.Last().Time)return;
    NetworkPosition=NetworkPose.Position;NetworkRotation=NetworkPose.Rotation;
    NetworkSpeed=NetworkPose.Velocity.Size2D()*.036f;
    NetworkSamples.Add(NetworkPose);if(NetworkSamples.Num()>20)NetworkSamples.RemoveAt(0);
}
void ABenchyPawn::SmoothNetworkPose(float DT,float ServerTime){
    if(NetworkSamples.IsEmpty())return;
    // Remote racers play a short buffered timeline; the player's own boat
    // advances the freshest velocity instead of adding another buffer delay.
    auto Pose=BenchySampleNetwork(NetworkSamples,ServerTime-(IsLocallyControlled()?0.f:.10f));
    bool Snap=!NetworkInitialized||FVector::DistSquared(GetActorLocation(),Pose.Position)>25000000;
    float Alpha=1.f-FMath::Exp(-25.f*FMath::Max(0.f,DT));
    FVector Position=Snap?Pose.Position:FMath::Lerp(GetActorLocation(),Pose.Position,Alpha);
    FQuat Rotation=Snap?Pose.Rotation.Quaternion():FQuat::Slerp(GetActorQuat(),Pose.Rotation.Quaternion(),Alpha);
    SetActorLocationAndRotation(Position,Rotation,false,nullptr,ETeleportType::TeleportPhysics);NetworkInitialized=true;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBenchyNetworkSmoothingTest,"Benchy.Network.BufferedMovement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBenchyNetworkSmoothingTest::RunTest(const FString&){
    TArray<FBenchyNetPose> Samples;
    // Uneven samples include a dropped update. Constant motion stays linear.
    for(float T:{1.f,1.033f,1.100f,1.167f}){FBenchyNetPose P;P.Time=T;P.Position=FVector(T*3000,0,0);P.Velocity=FVector(3000,0,0);Samples.Add(P);}
    for(int32 N=0;N<100;++N){float T=1.f+N*.003f;auto P=BenchySampleNetwork(Samples,T);
        TestTrue(TEXT("Jitter and missed update preserve continuous motion"),FMath::Abs(P.Position.X-T*3000)<.01);}
    TestTrue(TEXT("Extrapolation stops after 150 ms"),FMath::Abs(BenchySampleNetwork(Samples,5).Position.X-(1.167f+.15f)*3000)<.01);
    Samples[0].Rotation.Yaw=179;Samples[1].Rotation.Yaw=-179;
    TestTrue(TEXT("Rotation crosses wrap by shortest path"),FMath::Abs(BenchySampleNetwork(Samples,1.0165f).Rotation.Yaw)>178);
    Samples[1].Generation=1;Samples[1].Position=FVector(50000,0,0);
    TestTrue(TEXT("Respawn is never interpolated across map"),BenchySampleNetwork(Samples,1.01f).Position.X==50000);
    TestTrue(TEXT("Empty timeline is safe"),BenchySampleNetwork({},2).Time<0);
    return true;
}
#endif
