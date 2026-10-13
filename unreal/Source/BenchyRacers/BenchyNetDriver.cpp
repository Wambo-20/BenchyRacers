#include "BenchyNetDriver.h"
#include "SocketSubsystem.h"
#include "SocketSubsystemEOS.h"
ISocketSubsystem* UBenchyNetDriver::GetSocketSubsystem(){return ISocketSubsystem::Get(bIsPassthrough?PLATFORM_SOCKETSUBSYSTEM:EOS_SOCKETSUBSYSTEM);}
bool UBenchyNetDriver::InitBase(bool Client,FNetworkNotify* InNotify,const FURL& URL,bool Reuse,FString& Error){InitialConnectTimeout=180;ConnectionTimeout=90;return Super::InitBase(Client,InNotify,URL,Reuse,Error);}
