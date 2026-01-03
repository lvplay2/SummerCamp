// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCharacter.h"
#include "SCPlayerState.h"
#include "SCMatchEndWidget_SP.h"
#include "SCGameState.h"
#include "SCSinglePlayerBlueprintLibrary.h"
#include "SCDossier.h"
#include "SCChallengeData.h"
#include "SCGameInstance.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.MatchEndWidget_SP"

USCMatchEndWidget_SP::USCMatchEndWidget_SP(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool USCMatchEndWidget_SP::Initialize()
{
	if (Super::Initialize())
	{
		if (UWorld* World = GetWorld())
		{
			if (const USCDossier* Dossier = USCSinglePlayerBlueprintLibrary::GetDossier(World))
			{
				if (const USCChallengeData* const ChallengeData = Dossier->GetActiveChallengeClass().GetDefaultObject())
				{
					XPEmoteUnlock = ChallengeData->XPEmoteUnlock ? ChallengeData->XPEmoteUnlock.GetDefaultObject() : nullptr;
					KillsEmoteUnlock = ChallengeData->XPEmoteUnlock ? ChallengeData->AllKillEmoteUnlock.GetDefaultObject() : nullptr;
					StealthEmoteUnlock = ChallengeData->XPEmoteUnlock ? ChallengeData->StealthEmoteUnlock.GetDefaultObject() : nullptr;

				}
			}
		}

		if (!XPEmoteUnlock || !KillsEmoteUnlock || !StealthEmoteUnlock)
		{
			DataLoadFailed();
			return false;
		}

		DataLoaded();
		return true;
	}

	DataLoadFailed();
	return false;
}

#undef LOCTEXT_NAMESPACE
