// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLOnlineSubsystem.h"
#include "ILLOnlineAsyncTaskManager.h"

void FILLOnlineAsyncTaskManager::OnlineTick()
{
	check(OnlineSubsystem);
	check(FPlatformTLS::GetCurrentThreadId() == OnlineThreadId || !FPlatformProcess::SupportsMultithreading());
}
