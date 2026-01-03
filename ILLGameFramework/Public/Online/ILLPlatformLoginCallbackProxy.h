// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "OnlineSessionInterface.h"
#include "OnlineEngineInterface.h"
#include "OnlineExternalUIInterface.h"
#include "ILLLocalPlayer.h"
#include "ILLPlatformLoginCallbackProxy.generated.h"

class AILLPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLPlatformLoginResult, AILLPlayerController*, PlayerController);

/**
 * @class UILLPlatformLoginCallbackProxy
 */
UCLASS(MinimalAPI)
class UILLPlatformLoginCallbackProxy
: public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UILLPlatformLoginCallbackProxy(const FObjectInitializer& ObjectInitializer);

	// Called when there is a successful login and privilege query
	UPROPERTY(BlueprintAssignable)
	FILLPlatformLoginResult OnSuccess;

	// Called when there is an unsuccessful login query
	UPROPERTY(BlueprintAssignable)
	FILLPlatformLoginResult OnLoginFailed;

	// Called when there is an unsuccessful privilege query
	UPROPERTY(BlueprintAssignable)
	FILLPlatformLoginResult OnPrivilegesFailed;

	/**
	 * Shows the login UI for the currently active online subsystem, if the subsystem supports a login UI.
	 *
	 * @param PlayerController Player to show the external login UI for.
	 * @param bForceLoginUI Show the login UI even if PlayerController is already logged into the platform.
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Online")
	static UILLPlatformLoginCallbackProxy* ShowPlatformLoginCheckEntitlements(UObject* WorldContextObject, AILLPlayerController* PlayerController, const bool bForceLoginUI = false);

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	// End of UBlueprintAsyncActionBase interface

private:
	/** Internal callback when the login UI closes, calls out to the public success/failure callbacks. */
	void OnShowLoginUICompleted(TSharedPtr<const FUniqueNetId> UniqueId, int LocalUserNum);

	/** Internal callback for when the auto-login process completes. */
	void OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& Error);

	/** Internal callback when the user privilege checks have completed. */
	void OnPrivilegeCheckCompleted(UILLLocalPlayer* LocalPlayer);

	// The player controller triggering things
	TWeakObjectPtr<AILLPlayerController> PlayerControllerWeakPtr;

	// The world context object in which this call is taking place
	UPROPERTY()
	UObject* WorldContextObject;

	// UniqueId that is pending privilege checks
	TSharedPtr<const FUniqueNetId> PendingUniqueId;

	// Display the login UI even if they are already logged in?
	bool bForceLoginUI;

	// Delegate for the Login UI process
	FOnLoginUIClosedDelegate LoginUIClosedDelegate;

	// Delegate for the auto-login process
	FOnlineAutoLoginComplete AutoLoginDelegate;

	// Delegate for the privilege collection process
	FILLOnPrivilegesCollectedDelegate PrivilegesCollectedDelegate;
};
