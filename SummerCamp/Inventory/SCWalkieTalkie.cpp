// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCWalkieTalkie.h"

#include "SCCounselorCharacter.h"
#include "SCGameState.h"
#include "SCInGameHUD.h"
#include "SCPlayerController.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCWalkieTalkie"

ASCWalkieTalkie::ASCWalkieTalkie(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	FriendlyName = LOCTEXT("FriendlyName", "Walkie Talkie");
	InventoryMax = 1;
}

void ASCWalkieTalkie::AttachToCharacter(bool AttachToBody, FName OverrideSocket/* = NAME_None*/)
{
	Super::AttachToCharacter(AttachToBody, OverrideSocket);

	if (ASCCounselorCharacter* CharacterOwner = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
	{
		CharacterOwner->OnPickupWalkieTalkie();

		// Keep track of players with walkie talkies
		if (const UWorld* World = GetWorld())
		{
			if (ASCGameState* GS = World->GetGameState<ASCGameState>())
			{
				GS->WalkieTalkiePlayers.AddUnique(CharacterOwner->GetPlayerState());
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
