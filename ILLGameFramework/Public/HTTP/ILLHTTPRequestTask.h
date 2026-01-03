// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Http.h"

#include "ILLHTTPRequestTask.generated.h"

class UILLHTTPRequestManager;

/** @return Debug string for the response code in Response, if valid. */
FORCEINLINE FString ToDebugString(FHttpResponsePtr Response)
{
	if (!Response.IsValid())
		return TEXT("INVALID");
	if (Response->GetResponseCode() >= 200 && Response->GetResponseCode() <= 299)
		return FString::Printf(TEXT("%i"), Response->GetResponseCode());
	return FString::Printf(TEXT("%i \"%s\""), Response->GetResponseCode(), *Response->GetContentAsString());
}

/**
 * @enum EILLHTTPVerb
 */
enum class EILLHTTPVerb : uint8
{
	Delete,
	Get,
	Head,
	Post,
	Put
};

/**
 * @return String version of Verb.
 */
FORCEINLINE const TCHAR* ToString(const EILLHTTPVerb Verb)
{
	switch (Verb)
	{
	case EILLHTTPVerb::Delete: return TEXT("DELETE");
	case EILLHTTPVerb::Get: return TEXT("GET");
	case EILLHTTPVerb::Head: return TEXT("HEAD");
	case EILLHTTPVerb::Post: return TEXT("POST");
	case EILLHTTPVerb::Put: return TEXT("PUT");
	}

	check(0);
	return TEXT("INVALID");
}

/**
 * @class UILLHTTPRequestTask
 * @brief Base container for an HTTP request task. Handles building the request and parsing the response asynchronously.
 */
UCLASS(Abstract, Transient, Config=Engine, Within=ILLHTTPRequestManager)
class ILLGAMEFRAMEWORK_API UILLHTTPRequestTask
: public UObject
{
	GENERATED_UCLASS_BODY()

public:
	/** @return Unique name of the request for logging/debugging. */
	FORCEINLINE const FString& GetDebugName() const { return DebugName; }

protected:
	// Debug name of the request, should be assigned in PrepareRequest
	FString DebugName;

	//////////////////////////////////////////////////
	// Request
public:
	/** @return Total time spent queued and processing. */
	FORCEINLINE float GetTotalRequestTime() const { return QueuedTime + ProcessingTime; }

	/** @return true if this request task has been queued. */
	FORCEINLINE bool HasBeenQueued() const { return bQueued; }

	/**
	 * Cancels this queued request.
	 */
	virtual void CancelRequest();

	/**
	 * Adds this request to the processing queue of our manager.
	 * AsyncPrepareRequest will be called first, before AsyncProcessRequest.
	 */
	virtual void QueueRequest();

protected:
	friend UILLHTTPRequestManager;

	/**
	 * Called to prepare this request on the game thread, from UILLHTTPRequestManager::CreateRequest.
	 *
	 * @return true if the request is valid.
	 */
	virtual bool PrepareRequest();

	/**
	 * Asynchronous call to prepare this request on the request manager thread.
	 * This is a good spot to build JSON, encrypt, etc.
	 */
	virtual void AsyncPrepareRequest() {}

	/**
	 * Polled periodically from the game and request manager threads to see if we are ready to call AsyncProcessRequest.
	 *
	 * @return true when AsyncProcessRequest can be called.
	 */
	virtual bool IsReadyToProcess() const;

	/**
	 * Asynchronous call to start processing this request on the request manager thread.
	 * Calls to IHTTPRequest::ProcessRequest, which will call OnProcessRequestComplete for any failure to start.
	 */
	virtual void AsyncProcessRequest();

	// HTTP request handle
	FHttpRequestPtr HttpRequest;

	// Timeout duration before being canceled after processing beings, 0 to use the HTTP module default
	float TimeoutDuration;

	// Total time this has been processed
	float ProcessingTime;

	// Total time this has been queued
	float QueuedTime;

	// Has this request been queued?
	uint32 bQueued:1;

	// Has this request started processing (been sent)?
	uint32 bProcessing:1;

	// Has this request been canceled?
	uint32 bCanceled:1;

	// Was this request timed out by our manager?
	uint32 bTimedOut:1;

	//////////////////////////////////////////////////
	// Response
protected:
	/** Asynchronous call to prepare the response on the request manager thread, before being queued for game thread processing. */
	virtual void AsyncPrepareResponse() {}

	/**
	 * Called to handle the response for this request on the game thread.
	 * Hand off information from here to it's destination, afterwards this object is orphaned by the request manager and marked for garbage collection.
	 */
	virtual void HandleResponse();

	// HTTP response handle
	FHttpResponsePtr HttpResponse;

private:
	/** Called when an HTTP response is received. */
	virtual void OnProcessRequestComplete(FHttpRequestPtr InHttpRequest, FHttpResponsePtr InHttpResponse, bool bSucceeded);
};
