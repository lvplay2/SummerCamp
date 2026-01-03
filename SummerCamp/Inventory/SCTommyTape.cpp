// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCTommyTape.h"

#include "ILLInventoryComponent.h"

#include "SCTommyTapeDataAsset.h"
#include "SCPlayerState.h"
#include "SCGameMode.h"

ASCTommyTape::ASCTommyTape(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASCTommyTape::AddedToInventory(UILLInventoryComponent* Inventory)
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
					if (PlayerState->UnlockedTommyTapes.Contains(TapeData))
					{
						// shuffle the tape to an unowned one.
						if (ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>())
						{
							TArray<TSubclassOf<USCTommyTapeDataAsset>> ActiveTommyTapes = GM->GetActiveTommyTapes();
							int TapeID = 0;
							do
							{
								TapeData = ActiveTommyTapes[TapeID++];
							}
							while (PlayerState->UnlockedTommyTapes.Contains(TapeData) && TapeID < ActiveTommyTapes.Num());
						}
					}
					PlayerState->AddTapeToInventory(TapeData);
				}
			}
		}
	}
}
