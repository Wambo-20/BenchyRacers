#pragma once
#include "CoreMinimal.h"
namespace BenchyFilaments {
constexpr int32 Count=7;
inline const TCHAR* Name(int32 Index) {
    static const TCHAR* Names[]={TEXT("Bambu Lab PLA Matte Schwarz"),TEXT("Bambu Lab PLA Matte Mandarin Orange"),TEXT("Bambu Lab PLA Basic Cyan"),TEXT("Bambu Lab PLA Basic Gelb"),TEXT("Prusament Galaxy Black"),TEXT("Prusament Glow in the Dark"),TEXT("Neon Gelb")};
    return Names[FMath::Clamp(Index,0,Count-1)];
}
inline bool IsGlow(int32 Index){return Index==5||Index==6;}
inline FLinearColor GlowColor(int32 Index){return Index==6?FLinearColor(1,.82,.015):FLinearColor(.1,.75,.25);}
}
