// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLHTTPRequestTask.h"

#include "ILLHTTPRequestManager.h"

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<float> CVarILLHTTPProcessingDelay(
	TEXT("ill.HTTPProcessingDelay"),
	0.f,
	TEXT("Used to artificially delay HTTP requests from being sent.\n")
);
#endif

UILLHTTPRequestTask::UILLHTTPRequestTask(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLHTTPRequestTask::CancelRequest()
{
	GetOuterUILLHTTPRequestManager()->CancelRequest(this);
}

void UILLHTTPRequestTask::QueueRequest()
{
	// Flag as queued first 'cause threads
	bQueued = true;

	// Cleanup any previous state in case this was re-queued
	ProcessingTime = 0.f;
	QueuedTime = 0.f;
	bProcessing = false;
	HttpResponse.Reset();

	// Listen for completion and enter the async queue
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnProcessRequestComplete);
	GetOuterUILLHTTPRequestManager()->QueueRequest(this);
}

bool UILLHTTPRequestTask::PrepareRequest()
{
	// Create a request object
	HttpRequest = FHttpModule::Get().CreateRequest();
	if (HttpRequest.IsValid())
	{
		DebugName = GetName();
		return true;
	}

	return false;
}

bool UILLHTTPRequestTask::IsReadyToProcess() const
{
	// Forbid processing from starting when flagged for cancellation
	if (bCanceled)
	{
		return false;
	}

#if UE_BUILD_SHIPPING
	return true;
#else
	return (QueuedTime >= CVarILLHTTPProcessingDelay.GetValueOnAnyThread());
#endif
}

void UILLHTTPRequestTask::AsyncProcessRequest()
{
	// Flag as processing
	bProcessing = true;

	// If this function fails OnProcessRequestComplete should fire
	HttpRequest->ProcessRequest();
}

void UILLHTTPRequestTask::HandleResponse()
{
	if (bTimedOut || bCanceled || !HttpResponse.IsValid() || HttpResponse->GetResponseCode() < 200 || HttpResponse->GetResponseCode() > 299)
	{
		UE_LOG(LogILLHTTP, Warning, TEXT("%s: %s %s after %.2fs (%.2fs queued)"), ANSI_TO_TCHAR(__FUNCTION__), *GetDebugName(), bTimedOut ? TEXT("TIMEDOUT") : (bCanceled ? TEXT("CANCELLED") : *ToDebugString(HttpResponse)), GetTotalRequestTime(), QueuedTime);
	}
	else
	{
		UE_LOG(LogILLHTTP, Verbose, TEXT("%s: %s %s after %.2fs (%.2fs queued)"), ANSI_TO_TCHAR(__FUNCTION__), *GetDebugName(), *ToDebugString(HttpResponse), GetTotalRequestTime(), QueuedTime);
	}
}

void UILLHTTPRequestTask::OnProcessRequestComplete(FHttpRequestPtr InHttpRequest, FHttpResponsePtr InHttpResponse, bool bSucceeded)
{
	// Hand the response back to the manager's thread
	HttpResponse = InHttpResponse;
	GetOuterUILLHTTPRequestManager()->QueueResponse(this);
}
