// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ILLBackendBlueprintLibrary.generated.h"

class AILLPlayerController;

extern ILLGAMEFRAMEWORK_API bool GILLUserRequestedBackendOfflineMode;

/**
 * @class UILLBackendBlueprintLibrary
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLBackendBlueprintLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** @return true if the local PlayerController needs to login to the backend or platform. */
	UFUNCTION(BlueprintPure, Category="Backend")
	static bool IsInOfflineMode(AILLPlayerController* PlayerController);

	/** @return true if the local PlayerController has logged into the backend. */
	UFUNCTION(BlueprintPure, Category="Backend")
	static bool HasBackendLogin(AILLPlayerController* PlayerController);

	/** @return true if the local PlayerController needs to login to the backend. */
	UFUNCTION(BlueprintPure, Category="Backend", meta=(DeprecatedFunction, DeprecationMessage="Use HasBackendLogin instead"))
	static bool NeedsAuthRequest(AILLPlayerController* PlayerController);

	/** @return true if the local PlayerController can proceed into offline mode. */
	UFUNCTION(BlueprintPure, Category="Backend")
	static bool CanProceedIntoOfflineMode(AILLPlayerController* PlayerController);

	/**
	 * Player has requested to go into offline mode. Flag the application as being in it.
	 * This will automatically be broken out of after the next backend login success, or dedicated server backend report send success.
	 */
	UFUNCTION(BlueprintCallable, Category="Backend")
	static void RequestOfflineMode(AILLPlayerController* PlayerController);

	/** @return true if a player has requested to go into offline mode. */
	UFUNCTION(BlueprintPure, Category="Backend")
	static bool HasUserRequestedOfflineMode() { return GILLUserRequestedBackendOfflineMode; }
};
