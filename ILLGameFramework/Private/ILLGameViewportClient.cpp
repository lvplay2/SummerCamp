// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameViewportClient.h"

#include "Framework/Application/IInputProcessor.h"

#include "ILLPlayerController.h"
#include "ILLPlayerInput.h"

/**
 * @class FILLSlateViewportInputProcessor
 */
class FILLSlateViewportInputProcessor
: public IInputProcessor
{
public:
	FILLSlateViewportInputProcessor(UILLGameViewportClient* InOuterViewport)
	: OuterViewport(InOuterViewport) {}

	virtual ~FILLSlateViewportInputProcessor() {}

	// Begin IInputProcessor interface
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) {}
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) { OuterViewport->LastKeyDownEvent = InKeyEvent; return false; }
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) { return false; }
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) { return false; }
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
	{
		// We use this event instead of overriding FViewportClient::MouseMove in UILLGameViewportClient because that function receives synthetic movements every tick, and doesn't indicate there's a difference
		// No longer using a gamepad, so update that
		if (ULocalPlayer* const TargetPlayer = GEngine->GetLocalPlayerFromControllerId(OuterViewport, MouseEvent.GetUserIndex()))
		{
			if (AILLPlayerController* PC = Cast<AILLPlayerController>(TargetPlayer->PlayerController))
			{
				if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
				{
					// No longer using a game pad
					Input->SetUsingGamepad(false);
				}
			}
		}

		return false;
	}
	// End IInputProcessor interface

protected:
	// Viewport this is contained in
	UILLGameViewportClient* OuterViewport;
};

/**
 * @class FDisableNavigationConfig
 */
class FDisableNavigationConfig
: public FNavigationConfig
{
public:
	FDisableNavigationConfig()
	{
		// Letting UILLUserWidget take the wheel...
		bTabNavigation = false;
		bKeyNavigation = false;
		bAnalogNavigation = false;
	}
};

UILLGameViewportClient::UILLGameViewportClient(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLGameViewportClient::BeginDestroy()
{
	if (!GExitPurge && ViewportInputPreprocessor.IsValid())
	{
		// Remove our Slate input pre-processor
		FSlateApplication::Get().UnregisterAllInputPreProcessors();

		// Resume Slate navigation
		FSlateApplication::Get().SetNavigationConfigFactory([]() { return MakeShareable(new FNavigationConfig); });
	}
	
	Super::BeginDestroy();
}

void UILLGameViewportClient::Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice/* = true*/)
{
	Super::Init(WorldContext, OwningGameInstance, bCreateNewAudioDevice);

	if (FSlateApplication::IsInitialized())
	{
		// Add our Slate input pre-processor
		ViewportInputPreprocessor = MakeShareable(new FILLSlateViewportInputProcessor(this));
		FSlateApplication::Get().RegisterInputPreProcessor(ViewportInputPreprocessor, 0);

		// Nuke Slate navigation, otherwise it will try to take the wheel!
		FSlateApplication::Get().SetNavigationConfigFactory([]() { return MakeShareable(new FDisableNavigationConfig); });
	}
}

void UILLGameViewportClient::PushSlatePreprocessor(TSharedPtr<IInputProcessor> Preprocessor)
{
	check(Preprocessor.IsValid());

	FSlateApplication::Get().RegisterInputPreProcessor(Preprocessor, InputPreprocessorStack.Add(Preprocessor)+1);
}

void UILLGameViewportClient::RemoveSlatePreprocessor(TSharedPtr<IInputProcessor> Preprocessor)
{
	check(Preprocessor.IsValid());

	int Index = INDEX_NONE;
	if (InputPreprocessorStack.Find(Preprocessor, Index))
	{
		// Was it the last element that changed? Then reset the stack
		InputPreprocessorStack.RemoveAt(Index);
		FSlateApplication::Get().UnregisterInputPreProcessor(Preprocessor);
	}
}
