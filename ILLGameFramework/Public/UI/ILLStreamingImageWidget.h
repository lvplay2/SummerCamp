// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Image.h"
#include "ILLStreamingImageWidget.generated.h"

class UTexture2D;

/**
 * @class UILLStreamingImageWidget
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLStreamingImageWidget
: public UImage
{
	GENERATED_BODY()

	// Being UWidget Interface
	virtual void SetVisibility(ESlateVisibility InVisibility) override;
	// End UWidget Interface

protected:
	// Texture to apply to the Image brush when loaded / visible
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
	TSoftObjectPtr<UTexture2D> StreamedTexture;

	/** Callback to apply StreamedTexture when loaded */
	void TextureLoaded();
};
