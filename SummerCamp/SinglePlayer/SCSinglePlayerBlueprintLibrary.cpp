// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSinglePlayerBlueprintLibrary.h"

// UE4
#include "MessageLog.h"
#include "UObjectToken.h"

// SummerCamp
#include "SCDossier.h"
#include "SCGameState_SPChallenges.h"
#include "SCLocalPlayer.h"
#include "SCSinglePlayerSaveGame.h"
#include "SCKillerCharacter.h"
#include "SCCounselorCharacter.h"
#include "SCVoiceOverComponent.h"

void USCSinglePlayerBlueprintLibrary::CompleteObjective(UObject* WorldContextObject, const TSubclassOf<USCObjectiveData> ObjectiveClass)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	ASCGameState_SPChallenges* GameState = World ? Cast<ASCGameState_SPChallenges>(World->GetGameState()) : nullptr;
	if (GameState && GameState->Dossier)
	{
		GameState->Dossier->CompleteObjective(ObjectiveClass);
	}
	else
	{
		const FString WarningMessage = FString::Printf(TEXT("USCSinglePlayerBlueprintLibrary::CompleteObjective: Could not get a valid game state (%s) or dossier from passed in world context object (%s), may be playing in the wrong mode or calling this at a bad time."), *GetNameSafe(GameState), *GetNameSafe(WorldContextObject));
		UE_LOG(LogSC, Warning, TEXT("%s"), *WarningMessage);

#if WITH_EDITOR
		FMessageLog("PIE").Warning()
			->AddToken(FTextToken::Create(FText::FromString(WarningMessage)));
#endif
	}
}

USCDossier* USCSinglePlayerBlueprintLibrary::GetDossier(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	ASCGameState_SPChallenges* GameState = World ? Cast<ASCGameState_SPChallenges>(World->GetGameState()) : nullptr;
	if (GameState && GameState->Dossier)
	{
		return GameState->Dossier;
	}
	else
	{
		const FString WarningMessage = FString::Printf(TEXT("USCSinglePlayerBlueprintLibrary::GetDossier: Could not get a valid game state (%s) or dossier from passed in world context object (%s), may be playing in the wrong mode or calling this at a bad time."), *GetNameSafe(GameState), *GetNameSafe(WorldContextObject));
		UE_LOG(LogSC, Warning, TEXT("%s"), *WarningMessage);

#if WITH_EDITOR
		FMessageLog("PIE").Warning()
			->AddToken(FTextToken::Create(FText::FromString(WarningMessage)));
#endif
	}

	return nullptr;
}

int32 USCSinglePlayerBlueprintLibrary::GetHighestCompletedChallengeIndex(UObject* WorldContextObject)
{
	if (UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (USCLocalPlayer* LocalPlayer = PlayerController ? Cast<USCLocalPlayer>(PlayerController->GetLocalPlayer()) : nullptr)
		{
			if (USCSinglePlayerSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>())
				return SaveGame->GetHighestCompletedChallengeIndex();
		}
	}

	return INDEX_NONE;
}

TSubclassOf<USCMapRegistry> USCSinglePlayerBlueprintLibrary::GetDefaultMapRegistryClass()
{
	static TSubclassOf<USCMapRegistry> DefaultMapRegistryClass = StaticLoadClass(USCMapRegistry::StaticClass(), nullptr, TEXT("/Game/Blueprints/MapRegistry/MapRegistry.MapRegistry_C"));
	return DefaultMapRegistryClass;
}

TSubclassOf<USCChallengeData> USCSinglePlayerBlueprintLibrary::GetChallengeByID(const FString& ChallengeID)
{
	TSubclassOf<USCMapRegistry> MapRegistryClass = GetDefaultMapRegistryClass();
	return USCMapRegistry::GetChallengeByID(MapRegistryClass, USCMapRegistry::FindModeByAlias(MapRegistryClass, TEXT("SPCH")), ChallengeID);
}

bool USCSinglePlayerBlueprintLibrary::HasEmoteUnlocked(UObject* WorldContextObject, FName InEmoteID)
{
	if (UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (USCLocalPlayer* LocalPlayer = PlayerController ? Cast<USCLocalPlayer>(PlayerController->GetLocalPlayer()) : nullptr)
		{
			if (USCSinglePlayerSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>())
				return SaveGame->IsEmoteUnlocked(InEmoteID);
		}
	}

	return false;
}

bool USCSinglePlayerBlueprintLibrary::HasViewedSPTutorial(UObject* WorldContextObject)
{
	if (UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (USCLocalPlayer* LocalPlayer = PlayerController ? Cast<USCLocalPlayer>(PlayerController->GetLocalPlayer()) : nullptr)
		{
			if (USCSinglePlayerSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>())
			{
				return SaveGame->bViewedChallengesTutorial;
			}
		}
	}

	return false;
}

void USCSinglePlayerBlueprintLibrary::SetViewedSPTutorial(UObject* WorldContextObject)
{
	if (UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
	{
		// We need an AILLPlayerController to save the save data out.
		AILLPlayerController* PlayerController = Cast<AILLPlayerController>(World->GetFirstPlayerController());
		if (USCLocalPlayer* LocalPlayer = PlayerController ? Cast<USCLocalPlayer>(PlayerController->GetLocalPlayer()) : nullptr)
		{
			if (USCSinglePlayerSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>())
			{
				SaveGame->SetHasViewedChallengesTutorial(PlayerController);
			}
		}
	}
}

void USCSinglePlayerBlueprintLibrary::RequestChallengeProfileLoad(UObject* WorldContextObject, const FSCChallengesLoadedDelegate& ProfileLoadCallback)
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		ProfileLoadCallback.ExecuteIfBound();
		return;
	}
#endif

	if (UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (USCLocalPlayer* LocalPlayer = PlayerController ? Cast<USCLocalPlayer>(PlayerController->GetLocalPlayer()) : nullptr)
		{
			if (USCSinglePlayerSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>())
			{
				SaveGame->OnChallengesLoaded = ProfileLoadCallback;
				SaveGame->RequestChallengeProfile(WorldContextObject);
			}
		}
	}
}

void USCSinglePlayerBlueprintLibrary::ClearAllDynamicMaterialParamaters(UPrimitiveComponent* Mesh)
{
	if (!Mesh)
		return;

	for (int32 iMat(0); iMat < Mesh->GetNumMaterials(); ++iMat)
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(iMat)))
		{
			DynamicMaterial->ClearParameterValues();
		}
	}
}

void USCSinglePlayerBlueprintLibrary::BeginSuperCinematicKill(AILLPlayerController* KillerController, TArray<ASCCharacter*> Victims)
{
	// Disable input for our local player while doing cinematic goodness.
	KillerController->DisableInput(KillerController);

	if (ASCKillerCharacter* KillerCharacter = Cast<ASCKillerCharacter>(KillerController->GetPawn()))
	{
		// If this is a killer character we need to kill screen effects sound blips, hide the pawn, and drop the held counselor if there is one.
		KillerCharacter->CancelScreenEffectAbilities();
		KillerCharacter->SetSoundBlipVisibility(false);
		KillerCharacter->SetActorHiddenInGame(true);
		KillerCharacter->DropCounselor();
		KillerCharacter->ContextKillStarted();
		KillerCharacter->CancelShift();
	}

	// Store the conversation manager so we don't have to grab it for each victim
	USCConversationManager* ConvoManager = nullptr;
	if (UWorld* World = KillerController->GetWorld())
	{
		if (ASCGameState* GameState = World->GetGameState<ASCGameState>())
		{
			ConvoManager = GameState->GetConversationManager();
		}
	}

	for (ASCCharacter* Victim : Victims)
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Victim))
		{
			// Hide the victim pawn
			Counselor->SetActorHiddenInGame(true);
			Counselor->ContextKillStarted();
			if (ConvoManager)
			{
				// Abort any currently playing VO including conversations.
				ConvoManager->AbortConversationByCounselor(Counselor);
				if (Counselor->VoiceOverComponent)
				{
					Counselor->VoiceOverComponent->StopVoiceOver();
				}
			}
		}
	}
}

void USCSinglePlayerBlueprintLibrary::EndSuperCinematicKill(AILLPlayerController* KillerController, TArray<ASCCharacter*> Victims, TSubclassOf<UDamageType> DamageType)
{
	if (KillerController)
		KillerController->EnableInput(KillerController);

	if (ASCKillerCharacter* KillerCharacter = Cast<ASCKillerCharacter>(KillerController->GetPawn()))
	{
		// Turn the sound blips back on and show the pawn again.
		KillerCharacter->SetSoundBlipVisibility(true);
		KillerCharacter->SetActorHiddenInGame(false);
		KillerCharacter->ContextKillEnded();
	}

	// Deal damage to each of our victims
	for (ASCCharacter* Victim : Victims)
	{
		UGameplayStatics::ApplyDamage(Victim, Victim->GetMaxHealth(), KillerController, KillerController->GetPawn(), DamageType);
		Victim->ContextKillEnded();
	}
}
