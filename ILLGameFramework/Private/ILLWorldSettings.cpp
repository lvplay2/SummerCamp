// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLWorldSettings.h"

#include "ILLGameNetworkManager.h"

AILLWorldSettings::AILLWorldSettings(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GameNetworkManagerClass = AILLGameNetworkManager::StaticClass();
}
