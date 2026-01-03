// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine/DataAsset.h"
#include "SCMapDefinition.h"
#include "SCModeDefinition.generated.h"

/**
 * @class USCModeDefinition
 */
UCLASS(Abstract, MinimalAPI, const, Blueprintable, BlueprintType)
class USCModeDefinition 
: public UDataAsset
{
	GENERATED_UCLASS_BODY()

	// Game mode alias to assign the game param on travel
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FString Alias;

	// Display name for the game mode
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FText DisplayName;

	// Maps that support this game mode
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TArray<TSubclassOf<USCMapDefinition>> SupportedMaps;
};
