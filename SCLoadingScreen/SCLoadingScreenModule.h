// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "CoreUObject.h"
#include "Engine.h"
#include "ModuleInterface.h"

class UGameViewportClient;

class UILLUserWidget;

/**
 * @class ISCLoadingScreenModule
 */
class ISCLoadingScreenModule
: public IModuleInterface
{
public:
	/**
	 * Displays the loading screen manually.
	 * When the MoviePlayer requests to initialize the loading screen it will re-use the same widget instance.
	 *
	 * @param ViewportClient Viewport to attach the loading screen to.
	 * @param LoadingScreenClass Loading screen widget class to use.
	 */
	virtual void DisplayLoadingScreen(UGameViewportClient* ViewportClient, TSubclassOf<UILLUserWidget> LoadingScreenClass) = 0;

	/**
	 * Removes the loading screen if present.
	 *
	 * @param ViewportClient Viewport which had the loading screen attached to it previously.
	 */
	virtual void RemoveLoadingScreen(UGameViewportClient* ViewportClient) = 0;

	/** @return true if a loading screen is being displayed. */
	virtual bool IsDisplayingLoadingScreen() const = 0;

	// Slate input preprocessor for BlockInputForTransitionOut
	TSharedPtr<class FILLNULLSlateViewportInputProcessor> SilentSlatePreprocessor;
};
