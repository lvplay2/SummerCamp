// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerState.h"
#include "SCPamelaTapesWidget.h"
#include "SCPamelaTapeDataAsset.h"
#include "SCTommyTapeDataAsset.h"

USCPamelaTapesWidget::USCPamelaTapesWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool USCPamelaTapesWidget::IsPamelaTapeUnlocked(TSubclassOf<class USCPamelaTapeDataAsset> PamelaTape)
{
	if (PamelaTape)
	{
		if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
		{
			for (int i = 0; i < PlayerState->UnlockedPamelaTapes.Num(); ++i)
			{
				if (PlayerState->UnlockedPamelaTapes[i] == PamelaTape)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool USCPamelaTapesWidget::IsTommyTapeUnlocked(TSubclassOf<class USCTommyTapeDataAsset> TommyTape)
{
	if (TommyTape)
	{
		if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
		{
			for (int i = 0; i < PlayerState->UnlockedTommyTapes.Num(); ++i)
			{
				if (PlayerState->UnlockedTommyTapes[i] == TommyTape)
				{
					return true;
				}
			}
		}
	}
	return false;
}


