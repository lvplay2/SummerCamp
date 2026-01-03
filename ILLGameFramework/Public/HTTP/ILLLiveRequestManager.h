// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLHTTPRequestManager.h"
#include "ILLLiveRequestTasks.h"
#include "ILLLiveRequestManager.generated.h"

/**
 * @class UILLLiveRequestManager
 * @brief Singleton frontend to the Xbox Live services, used by dedicated servers.
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLLiveRequestManager
: public UILLHTTPRequestManager
{
	GENERATED_UCLASS_BODY()
};
