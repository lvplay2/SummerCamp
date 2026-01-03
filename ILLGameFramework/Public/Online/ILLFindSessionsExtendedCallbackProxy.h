// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineBlueprintCallProxyBase.h"
#include "FindSessionsCallbackProxy.h"
#include "ILLFindSessionsExtendedCallbackProxy.generated.h"

class APlayerController;

class FILLOnlineSessionSearch;

/**
 * @class UILLFindSessionsExtendedCallbackProxy
 * @brief Extended version of FindSessions which also support more search filters.
 */
UCLASS(MinimalAPI)
class UILLFindSessionsExtendedCallbackProxy
: public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when there is a successful query
	UPROPERTY(BlueprintAssignable)
	FBlueprintFindSessionsResultDelegate OnSuccess;

	// Called when there is an unsuccessful query
	UPROPERTY(BlueprintAssignable)
	FBlueprintFindSessionsResultDelegate OnFailure;

	// Searches for advertised sessions with the default online subsystem
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly = "true", WorldContext="WorldContextObject"), Category = "Online|Session")
	static UILLFindSessionsExtendedCallbackProxy* FindSessionsExtended(UObject* WorldContextObject, APlayerController* PlayerController, const int32 MaxResults, const bool bLanQuery, const bool bDedicatedServersOnly, const bool bNonEmptyOnly);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

private:
	/** Internal callback when the session search completes, calls out to the public success/failure callbacks. */
	void OnCompleted(bool bSuccess);

private:
	// The world context object in which this call is taking place
	UObject* WorldContextObject;

	// The player controller triggering things
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	// The delegate executed by the online subsystem
	FOnFindSessionsCompleteDelegate Delegate;

	// Handle to the registered OnCompleted delegate
	FDelegateHandle DelegateHandle;

	// Object to track search results
	TSharedPtr<FILLOnlineSessionSearch> SearchObject;

	// Maximum number of results to return
	int MaxResults;

	// Broadcast search for LAN matches?
	bool bIsLanQuery;

	// Find only dedicated servers?
	bool bDedicatedOnly;

	// Find only non-empty servers?
	bool bNonEmptySearch;
};
