// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLOnlineSubsystem.h"
#include "ILLOnlineAsyncTaskManager.h"

bool FILLOnlineSubsystem::Tick(float DeltaTime)
{
	if (!FOnlineSubsystemImpl::Tick(DeltaTime))
	{
		return false;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		OnlineAsyncTaskThreadRunnable->GameTick();
	}

	return true;
}

bool FILLOnlineSubsystem::Init()
{
	const bool bInit = true;
	
	if (bInit)
	{
		// Create the online async task thread
		OnlineAsyncTaskThreadRunnable = new FILLOnlineAsyncTaskManager(this);
		check(OnlineAsyncTaskThreadRunnable);
		OnlineAsyncTaskThread = FRunnableThread::Create(OnlineAsyncTaskThreadRunnable, *FString::Printf(TEXT("ILLOnlineAsyncTaskThread %s"), *InstanceName.ToString()), 128 * 1024, TPri_Normal);
		check(OnlineAsyncTaskThread);
		UE_LOG_ONLINE(Verbose, TEXT("Created thread (ID:%d)."), OnlineAsyncTaskThread->GetThreadID());
	}
	else
	{
		Shutdown();
	}

	return bInit;
}

bool FILLOnlineSubsystem::Shutdown()
{
	UE_LOG_ONLINE(Display, TEXT("FILLOnlineSubsystem::Shutdown()"));

	FOnlineSubsystemImpl::Shutdown();

	if (OnlineAsyncTaskThread)
	{
		// Destroy the online async task thread
		delete OnlineAsyncTaskThread;
		OnlineAsyncTaskThread = nullptr;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		delete OnlineAsyncTaskThreadRunnable;
		OnlineAsyncTaskThreadRunnable = nullptr;
	}
	
#define DESTRUCT_INTERFACE(Interface) \
	if (Interface.IsValid()) \
	{ \
		ensure(Interface.IsUnique()); \
		Interface = nullptr; \
	}
 
	// Destruct the interfaces
	
#undef DESTRUCT_INTERFACE
	
	return true;
}

bool FILLOnlineSubsystem::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FOnlineSubsystemImpl::Exec(InWorld, Cmd, Ar))
	{
		return true;
	}
	return false;
}

IMPLEMENT_MODULE(FILLOnlineSubsystemModule, ILLOnlineSubsystem);

/**
 * Class responsible for creating instance(s) of the subsystem
 */
class FILLOnlineFactory
: public IOnlineFactory
{
public:
	FILLOnlineFactory() {}
	virtual ~FILLOnlineFactory() {}

	// Begin IOnlineFactory interface
	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName) override
	{
		FILLOnlineSubsystemPtr OnlineSub = MakeShareable(new FILLOnlineSubsystem(ILL_SUBSYSTEM, InstanceName));
		if (!OnlineSub->Init())
		{
			UE_LOG_ONLINE(Warning, TEXT("ILL API failed to initialize!"));
			OnlineSub->Shutdown();
			OnlineSub = nullptr;
		}

		return OnlineSub;
	}
	// End IOnlineFactory interface
};

void FILLOnlineSubsystemModule::StartupModule()
{
	IllFactory = new FILLOnlineFactory();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(ILL_SUBSYSTEM, IllFactory);
}

void FILLOnlineSubsystemModule::ShutdownModule()
{
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(ILL_SUBSYSTEM);
	
	delete IllFactory;
	IllFactory = nullptr;
}
