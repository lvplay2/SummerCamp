// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLLatentBlueprintCallProxyBase.h"
#include "ILLCreatePartyCallbackProxy.generated.h"

class AILLPlayerController;

/**
 * @class UILLCreatePartyCallbackProxy
 * @brief Blueprint hook for UILLOnlineSessionClient::CreateParty.
 */
UCLASS(MinimalAPI)
class UILLCreatePartyCallbackProxy
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
	static UILLCreatePartyCallbackProxy* CreateParty(AILLPlayerController* PlayerController);

private:
	/** Internal callback for completion. */
	UFUNCTION()
	void OnPartyCreationComplete(bool bWasSuccessful);

	// Player controller triggering things
	TWeakObjectPtr<AILLPlayerController> PlayerControllerWeakPtr;
};
