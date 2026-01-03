// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerController_SPChallenges.h"

#include "SCSPInGameHUD.h"
#include "SCGameState_SPChallenges.h"
#include "SCObjectiveData.h"

ASCPlayerController_SPChallenges::ASCPlayerController_SPChallenges(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ASCPlayerController_SPChallenges::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState_SPChallenges* GS = Cast<ASCGameState_SPChallenges>(World->GetGameState()))
		{
			GS->Dossier->OnObjectiveCompleted.AddDynamic(this, &ASCPlayerController_SPChallenges::OnObjectiveCompleted);
			GS->Dossier->OnSkullCompleted.AddDynamic(this, &ASCPlayerController_SPChallenges::OnSkullCompleted);
			GS->Dossier->OnSkullFailed.AddDynamic(this, &ASCPlayerController_SPChallenges::OnSkullFailed);
		}
	}
}

void ASCPlayerController_SPChallenges::OnObjectiveCompleted(const FSCObjectiveState ObjectiveState)
{
	if (ASCSPInGameHUD* Hud = Cast<ASCSPInGameHUD>(GetSCHUD()))
	{
		const USCObjectiveData* Objective = ObjectiveState.ObjectiveClass.GetDefaultObject();
		Hud->OnShowObjectiveEvent(Objective->GetObjectiveTitle().ToString());
	}
}

void ASCPlayerController_SPChallenges::OnSkullCompleted(const ESCSkullTypes SkullType)
{
	if (ASCSPInGameHUD* Hud = Cast<ASCSPInGameHUD>(GetSCHUD()))
	{
		Hud->OnSkullCompleted(SkullType);
	}
}

void ASCPlayerController_SPChallenges::OnSkullFailed(const ESCSkullTypes SkullType)
{
	if (ASCSPInGameHUD* Hud = Cast<ASCSPInGameHUD>(GetSCHUD()))
	{
		Hud->OnSkullFailed(SkullType);
	}
}
