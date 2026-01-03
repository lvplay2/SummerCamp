// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorEmoteSelectWidget.h"

#include "SceneViewport.h"

#include "SCCounselorCharacter.h"
#include "SCEmoteData.h"
#include "SCLocalPlayer.h"
#include "SCPlayerController.h"
#include "SCSaveGameBlueprintLibrary.h"

USCCounselorEmoteSelectWidget::USCCounselorEmoteSelectWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bUseRightStick = false;
}

bool USCCounselorEmoteSelectWidget::Initialize()
{
	if (Super::Initialize())
	{
		OnInitialize();

		// Set the cursor to center screen
		if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
		{
			if (LocalPlayer->ViewportClient)
			{
				if (FSceneViewport* SceneViewport = LocalPlayer->ViewportClient->GetGameViewport())
				{
					FIntPoint Size = SceneViewport->GetSize();
					SceneViewport->SetMouse(Size.X >> 1, Size.Y >> 1);
				}
			}
		}

		return true;
	}

	return false;
}

void USCCounselorEmoteSelectWidget::UpdateEmote(int32 Index)
{
	if (Index == INDEX_NONE)
		return;

	OnUpdateEmote(Index, PendingEmote);
}

void USCCounselorEmoteSelectWidget::PlayEmote(int32 Index)
{
	if (Index == INDEX_NONE)
		return;

	AILLPlayerController* PC = GetOwningILLPlayer();
	if (!PC)
		return;

	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(PC->GetPawn());
	if (!Counselor)
		return;

	const TArray<TSubclassOf<USCEmoteData>> Emotes = USCSaveGameBlueprintLibrary::GetEmotesForCounselor(PC, Counselor->GetClass());
	if (Index < Emotes.Num() && Emotes[Index])
	{
		Counselor->SERVER_PlayEmote(Emotes[Index]);
	}
}

UTexture2D* USCCounselorEmoteSelectWidget::GetEmoteIcon(TSubclassOf<USCEmoteData> EmoteData) const
{
	if (EmoteData)
	{
		if (const USCEmoteData* const Emote = Cast<USCEmoteData>(EmoteData->GetDefaultObject()))
		{
			return Emote->EmoteIcon;
		}
	}

	return nullptr;
}
