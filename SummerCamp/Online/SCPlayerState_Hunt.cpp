// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerState_Hunt.h"

#include "SCPlayerController.h"

ASCPlayerState_Hunt::ASCPlayerState_Hunt(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ASCPlayerState_Hunt::SetUniqueId(const FUniqueNetIdRepl& InUniqueId)
{
	Super::SetUniqueId(InUniqueId);

	if (Role == ROLE_Authority && UniqueId.IsValid())
	{
		// Get this player's platform achievements ready
		QueryAchievements();
	}
}

bool ASCPlayerState_Hunt::ServerSetSpawnPreference_Validate(ESCCharacterPreference InSpawnPreference)
{
	return true;
}

void ASCPlayerState_Hunt::ServerSetSpawnPreference_Implementation(ESCCharacterPreference InSpawnPreference)
{
	SpawnPreference = InSpawnPreference;
}

void ASCPlayerState_Hunt::OnRevivePlayer()
{
	Super::OnRevivePlayer();

	SpawnedCharacterClass = nullptr;
}
