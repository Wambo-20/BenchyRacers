#pragma once
#include "CoreMinimal.h"
namespace BenchyFilaments {
constexpr int32 Count=7;
inline const TCHAR* Name(int32 Index) {
    static const TCHAR* Names[]={TEXT("Rabenschwarz Matt"),TEXT("Mandarinenrausch"),TEXT("Lagunenblau"),TEXT("Honiggold"),TEXT("Sternenstaub Schwarz"),TEXT("Glühwürmchengrün"),TEXT("Neonbanane")};
    return Names[FMath::Clamp(Index,0,Count-1)];
}
inline bool IsGlow(int32 Index){return Index==5||Index==6;}
inline FLinearColor GlowColor(int32 Index){return Index==6?FLinearColor(1,.82,.015):FLinearColor(.1,.75,.25);}
}
