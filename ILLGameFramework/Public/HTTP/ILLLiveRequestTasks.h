// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLHTTPRequestTask.h"
#include "ILLLiveRequestTasks.generated.h"

/**
 * @class UILLLiveHTTPRequestTask
 */
UCLASS(Abstract, Within=ILLLiveRequestManager)
class ILLGAMEFRAMEWORK_API UILLLiveHTTPRequestTask
: public UILLHTTPRequestTask
{
	GENERATED_BODY()

public:
	UILLLiveHTTPRequestTask()
	{
		TimeoutDuration = 30.f;
	}
};
