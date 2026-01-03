// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineAsyncTaskManager.h"

class FILLOnlineSubsystem;

/**
 * @class FILLOnlineAsyncTaskManager
 */
class FILLOnlineAsyncTaskManager
: public FOnlineAsyncTaskManager
{
public:
	FILLOnlineAsyncTaskManager(FILLOnlineSubsystem* InOnlineSubsystem)
	: OnlineSubsystem(InOnlineSubsystem) {}

	~FILLOnlineAsyncTaskManager() {}

	// Begin FOnlineAsyncTaskManager interface
	virtual void OnlineTick() override;
	// End FOnlineAsyncTaskManager interface

protected:
	// Cached reference to the main online subsystem
	FILLOnlineSubsystem* OnlineSubsystem;
};
