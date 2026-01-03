// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMenuWidget.h"
#include "SCPlayerState.h"
#include "SCChallengeLobbyWidget.generated.h"

class USCDossier;
class USCModeDefinition;
class USCMapRegistry;
class USCChallengeData;

/**
* @class USCChallengeLobbyWidget
*/
UCLASS()
class SUMMERCAMP_API USCChallengeLobbyWidget
: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

public:

	// Begin UUserWidget interface
	virtual bool Initialize() override;
	// Begin UUserWidget interface

	// Called When Start button is selected.
	UFUNCTION(BlueprintCallable, Category = "Challenge Data")
	void BeginChallenge();

	// Return the active Dossier
	UFUNCTION(BlueprintCallable, Category = "Challenge Data")
	USCDossier* GetCurrentDossier() const;

	UFUNCTION(BlueprintCallable, Category = "Player State")
	ASCPlayerState* GetActivePlayerState() const;

	// All challenges sorted by index
	UPROPERTY(BlueprintReadOnly, Category = "Challenge Data")
	TArray<TSubclassOf<USCChallengeData>> SortedChallenges;

	// Selected challenge index
	UPROPERTY(BlueprintReadWrite, Category = "Challenge Data")
	int32 CurrentChallengeIndex;

private:

	// MapRegistry class reference
	UPROPERTY(EditDefaultsOnly, Category = "Map Data")
	TSubclassOf<USCMapRegistry> MapRegistryClass;

	// Mode definition for challenge game mode
	UPROPERTY(EditDefaultsOnly, Category = "Map Data")
	TSubclassOf<USCModeDefinition> ChallengeGameModeClass;

	// Get the map ID for the selected challenge
	UFUNCTION()
	FString GetSelectedLobbyMap() const;

	// Get the game mode ID for the selected challenge
	UFUNCTION()
	FString GetSelectedGameMode() const;

	// Get any level loading options for the selected challenge
	UFUNCTION()
	FString GetLevelLoadOptions() const;
};