// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlayerController.h"
#include "ILLHUD.h"
#include "ILLMinimapIconComponent.h"

UILLMinimapIconComponent::UILLMinimapIconComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bStartVisible(true)
{
}

void UILLMinimapIconComponent::BeginPlay()
{
	Super::BeginPlay();

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if ((*Iterator)->IsLocalPlayerController())
		{
			if (AILLPlayerController* PC = Cast<AILLPlayerController>((*Iterator)))
			{
				if (AILLHUD* HUD = Cast<AILLHUD>(PC->GetHUD()))
				{
					HUD->RegisterMinimapIcon(this);
				}
			}
		}
	}

	SetVisibility(bStartVisible);
}

void UILLMinimapIconComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			if ((*Iterator)->IsLocalPlayerController())
			{
				if (AILLPlayerController* PC = Cast<AILLPlayerController>((*Iterator)))
				{
					if (AILLHUD* HUD = Cast<AILLHUD>(PC->GetHUD()))
					{
						HUD->UnregisterMinimapIcon(this);
					}
				}
			}
		}
	}
}
