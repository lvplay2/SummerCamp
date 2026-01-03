// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCharacter.h"
#include "SCPlayerState.h"
#include "SCMatchEndLobbyWidget.h"
#include "SCGameState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.MatchEndLobbyWidget"

USCMatchEndLobbyWidget::USCMatchEndLobbyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CachedPlayerLevel(1)
	, AccumulatedScore(1)
	, ProcessedHunterScore(false)
	, DisplayPlayerScoreDelay(2.f)
	, DisplayHunterScoreDelay(3.f)
	, LevelUpDisplayDelay(0.5f)
{
}

void USCMatchEndLobbyWidget::RemoveFromParent()
{
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void USCMatchEndLobbyWidget::Native_UpdateScoreEvents(const TArray<FSCCategorizedScoreEvent>& ScoreEvents, const int32& InOldPlayerLevel, const TArray<FSCBadgeAwardInfo>& BadgesAwarded)
{
	CachedPlayerLevel = InOldPlayerLevel;
	CachedBadgesAwarded = BadgesAwarded;
	OnUpdateScoreEvents(ScoreEvents, InOldPlayerLevel, BadgesAwarded);
	if (ScoreEvents.Num() > 0)
	{
		CachedScoreEvents = ScoreEvents;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_EndMatchAnimTimer, this, &USCMatchEndLobbyWidget::DisplayScore, DisplayPlayerScoreDelay, false);
	}
}

void USCMatchEndLobbyWidget::DisplayScore()
{
	DisplayPlayerScore(CachedScoreEvents, false);
}

void USCMatchEndLobbyWidget::DisplayPlayerScore(const TArray<FSCCategorizedScoreEvent>& ScoreEvents, bool AsHunter)
{
	float delayCounter = 1.0f;
	AccumulatedScore = 0;
	for (FSCCategorizedScoreEvent Event : ScoreEvents)
	{
		if (Event.bAsHunter != AsHunter)
			continue;

		AccumulatedScore += Event.TotalScore;
		OnAddMatchEndLine(NSCScoreEvents::ToString(Event.Category), FText::AsNumber(Event.TotalScore), delayCounter++);
	}

	UWorld* World = GetWorld();
	if (World)
	{
		if (ASCGameState* GameState = Cast<ASCGameState>(World->GetGameState()))
		{
			if (GameState->XPModifier != 1)
			{
				AccumulatedScore *= GameState->XPModifier;
				OnAddXPBonusLine(FText::Format(LOCTEXT("XPBonusString", "x{0}"), FText::AsNumber(GameState->XPModifier)), delayCounter);
			}
		}

		AccumulatedScore = FMath::Clamp(AccumulatedScore, 0, FMath::Abs(AccumulatedScore));
		World->GetTimerManager().SetTimer(TimerHandle_EndMatchAnimTimer, this, &USCMatchEndLobbyWidget::ShowTotal, delayCounter * 0.2f, false);
	}
}

void USCMatchEndLobbyWidget::ShowTotal()
{
	OnUpdateTotalScore(FText::Format(LOCTEXT("TotalScoreString", "{0} xp"), FText::AsNumber(AccumulatedScore)));
	CheckLevelUp();
}

void USCMatchEndLobbyWidget::CheckLevelUp()
{
	if (LocalPlayerState->GetPlayerLevel() <= CachedPlayerLevel)
		return;

	if (LocalPlayerState->GetIsHunter())
	{
		if (!ProcessedHunterScore)
			return;
	}

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_EndMatchAnimTimer, this, &USCMatchEndLobbyWidget::PlayLevelUp, LevelUpDisplayDelay, false);
}

void USCMatchEndLobbyWidget::GetCharacterDetails(TSoftClassPtr<ASCCharacter> CharacterClass, FText& CharacterName, FText& SecondaryName, UTexture2D*& LargeImage) const
{
	if (!CharacterClass.IsNull())
	{
		CharacterClass.LoadSynchronous(); // TODO: Make Async
		if (const ASCCharacter* const Character = Cast<ASCCharacter>(CharacterClass.Get()->GetDefaultObject()))
		{
			CharacterName = Character->GetCharacterName();
			LargeImage = Character->GetLargeImage();
			SecondaryName = Character->GetCharacterTropeName();
		}
	}
}

void USCMatchEndLobbyWidget::Native_DisplayUnlocks()
{
	for (TSoftClassPtr<ASCCharacter> Character : UnlockableCharacters)
	{
		if (WasCharacterJustUnlocked(Character))
		{
			UnlockedCharacter = Character;
			break;
		}
	}

	OnDisplayUnlock(CachedBadgesAwarded);
}

bool USCMatchEndLobbyWidget::WasCharacterJustUnlocked(TSoftClassPtr<ASCCharacter> CharacterClass)
{
	bool WasJustUnlocked = false;
	if (!CharacterClass.IsNull())
	{
		if (ASCPlayerState *PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
		{
			CharacterClass.LoadSynchronous(); // TODO: Make Async
			if (const ASCCharacter* const Character = Cast<ASCCharacter>(CharacterClass.Get()->GetDefaultObject()))
			{
				if (PlayerState->GetPlayerLevel() == Character->UnlockLevel)
				{
					WasJustUnlocked = true;
				}
			}
		}
	}
	return WasJustUnlocked;
}

void USCMatchEndLobbyWidget::SetLocalPlayer(ASCPlayerState* InPlayerState)
{
	if (!IsValid(InPlayerState))
		return;

	LocalPlayerState = InPlayerState;
	OnPlayerStateAcquired(LocalPlayerState);
}

ASCPlayerState* USCMatchEndLobbyWidget::GetLocalPlayerState()
{
	return LocalPlayerState;
}

bool USCMatchEndLobbyWidget::GetIsHunter()
{
	return LocalPlayerState ? LocalPlayerState->GetIsHunter() : false;
}

void USCMatchEndLobbyWidget::Native_OnNext()
{
	OnNext();
	if (LocalPlayerState->GetIsHunter() && !ProcessedHunterScore)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_EndMatchAnimTimer, this, &USCMatchEndLobbyWidget::ShowHunterScore, DisplayHunterScoreDelay, false);
		return;
	}

	Native_DisplayUnlocks();
}

void USCMatchEndLobbyWidget::ShowHunterScore()
{
	DisplayPlayerScore(CachedScoreEvents, true);
	ProcessedHunterScore = true;
}

#undef LOCTEXT_NAMESPACE