#pragma once
#include "CoreMinimal.h"
#include "BenchyLogoSpec.generated.h"
USTRUCT()
struct FBenchyLogoSpec {
    GENERATED_BODY()
    UPROPERTY() FString Hash;
    UPROPERTY() int32 Side=0;
    UPROPERTY() float X=.5f;
    UPROPERTY() float Y=.25f;
    UPROPERTY() float Size=.34f;
    UPROPERTY() float Angle=0;
};
struct FBenchyLogoTransfer {FString Hash;TArray<uint8> Bytes;int32 Offset=0,Total=0;float Started=0;FBenchyLogoSpec Spec;};
