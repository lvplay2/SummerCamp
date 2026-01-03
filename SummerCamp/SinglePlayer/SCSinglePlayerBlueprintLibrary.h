// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGameBlueprintLibrary.h"
#include "SCChallengeData.h"
#include "SCMapRegistry.h"
#include "SCSinglePlayerSaveGame.h"
#include "SCSinglePlayerBlueprintLibrary.generated.h"

class USCDossier;

/**
 * @class USCSinglePlayerBlueprintLibrary
 */
UCLASS()
class SUMMERCAMP_API USCSinglePlayerBlueprintLibrary
: public UILLGameBlueprintLibrary
{
	GENERATED_BODY()

	//////////////////////////////////////////////////
	// Dossier
public:
	/** Calls complete objective on the active dossier */
	UFUNCTION(BlueprintCallable, Category = "Dossier", meta=(WorldContext="WorldContextObject"))
	static void CompleteObjective(UObject* WorldContextObject, const TSubclassOf<USCObjectiveData> ObjectiveClass);

	/** Gets the current dossier from the game state (will be null in non-single player game states) */
	UFUNCTION(BlueprintCallable, Category = "Dossier", meta=(WorldContext="WorldContextObject"))
	static USCDossier* GetDossier(UObject* WorldContextObject);

	/** Gets the highest completed challenge index from the local player, -1 if none is found, INT_MAX if sc.UnlockAllChallenges is on */
	UFUNCTION(BlueprintPure, Category = "Dossier", meta=(WorldContext="WorldContextObject"))
	static int32 GetHighestCompletedChallengeIndex(UObject* WorldContextObject);

	/** Helper for grabbing the default map registry BP class */
	UFUNCTION()
	static TSubclassOf<USCMapRegistry> GetDefaultMapRegistryClass();

	/** Calls GetChallengeByID using the default registry BP class (see GetDefaultMapRegistryClass) and the default single player game mode */
	UFUNCTION()
	static TSubclassOf<USCChallengeData> GetChallengeByID(const FString& ChallengeID);

	/** Returns true if the given emote ID has already been earned locally */
	UFUNCTION(BlueprintPure, Category = "Dossier", meta=(WorldContext="WorldContextObject"))
	static bool HasEmoteUnlocked(UObject* WorldContextObject, FName InEmoteID);

	/** Returns true if the player has viewed any of the single player tutorial screens */
	UFUNCTION(BlueprintPure, Category = "Single Player", meta=(WorldContext="WorldContextObject"))
	static bool HasViewedSPTutorial(UObject* WorldContextObject);

	/** Notifies the save game that we have viewed the Challenges tutorial */
	UFUNCTION(BlueprintCallable, Category = "Single Player", meta=(WorldContext="WorldContextObject"))
	static void SetViewedSPTutorial(UObject* WorldContextObject);

	//////////////////////////////////////////////////
	// DataLoad
public:
	/** Request the single player challenge data be loaded with a callback when complete */
	UFUNCTION(BlueprintCallable, Category = "Single Player", meta=(WorldContext="WorldContextObject"))
	static void RequestChallengeProfileLoad(UObject* WorldContextObject, const FSCChallengesLoadedDelegate& ProfileLoadCallback);

	//////////////////////////////////////////////////
	// Design Helpers
public:
	UFUNCTION(BlueprintCallable, Category = "Mesh")
	static void ClearAllDynamicMaterialParamaters(UPrimitiveComponent* Mesh);

	/** Setup generic stuff when beginning a super cinematic kill
	* @param Player - Local Player Controller
	* @param Killer - Active Killer character
	* @param Victim - The character being killed
	* This function will disable local input, remove ability and sound screen effects, and kill and VO or conversation the victim is in */
	UFUNCTION(BlueprintCallable, Category = "Single Player")
	static void BeginSuperCinematicKill(AILLPlayerController* KillerController, TArray<ASCCharacter*> Victims);

	/** Revert generic stuff when ending a super cinematic kill
	* @param Player - Local Player Controller
	* @param Killer - Active Killer character
	* @param Victim - The character being killed
	* @param DamageType - The damage type to be used when killing the victim
	* This function will re-enable local input, allow sound screen effects, and kill the victim given the specified damage type */
	UFUNCTION(BlueprintCallable, Category = "Single Player")
	static void EndSuperCinematicKill(AILLPlayerController* KillerController, TArray<ASCCharacter*> Victims, TSubclassOf<UDamageType> DamageType);

};
