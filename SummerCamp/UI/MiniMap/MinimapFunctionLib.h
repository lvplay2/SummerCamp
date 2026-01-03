// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MinimapFunctionLib.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API UMinimapFunctionLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintPure, Category = "TextureData")
	static FColor GetTextureColorAtUV(UTexture2D* texture, FVector2D coord);
	
	UFUNCTION(BlueprintPure, Category = "TextureData")
	static FColor GetTextureColorAtPixel(UTexture2D* texture, FVector2D coord);
};
