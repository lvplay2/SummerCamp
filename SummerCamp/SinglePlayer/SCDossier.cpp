// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCDossier.h"

#include "SCChallengeData.h"
#include "SCGameState.h"
#include "SCLocalPlayer.h"
#include "SCObjectiveData.h"
#include "SCPlayerState.h"
#include "SCSinglePlayerBlueprintLibrary.h"

// UE4
#include "MessageLog.h"
#include "UObjectToken.h"

TAutoConsoleVariable<int32> CVarDebugDossier(TEXT("sc.DebugDossier"), 0,
	TEXT("Displays debug information for the active challenge dossier.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On\n"));

// This is the number of skulls you must earn in a playthrough to clear it and move on to the next level
#define MIN_SKULLS_TO_CLEAR 2

namespace SCSkullNames
{
static const FString NoEscape(TEXT("NoEscape"));
static const FString NotSeen(TEXT("NotSeen"));
static const FString Score(TEXT("Score"));
};

USCDossier::USCDossier(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCDossier::InitializeChallengeObjectivesFromClass(const TSubclassOf<USCChallengeData> ChallengeClass, const bool bInGame /*=true*/, const bool bSkipSaveFile /*= false*/)
{
	check(ChallengeClass);

	EscapedCounselorCount = 0;

	ActiveChallengeClass = ChallengeClass;
	ActiveObjectives.Empty();

	const USCChallengeData* const ChallengeData = ChallengeClass->GetDefaultObject<const USCChallengeData>();
	const TArray<TSubclassOf<USCObjectiveData>>& ChallengeObjectives = ChallengeData->GetChallengeObjectives();
	for (const TSubclassOf<USCObjectiveData>& ObjectiveClass : ChallengeObjectives)
	{
		FSCObjectiveState State;
		State.ObjectiveClass = ObjectiveClass;
		State.bPreviouslyCompleted = State.bCompleted = false;
		ActiveObjectives.Add(State);
	}

	bStealthSkullUnlocked = bInGame; // On by default since this function will mostly be used for playthroughs and you start out unseen (the json load will override this value), unless we're loading it for the menu.
	bPartyWipeSkullUnlocked = false;
	bHighScoreSkullUnlocked = false;

	if (!bSkipSaveFile)
	{
		UWorld* World = GetWorld();
		// Update our previously completed objectives based on the passed in player state
		APlayerController* LocalPlayerController = World ? GEngine->GetFirstLocalPlayerController(World) : nullptr;
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(LocalPlayerController ? LocalPlayerController->GetLocalPlayer() : nullptr))
		{
			if (USCSinglePlayerSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>())
			{
				if (USCDossier* BackendDossier = SaveGame->GetChallengeDossierByClass(ChallengeClass, true))
				{
					for (FSCObjectiveState& ObjectiveState : ActiveObjectives)
					{
						for (FSCObjectiveState& BackendObjectiveState : BackendDossier->ActiveObjectives)
						{
							if (BackendObjectiveState.ObjectiveClass == ObjectiveState.ObjectiveClass)
							{
								ObjectiveState.bPreviouslyCompleted = BackendObjectiveState.bPreviouslyCompleted;
								break;
							}
						}
					}
				}
			}
		}
	}
}

void USCDossier::InitializeChellengeObjectivesFromJson(const TSharedPtr<FJsonValue>& JsonChallengeValue)
{
	/* Sample JSON:
	{
		"ChallengeID": "DEMO",
		"Objectives": [			// Contains completed objectives ONLY
			"DEMO-01",
			"DEMO-02"
		],
		"Skulls": [				// Contains completed skulls ONLY
			"NoEscape",
			"NotSeen",
			"Score"
		],
		"LastScore": 500,		// Ignored
		"HighestScore": 1000,
		"LastClearTime": 60,	// Ignored
		"BestClearTime": 55,
		"AttemptedCount": 10,
		"ClearedCount": 0
	}
	*/

	const TSharedPtr<FJsonObject>* JChallengeObj;
	JsonChallengeValue->TryGetObject(JChallengeObj);
	if (!JChallengeObj)
	{
		UE_LOG(LogSC, Warning, TEXT("Invalid json, dossier will not be initialized"));
		return;
	}

	FString JsonChallengeID;
	(*JChallengeObj)->TryGetStringField(TEXT("ChallengeID"), JsonChallengeID);
	if (JsonChallengeID.IsEmpty())
	{
		UE_LOG(LogSC, Warning, TEXT("Blank challenge ID from passed in json, dossier will not be initialized"));
		return;
	}

	TSubclassOf<USCChallengeData> SelectedChallengeClass = USCSinglePlayerBlueprintLibrary::GetChallengeByID(JsonChallengeID);
	if (!IsValid(*SelectedChallengeClass))
	{
		UE_LOG(LogSC, Warning, TEXT("Invalid challenge ID (%s), dossier will not be initialized"), *JsonChallengeID);
		return;
	}

	// Let the class load get us default values
	InitializeChallengeObjectivesFromClass(SelectedChallengeClass, /*bInGame=*/false, /*bSkipSaveFile=*/true);

	// Set completed objectives
	TArray<FString> JsonObjectives;
	if ((*JChallengeObj)->TryGetStringArrayField(TEXT("Objectives"), JsonObjectives))
	{
		for (const FString& JsonObjectiveID : JsonObjectives)
		{
			for (FSCObjectiveState& ObjectiveState : ActiveObjectives)
			{
				const USCObjectiveData* const ObjectiveData = ObjectiveState.ObjectiveClass.GetDefaultObject();
				FString FullObjectiveID = FString::Printf(TEXT("%s-%s"), *JsonChallengeID, ObjectiveData ? *ObjectiveData->GetObjectiveID() : TEXT(""));
				if (FullObjectiveID.Compare(JsonObjectiveID) == 0)
				{
					ObjectiveState.bPreviouslyCompleted = true;
					break;
				}
			}
		}
	}

	// Skulls
	TArray<FString> JsonSkulls;
	if ((*JChallengeObj)->TryGetStringArrayField(TEXT("Skulls"), JsonSkulls))
	{
		bStealthSkullUnlocked = JsonSkulls.Contains(SCSkullNames::NotSeen);
		bPartyWipeSkullUnlocked = JsonSkulls.Contains(SCSkullNames::NoEscape);
		bHighScoreSkullUnlocked = JsonSkulls.Contains(SCSkullNames::Score);
	}

	// Score and whatnot!
	(*JChallengeObj)->TryGetNumberField(TEXT("HighestScore"), XPHighScore);
	(*JChallengeObj)->TryGetNumberField(TEXT("BestClearTime"), FastestCompletionTime);
	(*JChallengeObj)->TryGetNumberField(TEXT("AttemptedCount"), AttemptCount);
	(*JChallengeObj)->TryGetNumberField(TEXT("ClearedCount"), ClearedCount);
}

void USCDossier::InitializeChallengeObjectivesFromSaveData(const FSCDossierSaveData& SaveData)
{
	TSubclassOf<USCChallengeData> SelectedChallengeClass = USCSinglePlayerBlueprintLibrary::GetChallengeByID(SaveData.ChallengeID);
	if (!SelectedChallengeClass)
	{
		UE_LOG(LogSC, Warning, TEXT("Invalid challenge ID (%s), dossier will not be initialized"), *SaveData.ChallengeID);
		return;
	}

	// Let the class load get us default values
	InitializeChallengeObjectivesFromClass(SelectedChallengeClass);

	for (const FString& ObjectiveID : SaveData.CompletedObjectives)
	{
		FSCObjectiveState* SavedObjectiveState = ActiveObjectives.FindByPredicate([&ObjectiveID](const FSCObjectiveState& ObjectiveState)
		{
			const USCObjectiveData* const ObjectiveData = ObjectiveState.ObjectiveClass->GetDefaultObject<const USCObjectiveData>();
			if (!ObjectiveData)
				return false;

			return ObjectiveData->GetObjectiveID() == ObjectiveID;
		});

		if (SavedObjectiveState)
		{
			SavedObjectiveState->bPreviouslyCompleted = true;
		}
	}

	AttemptCount = SaveData.AttemptCount;
	ClearedCount = SaveData.ClearedCount;
	XPHighScore = SaveData.XPHighScore;
	FastestCompletionTime = SaveData.FastestCompletionTime;
	bStealthSkullUnlocked = SaveData.bStealthSkullUnlocked;
	bPartyWipeSkullUnlocked = SaveData.bPartyWipeSkullUnlocked;
	bHighScoreSkullUnlocked = SaveData.bHighScoreSkullUnlocked;
}

FSCDossierSaveData USCDossier::GenerateSaveData() const
{
	const USCChallengeData* const ChallengeData = ActiveChallengeClass->GetDefaultObject<const USCChallengeData>();

	FSCDossierSaveData OutputData;
	OutputData.ChallengeID = ChallengeData->GetChallengeID();

	for (const FSCObjectiveState& ObjectiveState : ActiveObjectives)
	{
		if (ObjectiveState.bCompleted || ObjectiveState.bPreviouslyCompleted)
		{
			const USCObjectiveData* const ObjectiveData = ObjectiveState.ObjectiveClass->GetDefaultObject<const USCObjectiveData>();
			OutputData.CompletedObjectives.Add(ObjectiveData->GetObjectiveID());
		}
	}

	OutputData.AttemptCount = AttemptCount;
	OutputData.ClearedCount = ClearedCount;
	OutputData.XPHighScore = XPHighScore;
	OutputData.FastestCompletionTime = FastestCompletionTime;
	OutputData.bStealthSkullUnlocked = bStealthSkullUnlocked;
	OutputData.bPartyWipeSkullUnlocked = bPartyWipeSkullUnlocked;
	OutputData.bHighScoreSkullUnlocked = bHighScoreSkullUnlocked;

	return OutputData;
}

FString USCDossier::GetMatchCompletionJson(UObject* WorldContextObject)
{
	/* Sample JSON:
	{
		"ChallengeID": "DEMO",
		"Cleared" : true,
		"Objectives": [
			"DEMO-01",
			"DEMO-02"
		],
		"Skulls": [
			"Skull1",
			"Skull2"
		],

		"Time": 60,
		"Score": 500,
		"Jason": "some jason"
	}
	*/

	const USCChallengeData* const ChallengeData = ActiveChallengeClass.GetDefaultObject();

	FString Payload;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload);
	JsonWriter->WriteObjectStart();
	JsonWriter->WriteValue(TEXT("ChallengeID"), ChallengeData->GetChallengeID());
	JsonWriter->WriteValue(TEXT("Cleared"), GetCompletedSkullCount() >= MIN_SKULLS_TO_CLEAR);
	JsonWriter->WriteArrayStart(TEXT("Objectives"));
	{
		for (const FSCObjectiveState& ObjectiveState : ActiveObjectives)
		{
			if (ObjectiveState.bCompleted)
			{
				// Objective IDs are ChallengeID-ObjectiveID (i.e. BRK1-01)
				JsonWriter->WriteValue(FString::Printf(TEXT("%s-%s"), *ChallengeData->GetChallengeID(), *ObjectiveState.ObjectiveClass.GetDefaultObject()->GetObjectiveID()));
			}
		}
	}
	JsonWriter->WriteArrayEnd();
	JsonWriter->WriteArrayStart(TEXT("Skulls"));
	{
		if (bStealthSkullUnlocked)		JsonWriter->WriteValue(SCSkullNames::NotSeen);
		if (bPartyWipeSkullUnlocked)	JsonWriter->WriteValue(SCSkullNames::NoEscape);
		if (bHighScoreSkullUnlocked)	JsonWriter->WriteValue(SCSkullNames::Score);
	}
	JsonWriter->WriteArrayEnd();

	if (UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
	{
		if (ASCGameState* GameState = Cast<ASCGameState>(World->GetGameState()))
		{
			JsonWriter->WriteValue(TEXT("Time"), GameState->ElapsedInProgressTime);
		}

		const APlayerController* LocalPlayerController = World->GetFirstPlayerController();
		if (const ASCPlayerState* PlayerState = LocalPlayerController ? Cast<const ASCPlayerState>(LocalPlayerController->PlayerState) : nullptr)
		{
			JsonWriter->WriteValue(TEXT("Score"), (int32)PlayerState->Score);

			TSubclassOf<ASCCharacter> KillerClass = PlayerState->GetActiveCharacterClass();
			FString ClassName = IsValid(KillerClass) && KillerClass->IsChildOf(ASCKillerCharacter::StaticClass()) ? KillerClass->GetName() : TEXT("Unknown");
			int32 CharIndex = INDEX_NONE;
			if (ClassName.FindChar('.', CharIndex))
				ClassName.RemoveAt(CharIndex, ClassName.Len() - CharIndex);
			JsonWriter->WriteValue(TEXT("Jason"), ClassName);
		}
	}
	JsonWriter->WriteObjectEnd();
	JsonWriter->Close();

	return Payload;
}

FString USCDossier::MergeDossiers(const USCDossier* BackendDossier)
{
	/* Sample JSON:
	{
		"ChallengeID": "DEMO",
		"Objectives": [
			"DEMO-01",
			"DEMO-02"
		],
		"Skulls": [
			"Skull1",
			"Skull2"
		],

		"SyncUpdate" : true,
		"AttemptedCount" : 15,
		"ClearedCount" : 1,
		"BestClearTime" : 500,
		"HighestScore" : 1550
	}
	*/

	// Set to true if we find any data in this dossier that isn't in the backend dossier
	bool bHadDeltas = false;

	const USCChallengeData* const ChallengeData = ActiveChallengeClass.GetDefaultObject();

	FString Payload;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload);
	JsonWriter->WriteObjectStart();

	JsonWriter->WriteValue(TEXT("ChallengeID"), ChallengeData->GetChallengeID());
	JsonWriter->WriteValue(TEXT("SyncUpdate"), true);

	// Find objective deltas
	JsonWriter->WriteArrayStart(TEXT("Objectives"));
	{
		// Find any objectives in the local save that were completed that aren't on the backend
		for (FSCObjectiveState& ObjectiveState : ActiveObjectives)
		{
			const FSCObjectiveState* BackendObjectiveState = BackendDossier ? BackendDossier->ActiveObjectives.FindByPredicate([&ObjectiveState](const FSCObjectiveState& BackendObjective) { return ObjectiveState.ObjectiveClass == BackendObjective.ObjectiveClass; }) : nullptr;
			if (BackendObjectiveState)
			{
				if (ObjectiveState.bPreviouslyCompleted != BackendObjectiveState->bPreviouslyCompleted)
				{
					if (ObjectiveState.bPreviouslyCompleted)
					{
						bHadDeltas = true;
						JsonWriter->WriteValue(FString::Printf(TEXT("%s-%s"), *ChallengeData->GetChallengeID(), *ObjectiveState.ObjectiveClass.GetDefaultObject()->GetObjectiveID()));
					}
					else
					{
						ObjectiveState.bPreviouslyCompleted = BackendObjectiveState->bPreviouslyCompleted;
					}
				}
			}
			else if (ObjectiveState.bPreviouslyCompleted)
			{
				bHadDeltas = true;
				JsonWriter->WriteValue(FString::Printf(TEXT("%s-%s"), *ChallengeData->GetChallengeID(), *ObjectiveState.ObjectiveClass.GetDefaultObject()->GetObjectiveID()));
			}
		}

		// Update our local save for any objectives completed on the backend
		// We're not flagging deltas or writting JSON here because we don't need to tell the backend we were out of date
		if (BackendDossier)
		{
			for (const FSCObjectiveState& BackendObjectiveState : BackendDossier->ActiveObjectives)
			{
				FSCObjectiveState* LocalObjectiveState = ActiveObjectives.FindByPredicate([&BackendObjectiveState](const FSCObjectiveState& LocalObjective) { return BackendObjectiveState.ObjectiveClass == LocalObjective.ObjectiveClass; });
				if (LocalObjectiveState)
				{
					LocalObjectiveState->bPreviouslyCompleted |= BackendObjectiveState.bPreviouslyCompleted;
				}
				else
				{
					// Somehow we were missing this state, let's add it
					FSCObjectiveState State;
					State.ObjectiveClass = BackendObjectiveState.ObjectiveClass;
					State.bCompleted = false;
					State.bPreviouslyCompleted = BackendObjectiveState.bPreviouslyCompleted;
					ActiveObjectives.Add(State);
				}
			}
		}
	}
	JsonWriter->WriteArrayEnd();

	// Find skull deltas
	JsonWriter->WriteArrayStart(TEXT("Skulls"));
	{
		// Stealth skull
		if (!BackendDossier || bStealthSkullUnlocked != BackendDossier->bStealthSkullUnlocked)
		{
			if (bStealthSkullUnlocked)
			{
				bHadDeltas = true;
				JsonWriter->WriteValue(SCSkullNames::NotSeen);
			}
			else if (BackendDossier)
			{
				bStealthSkullUnlocked = BackendDossier->bStealthSkullUnlocked;
			}
		}

		// Kill skull
		if (!BackendDossier || bPartyWipeSkullUnlocked != BackendDossier->bPartyWipeSkullUnlocked)
		{
			if (bPartyWipeSkullUnlocked)
			{
				bHadDeltas = true;
				JsonWriter->WriteValue(SCSkullNames::NoEscape);
			}
			else if (BackendDossier)
			{
				bPartyWipeSkullUnlocked = BackendDossier->bPartyWipeSkullUnlocked;
			}
		}

		// Score skull
		if (!BackendDossier || bHighScoreSkullUnlocked != BackendDossier->bHighScoreSkullUnlocked)
		{
			if (bHighScoreSkullUnlocked)
			{
				bHadDeltas = true;
				JsonWriter->WriteValue(SCSkullNames::Score);
			}
			else if (BackendDossier)
			{
				bHighScoreSkullUnlocked = BackendDossier->bHighScoreSkullUnlocked;
			}
		}
	}
	JsonWriter->WriteArrayEnd();

	// Number of attempts
	if (AttemptCount > 0 && (!BackendDossier || AttemptCount > BackendDossier->AttemptCount))
	{
		bHadDeltas = true;
		JsonWriter->WriteValue(TEXT("AttemptedCount"), AttemptCount);
	}
	else if (BackendDossier)
	{
		AttemptCount = BackendDossier->AttemptCount;
	}

	// Number of clears
	if (ClearedCount > 0 && (!BackendDossier || ClearedCount > BackendDossier->ClearedCount))
	{
		bHadDeltas = true;
		JsonWriter->WriteValue(TEXT("ClearedCount"), ClearedCount);
	}
	else if (BackendDossier)
	{
		ClearedCount = BackendDossier->ClearedCount;
	}

	// Completion time (smaller is better)
	if (FastestCompletionTime > 0 && (!BackendDossier || FastestCompletionTime < BackendDossier->FastestCompletionTime))
	{
		bHadDeltas = true;
		JsonWriter->WriteValue(TEXT("BestClearTime"), FastestCompletionTime);
	}
	else if (BackendDossier)
	{
		FastestCompletionTime = BackendDossier->FastestCompletionTime;
	}

	// High score
	if (XPHighScore > 0 && (!BackendDossier || XPHighScore > BackendDossier->XPHighScore))
	{
		bHadDeltas = true;
		JsonWriter->WriteValue(TEXT("HighestScore"), XPHighScore);
	}
	else if (BackendDossier)
	{
		XPHighScore = BackendDossier->XPHighScore;
	}

	JsonWriter->WriteObjectEnd();
	JsonWriter->Close();

	if (!bHadDeltas)
		Payload.Empty();
#if !UE_BUILD_SHIPPING
	else
		UE_LOG(LogSC, Warning, TEXT("Found deltas with challenge, submitting payload to backend: %s"), *Payload);
#endif

	return Payload;
}

void USCDossier::CompleteObjective(const TSubclassOf<USCObjectiveData> ObjectiveClass)
{
	check(ObjectiveClass);

	for (FSCObjectiveState& State : ActiveObjectives)
	{
		if (State.ObjectiveClass == ObjectiveClass)
		{
			State.bCompleted = true;
			OnObjectiveCompleted.Broadcast(State);
			return;
		}
	}

	// In general this is probably fine. We're going to have some Objectives that trigger based on generic actions and are only specific to one level
	UE_LOG(LogSC, Warning, TEXT("Tried to complete Objective %s when it is not in the active Objective list."), *ObjectiveClass->GetName());
}

void USCDossier::HandleMatchCompleted(UObject* WorldContextObject, const USCDossier* MatchDossier)
{
	if (!MatchDossier)
		return;

	// Find our high scores
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	check(World);

	const APlayerController* LocalPlayerController = World->GetFirstPlayerController();
	const ASCPlayerState* PlayerState = LocalPlayerController ? Cast<const ASCPlayerState>(LocalPlayerController->PlayerState) : nullptr;
	const ASCGameState* GameState = Cast<ASCGameState>(World ? World->GetGameState() : nullptr);
	const int32 ElapsedTime = GameState ? GameState->ElapsedInProgressTime : 0;

	// Increment attempts/clears and best time only when successfully completed.
	++AttemptCount;
	if (MatchDossier->GetCompletedSkullCount() >= MIN_SKULLS_TO_CLEAR)
	{
		++ClearedCount;

		if (ElapsedTime > 0)
		{
			if (FastestCompletionTime > 0)
				FastestCompletionTime = FMath::Min(FastestCompletionTime, ElapsedTime);
			else
				FastestCompletionTime = ElapsedTime;
		}
	}

	XPHighScore = FMath::Max(XPHighScore, PlayerState ? (int32)PlayerState->Score : 0);

	// Update skulls
	bStealthSkullUnlocked |= MatchDossier->bStealthSkullUnlocked;
	bPartyWipeSkullUnlocked |= MatchDossier->bPartyWipeSkullUnlocked;
	bHighScoreSkullUnlocked |= MatchDossier->bHighScoreSkullUnlocked;

	// Update objectives
	for (const FSCObjectiveState& MatchObjectiveState : MatchDossier->ActiveObjectives)
	{
		FSCObjectiveState* LocalObjectiveState = ActiveObjectives.FindByPredicate([&MatchObjectiveState](const FSCObjectiveState& LocalObjective) { return MatchObjectiveState.ObjectiveClass == LocalObjective.ObjectiveClass; });
		if (LocalObjectiveState)
		{
			// Match state will be completed, long term storage treats it as previously completed
			LocalObjectiveState->bPreviouslyCompleted |= MatchObjectiveState.bCompleted;
		}
		else
		{
			// Rare, but good to check for
			FSCObjectiveState State;
			State.ObjectiveClass = MatchObjectiveState.ObjectiveClass;
			State.bCompleted = false;
			State.bPreviouslyCompleted = MatchObjectiveState.bCompleted;
			ActiveObjectives.Add(State);
		}
	}
}

bool USCDossier::IsObjectiveHidden(const TSubclassOf<USCObjectiveData> ObjectiveClass) const
{
	check(ObjectiveClass);

	for (const FSCObjectiveState& State : ActiveObjectives)
	{
		if (State.ObjectiveClass == ObjectiveClass)
		{
			// Only hidden when not completed
			return State.ObjectiveClass.GetDefaultObject()->GetIsHidden() && !State.bCompleted;
		}
	}

	return false;
}

bool USCDossier::IsObjectiveCompleted(const TSubclassOf<USCObjectiveData> ObjectiveClass) const
{
	check(ObjectiveClass);

	for (const FSCObjectiveState& State : ActiveObjectives)
	{
		if (State.ObjectiveClass == ObjectiveClass)
			return State.bCompleted;
	}

	return false;
}

bool USCDossier::HasObjectiveEverBeenCompleted(const TSubclassOf<USCObjectiveData> ObjectiveClass) const
{
	check(ObjectiveClass);

	for (const FSCObjectiveState& State : ActiveObjectives)
	{
		if (State.ObjectiveClass == ObjectiveClass)
			return State.bCompleted || State.bPreviouslyCompleted;
	}

	return false;
}

float USCDossier::GetXPPercentComplete(UObject* WorldContextObject) const
{
	// Don't crash in editor builds when we're just playing around!
	if (!ActiveChallengeClass)
	{
		const FString WarningMessage(TEXT("Dosser is missing the active challenge. Please make sure to set your challenge in world settings."));
		UE_LOG(LogSC, Warning, TEXT("%s"), *WarningMessage);

#if WITH_EDITOR
		FMessageLog("PIE").Warning()
			->AddToken(FTextToken::Create(FText::FromString(WarningMessage)));
#endif
		return 0.f;
	}

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	check(World);

	// Current XP is stored in the local player's player state... so let's grab that
	const APlayerController* LocalPlayerController = World->GetFirstPlayerController();
	const ASCPlayerState* PlayerState = LocalPlayerController ? Cast<const ASCPlayerState>(LocalPlayerController->PlayerState) : nullptr;

	const float CurrentXP = PlayerState ? PlayerState->Score : 0.f;
	const float XPRequired = FMath::Max((float)ActiveChallengeClass.GetDefaultObject()->GetXPRequiredForSkull(), 1.f);
	return FMath::Min(CurrentXP / XPRequired, 1.f);
}

void USCDossier::OnSpottedByAI()
{
	// Local bool used for showing the skull notification only once.
	const bool bStealthSkullBeforeBeingSpotted = bStealthSkullUnlocked;
	bStealthSkullUnlocked = false;

	if (bStealthSkullBeforeBeingSpotted != bStealthSkullUnlocked)
	{
		OnSkullFailed.Broadcast(ESCSkullTypes::Stealth);
	}
}

void USCDossier::OnCounselorKilled(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	check(World);

	APlayerController* LocalPlayerController = World->GetFirstPlayerController();
	ASCPlayerState* PlayerState = LocalPlayerController ? Cast<ASCPlayerState>(LocalPlayerController->PlayerState) : nullptr;
	ASCGameState* GameState = Cast<ASCGameState>(World->GetGameState());
	if (GameState && PlayerState)
	{
		bPartyWipeSkullUnlocked = (PlayerState->GetKills() >= GameState->SpawnedCounselorCount);
	}
}

void USCDossier::OnXPEarned(UObject* WorldContextObject)
{
	// Local bool used for showing the skull notification only once.
	const bool WasSkullUnlocked = bHighScoreSkullUnlocked;
	bHighScoreSkullUnlocked = (GetXPPercentComplete(WorldContextObject) >= 1.f);

	if (!WasSkullUnlocked && bHighScoreSkullUnlocked)
	{
		OnSkullCompleted.Broadcast(ESCSkullTypes::Score);
	}
}

void USCDossier::OnCounselorEscaped()
{
	bPartyWipeSkullUnlocked = false;
	
	// Only show the failed notification once per challenge match. 
	++EscapedCounselorCount;
	if (EscapedCounselorCount == 1)
	{
		OnSkullFailed.Broadcast(ESCSkullTypes::KillAll);
	}
}

int32 USCDossier::GetXPCurrent() const
{
	if (const UWorld* World = GetWorld())
	{
		// Current XP is stored in the local player's player state... so let's grab that
		const APlayerController* LocalPlayerController = World->GetFirstPlayerController();
		const ASCPlayerState* PlayerState = LocalPlayerController ? Cast<const ASCPlayerState>(LocalPlayerController->PlayerState) : nullptr;
		return PlayerState ? PlayerState->Score : 0;
	}
	
	return 0;
}

int32 USCDossier::GetXPRequired() const
{
	if (!ActiveChallengeClass)
	{
		UE_LOG(LogSC, Warning, TEXT("USCDossier::GetXPRequired() - ActiveChallengeClass is NULL. Returning 0 XPRequired"));
		return 0;
	}

	return ActiveChallengeClass.GetDefaultObject()->GetXPRequiredForSkull();
}

void USCDossier::DebugDraw(const ASCGameState* GameState) const
{
#if !UE_BUILD_SHIPPING
	if (CVarDebugDossier.GetValueOnGameThread() <= 0)
		return;

	UWorld* World = GameState ? GameState->GetWorld() : nullptr;
	APlayerController* LocalPlayerController = World ? World->GetFirstPlayerController() : nullptr;
	ASCPlayerState* PlayerState = LocalPlayerController ? Cast<ASCPlayerState>(LocalPlayerController->PlayerState) : nullptr;

	if (const USCChallengeData* const ActiveChallenge = ActiveChallengeClass.GetDefaultObject())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Dossier %s"), *ActiveChallenge->GetChallengeID()), false);
		for (const FSCObjectiveState& State : ActiveObjectives)
		{
			const USCObjectiveData* const ObjectiveData = State.ObjectiveClass.GetDefaultObject();
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, State.bCompleted ? FColor::Emerald : State.bPreviouslyCompleted ? FColor::Yellow : FColor::Orange, FString::Printf(TEXT(" - Objective %s. %s is %scomplete (was%s previously completed)"), 
				*ObjectiveData->GetObjectiveID(), *ObjectiveData->GetObjectiveTitle().ToString(), State.bCompleted ? TEXT("") : TEXT("not "), State.bPreviouslyCompleted ? TEXT("") : TEXT("n't")), false);
		}

		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, bStealthSkullUnlocked ? FColor::Emerald : FColor::Orange, FString::Printf(TEXT("Stealth skull %s"), bStealthSkullUnlocked ? TEXT("unlocked") : TEXT("locked")), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, bPartyWipeSkullUnlocked ? FColor::Emerald : FColor::Orange, FString::Printf(TEXT("Kill everyone skull %s (%d / %d)"), bPartyWipeSkullUnlocked ? TEXT("unlocked") : TEXT("locked"), PlayerState ? PlayerState->GetKills() : -1, GameState ? GameState->SpawnedCounselorCount : -1), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, EscapedCounselorCount == 0 ? FColor::Emerald : FColor::Orange, FString::Printf(TEXT("Escaped counselors %d"), EscapedCounselorCount), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, bHighScoreSkullUnlocked ? FColor::Emerald : FColor::Orange, FString::Printf(TEXT("High score skull %s (%d / %d)"), bHighScoreSkullUnlocked ? TEXT("unlocked") : TEXT("locked"), PlayerState ? (int32)PlayerState->Score : -1, ActiveChallenge->GetXPRequiredForSkull()), false);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, TEXT("No Active Challenge for Dossier"));
	}
#endif
}
