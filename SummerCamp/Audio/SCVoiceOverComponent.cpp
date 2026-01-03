// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCVoiceOverComponent.h"

#include "SCCharacter.h"

USCVoiceOverComponent::USCVoiceOverComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCVoiceOverComponent::PlayVoiceOver(const FName VoiceOverName, bool CinematicKill/*=false*/, bool Queue /*= false*/)
{
}

void USCVoiceOverComponent::PlayVoiceOverBySoundCue(USoundCue *SoundCue)
{
}

void USCVoiceOverComponent::StopVoiceOver()
{
}

bool USCVoiceOverComponent::IsVoiceOverPlaying(const FName VoiceOverName)
{
	return false;
}

void USCVoiceOverComponent::CleanupVOComponent()
{
}

bool USCVoiceOverComponent::IsOnCooldown(const FName VOName)
{
	// No world? No time.
	UWorld* World = GetWorld();
	if (!World)
		return false;

	const float CurrentTime = World->GetTimeSeconds();

	bool bOnCooldown = false;
	for (int32 iVO(0); iVO < VOCooldownList.Num(); )
	{
		TPair<FName, float>& VOPair = VOCooldownList[iVO];
		if (VOPair.Value <= CurrentTime)
		{
			VOCooldownList.RemoveAtSwap(iVO);
			continue;
		}

		 ++iVO; // Only increment if we're leaving this VO on cooldown

		// Still in the list? We're on cooldown
		if (VOPair.Key == VOName)
			bOnCooldown = true;
	}

	return bOnCooldown;
}

void USCVoiceOverComponent::StartCooldown(const FName VOName, const float CooldownTime)
{
	// Already on cooldown, don't do anything
	if (IsOnCooldown(VOName))
		return;

	if (UWorld* World = GetWorld())
	{
		const float EndTimestamp = World->GetTimeSeconds() + CooldownTime;
		VOCooldownList.Add(TPair<FName, float>(VOName, EndTimestamp));
	}
}

bool USCVoiceOverComponent::IsLocallyControlled() const
{
	return (GetCharacterOwner() ? GetCharacterOwner()->IsLocallyControlled() : false);
}

ASCCharacter* USCVoiceOverComponent::GetCharacterOwner() const
{
	return Cast<ASCCharacter>(GetOwner());
}
