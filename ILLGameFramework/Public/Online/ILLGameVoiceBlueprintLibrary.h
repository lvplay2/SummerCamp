// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ILLGameVoiceBlueprintLibrary.generated.h"

/**
 * @class UILLGameVoiceBlueprintLibrary
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLGameVoiceBlueprintLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	//////////////////////////////////////////////////
	// Muting
public:
	/** @return true if PlayerState can be muted for Listener. */
	UFUNCTION(BlueprintPure, Category="Online|Voice")
	static bool CanPlayerBeMuted(AILLPlayerController* Listener, AILLPlayerState* PlayerState);

	/**
	 * Mutes PlayerToMute for PlayerController.
	 *
	 * @param PlayerController Player controller requesting the mute.
	 * @param PlayerToMute Player to be muted.
	 */
	UFUNCTION(BlueprintCallable, Category="Online|Voice")
	static void MutePlayerState(AILLPlayerController* PlayerController, AILLPlayerState* PlayerToMute);

	/**
	 * Un-mutes PlayerToUnmute for PlayerController.
	 *
	 * @param PlayerController Player controller requesting the un-mute.
	 * @param PlayerToUnmute Player to be un-muted.
	 */
	UFUNCTION(BlueprintCallable, Category="Online|Voice")
	static void UnmutePlayerState(AILLPlayerController* PlayerController, AILLPlayerState* PlayerToUnmute);

	/** @return true if PlayerState is muted for Listener. */
	UFUNCTION(BlueprintPure, Category="Online|Voice")
	static bool IsPlayerMuted(AILLPlayerController* Listener, AILLPlayerState* PlayerState);
};
