// Copyright (C) 2015-2018 IllFonic, LLC.

#include "SummerCamp.h"
#include "SCSinglePlayerSaveGame.h"

#include "SCBackendRequestManager.h"
#include "SCChallengeData.h"
#include "SCDossier.h"
#include "SCGameInstance.h"
#include "SCSinglePlayerBlueprintLibrary.h"

TAutoConsoleVariable<int32> CVarUnlockAllChallenges(TEXT("sc.UnlockAllChallenges"), 0,
	TEXT("Allows access to all single player challenges.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On\n"));

TAutoConsoleVariable<int32> CVarUnlockAllEmotes(TEXT("sc.UnlockAllEmotes"), 0,
	TEXT("Allows access to all single player Emotes.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On\n"));

USCSinglePlayerSaveGame::USCSinglePlayerSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bBackendLoaded = false;
}

void USCSinglePlayerSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave /*= true*/)
{
	OwningController = PlayerController;

	// Only update our save data if we're actually going to save (otherwise we probably just loaded)
	if (bAndSave)
	{
		ChallengeSaveData.Empty();

		for (const USCDossier* Dossier : ChallengeDossiers)
		{
			ChallengeSaveData.Add(Dossier->GenerateSaveData());
		}
	}

	Super::ApplyPlayerSettings(PlayerController, bAndSave);
}

bool USCSinglePlayerSaveGame::HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const
{
	USCSinglePlayerSaveGame* Other = CastChecked<USCSinglePlayerSaveGame>(OtherSave);

	// TODO: Implement

	return true;
}

void USCSinglePlayerSaveGame::SaveGameLoaded(AILLPlayerController* PlayerController)
{
	Super::SaveGameLoaded(PlayerController);

	for (const FSCDossierSaveData& SaveData : ChallengeSaveData)
	{
		USCDossier* Dossier = GetChallengeDossierByID(SaveData.ChallengeID, false);
		Dossier->InitializeChallengeObjectivesFromSaveData(SaveData);
	}
}

USCDossier* USCSinglePlayerSaveGame::GetChallengeDossierByClass(const TSubclassOf<USCChallengeData>& ChallengeClass, const bool bRequireBackendDossier)
{
	if (!ChallengeClass)
		return nullptr;

	// Find dossier by class
	for (USCDossier* Dossier : ChallengeDossiers)
	{
		if (Dossier->GetActiveChallengeClass() == ChallengeClass)
			return Dossier;
	}

	// If none and bRequireBackendDossier is false
	if (!bRequireBackendDossier)
	{
		// Make a new dossier based on the class
		USCDossier* RequestedDossier = NewObject<USCDossier>(this);
		RequestedDossier->InitializeChallengeObjectivesFromClass(ChallengeClass, /*bInGame=*/ false);

		// Storing this dossier so future requests are cheaper
		ChallengeDossiers.Add(RequestedDossier);
		return RequestedDossier;
	}

	return nullptr;
}

USCDossier* USCSinglePlayerSaveGame::GetChallengeDossierByID(const FString& ChallengeID, const bool bRequireBackendDossier)
{
	// Find challenge class by ID
	TSubclassOf<USCChallengeData> ChallengeClass = USCSinglePlayerBlueprintLibrary::GetChallengeByID(ChallengeID);
	return GetChallengeDossierByClass(ChallengeClass, bRequireBackendDossier);
}

void USCSinglePlayerSaveGame::OnMatchCompleted(AILLPlayerController* PlayerController, const USCDossier* MatchDossier)
{
	if (!MatchDossier)
		return;

	// Update our internal dossier with the results of this new dossier
	if (USCDossier* LocalDossier = GetChallengeDossierByClass(MatchDossier->GetActiveChallengeClass(), false))
	{
		LocalDossier->HandleMatchCompleted(PlayerController, MatchDossier);

		// Add any unlocked emotes to our list
		const USCChallengeData* const ChallengeData = LocalDossier->GetActiveChallengeClass().GetDefaultObject();
		if (ChallengeData)
		{
			if (LocalDossier->IsStealthSkullUnlocked() && ChallengeData->StealthEmoteUnlock)
			{
				const USCEmoteData* const Emote = ChallengeData->StealthEmoteUnlock.GetDefaultObject();
				EmoteUnlockIDs.AddUnique(Emote->SinglePlayerEmoteUnlockID);
			}

			if (LocalDossier->IsPartyWipeSkillUnlocked() && ChallengeData->AllKillEmoteUnlock)
			{
				const USCEmoteData* const Emote = ChallengeData->AllKillEmoteUnlock.GetDefaultObject();
				EmoteUnlockIDs.AddUnique(Emote->SinglePlayerEmoteUnlockID);
			}

			if (LocalDossier->IsHighScoreSkullUnlocked() && ChallengeData->XPEmoteUnlock)
			{
				const USCEmoteData* const Emote = ChallengeData->XPEmoteUnlock.GetDefaultObject();
				EmoteUnlockIDs.AddUnique(Emote->SinglePlayerEmoteUnlockID);
			}
		}

		// Save out
		ApplyPlayerSettings(PlayerController, true);
	}
}

int32 USCSinglePlayerSaveGame::GetHighestCompletedChallengeIndex() const
{
#if !UE_BUILD_SHIPPING
	if (CVarUnlockAllChallenges.GetValueOnGameThread())
		return INT32_MAX;
#endif

	int32 HighestCompletedIndex = 0;
	for (const USCDossier* Dossier : ChallengeDossiers)
	{
		// Any completion time means we've completed this challenge at least once
		if (Dossier->GetClearedCount() > 0)
		{
			const USCChallengeData* const ChallengeData = Dossier->GetActiveChallengeClass().GetDefaultObject();
			if (ChallengeData)
				HighestCompletedIndex = FMath::Max(HighestCompletedIndex, ChallengeData->GetChallengeIndex());
		}
	}

	return HighestCompletedIndex;
}

void USCSinglePlayerSaveGame::SetHasViewedChallengesTutorial(AILLPlayerController* PlayerController)
{
	bViewedChallengesTutorial = true;
	ApplyPlayerSettings(PlayerController);
}

void USCSinglePlayerSaveGame::RequestChallengeProfile(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
#if WITH_EDITOR
	if (!World || World->IsPlayInEditor())
		return;
#endif

	if (AILLPlayerController* OwnerController = Cast<AILLPlayerController>(World ? World->GetFirstPlayerController() : nullptr))
	{
		USCGameInstance* GI = Cast<USCGameInstance>(World->GetGameInstance());
		USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
		if (RequestManager->CanSendAnyRequests())
		{
			const FILLBackendPlayerIdentifier& AccountId = OwnerController->GetBackendAccountId();
			if (ensure(AccountId.IsValid()))
			{
				FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnRetrieveChallengeProfile);
				RequestManager->GetPlayerChallengeProfile(AccountId, Delegate);
			}
			else
			{
				UE_LOG(LogSC, Warning, TEXT("RequestChallengeProfile called with blank AccountId!"));
			}
		}
	}
}

bool USCSinglePlayerSaveGame::IsEmoteUnlocked(FName EmoteUnlockID)
{
	if (CVarUnlockAllEmotes.GetValueOnGameThread() > 0)
		return true;

	// This emote was never locked...
	if (EmoteUnlockID.IsNone())
		return true;

	// We do have this emote registered as unlocked
	if (EmoteUnlockIDs.Contains(EmoteUnlockID))
		return true;

	return false;
}

void USCSinglePlayerSaveGame::OnRetrieveChallengeProfile(int32 ResponseCode, const FString& ResponseContents)
{
	/* Sample JSON:
	{
		"Challenges": [
			{
				"ChallengeID": "DEMO",
				"Objectives": [
					"DEMO-01",
					"DEMO-02"
				],
				"Skulls": [
					"NoEscape",
					"NotSceen",
					"Score"
				],
				"LastScore": 500,
				"HighestScore": 1000,
				"LastClearTime": 60,
				"BestClearTime": 55,
				"AttemptedCount": 10,
				"ClearedCount": 0
			}
		],
		"MergedIDs": [ // Ignored
			"merged_id1"
		],
		"NewProfile": false // Ignored
	}
	*/

	bBackendLoaded = true;

	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("OnRetrieveChallengeProfile: ResponseCode %i"), ResponseCode);
		OnChallengesLoaded.ExecuteIfBound(); // Let the lobby know the data wasn't loaded properly.
		return;
	}

	// Make sure we start fresh
	TArray<USCDossier*> BackendChallengeDossiers;

	// Parse Json
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContents);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		// Each challenge object will be handled by the dossier itself
		const TArray<TSharedPtr<FJsonValue>>* JsonChallenges = nullptr;
		if (JsonObject->TryGetArrayField(TEXT("Challenges"), JsonChallenges))
		{
			for (const TSharedPtr<FJsonValue>& JsonChallenge : *JsonChallenges)
			{
				USCDossier* ChallengeDossier = NewObject<USCDossier>(this);
				ChallengeDossier->InitializeChellengeObjectivesFromJson(JsonChallenge);
				BackendChallengeDossiers.Add(ChallengeDossier);
			}
		}
	}

	UWorld* World = OwningController ? OwningController->GetWorld() : nullptr;
	USCGameInstance* GI = Cast<USCGameInstance>(World ? World->GetGameInstance() : nullptr);
	USCBackendRequestManager* RequestManager = GI ? GI->GetBackendRequestManager<USCBackendRequestManager>() : nullptr;
	const bool bCanSendRequests = OwningController && OwningController->GetBackendAccountId().IsValid() && RequestManager && RequestManager->CanSendAnyRequests();

	// Merge our backend data with our local data
	if (BackendChallengeDossiers.Num() > 0)
	{
		// Merge any backend dossiers with our full dossier
		for (USCDossier* BackendDossier : BackendChallengeDossiers)
		{
			// Find our local dossier we loaded from our save file and startup
			USCDossier** LocalDossier = ChallengeDossiers.FindByPredicate([&BackendDossier](USCDossier* TestDossier) { return TestDossier->GetActiveChallengeClass() == BackendDossier->GetActiveChallengeClass(); });
			if (LocalDossier)
			{
				// See if there's anything we need to merge
				FString MergeJson = (*LocalDossier)->MergeDossiers(BackendDossier);

				// If we did merge, make sure we send that to the backend (one challenge at a time)
				if (MergeJson.Len())
				{
					if (bCanSendRequests)
					{
						FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnChallengeCompletedPosted);
						RequestManager->ReportChallengeComplete(OwningController->GetBackendAccountId(), MergeJson, Delegate);
						++PendingChallengeUpdates;
					}
				}
			}
			else
			{
				// We don't have a local version of this backend dossier, so add it to our local list (and then eventually save it out)
				ChallengeDossiers.Add(BackendDossier);

				// Oh great server gods, please share with me my unlocked emotes!
				const USCChallengeData* const ChallengeData = BackendDossier->GetActiveChallengeClass().GetDefaultObject();
				if (ChallengeData)
				{
					if (BackendDossier->IsStealthSkullUnlocked() && ChallengeData->StealthEmoteUnlock)
					{
						const USCEmoteData* const Emote = ChallengeData->StealthEmoteUnlock.GetDefaultObject();
						EmoteUnlockIDs.AddUnique(Emote->SinglePlayerEmoteUnlockID);
					}

					if (BackendDossier->IsPartyWipeSkillUnlocked() && ChallengeData->AllKillEmoteUnlock)
					{
						const USCEmoteData* const Emote = ChallengeData->AllKillEmoteUnlock.GetDefaultObject();
						EmoteUnlockIDs.AddUnique(Emote->SinglePlayerEmoteUnlockID);
					}

					if (BackendDossier->IsHighScoreSkullUnlocked() && ChallengeData->XPEmoteUnlock)
					{
						const USCEmoteData* const Emote = ChallengeData->XPEmoteUnlock.GetDefaultObject();
						EmoteUnlockIDs.AddUnique(Emote->SinglePlayerEmoteUnlockID);
					}
				}
			}
		}
	}

	// Upload any unique local data
	for (USCDossier* LocalDossier : ChallengeDossiers)
	{
		// We've already processed these, don't merge them again...
		USCDossier** SharedDossier = BackendChallengeDossiers.FindByPredicate([&LocalDossier](USCDossier* TestDossier) { return TestDossier->GetActiveChallengeClass() == LocalDossier->GetActiveChallengeClass(); });
		if (SharedDossier)
			continue;

		// Build the sync json
		FString MergeJson = LocalDossier->MergeDossiers(nullptr);

		// If we did merge, make sure we send that to the backend (one challenge at a time)
		if (MergeJson.Len())
		{
			if (bCanSendRequests)
			{
				FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnChallengeCompletedPosted);
				RequestManager->ReportChallengeComplete(OwningController->GetBackendAccountId(), MergeJson, Delegate);
				++PendingChallengeUpdates;
			}
		}
	}

	// Make sure we save off any new data
	if (OwningController)
		ApplyPlayerSettings(OwningController, true);

	OnChallengesLoaded.ExecuteIfBound();
}

void USCSinglePlayerSaveGame::OnChallengeCompletedPosted(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("OnChallengeCompletedPosted: ResponseCode %i Reason: %s"), ResponseCode, *ResponseContents);
	}

	// We updated the backend, make sure the backend updates us!
	--PendingChallengeUpdates;
	if (PendingChallengeUpdates == 0)
	{
		if (ASCPlayerState* PlayerState = OwningController ? Cast<ASCPlayerState>(OwningController->PlayerState) : nullptr)
		{
			PlayerState->SERVER_RequestBackendProfileUpdate();
		}
	}
}
