// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "SCCharacterSelectionsSaveGame.h"
#include "SCSaveGameBlueprintLibrary.generated.h"

class ASCCounselorCharacter;
class USCEmoteData;

/**
 * @class USCSaveGameBlueprintLibrary
 */
UCLASS()
class SUMMERCAMP_API USCSaveGameBlueprintLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	//////////////////////////////////////////////////
	// Save game access
public:
	/**
	 * Retreives Counselor Profiles from Save Game data
	 *
	 * @param PlayerController Local Player Controller
	 * @return Array of Counselor Profiles
	 */
	UFUNCTION(BlueprintCallable, Category="Menu|Save Game")
	static TArray<FSCCounselorCharacterProfile> GetCounselorProfiles(AILLPlayerController* PlayerController);

	/**
	 * Retreives Counselor Profile from Save Game data
	 *
	 * @param PlayerController Local Player Controller
	 * @param CounselorClass The Counselor Class we want a Profile for
	 * @return The Counselor Profile for the passed in Counselor Class
	 */
	UFUNCTION(BlueprintCallable, Category="Menu|Save Game")
	static FSCCounselorCharacterProfile GetCounselorProfile(AILLPlayerController* PlayerController, TSoftClassPtr<ASCCounselorCharacter> CounselorClass);

	/**
	 * Saves a Counselor Character Profile
	 *
	 * @param PlayerController Local Player Controller
	 * @param CounselorProfile The Profile to save
	 */
	UFUNCTION(BlueprintCallable, Category="Menu|Save Game")
	static void SaveCounselorProfile(AILLPlayerController* PlayerController, const FSCCounselorCharacterProfile& CounselorProfile);

	UFUNCTION(BlueprintCallable, Category="Menu|Save Game")
	static TArray<TSubclassOf<USCEmoteData>> GetEmotesForCounselor(AILLPlayerController* PlayerController, TSoftClassPtr<ASCCounselorCharacter> CounselorClass);

	/** 
	 * returns if this player has custom keybinds set up
	 * 
	 * @param PlayerController Local Player Controller
	 */
	UFUNCTION(BlueprintCallable, Category="Menu|Save Game")
	static bool HasCustomKeybinding(AILLPlayerController* PlayerController);
};
