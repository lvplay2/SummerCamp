// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Core.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemImpl.h"
#include "ModuleInterface.h"

#undef ONLINE_LOG_PREFIX
#define ONLINE_LOG_PREFIX TEXT("ILL: ")

#ifndef ILL_SUBSYSTEM
#define ILL_SUBSYSTEM FName(TEXT("ILL"))
#endif

class FILLOnlineAsyncTaskManager;
class FILLOnlineFactory;
class FRunnableThread;
class UWorld;

/**
 * @class FILLOnlineSubsystem
 */
class ILLONLINESUBSYSTEM_API FILLOnlineSubsystem
: public FOnlineSubsystemImpl
{
public:
	virtual ~FILLOnlineSubsystem() {}

	// Begin IOnlineSubsystem interface
	virtual IOnlineSessionPtr GetSessionInterface() const override { return nullptr; }
	virtual IOnlineFriendsPtr GetFriendsInterface() const override { return nullptr; }
	virtual IOnlinePartyPtr GetPartyInterface() const override { return nullptr; }
	virtual IOnlineGroupsPtr GetGroupsInterface() const override { return nullptr; }
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override { return nullptr; }
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override { return nullptr; }
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override { return nullptr; }
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override { return nullptr; }
	virtual IOnlineVoicePtr GetVoiceInterface() const override { return nullptr; }
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override { return nullptr; }	
	virtual IOnlineTimePtr GetTimeInterface() const override { return nullptr; }
	virtual IOnlineIdentityPtr GetIdentityInterface() const override { return nullptr; }
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override { return nullptr; }
	virtual IOnlineStorePtr GetStoreInterface() const override { return nullptr; }
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override { return nullptr; }
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override { return nullptr; }
	virtual IOnlineEventsPtr GetEventsInterface() const override { return nullptr; }
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override { return nullptr; }
	virtual IOnlineSharingPtr GetSharingInterface() const override { return nullptr; }
	virtual IOnlineUserPtr GetUserInterface() const override { return nullptr; }
	virtual IOnlineMessagePtr GetMessageInterface() const override { return nullptr; }
	virtual IOnlinePresencePtr GetPresenceInterface() const override { return nullptr; }
	virtual IOnlineChatPtr GetChatInterface() const override { return nullptr; }
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override { return nullptr; }

	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override { return FString(); }
	virtual FText GetOnlineServiceName() const { return FText(); }
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	// End IOnlineSubsystem interface

	// Begin FTickerObjectBase interface
	virtual bool Tick(float DeltaTime) override;
	// End FTickerObjectBase interface

PACKAGE_SCOPE:
	FILLOnlineSubsystem(FName InSubsystemName, FName InInstanceName)
	: FOnlineSubsystemImpl(InSubsystemName, InInstanceName)
	, OnlineAsyncTaskThreadRunnable(nullptr)
	, OnlineAsyncTaskThread(nullptr) {}

	FILLOnlineSubsystem()
	: OnlineAsyncTaskThreadRunnable(nullptr)
	, OnlineAsyncTaskThread(nullptr) {}

private:
	// Online async task runnable
	FILLOnlineAsyncTaskManager* OnlineAsyncTaskThreadRunnable;

	// Online async task thread
	FRunnableThread* OnlineAsyncTaskThread;
};

typedef TSharedPtr<FILLOnlineSubsystem, ESPMode::ThreadSafe> FILLOnlineSubsystemPtr;

/**
 * @class FILLOnlineSubsystemModule
 */
class FILLOnlineSubsystemModule
: public IModuleInterface
{
public:
	FILLOnlineSubsystemModule()
	: IllFactory(nullptr) {}

	virtual ~FILLOnlineSubsystemModule() {}

	// Begin IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override { return false; }
	virtual bool SupportsAutomaticShutdown() override { return false; }
	// End IModuleInterface interface

private:
	// Class responsible for creating instance(s) of the subsystem
	FILLOnlineFactory* IllFactory;
};
