#pragma once
#include "CoreMinimal.h"
#include "NetDriverEOS.h"
#include "BenchyNetDriver.generated.h"
UCLASS(Transient,Config=Engine)
class BENCHYRACERS_API UBenchyNetDriver : public UNetDriverEOS {
    GENERATED_BODY()
public:
    virtual ISocketSubsystem* GetSocketSubsystem() override;
    virtual bool InitBase(bool Client,FNetworkNotify* InNotify,const FURL& URL,bool Reuse,FString& Error) override;
};
