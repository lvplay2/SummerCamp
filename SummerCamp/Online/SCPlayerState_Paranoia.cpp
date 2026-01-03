// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerState_Paranoia.h"
#include "SCPlayerController_Paranoia.h"


ASCPlayerState_Paranoia::ASCPlayerState_Paranoia(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASCPlayerState_Paranoia::HandleParanoiaPointEvent(const USCParanoiaScore* ScoreEvent, const float ScoreModifier /*= 1.f*/)
{
	check(ScoreEvent);

	if (ScoreEvent->bOnlyScoreOnce && PlayerPointEvents.Contains(ScoreEvent))
		return;

	const int32 AddedPts = ScoreEvent->GetModifiedScore(ScoreModifier, IsActiveCharacterKiller() ? TArray<USCPerkData*>() : ActivePerks);
	if (ASCPlayerController* OwnerController = Cast<ASCPlayerController>(GetOwner()))
	{
		FSCCategorizedScoreEvent* ExistingEvent = CategorizedPointEvents.FindByPredicate(
			[&](FSCCategorizedScoreEvent inEvent)
			{
				return inEvent.Category == ScoreEvent->EventCategory && inEvent.bAsHunter == GetIsHunter();
			}
			);

		if (ExistingEvent)
		{
			ExistingEvent->TotalScore += AddedPts;
		}
		else
		{
			CategorizedPointEvents.Add(FSCCategorizedScoreEvent(ScoreEvent->EventCategory, AddedPts, bIsHunter));
		}

		if (ASCPlayerController_Paranoia* PCP = Cast<ASCPlayerController_Paranoia>(OwnerController))
			PCP->ClientDisplayPointsEvent(ScoreEvent->GetClass(), (uint8)ScoreEvent->EventCategory, ScoreModifier);
	}

	PlayerPointEvents.Add(ScoreEvent);
	AccumulatedScore += AddedPts;
}

void ASCPlayerState_Paranoia::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCPlayerState_Paranoia, AccumulatedScore);
}
