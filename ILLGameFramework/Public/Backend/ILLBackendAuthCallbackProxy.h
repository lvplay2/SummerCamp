// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLBackendRequestManager.h"
#include "ILLLatentBlueprintCallProxyBase.h"
#include "ILLBackendAuthCallbackProxy.generated.h"

/**
 * @class UILLBackendAuthCallbackProxy
 * @brief Blueprint hooks for UILLBackendRequestManager::StartAuthRequest.
 */
UCLASS(MinimalAPI)
class UILLBackendAuthCallbackProxy
: public UILLLatentBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Begin UILLLatentBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End UILLLatentBlueprintCallProxyBase interface

	/** Makes an authentication request to the backend. */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true"), Category="Backend")
	static UILLBackendAuthCallbackProxy* SendAuthRequest(class AILLPlayerController* PlayerController);

	/** Makes a Steam authentication handshake with the backend. */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true"), Category="Backend", meta=(DeprecatedFunction, DeprecationMessage="Use SendAuthRequest instead"))
	static UILLBackendAuthCallbackProxy* SendSteamAuthRequest(class AILLPlayerController* PlayerController);

	// Called when authentication was successful
	UPROPERTY(BlueprintAssignable)
	FEmptyILLLatentDelegate OnSuccess;

	// Called when authentication fails for any reason
	UPROPERTY(BlueprintAssignable)
	FEmptyILLLatentDelegate OnFailure;

	// Called when authentication fails due to being banned
	UPROPERTY(BlueprintAssignable)
	FEmptyILLLatentDelegate OnBannedFailure;

	// Called for other authentication failures
	UPROPERTY(BlueprintAssignable)
	FEmptyILLLatentDelegate OnGeneralFailure;

private:
	/** Internal callback for auth completion. */
	void OnCompleted(EILLBackendAuthResult AuthResult);

	// Player controller triggering things
	TWeakObjectPtr<AILLPlayerController> PlayerControllerWeakPtr;
};
