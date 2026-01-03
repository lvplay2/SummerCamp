// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Icmp.h"

extern ILLGAMEFRAMEWORK_API void ILLUDPEcho(const FString& TargetAddress, float Timeout, FIcmpEchoResultCallback HandleResult);

static FORCEINLINE void ILLUDPEcho(const FString& TargetAddress, float Timeout, FIcmpEchoResultDelegate ResultDelegate)
{
	ILLUDPEcho(TargetAddress, Timeout, [ResultDelegate](FIcmpEchoResult Result)
	{
		ResultDelegate.ExecuteIfBound(Result);
	});
}
