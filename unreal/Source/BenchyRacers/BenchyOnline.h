#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BenchyOnline.generated.h"
struct FBenchyEOS;
UCLASS()
class BENCHYRACERS_API UBenchyOnline : public UObject {
    GENERATED_BODY()
public:
    UBenchyOnline();
    virtual ~UBenchyOnline();
    void Initialize();
    void Stop();
    void Host();
    void Join(FString Code);
    void Leave();
    void Login();
    void Ready(void* User);
    void CreateRoom();
    void FindRoom();
    void PublishRoom();
    void TravelToHost();
    void Fail(const TCHAR* Action,int32 Result);
    bool Configured=false,Busy=false,Hosting=false;
    FString RoomCode,Status;
    TSharedPtr<FBenchyEOS> EOS;
private:
    bool PendingHost=false;
};
