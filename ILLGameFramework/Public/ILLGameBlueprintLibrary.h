// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "ILLBuildDefines.h"
#include "ILLInputMappingTypes.h"

#include "ILLGameBlueprintLibrary.generated.h"

class AILLPlayerController;

/**
 * @class UILLGameBlueprintLibrary
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLGameBlueprintLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Typically queried at the start of the Entry level.
	 * Always returns true when there is no platform login.
	 * When not in user-requested offline mode, verifies backend authentication too.
	 *
	 * @return true if the initial title screen should be displayed instead of the main menu.
	 */
	UFUNCTION(BlueprintPure, Category="UserLogin")
	static bool ShouldShowTitleScreen(AILLPlayerController* PlayerController);

	/** @return true if the local player for the PlayerController is or will be loading a save game. Useful for blocking save game dependent operations until they are loaded. */
	UFUNCTION(BlueprintPure, Category="SaveGame")
	static bool IsLoadingSaveGame(AILLPlayerController* PlayerController);

	/** @return true if the local player for the PlayerController has an active save game thread (reading or writing). */
	UFUNCTION(BlueprintPure, Category="SaveGame")
	static bool IsSaveGameThreadActive(AILLPlayerController* PlayerController);

	//////////////////////////////////////////////////
	// General object management
public:
	/** Marks an object pending kill (garbage collection). */
	UFUNCTION(BlueprintCallable, Category="Object|Advanced")
	static void MarkObjectPendingKill(UObject* Object);

	//////////////////////////////////////////////////
	// String utility
public:
	/** Formats a time in seconds to a string in the format "00:00:00". Defaults to not showing hours/minutes when TimeSeconds is less than that. */
	UFUNCTION(BlueprintCallable, Category="String")
	static FString TimeSecondsToTimeString(const int32 TimeSeconds, const bool bAlwaysShowMinute = false, const bool bAlwaysShowHour = false); // CLEANUP: pjackson: Make BlueprintPure
	
	//////////////////////////////////////////////////
	// Input
public:
	/**
	 * Returns true when the last input was from a gamepad, otherwise the input is from keyboard/mouse
	 * @param PlayerController - Player controller to check input type on
	 * @return Returns false if the player is using keyboard/mouse OR player controller or input is null, true only when they're using a gamepad
	 */
	UFUNCTION(BlueprintPure, Category="Input")
	static bool IsUsingGamepad(APlayerController* PlayerController);

	//////////////////////////////////////////////////
	// Player controllers
public:
	/** @return First local player controller found in WorldContextObject.*/
	UFUNCTION(BlueprintPure, Category="Player", meta=(WorldContext="WorldContextObject"))
	static AILLPlayerController* GetFirstLocalPlayerController(UObject* WorldContextObject);

	//////////////////////////////////////////////////////////////////////////
	// Friends
public:
	/** @return true if this PlayerState is friends with the passed in player state (friendship is not always bi-direcitonal!) */
	UFUNCTION(BlueprintPure, Category="Social")
	static bool IsFriendsWith(APlayerController* LocalPlayerController, APlayerState* OtherPlayerState);

	//////////////////////////////////////////////////
	// Actor utility functions
public:
	/** @return ActorList sorted so that the closest actors are at the beginning of the list. */
	UFUNCTION(BlueprintCallable, Category="Actor")
	static TArray<AActor*> SortActorsByDistanceTo(const TArray<AActor*>& ActorList, AActor* OtherActor);
	
	//////////////////////////////////////////////////
	// UI functions
public:
	/** @return Cached estimated loading percentage in [0-100] percentage form. -1 for unknown. */
	UFUNCTION(BlueprintPure, Category="LoadingScreen")
	static int32 GetEstimatedLevelLoadPercentage();

	/** Calls to UILLGameInstance::ShowLoadingWidget. */
	UFUNCTION(BlueprintCallable, Category="LoadingScreen", meta=(WorldContext="WorldContextObject"))
	static void ShowLoadingWidget(UObject* WorldContextObject, TSubclassOf<UILLUserWidget> LoadingScreenWidget);

	/** Calls to UILLGameInstance::HideLoadingScreen. */
	UFUNCTION(BlueprintCallable, Category="LoadingScreen", meta=(WorldContext="WorldContextObject"))
	static void HideLoadingScreen(UObject* WorldContextObject);

	//////////////////////////////////////////////////
	// Travel
public:
	/**
	 * Display a loading screen then travel.
	 * When server: ServerTravel and notify all ILLPlayerControllers to display their loading screens as well as you.
	 * When client: ClientTravel and display the loading screen for yourself.
	 *
	 * @param WorldContextObject World context object.
	 * @param TravelPath Path to the level.
	 * @param bAbsolute Absolute travel?
	 * @param Options Travel options.
	 * @param bForceClientTravel Force client travel, dropping connected clients? Only affects listen and dedicated servers.
	 */
	UFUNCTION(BlueprintCallable, Category="Online|Travel", meta=(WorldContext="WorldContextObject"))
	static void ShowLoadingAndTravel(UObject* WorldContextObject, const FString& TravelPath, bool bAbsolute = true, FString Options = FString(TEXT("")), bool bForceClientTravel = false);

	/**
	 * Display a loading screen then load the main menu.
	 * Notifies clients that the host closed the game when called on a server PlayerController.
	 *
	 * @param PlayerController Player wanting to return to the main menu.
	 * @param ReturnReason Kick / failure reason for returning.
	 */
	UFUNCTION(BlueprintCallable, Category="Online|Travel")
	static void ShowLoadingAndReturnToMainMenu(AILLPlayerController* PlayerController, FText ReturnReason);
};
