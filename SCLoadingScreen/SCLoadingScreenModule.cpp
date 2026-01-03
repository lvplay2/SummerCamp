// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SCLoadingScreenModule.h"

#include "Engine/GameViewportClient.h"
#include "MoviePlayer.h"
#include "UMG.h"

#include "ILLGameInstance.h"
#include "ILLHUD.h"
#include "ILLMenuStackHUDComponent.h"
#include "ILLGameViewportClient.h"

/**
 * @class FSCLoadingScreenModule
 */
class FSCLoadingScreenModule
: public ISCLoadingScreenModule
{
	// Begin IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override
	{
		return true;
	}
	// End IModuleInterface interface

	// Begin ISCLoadingScreenModule interface
	virtual void DisplayLoadingScreen(UGameViewportClient* ViewportClient, TSubclassOf<UILLUserWidget> LoadingScreenClass) override;
	virtual void RemoveLoadingScreen(UGameViewportClient* ViewportClient) override;
	virtual bool IsDisplayingLoadingScreen() const override { return LoadingScreenInstance.IsValid(); }
	// End ISCLoadingScreenModule interface

protected:
	/** Spawns LoadingScreenInstance when it is not valid. */
	void RecreateLoadingScreen(TSubclassOf<UILLUserWidget> LoadingScreenClass);

	/** Callback to handle the IGameMoviePlayer::OnPrepareLoadingScreen */
	void HandlePrepareLoadingScreen();

	/** Callback to handle loading screen transition out completion. */
	UFUNCTION()
	void OnLoadingScreenFinishedTransitionOut(UILLUserWidget* Widget);

	// Short-load loading screen
	UPROPERTY()
	TSubclassOf<UILLUserWidget> DefaultLoadingScreenClass;

	// Last viewport client passed in
	UPROPERTY()
	UGameViewportClient* LastViewportClient;

	// Loading screen widget instance
	TWeakObjectPtr<UILLUserWidget> LoadingScreenInstance;

	// Widget instance for the fullscreen loading screen
	TMap<TWeakObjectPtr<UGameViewportClient>, TSharedPtr<SWidget>> FullScreenWidgets;

	// Z-order to put the loading screen at, to ensure it is on top of everything
	int32 LoadingScreenZOrder;

	// Did we tell the movie player to show a loading screen?
	bool bStopMovie;
};

IMPLEMENT_GAME_MODULE(FSCLoadingScreenModule, SCLoadingScreen);

void FSCLoadingScreenModule::StartupModule()
{
	LoadingScreenZOrder = INT_MAX;
	bStopMovie = false;

	if (!IsRunningDedicatedServer() && FSlateApplication::IsInitialized())
	{
		DefaultLoadingScreenClass = StaticLoadClass(UILLUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/Loading/ShortLoadScreenWidget.ShortLoadScreenWidget_C"));

		if (IsMoviePlayerEnabled())
		{
			GetMoviePlayer()->OnPrepareLoadingScreen().AddRaw(this, &FSCLoadingScreenModule::HandlePrepareLoadingScreen);
		}
	}
}

void FSCLoadingScreenModule::ShutdownModule()
{
	if (!IsRunningDedicatedServer())
	{
		GetMoviePlayer()->OnPrepareLoadingScreen().RemoveAll(this);
	}
}

void FSCLoadingScreenModule::DisplayLoadingScreen(UGameViewportClient* ViewportClient, TSubclassOf<UILLUserWidget> LoadingScreenClass)
{
	check(ViewportClient);

	LastViewportClient = ViewportClient;
	RecreateLoadingScreen(LoadingScreenClass);

	if (LoadingScreenInstance.IsValid() && !FullScreenWidgets.Contains(ViewportClient))
	{
		// Create a Slate widget to stick in the viewport for now
		TSharedRef<SConstraintCanvas> FullScreenCanvas = SNew(SConstraintCanvas);
		TSharedRef<SWidget> UserSlateWidget = LoadingScreenInstance->TakeWidget();
		FullScreenCanvas->AddSlot()
			.Anchors(FAnchors(0, 0, 1, 1))
			[
				UserSlateWidget
			];
			
		// Add it to the screen now, until the MoviePlayer finally kicks in
		FullScreenWidgets.Add(ViewportClient, FullScreenCanvas);
		ViewportClient->AddViewportWidgetContent(FullScreenCanvas, LoadingScreenZOrder);
	}

	// Spawn an input blocker
	if (!SilentSlatePreprocessor.IsValid())
	{
		if (UILLGameViewportClient* VPC = Cast<UILLGameViewportClient>(ViewportClient))
		{
			SilentSlatePreprocessor = MakeShareable(new FILLNULLSlateViewportInputProcessor());
			VPC->PushSlatePreprocessor(SilentSlatePreprocessor);
		}
	}
}

void FSCLoadingScreenModule::RemoveLoadingScreen(UGameViewportClient* ViewportClient)
{
	check(ViewportClient);

	LastViewportClient = ViewportClient;

	if (LoadingScreenInstance.IsValid())
	{
		// Override transition-out completion then trigger it
		LoadingScreenInstance->OnTransitionCompleteOverride.BindRaw(this, &FSCLoadingScreenModule::OnLoadingScreenFinishedTransitionOut);
		LoadingScreenInstance->PlayTransitionOut();
	}
	else
	{
		OnLoadingScreenFinishedTransitionOut(nullptr);
	}
}

void FSCLoadingScreenModule::RecreateLoadingScreen(TSubclassOf<UILLUserWidget> LoadingScreenClass)
{
	// Fallback to default
	if (!LoadingScreenClass)
	{
		LoadingScreenClass = DefaultLoadingScreenClass;
	}

	if (LoadingScreenInstance.IsValid())
	{
		if (LoadingScreenInstance->GetClass() == LoadingScreenClass)
		{
			// Skip unnecessary recreations
			return;
		}

		if (LastViewportClient)
		{
			// Cleanup the previous viewport addition
			TSharedPtr<SWidget>* ExistingWidget = FullScreenWidgets.Find(LastViewportClient);
			if (ExistingWidget && ExistingWidget->IsValid())
			{
				LastViewportClient->RemoveViewportWidgetContent(ExistingWidget->ToSharedRef());
				FullScreenWidgets.Remove(LastViewportClient);
			}
		}

		// Clean up
		LoadingScreenInstance->SetVisibility(ESlateVisibility::Collapsed);
		LoadingScreenInstance->RemoveFromRoot();
		LoadingScreenInstance.Reset();
	}

	// Create a new one now
	UILLGameInstance* GameInstance = LastViewportClient ? Cast<UILLGameInstance>(LastViewportClient->GetGameInstance()) : nullptr;
	if (GameInstance)
		LoadingScreenInstance = NewObject<UILLUserWidget>(GameInstance, LoadingScreenClass);
	else
		LoadingScreenInstance = NewObject<UILLUserWidget>(GetTransientPackage(), LoadingScreenClass);

	LoadingScreenInstance->bWorldForbidden = true;
	LoadingScreenInstance->AddToRoot();
	check(!LoadingScreenInstance->GetWorld());
	LoadingScreenInstance->Initialize();
}

void FSCLoadingScreenModule::HandlePrepareLoadingScreen()
{
	if (!LoadingScreenInstance.IsValid())
	{
		// Only create a loading screen when none exists in this case
		RecreateLoadingScreen(DefaultLoadingScreenClass);
	}

	if (LoadingScreenInstance.IsValid())
	{
		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.MinimumLoadingScreenDisplayTime = .2f;
		LoadingScreen.WidgetLoadingScreen = LoadingScreenInstance->TakeWidget();
		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
		bStopMovie = true;
	}
}

void FSCLoadingScreenModule::OnLoadingScreenFinishedTransitionOut(UILLUserWidget* Widget)
{
	// Tell the movie player to stop showing it
	if (bStopMovie)
	{
		GetMoviePlayer()->StopMovie();
		bStopMovie = false;
	}
	
	// Cleanup the input blocker
	if (SilentSlatePreprocessor.IsValid())
	{
		if (UILLGameViewportClient* VPC = Cast<UILLGameViewportClient>(LastViewportClient))
		{
			VPC->RemoveSlatePreprocessor(SilentSlatePreprocessor);
		}
		SilentSlatePreprocessor = nullptr;
	}

	// Remove the full screen Slate widget
	TSharedPtr<SWidget>* ExistingWidget = FullScreenWidgets.Find(LastViewportClient);
	if (ExistingWidget && ExistingWidget->IsValid())
	{
		LastViewportClient->RemoveViewportWidgetContent(ExistingWidget->ToSharedRef());
		FullScreenWidgets.Remove(LastViewportClient);
	}

	// Collapse, clean up, reset
	if (LoadingScreenInstance.IsValid())
	{
		LoadingScreenInstance->SetVisibility(ESlateVisibility::Collapsed);
		LoadingScreenInstance->RemoveFromRoot();
		LoadingScreenInstance.Reset();
	}
}
