// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCChallengeLobbyWidget.h"

#include "SCChallengeData.h"
#include "SCChallengeMapDefinition.h"
#include "SCDossier.h"
#include "SCLocalPlayer.h"
#include "SCMapRegistry.h"
#include "SCSinglePlayerBlueprintLibrary.h"

#include "ILLGameBlueprintLibrary.h"

USCChallengeLobbyWidget::USCChallengeLobbyWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

bool USCChallengeLobbyWidget::Initialize()
{
	bool Initialized = Super::Initialize();

	// Get our challenge list
	SortedChallenges = USCMapRegistry::GetSortedChallenges(MapRegistryClass, ChallengeGameModeClass);

	return Initialized;
}

void USCChallengeLobbyWidget::BeginChallenge()
{
	const FString MapPath = GetSelectedLobbyMap();
	const FString GameModeName = GetSelectedGameMode();
	const FString LevelLoadOptions = GetLevelLoadOptions();

	if (!MapPath.IsEmpty())
		UILLGameBlueprintLibrary::ShowLoadingAndTravel(this, MapPath, true, *FString::Printf(TEXT("game=%s%s"), *GameModeName, *LevelLoadOptions));
	else
		UILLGameBlueprintLibrary::ShowLoadingAndReturnToMainMenu(GetOwningILLPlayer(), FText());
}

USCDossier* USCChallengeLobbyWidget::GetCurrentDossier() const
{
	if (UWorld* World = GetWorld())
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (USCLocalPlayer* LocalPlayer = PlayerController ? Cast<USCLocalPlayer>(PlayerController->GetLocalPlayer()) : nullptr)
		{
			if (USCSinglePlayerSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>())
				return SaveGame->GetChallengeDossierByClass(SortedChallenges[CurrentChallengeIndex], false);
		}
	}

	return nullptr;
}

ASCPlayerState* USCChallengeLobbyWidget::GetActivePlayerState() const
{
	if (UWorld* World = GetWorld())
	{
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(World->GetFirstPlayerController() == nullptr ? nullptr : World->GetFirstPlayerController()->PlayerState))
		{
			return PS;
		}
	}
	
	return nullptr;
}

FString USCChallengeLobbyWidget::GetSelectedLobbyMap() const
{
	USCDossier* Dossier = GetCurrentDossier();
	if (!IsValid(Dossier))
		return FString();

	TSubclassOf<USCChallengeMapDefinition> MapDefinition = USCMapRegistry::GetMapDefinitionFromChallenge(MapRegistryClass, Dossier->GetActiveChallengeClass(), ChallengeGameModeClass);

	return MapDefinition ? MapDefinition->GetDefaultObject<USCChallengeMapDefinition>()->Path.FilePath : FString();
}

FString USCChallengeLobbyWidget::GetSelectedGameMode() const
{
	if (ChallengeGameModeClass)
	{
		return ChallengeGameModeClass->GetDefaultObject<USCModeDefinition>()->Alias;
	}
	else
	{
		if (USCMapRegistry* DefaultRegistry = MapRegistryClass ? MapRegistryClass->GetDefaultObject<USCMapRegistry>() : nullptr)
		{
			return DefaultRegistry->QuickPlayGameMode->GetDefaultObject<USCModeDefinition>()->Alias;
		}
	}

	return TEXT("spch");
}

FString USCChallengeLobbyWidget::GetLevelLoadOptions() const
{
	FString Options;
	USCDossier* Dossier = GetCurrentDossier();
	if (IsValid(Dossier))
	{
		if (const USCChallengeData* const Challenge = Dossier->GetActiveChallengeClass().GetDefaultObject())
		{
			Options.Append(TEXT("?chlng="));
			Options.Append(Challenge->GetChallengeID());
		}
	}

	return Options;
}