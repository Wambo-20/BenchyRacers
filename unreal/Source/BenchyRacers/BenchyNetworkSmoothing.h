#pragma once
#include "BenchyPawn.h"

// Samples use the shared server clock, never packet arrival intervals.
inline FBenchyNetPose BenchySampleNetwork(const TArray<FBenchyNetPose>& Samples,float Time,float MaxExtrapolation=.15f){
    if(Samples.IsEmpty())return FBenchyNetPose();
    if(Time<=Samples[0].Time)return Samples[0];
    for(int32 N=1;N<Samples.Num();++N){
        const auto& B=Samples[N];if(Time>B.Time)continue;const auto& A=Samples[N-1];
        if(A.Generation!=B.Generation)return B;
        float Alpha=FMath::Clamp((Time-A.Time)/FMath::Max(.0001f,B.Time-A.Time),0.f,1.f);
        FBenchyNetPose Out=B;Out.Position=FMath::Lerp(A.Position,B.Position,Alpha);
        Out.Velocity=FMath::Lerp(A.Velocity,B.Velocity,Alpha);
        Out.Rotation=FQuat::Slerp(A.Rotation.Quaternion(),B.Rotation.Quaternion(),Alpha).Rotator();Out.Time=Time;return Out;
    }
    FBenchyNetPose Out=Samples.Last();float Ahead=FMath::Clamp(Time-Out.Time,0.f,MaxExtrapolation);
    Out.Position+=Out.Velocity*Ahead;
    if(Samples.Num()>1){const auto& Prev=Samples[Samples.Num()-2];float Delta=Out.Time-Prev.Time;
        if(Delta>.001f&&Prev.Generation==Out.Generation){float Rate=FMath::FindDeltaAngleDegrees(Prev.Rotation.Yaw,Out.Rotation.Yaw)/Delta;
            Out.Rotation.Yaw+=FMath::Clamp(Rate,-180.f,180.f)*Ahead;}}
    return Out;
}
