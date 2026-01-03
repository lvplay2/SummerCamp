// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine/DataAsset.h"
#include "SCStengthOrWeaknessDataAsset.generated.h"

/**
 * 
 */
UCLASS(MinimalAPI, const, Blueprintable, BlueprintType)
class USCStengthOrWeaknessDataAsset
	: public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Title;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UTexture2D* Icon;
};
