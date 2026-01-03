// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPamelaTape.h"

#include "ILLInventoryComponent.h"

#include "SCPamelaTapeDataAsset.h"
#include "SCPlayerState.h"
#include "SCGameMode.h"

ASCPamelaTape::ASCPamelaTape(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASCPamelaTape::AddedToInventory(UILLInventoryComponent* Inventory)
{
	Super::AddedToInventory(Inventory);

	if (!Inventory)
		return;

	if (Role == ROLE_Authority)
	{
		if (AILLCharacter* Character = Inventory->GetCharacterOwner())
		{
			if (AController* Controller = Character->GetController())
			{
				if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(Controller->PlayerState))
				{
					if (PlayerState->UnlockedPamelaTapes.Contains(TapeData))
					{
						// shuffle the tape to an unowned one.
						if (ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>())
						{
							TArray<TSubclassOf<USCPamelaTapeDataAsset>> ActivePamelaTapes = GM->GetActivePamelaTapes();
							int TapeID = 0;
							do
							{
								TapeData = ActivePamelaTapes[TapeID++];
							}
							while (PlayerState->UnlockedPamelaTapes.Contains(TapeData) && TapeID < ActivePamelaTapes.Num());
						}
					}
					PlayerState->AddTapeToInventory(TapeData);
				}
			}
		}
	}
}
