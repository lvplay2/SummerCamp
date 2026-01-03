// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine/DataAsset.h"
#include "SCMapDefinition.generated.h"

/**
 * @class USCMapDefinition
 */
UCLASS(Abstract, MinimalAPI, const, Blueprintable, BlueprintType)
class USCMapDefinition 
: public UDataAsset
{
	GENERATED_UCLASS_BODY()

	// Display name for the map
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FText DisplayName;

	// Path to load the map
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta=(RelativeToGameContentDir, LongPackageName))
	FFilePath Path;

	// Thumbnail images to use for this map in the lobby
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TArray<TSoftObjectPtr<UTexture2D>> LobbyThumbnailImages;

	/** @return LobbyThumbnailImages for a MapDefinitionClass. */
	UFUNCTION(BlueprintPure, Category="MapDefinition")
	static TArray<UTexture2D*> GetLobbyThumbnailImages(TSubclassOf<USCMapDefinition> MapDefinitionClass)
	{
		TArray<UTexture2D*> OutData;
		if (MapDefinitionClass)
		{
			for (TSoftObjectPtr<UTexture2D> Image : MapDefinitionClass->GetDefaultObject<USCMapDefinition>()->LobbyThumbnailImages)
			{
				OutData.Add(Image.LoadSynchronous());
			}
		}

		return OutData;
	}
};
