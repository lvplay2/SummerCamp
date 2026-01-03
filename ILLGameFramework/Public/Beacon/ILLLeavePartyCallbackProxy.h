// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLLatentBlueprintCallProxyBase.h"
#include "ILLLeavePartyCallbackProxy.generated.h"

class AILLPlayerController;

/**
 * @class UILLLeavePartyCallbackProxy
 * @brief Blueprint hook for UILLOnlineSessionClient::LeaveParty.
 */
UCLASS(MinimalAPI)
class UILLLeavePartyCallbackProxy
: public UILLLatentBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Begin UILLLatentBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End UILLLatentBlueprintCallProxyBase interface

	/** Called when party creation was successful. */
	UPROPERTY(BlueprintAssignable)
	FEmptyILLLatentDelegate OnSuccess;

	/** Called when party creation fails. */
	UPROPERTY(BlueprintAssignable)
	FEmptyILLLatentDelegate OnFailure;

	/** Attempts to start a party. */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true"), Category="Parties")
	static UILLLeavePartyCallbackProxy* LeaveParty(AILLPlayerController* PlayerController);

private:
	/** Internal callback for completion. */
	UFUNCTION()
	void OnPartyDestructionComplete(bool bWasSuccessful);

	// Player controller triggering things
	TWeakObjectPtr<AILLPlayerController> PlayerControllerWeakPtr;
};
