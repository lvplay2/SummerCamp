// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Engine/GameViewportClient.h"
#include "ILLGameViewportClient.generated.h"

class IInputProcessor;

/**
 * @class UILLGameViewportClient
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLGameViewportClient
: public UGameViewportClient
{
	GENERATED_UCLASS_BODY()

	// Begin UObject Interface
	virtual void BeginDestroy() override;
	// End UObject Interface

	// Begin UGameViewportClient Interface
	virtual void Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice = true) override;
	// End UGameViewportClient Interface

	/** Pushes Preprocessor to the stop of the Slate input preprocessor stack. */
	virtual void PushSlatePreprocessor(TSharedPtr<IInputProcessor> Preprocessor);

	/** Removes preprocessor from anywhere in the stack and resets the active input preprocessor when Preprocessor was the last element. */
	virtual void RemoveSlatePreprocessor(TSharedPtr<IInputProcessor> Preprocessor);

	// Last key down event sent to this game viewport
	FKeyEvent LastKeyDownEvent;

protected:
	// Base Slate input preprocessor instance
	TSharedPtr<IInputProcessor> ViewportInputPreprocessor;

	// Slate input preprocessor stack
	TArray<TSharedPtr<IInputProcessor>> InputPreprocessorStack;
};
