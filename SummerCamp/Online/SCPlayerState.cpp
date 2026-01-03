// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerState.h"

#include "OnlineAchievementsInterface.h"
#include "OnlineEventsInterface.h"
#include "OnlineKeyValuePair.h"
#include "OnlineLeaderboardInterface.h"
#include "OnlinePresenceInterface.h"
#include "OnlineSubsystem.h"

#include "SCAchievementInfo.h"
#include "SCAudioSettingsSaveGame.h"
#include "SCBackendRequestManager.h"
#include "SCContextKillActor.h"
#include "SCContextKillDamageType.h"
#include "SCCounselorWardrobe.h"
#include "SCDriveableVehicle.h"
#include "SCDynamicContextKill.h"
#include "SCFirstAid.h"
#include "SCGameBlueprintLibrary.h"
#include "SCGame_Menu.h"
#include "SCGameInstance.h"
#include "SCGameMode_Hunt.h"
#include "SCGameSession.h"
#include "SCGameState.h"
#include "SCGameState_Lobby.h"
#include "SCGun.h"
#include "SCInGameHUD.h"
#include "SCKillerMask.h"
#include "SCLocalPlayer.h"
#include "SCNoiseMaker.h"
#include "SCPamelaSweater.h"
#include "SCPamelaTapeDataAsset.h"
#include "SCTommyTapeDataAsset.h"
#include "SCPartyBeaconMemberState.h"
#include "SCPlayerController_Lobby.h"
#include "SCPlayerController.h"
#include "SCPocketKnife.h"
#include "SCRepairPart.h"
#include "SCSinglePlayerSaveGame.h"
#include "SCSpectatorPawn.h"
#include "SCThrowingKnifeProjectile.h"
#include "SCTrap.h"
#include "SCWalkieTalkie.h"
#include "SCWeapon.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCPlayerState"

ASCPlayerState::ASCPlayerState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ASCPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCPlayerState, TotalExperience);
	DOREPLIFETIME(ASCPlayerState, PlayerLevel);
	DOREPLIFETIME(ASCPlayerState, OldPlayerLevel);
	DOREPLIFETIME(ASCPlayerState, XPAmountToCurrentLevel);
	DOREPLIFETIME(ASCPlayerState, XPAmountToNextLevel);
	DOREPLIFETIME(ASCPlayerState, TotalCustomizationPoints);
	DOREPLIFETIME(ASCPlayerState, NumKills);
	DOREPLIFETIME(ASCPlayerState, bHasTakenDamage);
	DOREPLIFETIME(ASCPlayerState, bDead);
	DOREPLIFETIME(ASCPlayerState, bEscaped);
	DOREPLIFETIME(ASCPlayerState, bIsHunter);
	DOREPLIFETIME(ASCPlayerState, bSpectating);
	DOREPLIFETIME(ASCPlayerState, ActiveCharacterClass);
	DOREPLIFETIME(ASCPlayerState, bIsPickedKiller);
	DOREPLIFETIME(ASCPlayerState, PickedCounselorClass);
	DOREPLIFETIME(ASCPlayerState, PickedPerks);
	DOREPLIFETIME(ASCPlayerState, PickedOutfit);
	DOREPLIFETIME(ASCPlayerState, PickedKillerClass);
	DOREPLIFETIME(ASCPlayerState, PickedKillerSkin);
	DOREPLIFETIME(ASCPlayerState, PickedKillerWeapon);
	DOREPLIFETIME(ASCPlayerState, DeathScoreboardDisplayText);

	// Replicated only to the owner
	DOREPLIFETIME_CONDITION(ASCPlayerState, OwnedJasonCharacters, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPlayerState, OwnedJasonKills, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPlayerState, OwnedCounselorCharacters, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPlayerState, OwnedCounselorPerks, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPlayerState, VirtualCabinRank, COND_OwnerOnly);
}

void ASCPlayerState::Reset()
{
	Super::Reset();

	NumKills = 0;
	bHasTakenDamage = false;
	bDead = false;
	bEscaped = false;
	bLoggedOut = false;
	bSpectating = false;
	bSpawnedInMathch = false;
	bPlayedSpawnIntro = false;
	ActivePerks.Empty();
	EventCategoryScores.Empty();
	PlayerScoringEvents.Empty();
	DeathScoreboardDisplayText = FText::FromName(TEXT(""));
	VirtualCabinRank = FString(TEXT(""));
}

void ASCPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (ASCPlayerState* ToPS = Cast<ASCPlayerState>(PlayerState))
	{
		ToPS->TotalExperience = TotalExperience;
		ToPS->PlayerLevel = PlayerLevel;
		ToPS->XPAmountToCurrentLevel = XPAmountToCurrentLevel;
		ToPS->XPAmountToNextLevel = XPAmountToNextLevel;
		ToPS->TotalCustomizationPoints = TotalCustomizationPoints;
		ToPS->NumKills = NumKills;
		ToPS->bHasTakenDamage = bHasTakenDamage;
		ToPS->bDead = bDead;
		ToPS->bEscaped = bEscaped;
		ToPS->bSpectating = bSpectating;
		ToPS->ActiveCharacterClass = ActiveCharacterClass;
		ToPS->ActivePerks = ActivePerks;
		ToPS->PickedOutfit = PickedOutfit;
		ToPS->EventCategoryScores = EventCategoryScores;
		ToPS->PlayerScoringEvents = PlayerScoringEvents;
		ToPS->OwnedJasonCharacters = OwnedJasonCharacters;
		ToPS->OwnedJasonKills = OwnedJasonKills;
		ToPS->OwnedCounselorCharacters = OwnedCounselorCharacters;
		ToPS->OwnedCounselorPerks = OwnedCounselorPerks;
		ToPS->PickedKillerSkin = PickedKillerSkin;
		ToPS->PickedKillerWeapon = PickedKillerWeapon;
		ToPS->SpawnedCharacterClass = SpawnedCharacterClass;
	}
}

bool ASCPlayerState::GetOverrideVoiceAudioComponent(UAudioComponent*& VoIPAudioComponent, bool& bShouldAttenuate, UAudioComponent* CurrentAudioComponent/* = nullptr*/) const
{
	UWorld* World = GetWorld();

	// Get the local player
	ULocalPlayer* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	APlayerController* LocalPC = LocalPlayer ? LocalPlayer->PlayerController : nullptr;
	ASCPlayerState* LocalPS = IsValid(LocalPC) ? Cast<ASCPlayerState>(LocalPC->PlayerState) : nullptr;

	// Default to using 3D audio
	VoIPAudioComponent = VoiceAudioComponent;
	bool bShouldPlay = true;
	bShouldAttenuate = true;

	if (ASCGameState* GS = World ? Cast<ASCGameState>(World->GetGameState()) : nullptr)
	{
		if (GS->IsMatchInProgress())
		{
			// Gather information about the speaking player
			const bool bSpeakerHasWalkieTalkie = GS->WalkieTalkiePlayers.Contains(this);
			const bool bSpeakerIsSpectating = bSpectating;
			const FVector SpeakerLocation = GetPlayerLocation();

			if (LocalPS)
			{
				// Gather information about the local player
				const bool bLocalPlayerHasWalkieTalkie = GS->WalkieTalkiePlayers.Contains(LocalPS);
				const bool bLocalPlayerIsSpectating = LocalPS->GetSpectating();
				const FVector LocalPlayerLocation = [&]() -> FVector
				{
					if (bLocalPlayerIsSpectating)
					{
						FVector ViewLocation;
						FRotator ViewRotation;
						LocalPC->GetPlayerViewPoint(ViewLocation, ViewRotation);
						return ViewLocation;
					}

					return LocalPS->GetPlayerLocation();
				}();

				// Distance from local player to speaking player
				const float DistanceToSpeaker = (LocalPlayerLocation - SpeakerLocation).Size();

				if (bSpeakerHasWalkieTalkie && bLocalPlayerHasWalkieTalkie)
				{
					// Both counselors have walkie talkies, lets see if we should use 2D or 3D audio for VoIP
					if (const ASCWalkieTalkie* const WalkieTalkie = ASCWalkieTalkie::StaticClass()->GetDefaultObject<ASCWalkieTalkie>())
					{
						if (DistanceToSpeaker > WalkieTalkie->MinVoIPDistance)
						{
							// Use 2D audio
							VoIPAudioComponent = nullptr;
							bShouldAttenuate = false;
							UE_LOG_ONLINE(VeryVerbose, TEXT("Both players have walkie talkies, using 2D VoIP."));
						}
					}
				}

				// Check if we are in attenuation range
				const bool bInAttenuationRange = VoiceAudioComponent ? (DistanceToSpeaker < VoiceAudioComponent->AttenuationOverrides.FalloffDistance) : true;

				// Make sure Jason always uses the 3D audio component
				if (IsActiveCharacterKiller() || LocalPS->IsActiveCharacterKiller())
				{
					VoIPAudioComponent = VoiceAudioComponent;
					UE_LOG_ONLINE(VeryVerbose, TEXT("%s player is Jason, using 3D VoIP."), IsActiveCharacterKiller() ? TEXT("Remote") : TEXT("Local"));
				}

				// Spectators always use 2D audio
				if (bSpeakerIsSpectating)
				{
					VoIPAudioComponent = nullptr;
					bShouldAttenuate = false;
					UE_LOG_ONLINE(VeryVerbose, TEXT("Speaker is spectating, using 2D VoIP."));
				}

				// Check if we should play the audio
				const bool bNotInSamePlayingState = (bSpeakerIsSpectating && !bLocalPlayerIsSpectating); // Local player and remote player aren't both alive/dead
				const bool bCantHearSpeaker = (VoIPAudioComponent && !bInAttenuationRange); // Not using a walkie talkie and not in attenuation range 
				if (bNotInSamePlayingState || bCantHearSpeaker)
				{
					bShouldPlay = false;
					UE_LOG_ONLINE(VeryVerbose, TEXT("Both players %s, we should not play VoIP audio."), bNotInSamePlayingState ? TEXT("aren't in the same playing state") : TEXT("aren't in attenuation range"));
				}
			}
		}
		else
		{
			// Use 2D when the match isn't in progress
			VoIPAudioComponent = nullptr;
			bShouldAttenuate = false;
			UE_LOG_ONLINE(VeryVerbose, TEXT("Match isn't in progress, using 2D VoIP."));
		}
	}

	// If we're already using a 2D audio component, keep using it
	if ((VoIPAudioComponent == nullptr) && (CurrentAudioComponent != VoiceAudioComponent) && bShouldPlay)
	{
		VoIPAudioComponent = CurrentAudioComponent;
		bShouldAttenuate = false;
		UE_LOG_ONLINE(VeryVerbose, TEXT("Continuing to use the same 2D audio component."));
	}

	// PC will always prefer using in-world audio, consoles are allowed to opt-in
#if PLATFORM_XBOXONE || PLATFORM_PS4
	if (VoIPAudioComponent)
	{
		if (UILLLocalPlayer* FirstLocalPlayer = World ? Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController()) : nullptr)
		{
			if (USCAudioSettingsSaveGame* AudioSettings = FirstLocalPlayer->GetLoadedSettingsSave<USCAudioSettingsSaveGame>())
			{
				if (AudioSettings->bUseHeadsetForGameVoice)
				{
					VoIPAudioComponent = nullptr;
				}
			}
		}
	}
#endif

	return bShouldPlay;
}

FVector ASCPlayerState::GetPlayerLocation() const
{
	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GS = Cast<ASCGameState>(World->GetGameState()))
		{
			if (bSpectating)
			{
				if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetOwner()))
				{
					return PC->LastSpectatorSyncLocation;
				}
			}
			else
			{
				for (auto& Player : GS->PlayerCharacterList)
				{
					if (IsValid(Player))
					{
						APlayerState* PS = Player->PlayerState;
						if (!PS)
						{
							if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Player))
							{
								if (Counselor->GetCurrentSeat())
									PS = Counselor->GetCurrentSeat()->ParentVehicle->PlayerState;
							}
						}

						if (PS == this)
							return Player->GetActorLocation();
					}
				}
			}
		}
	}

	return Super::GetPlayerLocation();
}

void ASCPlayerState::OverrideWith(class APlayerState* PlayerState)
{
	Super::OverrideWith(PlayerState);

	if (ASCPlayerState* FromPS = Cast<ASCPlayerState>(PlayerState))
	{
		SpawnedCharacterClass = FromPS->SpawnedCharacterClass;
	}
}

void ASCPlayerState::SetAccountId(const FILLBackendPlayerIdentifier& InAccountId) 
{
	Super::SetAccountId(InAccountId);

	// Return persistent player data to this player
	if (Role == ROLE_Authority)
	{
		USCGameInstance* GI = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
		if (GI->IsPickedKillerPlayer(this))
		{
			SetIsPickedKiller(true);
		}
	}
}

bool ASCPlayerState::SERVER_RequestBackendProfileUpdate_Validate()
{
	return true;
}

void ASCPlayerState::SERVER_RequestBackendProfileUpdate_Implementation()
{
	RequestBackendProfileUpdate();
}

void ASCPlayerState::RequestBackendProfileUpdate()
{
	check(Role == ROLE_Authority);

#if WITH_EDITOR
	if (GetWorld() && GetWorld()->IsPlayInEditor())
		return;
#endif

	if (AILLPlayerController* OwnerController = Cast<AILLPlayerController>(GetOwner()))
	{
		if (USCLocalPlayer* LocalPlayer = OwnerController ? Cast<USCLocalPlayer>(OwnerController->GetLocalPlayer()) : nullptr)
		{
			if (USCSinglePlayerSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>())
			{
				if (!SaveGame->HasReceivedProfile())
				{
					SaveGame->OnChallengesLoaded.BindDynamic(this, &ASCPlayerState::VerifyProfileData);
					SaveGame->RequestChallengeProfile(OwnerController);
				}
			}
		}

		USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
		USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
		if (RequestManager->CanSendAnyRequests())
		{
			if (ensure(AccountId.IsValid()))
			{
				RequestManager->GetPlayerAccountProfile(AccountId, FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnRetrieveBackendProfile));
			}
			else
			{
				UE_LOG(LogSC, Warning, TEXT("RequestBackendProfileUpdate called with blank AccountId!"));
			}
		}
	}
}

void ASCPlayerState::RequestPlayerStats()
{
	check(Role == ROLE_Authority);

#if WITH_EDITOR
	if (GetWorld()->IsPlayInEditor())
		return;
#endif

	if (AILLPlayerController* OwnerController = Cast<AILLPlayerController>(GetOwner()))
	{
		USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
		USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
		if (RequestManager->CanSendAnyRequests())
		{
			if (ensure(AccountId.IsValid()))
			{
				FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnRetrievePlayerStats);
				RequestManager->GetPlayerStats(AccountId, Delegate);
			}
			else
			{
				UE_LOG(LogSC, Warning, TEXT("RequestPlayerStats called with blank AccountId!"));
			}
		}
	}
}

void ASCPlayerState::RequestPlayerBadges()
{
	check(Role == ROLE_Authority);

#if WITH_EDITOR
	if (GetWorld()->IsPlayInEditor())
		return;
#endif

	if (AILLPlayerController* OwnerController = Cast<AILLPlayerController>(GetOwner()))
	{
		USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
		USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
		if (RequestManager->CanSendAnyRequests())
		{
			if (ensure(AccountId.IsValid()))
			{
				RequestManager->GetPlayerAchievements(AccountId, FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnRetrievePlayerBadges));
			}
			else
			{
				UE_LOG(LogSC, Warning, TEXT("RequestPlayerBadges called with blank AccountId!"));
			}
		}
	}
}

void ASCPlayerState::OnRetrieveBackendProfile(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("OnRetrieveBackendProfile: ResponseCode %i"), ResponseCode);
		return;
	}

	// Parse Json
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContents);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		// Get Account Object
		const TSharedPtr<FJsonObject>* JAccountObj;
		if (JsonObject->TryGetObjectField(TEXT("Account"), JAccountObj))
		{
			// Get Experience and Player level info from the Profile
			const TSharedPtr<FJsonObject>* JExpObj;
			if ((*JAccountObj)->TryGetObjectField(TEXT("ExperienceProfile"), JExpObj))
			{
				(*JExpObj)->TryGetNumberField(TEXT("TotalExperience"), TotalExperience);
				OldPlayerLevel = PlayerLevel;
				(*JExpObj)->TryGetNumberField(TEXT("Level"), PlayerLevel);


				UE_LOG(LogSC, Log, TEXT("Received Backend data: Received Player level: %i"), PlayerLevel);

				(*JExpObj)->TryGetNumberField(TEXT("ToCurrentLevel"), XPAmountToCurrentLevel);
				(*JExpObj)->TryGetNumberField(TEXT("ToNextLevel"), XPAmountToNextLevel);
			}

			// Get Wallet Profile to retrieve Customization Points
			const TSharedPtr<FJsonObject>* JWalletObj;
			if ((*JAccountObj)->TryGetObjectField(TEXT("WalletProfile"), JWalletObj))
			{
				(*JWalletObj)->TryGetNumberField(TEXT("CustomizationPoints"), TotalCustomizationPoints);
			}

			// Get Inventory Profile
			const TSharedPtr<FJsonObject>* JInventoryObj;
			if ((*JAccountObj)->TryGetObjectField(TEXT("InventoryProfile"), JInventoryObj))
			{
				// Get the player's unlocked Jason Characters
				const TArray<TSharedPtr<FJsonValue> > * JsonJasons = nullptr;
				if ((*JInventoryObj)->TryGetArrayField(TEXT("Jasons"), JsonJasons))
				{
					TArray<TSoftClassPtr<ASCKillerCharacter>> NewOwnedJasonCharacters;
					for (TSharedPtr<FJsonValue> JsonItem : *JsonJasons)
					{
						FString KillerCharacter = JsonItem->AsString();
						int32 SlashPosition;

						if (KillerCharacter.FindChar('/', SlashPosition))
						{
							FString KillerPath = KillerCharacter.Left(SlashPosition);
							FString KillerClassname = KillerCharacter.Mid(SlashPosition + 1);

							FString ClassString = FString::Printf(TEXT("/Game/Characters/Killers/Jason/%s/%s.%s_C"), *KillerPath, *KillerClassname, *KillerClassname);
							NewOwnedJasonCharacters.Add(TSoftClassPtr<ASCKillerCharacter>(FSoftObjectPath(ClassString)));
						}
					}
					
					if (NewOwnedJasonCharacters.Num() > OwnedJasonCharacters.Num())
					{
						OwnedJasonCharacters = NewOwnedJasonCharacters;
					}
				}

				// Get the player's owned Jason Kills
				const TArray<TSharedPtr<FJsonValue> > * JsonJasonKills = nullptr;
				if ((*JInventoryObj)->TryGetArrayField(TEXT("JasonKills"), JsonJasonKills))
				{
					TArray<TSubclassOf<ASCDynamicContextKill>> NewOwnedJasonKills;
					for (TSharedPtr<FJsonValue> JsonItem : *JsonJasonKills)
					{						
						FString ClassString = FString::Printf(TEXT("/Game/Blueprints/ContextKills/%s.%s_C"), *JsonItem->AsString(), *JsonItem->AsString());
						NewOwnedJasonKills.Add(LoadObject<UClass>(NULL, *ClassString, NULL, LOAD_None, NULL));
					}

					if (NewOwnedJasonKills.Num() > OwnedJasonKills.Num())
					{
						OwnedJasonKills = NewOwnedJasonKills;
						OnRep_OwnedJasonKills();
					}
				}

				// Get the player's unlocked Counselor Characters
				const TArray<TSharedPtr<FJsonValue> > * JsonCounselors = nullptr;
				if ((*JInventoryObj)->TryGetArrayField(TEXT("Counselors"), JsonCounselors))
				{
					TArray<TSoftClassPtr<ASCCounselorCharacter>> NewOwnedCounselorCharacters;
					for (TSharedPtr<FJsonValue> JsonItem : *JsonCounselors)
					{
						FString CounselorCharacter = JsonItem->AsString();
						int32 SlashPosition;

						if (CounselorCharacter.FindChar('/', SlashPosition))
						{
							FString CounselorPath = CounselorCharacter.Left(SlashPosition);
							FString CounselorClassname = CounselorCharacter.Mid(SlashPosition + 1);

							FString ClassString = FString::Printf(TEXT("/Game/Characters/Counselors/%s/%s.%s_C"), *CounselorPath, *CounselorClassname, *CounselorClassname);
							NewOwnedCounselorCharacters.Add(TSoftClassPtr<ASCCounselorCharacter>(FSoftObjectPath(ClassString)));
						}
					}

					if (NewOwnedCounselorCharacters.Num() > OwnedCounselorCharacters.Num())
					{
						OwnedCounselorCharacters = NewOwnedCounselorCharacters;
					}
				}

				// Get the player's owned Counselor Perks
				const TArray<TSharedPtr<FJsonValue> > * JsonCounselorPerks = nullptr;
				if ((*JInventoryObj)->TryGetArrayField(TEXT("CounselorPerkInstances"), JsonCounselorPerks))
				{
					TArray<FSCPerkBackendData> NewOwnedCounselorPerks;
					for (TSharedPtr<FJsonValue> JsonItem : *JsonCounselorPerks)
					{
						// Get it as an object.
						const TSharedPtr<FJsonObject>* JCounselorPerkObj;
						JsonItem->TryGetObject(JCounselorPerkObj);
						if (!JCounselorPerkObj) // Paranoid
							continue;

						FString InstanceID;
						(*JCounselorPerkObj)->TryGetStringField(TEXT("InstanceID"), InstanceID);
						if (InstanceID.IsEmpty())
						{
							UE_LOG(LogSC, Warning, TEXT("Blank perk instance id retrieved from backend."));
							continue;
						}

						// Get the Class Name
						FString ClassName;
						(*JCounselorPerkObj)->TryGetStringField(TEXT("ClassName"), ClassName);
						if (ClassName.IsEmpty())
						{
							UE_LOG(LogSC, Warning, TEXT("Blank perk class retrieved from backend. Perk InstanceID: %s"), *InstanceID);
							continue;
						}

						// Format the Class String
						FString ClassString = FString::Printf(TEXT("/Game/Blueprints/Perks/%s.%s_C"), *ClassName, *ClassName);

						FSCPerkBackendData PerkBackendData;

						// Save off the InstanceID which is used when cashing in a perk.
						PerkBackendData.InstanceID = InstanceID;

						// Add the class string to the Perk Backend data struct.
						PerkBackendData.PerkClass = LoadObject<UClass>(NULL, *ClassString, NULL, LOAD_None, NULL);

						// Grab the Perk modifier variables that we need to modify.
						FSCPerkModifier PositiveModifier;
						FSCPerkModifier NegativeModifier;
						FSCPerkModifier LegendaryModifier;
						const TArray<TSharedPtr<FJsonValue>> * JsonPerkModifiers = nullptr;
						if ((*JCounselorPerkObj)->TryGetArrayField(TEXT("Modifiers"), JsonPerkModifiers))
						{
							int index = 0;
							for (TSharedPtr<FJsonValue> JsonPerkModifier : *JsonPerkModifiers)
							{
								const TSharedPtr<FJsonObject>* JsonPerkModifierObj;
								JsonPerkModifier->TryGetObject(JsonPerkModifierObj);

								if (index == 0)
								{
									(*JsonPerkModifierObj)->TryGetStringField(TEXT("Name"), PositiveModifier.ModifierVariableName);
									(*JsonPerkModifierObj)->TryGetNumberField(TEXT("Value"), PositiveModifier.ModifierPercentage);
								}
								else if (index == 1)
								{
									(*JsonPerkModifierObj)->TryGetStringField(TEXT("Name"), NegativeModifier.ModifierVariableName);
									(*JsonPerkModifierObj)->TryGetNumberField(TEXT("Value"), NegativeModifier.ModifierPercentage);
								}
								else
								{
									(*JsonPerkModifierObj)->TryGetStringField(TEXT("Name"), LegendaryModifier.ModifierVariableName);
									(*JsonPerkModifierObj)->TryGetNumberField(TEXT("Value"), LegendaryModifier.ModifierPercentage);
								}
								++index;
							}
						}

						// Set the modifiers into our backend struct.
						PerkBackendData.PositivePerkModifiers = PositiveModifier;
						PerkBackendData.NegativePerkModifiers = NegativeModifier;
						PerkBackendData.LegendaryPerkModifiers = LegendaryModifier;

						// Grab the cash in value.
						(*JCounselorPerkObj)->TryGetNumberField(TEXT("CashInValue"), PerkBackendData.BuyBackCP);

						// Grab the bonus cash in value (for legendary perks).
						(*JCounselorPerkObj)->TryGetNumberField(TEXT("CashInBonus"), PerkBackendData.CashInBonusCP);

						// Add it to the Owned Counselor perk array.
						NewOwnedCounselorPerks.Add(PerkBackendData);
					}

					// Doing this so that it serializes across the network.
					if (NewOwnedCounselorPerks.Num() != OwnedCounselorPerks.Num())
					{
						// Sort the perks before replicating.
						NewOwnedCounselorPerks.Sort([](const FSCPerkBackendData& A, const FSCPerkBackendData& B) 
						{ 
							FString NameA;
							FString NameB;
							if (const USCPerkData* const PerkA = Cast<USCPerkData>(A.PerkClass->GetDefaultObject()))
								NameA = PerkA->GetName();

							if (const USCPerkData* const PerkB = Cast<USCPerkData>(B.PerkClass->GetDefaultObject()))
								NameB = PerkB->GetName();

							return NameA < NameB; 
						});

						OwnedCounselorPerks = NewOwnedCounselorPerks;
						OnRep_OwnedCounselorPerks();
					}
				}

				// Get the player's unlocked Pamela Tapes.
				const TArray<TSharedPtr<FJsonValue> > * JsonPamelaTapes = nullptr;
				if ((*JInventoryObj)->TryGetArrayField(TEXT("PamelaTapes"), JsonPamelaTapes))
				{
					TArray<TSubclassOf<USCPamelaTapeDataAsset>> NewUnlockedPamelaTapes;
					for (TSharedPtr<FJsonValue> PamelaTape : *JsonPamelaTapes)
					{
						FString ClassString = FString::Printf(TEXT("/Game/Blueprints/PamelaTapes/%s.%s_C"), *PamelaTape->AsString(), *PamelaTape->AsString());
						NewUnlockedPamelaTapes.Add(LoadObject<UClass>(NULL, *ClassString, NULL, LOAD_None, NULL));
					}

					if (NewUnlockedPamelaTapes.Num() != UnlockedPamelaTapes.Num())
					{
						UnlockedPamelaTapes = NewUnlockedPamelaTapes;
					}
				}

				// Get the player's unlocked Tommy Tapes.
				const TArray<TSharedPtr<FJsonValue> > * JsonTommyTapes = nullptr;
				if ((*JInventoryObj)->TryGetArrayField(TEXT("TommyTapes"), JsonTommyTapes))
				{
					TArray<TSubclassOf<USCTommyTapeDataAsset>> NewUnlockedTommyTapes;
					for (TSharedPtr<FJsonValue> TommyTape : *JsonTommyTapes)
					{
						FString ClassString = FString::Printf(TEXT("/Game/Blueprints/TommyTapes/%s.%s_C"), *TommyTape->AsString(), *TommyTape->AsString());
						NewUnlockedTommyTapes.Add(LoadObject<UClass>(NULL, *ClassString, NULL, LOAD_None, NULL));
					}

					if (NewUnlockedTommyTapes.Num() != UnlockedTommyTapes.Num())
					{
						UnlockedTommyTapes = NewUnlockedTommyTapes;
					}
				}
			}
		}

		// Update the party state
		if (ASCPartyBeaconMemberState* PartyState = Cast<ASCPartyBeaconMemberState>(GetPartyMemberState()))
		{
			PartyState->UpdateBackendProfileFrom(this);
		}

		// Verify ownership of selections
		if (!PickedCounselorClass.IsNull())
		{
			SetCounselorClass(PickedCounselorClass);
		}
		SetCounselorPerks(PickedPerks);
		// Outfits?

		if (!PickedKillerClass.IsNull())
		{
			SetKillerClass(PickedKillerClass);
		}
		SetKillerGrabKills(PickedGrabKills);

		VerifyProfileData();
	}
}

void ASCPlayerState::OnRetrievePlayerStats(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("OnRetrievePlayerStats: ResponseCode %i Reason: %s"), ResponseCode, *ResponseContents);
		return;
	}

	// Parse the persistent stats
	PersistentStats.ParseStats(ResponseContents, this);
}

void ASCPlayerState::OnRetrievePlayerBadges(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("OnRetrievePlayerBadges: ResponseCode %i Reason: %s"), ResponseCode, *ResponseContents);
		return;
	}

	BadgesAwarded.Empty();

	// Parse response
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContents);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		const TSharedPtr<FJsonObject>* AchievementProfile;
		if (JsonObject->TryGetObjectField(TEXT("AchievementsProfile"), AchievementProfile))
		{
			const TArray<TSharedPtr<FJsonValue>>* Badges = nullptr;
			if ((*AchievementProfile)->TryGetArrayField(TEXT("Badges"), Badges))
			{
				for (TSharedPtr<FJsonValue> Badge : *Badges)
				{
					const TSharedPtr<FJsonObject>* BadgeData;
					if (Badge->TryGetObject(BadgeData))
					{
						FString BadgeName;
						int32 AwardCount;
						(*BadgeData)->TryGetStringField(TEXT("ClassName"), BadgeName);
						(*BadgeData)->TryGetNumberField(TEXT("AwardCount"), AwardCount);

						BadgesAwarded.Add(*BadgeName, AwardCount);
					}
				}
			}

			const TArray<TSharedPtr<FJsonValue>>* Awards = nullptr;
			if ((*AchievementProfile)->TryGetArrayField(TEXT("Awards"), Awards))
			{
				for (TSharedPtr<FJsonValue> Award : *Awards)
				{
					const TSharedPtr<FJsonObject>* AwardData;
					if (Award->TryGetObject(AwardData))
					{
						FString AwardName;
						FString AwardValue;
						(*AwardData)->TryGetStringField(TEXT("AwardID"), AwardName);
						(*AwardData)->TryGetStringField(TEXT("Value"), AwardValue);

						// We've completed the virtual cabin.
						if (AwardName == TEXT("u0"))
						{
							VirtualCabinRank = AwardValue;
						}
					}
				}
			}
		}
	}
}

void ASCPlayerState::VerifyProfileData()
{
	// Backend updated, validate our save
	AILLPlayerController* OwnerController = Cast<AILLPlayerController>(GetOwner());
	USCLocalPlayer* LocalPlayer = OwnerController ? Cast<USCLocalPlayer>(OwnerController->GetLocalPlayer()) : nullptr;
	USCCharacterSelectionsSaveGame* Settings = LocalPlayer ? LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>() : nullptr;
	if (Settings)
		Settings->VerifySettingOwnership(OwnerController, true);
}

void ASCPlayerState::OnRep_OwnedCounselorPerks()
{
	OwnedPerksUpdated.Broadcast();
}

void ASCPlayerState::OnRep_OwnedJasonKills()
{
	OwnedJasonKillsUpdated.Broadcast();
}

const TArray<FSCBadgeAwardInfo> ASCPlayerState::GetBadgesUnlocked() const
{
	TArray<FSCBadgeAwardInfo> BadgesUnlocked;

	// Go through our badges earned this match and see if we've unlocked a one, two, or three star version of the badge
	for (TSubclassOf<USCStatBadge> BadgeEarned : BadgesEarned)
	{
		if (const USCStatBadge* const Badge = BadgeEarned->GetDefaultObject<USCStatBadge>())
		{
			// Get the number of times we've earned this badge
			const int32 TimesAwarded = GetBadgeAwardCount(*Badge->BackendId, *Badge->DisplayName.ToString()) + 1; // +1 since we've earned it this match

			// Check for one, two, or three star unlocks
			if (TimesAwarded == Badge->OneStarUnlock)
				BadgesUnlocked.Add(FSCBadgeAwardInfo(BadgeEarned, 1));
			else if (TimesAwarded == Badge->TwoStarUnlock)
				BadgesUnlocked.Add(FSCBadgeAwardInfo(BadgeEarned, 2));
			else if (TimesAwarded == Badge->ThreeStarUnlock)
				BadgesUnlocked.Add(FSCBadgeAwardInfo(BadgeEarned, 3));
		}
	}

	return BadgesUnlocked;
}

int32 ASCPlayerState::GetBadgeAwardCount(const FName BackendId, const FName BadgeDisplayName) const
{
	int BadgeAwardCount = 0;

	// Check the times awarded since changing to BackendId
	if (BadgesAwarded.Contains(BackendId))
		BadgeAwardCount += BadgesAwarded[BackendId];

	// Check the times awarded before BackendId was used
	if (BackendId != BadgeDisplayName)
	{
		if (BadgesAwarded.Contains(BadgeDisplayName))
			BadgeAwardCount += BadgesAwarded[BadgeDisplayName];
	}

	return BadgeAwardCount;
}

float ASCPlayerState::GetCurrentLevelPercentage() const
{
	if (TotalExperience > 0)
	{
		const double ExperienceIntoLevel = TotalExperience - XPAmountToCurrentLevel;
		const double TotalExperienceForLevel = (TotalExperience + XPAmountToNextLevel) - XPAmountToCurrentLevel;
		if (TotalExperienceForLevel > FLT_EPSILON)
		{
			return (float)(ExperienceIntoLevel / TotalExperienceForLevel);
		}
	}

	return 0.f;
}

bool ASCPlayerState::DoesPlayerOwnPerk(TSubclassOf<USCPerkData> PerkClass)
{
	for (int i = 0; i < OwnedCounselorPerks.Num(); ++i)
	{
		if (OwnedCounselorPerks[i].PerkClass == PerkClass)
		{
			return true;
		}
	}
	return false;
}

bool ASCPlayerState::DoesPlayerOwnKill(TSubclassOf<ASCContextKillActor> KillClass)
{
	// Not a kill, I guess you own that
	if (!KillClass)
		return true;

	if (AILLPlayerController* OwnerController = Cast<AILLPlayerController>(GetOwner()))
	{
		if (const ASCContextKillActor* const ContextKill = KillClass ? KillClass->GetDefaultObject<ASCContextKillActor>() : nullptr)
		{
			// If this is part of A DLC pack we can check entitlements, otherwise Compare against our owned kills. The owned kills list does not contain DLC kills.
			if (!ContextKill->DLCEntitlement.IsEmpty() || OwnedJasonKills.Contains(KillClass) || ContextKill->GetCost() == 0)
			{
				// Check our DLC
				if (!OwnerController->DoesUserOwnEntitlement(ContextKill->DLCEntitlement))
					return false;

				// Check our level
				if (PlayerLevel < ContextKill->LevelRequirement)
					return false;

				// All good!
				return true;
			}
		}
	}

	return false;
}

void ASCPlayerState::InformAboutKill_Implementation(const UDamageType* const KillerDamageType, ASCPlayerState* VictimPlayerState)
{
}

void ASCPlayerState::InformAboutKilled_Implementation(ASCPlayerState* KillerPlayerState, const UDamageType* const KillerDamageType)
{
	if (ASCPlayerController* OwnerController = Cast<ASCPlayerController>(GetOwner()))
	{
		// Notify local HUD
		if (OwnerController->IsLocalController())
		{
			if (ASCInGameHUD* Hud = OwnerController->GetSCHUD())
			{
				Hud->OnCharacterDeath(KillerPlayerState, KillerDamageType);
			}
		}
	}
}

void ASCPlayerState::InformAboutDeath_Implementation(ASCPlayerState* KillerPlayerState, const UDamageType* const KillerDamageType, ASCPlayerState* VictimPlayerState)
{
}

void ASCPlayerState::InformAboutSuicide_Implementation(const UDamageType* const KillerDamageType)
{
}

void ASCPlayerState::InformAboutEscape_Implementation(ASCPlayerState* EscapedPlayerState)
{
}

bool ASCPlayerState::InformIntroCompleted_Validate()
{
	return true;
}

void ASCPlayerState::InformIntroCompleted_Implementation()
{
	// Mark ourselves as completed, then let the game settings know
	bIntroSequenceCompleted = true;
	Cast<ASCWorldSettings>(GetWorld()->GetAuthGameMode()->GetWorldSettings())->CheckClientsIntroState();
}

void ASCPlayerState::ScoreKill(const ASCPlayerState* Victim, const int32 Points, const UDamageType* const KillInfo)
{
	NumKills++;
	ScorePoints(Points);

	if (Victim && Victim != this)
	{
		if (IsActiveCharacterKiller())
		{
			// Hunter doesn't count as a Counselor Kill.
			if (!Victim->bIsHunter)
				MatchStats.CounselorsKilled++;

			if (Role == ROLE_Authority)
			{
				// Kill count achievements
				UpdateAchievement(ACH_FIRST_BLOOD);

				const int32 TotalCounselorsKilled = MatchStats.CounselorsKilled + PersistentStats.CounselorsKilled;
				UpdateLeaderboardStats(LEADERBOARD_STAT_COUNSELORS_KILLED, TotalCounselorsKilled);

				const float LuckyNumberProgress = ((float)TotalCounselorsKilled / (float)SCAchievements::LuckNumberKillCount) * 100.0f;
				UpdateAchievement(ACH_MY_LUCKY_NUMBER, LuckyNumberProgress);

				const float EvilLurksProgress = ((float)TotalCounselorsKilled / (float)SCAchievements::EvilLurksKillCount) * 100.0f;
				UpdateAchievement(ACH_EVIL_LURKS, EvilLurksProgress);

				const float KillEmAllProgress = ((float)TotalCounselorsKilled / (float)SCAchievements::KillEmAllKillCount) * 100.0f;
				UpdateAchievement(ACH_GOTTA_KILL_EM_ALL, KillEmAllProgress);
			}
		}
		else
		{
			if (Victim->IsActiveCharacterKiller())
			{
				MatchStats.KilledJasonCount++;

				if (Role == ROLE_Authority)
				{
					// Killed Jason
					UpdateLeaderboardStats(LEADERBOARD_STAT_ROLL_CREDITS);
					UpdateAchievement(ACH_ROLL_CREDITS);
				}
			}
			else
			{
				MatchStats.KilledAnotherCounselorCount++;
			}
		}
	}

	if (KillInfo && KillInfo->IsA<USCContextKillDamageType>())
	{
		PerformedContextKill(KillInfo->GetClass());
	}
}

void ASCPlayerState::ScoreDeath(const ASCPlayerState* KilledBy, const int32 Points, const UDamageType* const KillInfo)
{
	bDead = true;
	bHasTakenDamage = true;
	DeathTime = FDateTime::UtcNow().ToUnixTimestamp();
	ScorePoints(Points);

	UWorld* World = GetWorld();
	AController* Controller = Cast<AController>(GetOwner());
	if (ASCGameState* GameState = Cast<ASCGameState>(World ? World->GetGameState() : nullptr))
		GameState->CharacterKilled(Cast<ASCCharacter>(Controller ? Controller->GetCharacter() : nullptr), KillInfo);

	if (KilledBy == this)
		MatchStats.Suicides++;
	else if (KilledBy && KilledBy->IsActiveCharacterKiller())
		MatchStats.DeathsByJason++;
	else if (KilledBy)
		MatchStats.DeathsByOtherCounselors++;

	const FText DeathDisplayText = [&]() -> FText
	{
		if (KillInfo && KillInfo->IsA<USCContextKillDamageType>())
		{
			const USCContextKillDamageType* const Kill = Cast<USCContextKillDamageType>(KillInfo);
			return Kill->ScoreboardDescription;
		}
		else if (KilledBy == this)
		{
			return LOCTEXT("DeathDisplay_Suicide","Suicide");
		}
		else if (KilledBy && !KilledBy->IsActiveCharacterKiller())
		{
			return LOCTEXT("DeathDisplay_Betrayed", "Betrayed");
		}

		return LOCTEXT("DeathDisplay_Murdered", "Murdered");
	}();

	DeathScoreboardDisplayText = DeathDisplayText;

	if (Role == ROLE_Authority)
	{
		// If we are a listen server attempt to let people spectate
		OnRep_DeadOrEscaped();

		if (KilledBy && KilledBy->IsActiveCharacterKiller())
		{
			UpdateAchievement(ACH_YOU_DIED);

			const int32 TotalDeathsByJason = MatchStats.DeathsByJason + PersistentStats.DeathsByJason;
			UpdateLeaderboardStats(LEADERBOARD_STAT_DEATH_BY_JASON, TotalDeathsByJason);

			const float YouDiedProgress = ((float)TotalDeathsByJason / (float)SCAchievements::YouDiedALotCount) * 100.0f;
			UpdateAchievement(ACH_YOU_DIED_ALOT, YouDiedProgress);
		}
	}
}

void ASCPlayerState::ScoreEscape(const int32 Points, const ASCDriveableVehicle* EscapeVehicle)
{
	bEscaped = true;
	EscapedTime = FDateTime::UtcNow().ToUnixTimestamp();
	ScorePoints(Points);

	ASCCounselorCharacter* Driver = EscapeVehicle ? Cast<ASCCounselorCharacter>(EscapeVehicle->Driver) : nullptr;
	const bool bWasDriver = (Driver && Driver->PlayerState == this) || (EscapeVehicle && EscapeVehicle->PlayerState == this); // Driver is possessing the car, so it has the PlayerState
	const int32 VehiclePassengerCount = EscapeVehicle ? EscapeVehicle->GetNumberOfPeopleInVehicle() : 0;

	// Track escape stats
	if (EscapeVehicle)
	{
		if (EscapeVehicle->VehicleType == EDriveableVehicleType::Car)
		{
			if (bWasDriver)
				MatchStats.EscapedInCarAsDriverCount++;
			else
				MatchStats.EscapedInCarAsPassengerCount++;
		}
		else if (EscapeVehicle->VehicleType == EDriveableVehicleType::Boat)
		{
			if (bWasDriver)
				MatchStats.EscapedInBoatAsDriverCount++;
			else
				MatchStats.EscapedInBoatAsPassengerCount++;
		}
	}
	else
	{
		// Police escape
		MatchStats.EscapedWithPoliceCount++;
	}

	
	if (Role == ROLE_Authority)
	{
		// If we are a listen server attempt to let people spectate
		OnRep_DeadOrEscaped();

		// Update escape achievements
		if (EscapeVehicle)
		{
			if (EscapeVehicle->VehicleType == EDriveableVehicleType::Car)
			{
				if (bWasDriver)
				{
					UpdateLeaderboardStats(LEADERBOARD_STAT_FRIDAY_DRIVER);
					UpdateAchievement(ACH_FRIDAY_DRIVER);

					if (ASCGameState* GS = GetWorld()->GetGameState<ASCGameState>())
					{
						// We didn't help anyone else escape
						if (VehiclePassengerCount == 1 && GS->GetAliveCounselorCount(this) > 0)
						{
							UpdateLeaderboardStats(LEADERBOARD_STAT_CHAD_IS_A_DICK);
							UpdateAchievement(ACH_CHAD_IS_A_DICK);
						}
					}
				}
				else
				{
					const int32 CarPassengerEscapes = MatchStats.EscapedInCarAsPassengerCount + PersistentStats.EscapedInCarAsPassengerCount;
					UpdateLeaderboardStats(LEADERBOARD_STAT_CAR_PASSENGER_ESCAPE, CarPassengerEscapes);

					const float AlongForTheRideProgress = ((float)CarPassengerEscapes / (float)SCAchievements::AlongForTheRideEscapeCount) * 100.0f;
					UpdateAchievement(ACH_ALONG_FOR_THE_RIDE, AlongForTheRideProgress);
				}

				EarnedBadge(EscapeVehicle->CarEscapeBadge);
			}
			else if (EscapeVehicle->VehicleType == EDriveableVehicleType::Boat)
			{
				if (bWasDriver)
				{
					UpdateLeaderboardStats(LEADERBOARD_STAT_AYE_AYE_CAPTAIN);
					UpdateAchievement(ACH_AYE_AYE_CAPTAIN);

					if (ASCGameState* GS = GetWorld()->GetGameState<ASCGameState>())
					{
						// We didn't help anyone else escape
						if (VehiclePassengerCount == 1 && GS->GetAliveCounselorCount(this) > 0)
						{
							UpdateLeaderboardStats(LEADERBOARD_STAT_CHAD_IS_A_DICK);
							UpdateAchievement(ACH_CHAD_IS_A_DICK);
						}
					}
				}
				else
				{
					const int32 BoatPassengerEscapes = MatchStats.EscapedInBoatAsPassengerCount + PersistentStats.EscapedInBoatAsPassengerCount;
					UpdateLeaderboardStats(LEADERBOARD_STAT_BOAT_PASSENGER_ESCAPE, BoatPassengerEscapes);

					const float OnABoatProgress = ((float)BoatPassengerEscapes / (float)SCAchievements::ImOnABoatEscapeCount) * 100.0f;
					UpdateAchievement(ACH_ON_A_BOAT, OnABoatProgress);
				}

				EarnedBadge(EscapeVehicle->BoatEscapeBadge);
			}
		}
		else
		{
			// Police escape
			const int32 PoliceEscapes = MatchStats.EscapedWithPoliceCount + PersistentStats.EscapedWithPoliceCount;
			UpdateLeaderboardStats(LEADERBOARD_STAT_POLICE_ESCAPES, PoliceEscapes);

			const float CrystalLakeFiveOProgress = ((float)PoliceEscapes / (float)SCAchievements::CrystalLakeFiveOCount) * 100.0f;
			UpdateAchievement(ACH_CRYSTAL_LAKE_FIVE_O, CrystalLakeFiveOProgress);

			if (ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>())
			{
				EarnedBadge(GM->EscapeWithPoliceBadge);
			}
		}
	}
}

void ASCPlayerState::ScorePoints(int32 Points)
{
	const int32 PreviousScore = Score;
	Score += Points;

	// Notify the player character owner
	if (ASCPlayerController* OwnerController = Cast<ASCPlayerController>(GetOwner()))
	{
		OwnerController->ScoreChanged(PreviousScore, Score);
	}

	// Notify the game state we're awesome (single player will use this to update skull unlocks)
	UWorld* World = GetWorld();
	if (ASCGameState* GameState = World ? Cast<ASCGameState>(World->GetGameState()) : nullptr)
	{
		GameState->CharacterEarnedXP(this, Points);
	}
}

void ASCPlayerState::BoatPartRepaired()
{
	MatchStats.BoatPartsRepaired++;

	if (Role == ROLE_Authority)
	{
		const int32 BoatPartsRepaired = MatchStats.BoatPartsRepaired + PersistentStats.BoatPartsRepaired;
		UpdateLeaderboardStats(LEADERBOARD_STAT_BOAT_REPAIRS, BoatPartsRepaired);

		const float ShipWrightProgress = ((float)BoatPartsRepaired / (float)SCAchievements::ShipwrightRepairCount) * 100.0f;
		UpdateAchievement(ACH_SHIPWRIGHT, ShipWrightProgress);
	}
}

void ASCPlayerState::CarPartRepaired()
{
	MatchStats.CarPartsRepaired++;

	if (Role == ROLE_Authority)
	{
		const int32 CarPartsRepaired = MatchStats.CarPartsRepaired + PersistentStats.CarPartsRepaired;
		UpdateLeaderboardStats(LEADERBOARD_STAT_CAR_REPAIRS, CarPartsRepaired);

		const float GreaseMonkeyProgress = ((float)CarPartsRepaired / (float)SCAchievements::GreaseMonkeyRepairCount) * 100.0f;
		UpdateAchievement(ACH_GREASE_MONKEY, GreaseMonkeyProgress);
	}
}

void ASCPlayerState::GridPartRepaired()
{
	MatchStats.GridPartsRepaired++;

	if (Role == ROLE_Authority)
	{
		const int32 GridPartsRepaired = MatchStats.GridPartsRepaired + PersistentStats.GridPartsRepaired;
		UpdateLeaderboardStats(LEADERBOARD_STAT_GRID_REPAIRS, GridPartsRepaired);

		const float ShockJockeyProgress = ((float)GridPartsRepaired / (float)SCAchievements::ShockJockeyRepairCount) * 100.0f;
		UpdateAchievement(ACH_SHOCK_JOCKEY, ShockJockeyProgress);
	}
}

void ASCPlayerState::CalledPolice()
{
	MatchStats.PoliceCallCount++;

	if (Role == ROLE_Authority)
	{
		const int32 PoliceCalledCount = MatchStats.PoliceCallCount + PersistentStats.PoliceCallCount;
		UpdateLeaderboardStats(LEADERBOARD_STAT_POLICE_CALLED, PoliceCalledCount);

		const float OperatorProgress = ((float)PoliceCalledCount / (float)SCAchievements::OperatorCallCount) * 100.0f;
		UpdateAchievement(ACH_OPERATOR, OperatorProgress);
	}
}

void ASCPlayerState::CalledHunter()
{
	MatchStats.TommyJarvisCallCount++;

	if (Role == ROLE_Authority)
	{
		const int32 TommyCalledCount = MatchStats.TommyJarvisCallCount + PersistentStats.TommyJarvisCallCount;
		UpdateLeaderboardStats(LEADERBOARD_STAT_TJ_CALLED, TommyCalledCount);

		const float DiscJockeyProgress = ((float)TommyCalledCount / (float)SCAchievements::DiscJockeyCallCount) * 100.0f;
		UpdateAchievement(ACH_DISC_JOCKEY, DiscJockeyProgress);
	}
}

void ASCPlayerState::SpawnedAsHunter()
{
	MatchStats.TommyJarvisMatchesPlayed++;

	if (Role == ROLE_Authority)
	{
		bIsHunter = true;

		const int32 TommyMatchesPlayed = MatchStats.TommyJarvisMatchesPlayed + PersistentStats.TommyJarvisMatchesPlayed;
		UpdateLeaderboardStats(LEADERBOARD_STAT_PLAY_AS_TJ, TommyMatchesPlayed);

		const float HeresTommyProgress = ((float)TommyMatchesPlayed / (float)SCAchievements::TommyPlayCount) * 100.0f;
		UpdateAchievement(ACH_HERE_IS_TOMMY, HeresTommyProgress);
	}

}

void ASCPlayerState::CarSlammed()
{
	MatchStats.CarSlamCount++;

	if (Role == ROLE_Authority)
	{
		const int32 TotalCarSlams = MatchStats.CarSlamCount + PersistentStats.CarSlamCount;
		UpdateLeaderboardStats(LEADERBOARD_STAT_SLAM_JAM, TotalCarSlams);

		const float SlamJamProgress = ((float)TotalCarSlams / (float)SCAchievements::SlamJamCount) * 100.0f;
		UpdateAchievement(ACH_SLAM_JAM, SlamJamProgress);
	}
}

void ASCPlayerState::BrokeDoorDown()
{
	if (IsActiveCharacterKiller())
	{
		MatchStats.DoorsBrokenDown++;

		if (Role == ROLE_Authority)
		{
			const int32 TotalDoorsBrokenDown = MatchStats.DoorsBrokenDown + PersistentStats.DoorsBrokenDown;
			UpdateLeaderboardStats(LEADERBOARD_STAT_DOOR_BREAKS, TotalDoorsBrokenDown);

			const float DoorWontOpenProgress = ((float)TotalDoorsBrokenDown / (float)SCAchievements::ThisDoorWontOpenCount) * 100.0f;
			UpdateAchievement(ACH_DOOR_WONT_OPEN, DoorWontOpenProgress);
		}
	}
}

void ASCPlayerState::ThrowableHitCounselor(TSubclassOf<ASCThrowable> ThrowableClass)
{
	if (ThrowableClass->IsChildOf(ASCThrowingKnifeProjectile::StaticClass()))
	{
		MatchStats.ThrowingKnifeHits++;

		if (Role == ROLE_Authority)
		{
			const int32 TotalCounselorThrowingKnifeHits = MatchStats.ThrowingKnifeHits + PersistentStats.ThrowingKnifeHits;
			UpdateLeaderboardStats(LEADERBOARD_STAT_THROWING_KNIFE_HIT, TotalCounselorThrowingKnifeHits);

			const float VoodooDollProgress = ((float)TotalCounselorThrowingKnifeHits / (float)SCAchievements::ThrowingKnifeHitCount) * 100.0f;
			UpdateAchievement(ACH_VOODOO_DOLL, VoodooDollProgress);
		}
	}

	if (Role == ROLE_Authority)
	{
		// Give this player a badge
		if (const ASCThrowable* const DefaultThrowable = ThrowableClass->GetDefaultObject<ASCThrowable>())
			EarnedBadge(DefaultThrowable->HitCounselorBadge);
	}
}

void ASCPlayerState::JumpedThroughClosedWindow()
{
	MatchStats.ClosedWindowsJumpedThrough++;

	if (Role == ROLE_Authority)
	{
		const int32 TotalClosedWindowsJumpedThrough = MatchStats.ClosedWindowsJumpedThrough + PersistentStats.ClosedWindowsJumpedThrough;
		UpdateLeaderboardStats(LEADERBOARD_STAT_CLOSED_WINDOWS, TotalClosedWindowsJumpedThrough);

		const float WindowsProgress = ((float)TotalClosedWindowsJumpedThrough / (float)SCAchievements::ClosedWindowJumpCount) * 100.0f;
		UpdateAchievement(ACH_WINDOWS_99, WindowsProgress);
	}
}

void ASCPlayerState::KnockedJasonsMaskOff()
{
	MatchStats.KnockedJasonMaskOffCount++;

	if (Role == ROLE_Authority)
	{
		const int32 TotalMaskKnockOffs = MatchStats.KnockedJasonMaskOffCount + PersistentStats.KnockedJasonMaskOffCount;
		UpdateLeaderboardStats(LEADERBOARD_STAT_KNOCKED_MASK_OFF, TotalMaskKnockOffs);

		const float FaceOffProgress = ((float)TotalMaskKnockOffs / (float)SCAchievements::FaceOffCount) * 100.0f;
		UpdateAchievement(ACH_FACE_OFF, FaceOffProgress);
	}
}

void ASCPlayerState::SweaterStunnedJason()
{
	MatchStats.SweaterStunCount++;

	if (Role == ROLE_Authority)
	{
		const int32 TotalSweaterStuns = MatchStats.SweaterStunCount + PersistentStats.SweaterStunCount;
		UpdateLeaderboardStats(LEADERBOARD_STAT_SWEATER_STUN, TotalSweaterStuns);

		const float GoodBoyProgress = ((float)TotalSweaterStuns / (float)SCAchievements::GoodBoyStunCount) * 100.0f;
		UpdateAchievement(ACH_GOOD_BOY, GoodBoyProgress);
	}
}

void ASCPlayerState::PocketKnifeEscape(TSubclassOf<ASCPocketKnife> PocketKnife)
{
	TrackItemUse(PocketKnife, (uint8)ESCItemStatFlags::Used);

	if (Role == ROLE_Authority)
	{
		const int32 TotalPocketKnifeEscapes = MatchStats.GetItemStats(PocketKnife).TimesUsed + PersistentStats.GetItemStats(PocketKnife).TimesUsed;
		UpdateLeaderboardStats(LEADERBOARD_STAT_POCKET_KNIFE_ESCAPE, TotalPocketKnifeEscapes);

		const float GetOutOfJailFreeProgress = ((float)TotalPocketKnifeEscapes / (float)SCAchievements::GetOutOfJailFreeCount) * 100.0f;
		UpdateAchievement(ACH_GET_OUT_OF_JAIL_FREE, GetOutOfJailFreeProgress);
	}
}

void ASCPlayerState::UsedFirstAidSpray(TSubclassOf<ASCFirstAid> FirstAid)
{
	TrackItemUse(FirstAid, (uint8)ESCItemStatFlags::Used);

	if (Role == ROLE_Authority)
	{
		const int32 TotalFirstAidsUsed = MatchStats.GetItemStats(FirstAid).TimesUsed + PersistentStats.GetItemStats(FirstAid).TimesUsed;
		UpdateLeaderboardStats(LEADERBOARD_STAT_FIRST_AID_USES, TotalFirstAidsUsed);

		const float NeedAMedicProgress = ((float)TotalFirstAidsUsed / (float)SCAchievements::INeedAMedicCount) * 100.0f;
		UpdateAchievement(ACH_I_NEED_A_MEDIC, NeedAMedicProgress);
	}
}

void ASCPlayerState::TrappedPlayer(TSubclassOf<ASCTrap> Trap)
{
	TrackItemUse(Trap, IsActiveCharacterKiller() ? (uint8)ESCItemStatFlags::HitCounselor : (uint8)ESCItemStatFlags::HitJason);

	if (Role == ROLE_Authority)
	{
		if (IsActiveCharacterKiller())
		{
			const int32 TotalCounselorsTrapped = MatchStats.GetItemStats(Trap).CounselorsHit + PersistentStats.GetItemStats(Trap).CounselorsHit;
			UpdateLeaderboardStats(LEADERBOARD_STAT_TRAP_COUNSELOR, TotalCounselorsTrapped);

			const float EenieMeenieProgress = ((float)TotalCounselorsTrapped / (float)SCAchievements::EenieMeenieTrapCount) * 100.0f;
			UpdateAchievement(ACH_EENIE_MEENIE, EenieMeenieProgress);
		}
		else
		{
			const int32 TotalJasonsTrapped = MatchStats.GetItemStats(Trap).JasonHits + PersistentStats.GetItemStats(Trap).JasonHits;
			UpdateLeaderboardStats(LEADERBOARD_STAT_TRAP_JASON, TotalJasonsTrapped);

			const float ItsATrapProgress = ((float)TotalJasonsTrapped / (float)SCAchievements::ItsATrapCount) * 100.0f;
			UpdateAchievement(ACH_ITS_A_TRAP, ItsATrapProgress);
		}
	}
}

void ASCPlayerState::ShotJasonWithFlare(TSubclassOf<ASCWeapon> FlareGun)
{
	TrackItemUse(FlareGun, (uint8)ESCItemStatFlags::HitJason | (uint8)ESCItemStatFlags::Stun);

	if (Role == ROLE_Authority)
	{
		const int32 TotalJasonFlareHits = MatchStats.GetItemStats(FlareGun).JasonHits + PersistentStats.GetItemStats(FlareGun).JasonHits;
		UpdateLeaderboardStats(LEADERBOARD_STAT_FLARE_HITS, TotalJasonFlareHits);

		const float FlaringUpProgress = ((float)TotalJasonFlareHits / (float)SCAchievements::FlaringUpCount) * 100.0f;
		UpdateAchievement(ACH_FLARING_UP, FlaringUpProgress);
	}
}

void ASCPlayerState::HitJasonWithFirecracker(TSubclassOf<ASCNoiseMaker> FireCracker)
{
	TrackItemUse(FireCracker, (uint8)ESCItemStatFlags::HitJason | (uint8)ESCItemStatFlags::Stun);

	if (Role == ROLE_Authority)
	{
		const int32 TotalJasonFirecrackerHits = MatchStats.GetItemStats(FireCracker).JasonHits + PersistentStats.GetItemStats(FireCracker).JasonHits;
		UpdateLeaderboardStats(LEADERBOARD_STAT_FIRECRACKER_STUNS, TotalJasonFirecrackerHits);

		const float SnapCrackleBoomProgress = ((float)TotalJasonFirecrackerHits / (float)SCAchievements::SnapCrackleBoomCount) * 100.0f;
		UpdateAchievement(ACH_SNAP_CRACK_BOOM, SnapCrackleBoomProgress);
	}
}

void ASCPlayerState::FoundTeddyBear()
{
	if (Role == ROLE_Authority)
	{
		UpdateLeaderboardStats(LEADERBOARD_STAT_TEDDY_BEARS_SEEN);
		UpdateAchievement(ACH_H20_DELIRIOUS);
	}
}

void ASCPlayerState::PerformedContextKill(TSubclassOf<USCContextKillDamageType> KillData)
{
	if (const USCContextKillDamageType* const KillInfo = KillData->GetDefaultObject<USCContextKillDamageType>())
	{
		MatchStats.PerformedKill(KillInfo->KillName);

		if (Role == ROLE_Authority)
		{
			// Kill achievements
			if (KillInfo->LinkedAchievements.Contains(ACH_DOOR_WONT_CLOSE))
			{
				UpdateLeaderboardStats(LEADERBOARD_STAT_THIS_DOOR_WONT_CLOSE);
				UpdateAchievement(ACH_DOOR_WONT_CLOSE);
			}
			else if (KillInfo->LinkedAchievements.Contains(ACH_A_CLASSIC))
			{
				UpdateLeaderboardStats(LEADERBOARD_STAT_A_CLASSIC);
				UpdateAchievement(ACH_A_CLASSIC);
			}
			else if (KillInfo->LinkedAchievements.Contains(ACH_COOKING_WITH_JASON))
			{
				UpdateLeaderboardStats(LEADERBOARD_STAT_COOKING_WITH_JASON);
				UpdateAchievement(ACH_COOKING_WITH_JASON);
			}
			else if (KillInfo->LinkedAchievements.Contains(ACH_LETS_SPLIT_UP))
			{
				UpdateLeaderboardStats(LEADERBOARD_STAT_LETS_SPLIT_UP);
				UpdateAchievement(ACH_LETS_SPLIT_UP);
			}

			// Check to see if we've performed every kill
			if (ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>())
			{
				const TArray<TSubclassOf<USCContextKillDamageType>> MasterKillList = GM->GetMasterKillList();
				if (MasterKillList.Num() > 0)
				{
					int32 TotalPhDKillsPerformed = 0;
					for (TSubclassOf<USCContextKillDamageType> KillClass : MasterKillList)
					{
						const USCContextKillDamageType* const KillCDO = KillClass->GetDefaultObject<USCContextKillDamageType>();
						if (!KillCDO->LinkedAchievements.Contains(ACH_PHD_IN_MURDER))
							continue;

						if (!MatchStats.HasPerformedKill(KillCDO->KillName) && !PersistentStats.HasPerformedKill(KillCDO->KillName))
							continue;

						TotalPhDKillsPerformed++;
					}

					UpdateLeaderboardStats(LEADERBOARD_STAT_PHD_IN_MURDER, TotalPhDKillsPerformed);

					const float PhDProgress = ((float)TotalPhDKillsPerformed / (float)SCAchievements::PhDInMurderCount) * 100.0f;
					UpdateAchievement(ACH_PHD_IN_MURDER, PhDProgress);
				}
			}

			// Award any badges linked to this kill
			EarnedBadge(KillInfo->KillBadge);

			// Check for any score events linked to this kill
			HandleScoreEvent(KillInfo->ScoreEvent);
		}
	}
}

bool ASCPlayerState::DidKillAllCounselors(const int32 TotalCounselors, const bool bKilledHunter)
{
	if (IsActiveCharacterKiller() && Role == ROLE_Authority)
	{
		if (MatchStats.CounselorsKilled >= TotalCounselors)
		{
			if (MatchStats.CounselorsKilled >= SCAchievements::NoHappyEndingKillCount)
			{
				UpdateLeaderboardStats(LEADERBOARD_STAT_NO_HAPPY_ENDINGS);
				UpdateAchievement(ACH_NO_HAPPY_ENDINGS);

				if (bKilledHunter)
				{
					UpdateLeaderboardStats(LEADERBOARD_STAT_ONE_FOR_GOOD_MEASURE);
					UpdateAchievement(ACH_ONE_FOR_GOOD_MEASURE);
				}
			}

			return true;
		}
	}

	return false;
}

void ASCPlayerState::TrackItemUse(TSubclassOf<ASCItem> ItemClass, const uint8 ItemStatFlags)
{
	if (bIsABot)
		return;

	const int32 ItemStatsIndex = MatchStats.GetItemStatsIndex(ItemClass);
	if (ItemStatsIndex != -1)
		MatchStats.ItemStats[ItemStatsIndex] += ItemStatFlags;
	else
		MatchStats.ItemStats.Add(FSCItemStats(ItemClass, ItemStatFlags));

	if (Role == ROLE_Authority)
	{
		const ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>();
		const TSoftClassPtr<ASCWeapon> BaseballBatClass = GM ? GM->GetBaseballBatClass() : nullptr;
		const TSoftClassPtr<ASCWeapon> ShotgunClass = GM ? GM->GetShotgunClass() : nullptr;

		// Track baseball bat hits on Jason
		if (BaseballBatClass.Get() && ItemClass == BaseballBatClass.Get() && (ItemStatFlags & (uint8)ESCItemStatFlags::HitJason))
		{
			const int32 TotalJasonBatHits = MatchStats.GetItemStats(BaseballBatClass.Get()).JasonHits + PersistentStats.GetItemStats(BaseballBatClass.Get()).JasonHits;
			UpdateLeaderboardStats(LEADERBOARD_STAT_BASEBALL_BAT_HITS, TotalJasonBatHits);

			const float SmashBrosProgress = ((float)TotalJasonBatHits / (float)SCAchievements::SmashBrosCount) * 100.0f;
			UpdateAchievement(ACH_SMASH_BROS, SmashBrosProgress);
		}
		// Track shotgun hits on Jason
		else if (ShotgunClass.Get() && ItemClass == ShotgunClass.Get() && (ItemStatFlags & (uint8)ESCItemStatFlags::HitJason))
		{
			const int32 TotalJasonShotgunHits = MatchStats.GetItemStats(ShotgunClass.Get()).JasonHits + PersistentStats.GetItemStats(ShotgunClass.Get()).JasonHits;
			UpdateLeaderboardStats(LEADERBOARD_STAT_SHOTGUN_JASON, TotalJasonShotgunHits);

			const float BoomStickProgress = ((float)TotalJasonShotgunHits / (float)SCAchievements::BoomStickCount) * 100.0f;
			UpdateAchievement(ACH_THIS_IS_MY_BOOMSTICK, BoomStickProgress);

			if (const ASCGun* const Shotgun = ShotgunClass->GetDefaultObject<ASCGun>())
			{
				HandleScoreEvent(Shotgun->StunScoreEventClass);
			}
		}
		// Check for Pamela's sweater pick up
		else if (ItemClass->IsChildOf(ASCPamelaSweater::StaticClass()) && (ItemStatFlags & (uint8)ESCItemStatFlags::PickedUp))
		{
			UpdateLeaderboardStats(LEADERBOARD_STAT_NEW_THREADS);
			UpdateAchievement(ACH_NEW_THREADS);
		}
		// Check for Jason's mask pick up
		else if (ItemClass->IsChildOf(ASCKillerMask::StaticClass()) && (ItemStatFlags & (uint8)ESCItemStatFlags::PickedUp))
		{
			UpdateLeaderboardStats(LEADERBOARD_STAT_GOALIE);
			UpdateAchievement(ACH_GOALIE);
		}
	}
}

void ASCPlayerState::HandleScoreEvent(TSubclassOf<USCScoringEvent> ScoreEvent, const float ScoreModifier/* = 1.f*/)
{
	if (bIsABot)
		return;

	check(ScoreEvent);
	const USCScoringEvent* const ScoreDefault = ScoreEvent->GetDefaultObject<USCScoringEvent>();

	if (ScoreDefault->bOnlyScoreOnce && PlayerScoringEvents.Contains(ScoreEvent))
		return;

	// Score points
	const int32 AddedXP = ScoreDefault->GetModifiedExperience(ScoreModifier, IsActiveCharacterKiller() ? TArray<USCPerkData*>() : ActivePerks);
	if (ASCPlayerController* OwnerController = Cast<ASCPlayerController>(GetOwner()))
	{
		FSCCategorizedScoreEvent* ExistingEvent = EventCategoryScores.FindByPredicate(
			[&](FSCCategorizedScoreEvent inEvent)
		{
			return inEvent.Category == ScoreDefault->EventCategory && inEvent.bAsHunter == GetIsHunter();
		}
		);
		if (ExistingEvent)
		{
			ExistingEvent->TotalScore += AddedXP;
		}
		else
		{
			EventCategoryScores.Add(FSCCategorizedScoreEvent(ScoreDefault->EventCategory, AddedXP, bIsHunter));
		}

		OwnerController->ClientDisplayScoreEvent(ScoreDefault->GetClass(), (uint8)ScoreDefault->EventCategory, ScoreModifier);
	}
	ScorePoints(AddedXP);
	PlayerScoringEvents.Add(ScoreEvent);

	// Award badges linked to this score event
	EarnedBadge(IsActiveCharacterKiller() ? ScoreDefault->JasonBadge : ScoreDefault->CounselorBadge);

	// Report any infractions
	if (Role == ROLE_Authority && ScoreDefault->EventCategory == EScoreEventCategory::Betrayal)
	{
		if (AGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr)
		{
			if (ASCGameSession* GameSession = Cast<ASCGameSession>(GameMode->GameSession))
			{
				if (GameSession->IsQuickPlayMatch())
				{
					USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
					USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
					RequestManager->ReportInfraction(AccountId, InfractionTypes::CounselorBetrayedCounselor);
				}
			}
		}
	}
}

void ASCPlayerState::EarnedBadge(TSubclassOf<USCStatBadge> BadgeInfo)
{
	if (BadgeInfo)
	{
		BadgesEarned.AddUnique(BadgeInfo);
	}
}

#if !UE_BUILD_SHIPPING
void ASCPlayerState::PrintPlayerStats() const
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Boat Parts Repaired: %i"), MatchStats.BoatPartsRepaired + PersistentStats.BoatPartsRepaired));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Car Parts Repaired: %i"), MatchStats.CarPartsRepaired + PersistentStats.CarPartsRepaired));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Closed Window Jumps: %i"), MatchStats.ClosedWindowsJumpedThrough + PersistentStats.ClosedWindowsJumpedThrough));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Matches Played: %i"), MatchStats.CounselorMatchesPlayed + PersistentStats.CounselorMatchesPlayed));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Deaths By Jason: %i"), MatchStats.DeathsByJason + PersistentStats.DeathsByJason));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Deaths By Counselors: %i"), MatchStats.DeathsByOtherCounselors + PersistentStats.DeathsByOtherCounselors));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Boat Driver Escapes: %i"), MatchStats.EscapedInBoatAsDriverCount + PersistentStats.EscapedInBoatAsDriverCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Boat Passenger Escapes: %i"), MatchStats.EscapedInBoatAsPassengerCount + PersistentStats.EscapedInBoatAsPassengerCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Car Driver Escapes: %i"), MatchStats.EscapedInCarAsDriverCount + PersistentStats.EscapedInCarAsDriverCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Car Passenger Escapes: %i"), MatchStats.EscapedInCarAsPassengerCount + PersistentStats.EscapedInCarAsPassengerCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Police Escapes: %i"), MatchStats.EscapedWithPoliceCount + PersistentStats.EscapedWithPoliceCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Grid Parts Repaired: %i"), MatchStats.GridPartsRepaired + PersistentStats.GridPartsRepaired));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Killed Jason: %i"), MatchStats.KilledJasonCount + PersistentStats.KilledJasonCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Counselors Killed: %i"), MatchStats.KilledAnotherCounselorCount + PersistentStats.KilledAnotherCounselorCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Knocked Jason's Mask Off: %i"), MatchStats.KnockedJasonMaskOffCount + PersistentStats.KnockedJasonMaskOffCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Police Called: %i"), MatchStats.PoliceCallCount + PersistentStats.PoliceCallCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Sweater Stuns: %i"), MatchStats.SweaterStunCount + PersistentStats.SweaterStunCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Tommy Jarvis Called: %i"), MatchStats.TommyJarvisCallCount + PersistentStats.TommyJarvisCallCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Tommy Jarvis Matches Played: %i"), MatchStats.TommyJarvisMatchesPlayed + PersistentStats.TommyJarvisMatchesPlayed));

	TArray<TSoftClassPtr<ASCCounselorCharacter>> CounselorsPlayed = MatchStats.CounselorsPlayed;
	for (const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass : PersistentStats.CounselorsPlayed)
		CounselorsPlayed.AddUnique(CounselorClass);
	FString Counselors;
	for (int i = 0; i < CounselorsPlayed.Num(); ++i)
	{
		Counselors += CounselorsPlayed[i]->GetName();
		if (i < (CounselorsPlayed.Num() - 1))
			Counselors += TEXT(", ");
		if ((i > 0) && ((i % 5) == 0))
			Counselors += TEXT("\n");
	}
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Counselors Played: %s"), *Counselors));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, TEXT("============ Counselor Stats ============"));

	
	TArray<TSoftClassPtr<ASCKillerCharacter>> JasonsPlayed = MatchStats.JasonsPlayed;
	for (const TSoftClassPtr<ASCKillerCharacter>& JasonClass : PersistentStats.JasonsPlayed)
		JasonsPlayed.AddUnique(JasonClass);
	FString Jasons;
	for (int i = 0; i < JasonsPlayed.Num(); ++i)
	{
		Jasons += JasonsPlayed[i]->GetName();
		if (i < (JasonsPlayed.Num() - 1))
			Jasons += TEXT(", ");
	}

	TArray<FSCKillStats> KillsPerformed = MatchStats.ContextKillsPerformed;
	for (auto KillInfo : PersistentStats.ContextKillsPerformed)
	{
		if (FSCKillStats* Kill = KillsPerformed.FindByKey(KillInfo))
		{
			Kill->KillCount += KillInfo.KillCount;
		}
		else
		{
			KillsPerformed.Add(KillInfo);
		}
	}

	FString Kills;
	for (int i = 0; i < KillsPerformed.Num(); ++i)
	{
		Kills += FString::Printf(TEXT("%s: %i"), *KillsPerformed[i].KillName.ToString(), KillsPerformed[i].KillCount);
		if (i < (KillsPerformed.Num() - 1))
			Kills += TEXT(", ");

		if ((i > 0) && ((i % 8) == 0))
			Kills += TEXT("\n");
	}
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Killed Performed: %s"), *Kills));

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Car Slams: %i"), MatchStats.CarSlamCount + PersistentStats.CarSlamCount));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Counselors Killed: %i"), MatchStats.CounselorsKilled + PersistentStats.CounselorsKilled));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Doors Broken Down: %i"), MatchStats.DoorsBrokenDown + PersistentStats.DoorsBrokenDown));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Deaths By Counselors: %i"), MatchStats.JasonDeathsByCounselors + PersistentStats.JasonDeathsByCounselors));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Matches Played: %i"), MatchStats.JasonMatchesPlayed + PersistentStats.JasonMatchesPlayed));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Throwing Knife Hits: %i"), MatchStats.ThrowingKnifeHits + PersistentStats.ThrowingKnifeHits));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Walls Broken Down: %i"), MatchStats.WallsBrokenDown + PersistentStats.WallsBrokenDown));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Jasons Played: %s"), *Jasons));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, TEXT("============ Jason Stats ============"));
}
#endif

void ASCPlayerState::UnlockAllAchievements()
{
#if !UE_BUILD_SHIPPING
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineAchievementsPtr Achievements = OnlineSub ? OnlineSub->GetAchievementsInterface() : nullptr;
	IOnlineLeaderboardsPtr Leaderboards = OnlineSub ? OnlineSub->GetLeaderboardsInterface() : nullptr;

	if (!Achievements.IsValid() || !Leaderboards.IsValid() || !UniqueId.IsValid())
		return;

	TArray<FOnlineAchievement> GameAchievements;
	Achievements->GetCachedAchievements(*UniqueId, GameAchievements);

	// Set all achievements to 100%
	for (FOnlineAchievement Achievement : GameAchievements)
	{
		UpdateAchievement(*Achievement.Id, 100.f);
	}

	// Set stats
	UpdateLeaderboardStats(LEADERBOARD_STAT_COUNSELOR_MATCHES, SCAchievements::HeadCounselorMatchCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_JASON_MATCHES, SCAchievements::FinalChapterMatchCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_COUNSELORS_KILLED, SCAchievements::KillEmAllKillCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_CAR_REPAIRS, SCAchievements::GreaseMonkeyRepairCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_BOAT_REPAIRS, SCAchievements::ShipwrightRepairCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_GRID_REPAIRS, SCAchievements::ShockJockeyRepairCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_POLICE_CALLED, SCAchievements::OperatorCallCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_TJ_CALLED, SCAchievements::DiscJockeyCallCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_CAR_PASSENGER_ESCAPE, SCAchievements::AlongForTheRideEscapeCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_BOAT_PASSENGER_ESCAPE, SCAchievements::ImOnABoatEscapeCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_TRAP_JASON, SCAchievements::ItsATrapCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_PLAY_AS_TJ, SCAchievements::TommyPlayCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_SWEATER_STUN, SCAchievements::GoodBoyStunCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_THROWING_KNIFE_HIT, SCAchievements::ThrowingKnifeHitCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_TRAP_COUNSELOR, SCAchievements::EenieMeenieTrapCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_DEATH_BY_JASON, SCAchievements::YouDiedALotCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_SLAM_JAM, SCAchievements::SlamJamCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_POLICE_ESCAPES, SCAchievements::CrystalLakeFiveOCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_FLARE_HITS, SCAchievements::FlaringUpCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_FIRECRACKER_STUNS, SCAchievements::SnapCrackleBoomCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_POCKET_KNIFE_ESCAPE, SCAchievements::GetOutOfJailFreeCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_KNOCKED_MASK_OFF, SCAchievements::FaceOffCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_DOOR_BREAKS, SCAchievements::ThisDoorWontOpenCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_CLOSED_WINDOWS, SCAchievements::ClosedWindowJumpCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_BASEBALL_BAT_HITS, SCAchievements::SmashBrosCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_FIRST_AID_USES, SCAchievements::INeedAMedicCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_SHOTGUN_JASON, SCAchievements::BoomStickCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_ROLL_CREDITS, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_NO_HAPPY_ENDINGS, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_ONE_FOR_GOOD_MEASURE, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_SEEN_EVERY_MOVIE, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_SUPER_FAN, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_FRIDAY_DRIVER, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_AYE_AYE_CAPTAIN, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_NEW_THREADS, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_GOALIE, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_THIS_DOOR_WONT_CLOSE, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_A_CLASSIC, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_COOKING_WITH_JASON, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_LETS_SPLIT_UP, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_CHAD_IS_A_DICK, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_PHD_IN_MURDER, SCAchievements::PhDInMurderCount);
	UpdateLeaderboardStats(LEADERBOARD_STAT_PLAYER_LEVEL, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_SOLO_FLIRT_SURVIVES, 13);
	UpdateLeaderboardStats(LEADERBOARD_STAT_TEDDY_BEARS_SEEN, 13);
#endif
}

void ASCPlayerState::ClearAchievements()
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineAchievementsPtr Achievements = OnlineSub ? OnlineSub->GetAchievementsInterface() : nullptr;
	IOnlineLeaderboardsPtr Leaderboards = OnlineSub ? OnlineSub->GetLeaderboardsInterface() : nullptr;

	if (!Achievements.IsValid() || !Leaderboards.IsValid() || !UniqueId.IsValid())
		return;

#if !UE_BUILD_SHIPPING
	// Clear achievements
	Achievements->ResetAchievements(*UniqueId);
	Achievements->QueryAchievements(*UniqueId);

	// Reset stats
	FOnlineLeaderboardWrite ResultsWriteObject;
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_COUNSELOR_MATCHES, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_JASON_MATCHES, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_COUNSELORS_KILLED, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_CAR_REPAIRS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_BOAT_REPAIRS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_GRID_REPAIRS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_POLICE_CALLED, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_TJ_CALLED, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_CAR_PASSENGER_ESCAPE, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_BOAT_PASSENGER_ESCAPE, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_TRAP_JASON, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_PLAY_AS_TJ, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_SWEATER_STUN, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_THROWING_KNIFE_HIT, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_TRAP_COUNSELOR, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_DEATH_BY_JASON, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_SLAM_JAM, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_POLICE_ESCAPES, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_FLARE_HITS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_FIRECRACKER_STUNS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_POCKET_KNIFE_ESCAPE, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_KNOCKED_MASK_OFF, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_DOOR_BREAKS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_CLOSED_WINDOWS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_BASEBALL_BAT_HITS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_FIRST_AID_USES, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_SHOTGUN_JASON, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_ROLL_CREDITS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_NO_HAPPY_ENDINGS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_ONE_FOR_GOOD_MEASURE, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_SEEN_EVERY_MOVIE, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_SUPER_FAN, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_FRIDAY_DRIVER, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_AYE_AYE_CAPTAIN, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_NEW_THREADS, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_GOALIE, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_THIS_DOOR_WONT_CLOSE, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_A_CLASSIC, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_COOKING_WITH_JASON, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_LETS_SPLIT_UP, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_CHAD_IS_A_DICK, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_PHD_IN_MURDER, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_PLAYER_LEVEL, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_SOLO_FLIRT_SURVIVES, 0);
	ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_TEDDY_BEARS_SEEN, 0);

	// Write the leaderboard stat
#if !PLATFORM_PS4 && !PLATFORM_XBOXONE
	Leaderboards->WritePlayerStats(*UniqueId, ResultsWriteObject, true);
#else
	Leaderboards->WriteLeaderboards(SessionName, *UniqueId, ResultsWriteObject);
	Leaderboards->FlushLeaderboards(SessionName);
#endif
#endif
}

void ASCPlayerState::QueryAchievements()
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineAchievementsPtr Achievements = OnlineSub ? OnlineSub->GetAchievementsInterface() : nullptr;

	if (!Achievements.IsValid() || !UniqueId.IsValid())
		return;

	Achievements->QueryAchievements(*UniqueId);
}

void ASCPlayerState::UpdateAchievement(const FName AchievementName, const float AchievementProgress /*= 100.0f*/)
{
	if (bIsABot)
		return;

	UE_LOG(LogOnline, Verbose, TEXT("Server: [%s]: Updating achievement %s with progress=%f"), *PlayerName, *AchievementName.ToString(), AchievementProgress);

	if (AchievementProgress < 100.0f)
		return;

	ClientUpdateAchievement(AchievementName, AchievementProgress);
}

void ASCPlayerState::ClientUpdateAchievement_Implementation(const FName AchievementName, const float AchievementProgress)
{
	UE_LOG(LogOnline, Verbose, TEXT("Client: Updating achievement %s with progress=%f"), *AchievementName.ToString(), AchievementProgress);

	if (AchievementProgress < 100.0f)
		return;

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineAchievementsPtr Achievements = OnlineSub ? OnlineSub->GetAchievementsInterface() : nullptr;

	if (!Achievements.IsValid() || !UniqueId.IsValid())
	{
		UE_LOG(LogOnline, Verbose, TEXT("Client: Failed to update achievement %s, AchiementPtr or UniqueId is invalid"), *AchievementName.ToString());
		return;
	}

	// Write achievement progress
	FOnlineAchievementsWritePtr WriteObject = MakeShareable(new FOnlineAchievementsWrite);
	WriteObject->SetFloatStat(AchievementName, AchievementProgress);

	FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();
	Achievements->WriteAchievements(*UniqueId, WriteObjectRef);

	// Check to see if we have unlocked all achievements
	if (AchievementName	!= ACH_KILLER_FRANCHISE)
		CheckAchievementCompletion();
}

void ASCPlayerState::UpdateLeaderboardStats_Implementation(const FName LeaderboardStat, const int StatValue /*=1*/)
{
	if (bIsABot)
		return;

	UE_LOG(LogOnline, Verbose, TEXT("StatUpdate: [%s]: Updating Stat %s with Value %i"), *PlayerName, *LeaderboardStat.ToString(), StatValue);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineLeaderboardsPtr Leaderboards = OnlineSub ? OnlineSub->GetLeaderboardsInterface() : nullptr;
	IOnlineEventsPtr Events = OnlineSub ? OnlineSub->GetEventsInterface() : nullptr;

	// Leaderboards
	if (Leaderboards.IsValid() && UniqueId.IsValid())
	{
		FOnlineLeaderboardWrite ResultsWriteObject;
		ResultsWriteObject.SetIntStat(LeaderboardStat, StatValue);

		// Write the leaderboard stat
#if !PLATFORM_PS4 && !PLATFORM_XBOXONE
		Leaderboards->WritePlayerStats(*UniqueId, ResultsWriteObject);
#else
		Leaderboards->WriteLeaderboards(SessionName, *UniqueId, ResultsWriteObject);
		Leaderboards->FlushLeaderboards(SessionName);
#endif
	}

	// Events (Xbox One)
	if (Events.IsValid() && UniqueId.IsValid())
	{
		FOnlineEventParms EventParam;
		EventParam.Add(TEXT("StatName"), FVariantData(LeaderboardStat.ToString()));
		EventParam.Add(TEXT("StatValue"), FVariantData(StatValue));
		Events->TriggerEvent(*UniqueId, TEXT("StatUpdate"), EventParam);
	}
}

void ASCPlayerState::CheckAchievementCompletion()
{
	if (bIsABot)
		return;

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineAchievementsPtr Achievements = OnlineSub ? OnlineSub->GetAchievementsInterface() : nullptr;

	if (!Achievements.IsValid() || !UniqueId.IsValid())
		return;

	FOnlineAchievement FinalAch;
	if (Achievements->GetCachedAchievement(*UniqueId, ACH_KILLER_FRANCHISE, FinalAch) == EOnlineCachedResult::Success)
	{
		if (FinalAch.Progress < 100.0)
		{
			TArray<FOnlineAchievement> PlayerAchievements;
			if (Achievements->GetCachedAchievements(*UniqueId, PlayerAchievements) == EOnlineCachedResult::Success)
			{
				if (PlayerAchievements.Num() > 0)
				{
					// Check to see if we have unlocked all achievements
					for (const FOnlineAchievement& Achievement : PlayerAchievements)
					{
						if (Achievement.Id == ACH_KILLER_FRANCHISE)
							continue;
						else if (Achievement.Progress < 100.0)
							return; // Don't have all achievements unlocked
					}

					// We went through the loop, so we have all other achievements unlocked
					UpdateAchievement(ACH_KILLER_FRANCHISE);
				}
			}
		}
	}
}

void ASCPlayerState::SetRichPresence(const FString& MapName, bool Counselor)
{
	FString FullPresenceString;

#if PLATFORM_XBOXONE
	if (MapName == TEXT("HigginsHaven"))
		FullPresenceString = Counselor ? TEXT("HigginsCounselor") : TEXT("HigginsJason");
	else if (MapName == TEXT("Packanack"))
		FullPresenceString = Counselor ? TEXT("PackanackCounselor") : TEXT("PackanackJason");
	else if (MapName == TEXT("CrystalLake"))
		FullPresenceString = Counselor ? TEXT("CrystalLakeCounselor") : TEXT("CrystalLakeJason");
	else if (MapName == TEXT("HigginsHaven_small"))
		FullPresenceString = Counselor ? TEXT("HigginsSmallCounselor") : TEXT("HigginsSmallJason");
	else if (MapName == TEXT("Packanack_sm"))
		FullPresenceString = Counselor ? TEXT("PackanackSmallCounselor") : TEXT("PackanackSmallJason");
	else if (MapName == TEXT("CrystalLake_Small"))
		FullPresenceString = Counselor ? TEXT("CrystalLakeSmallCounselor") : TEXT("CrystalLakeSmallJason");
	else if (MapName == TEXT("JarvisHouse_Small"))
		FullPresenceString = Counselor ? TEXT("JarvisCounselor") : TEXT("JarvisJason");
	else if (MapName == TEXT("F13_Pinehurst_Level"))
		FullPresenceString = Counselor ? TEXT("PinehurstCounselor") : TEXT("PinehurstJason");
	// single player challenge maps
	else if (MapName == TEXT("SP_Broken_Down") || MapName == TEXT("SP_Packanack_sm") || MapName == TEXT("SP_HigginsHaven_Small_C3") ||
			 MapName == TEXT("SP_HigginsHaven_Small_C4") || MapName == TEXT("SP_CrystalLake_Small_C5") || MapName == TEXT("SP_CrystalLake_Small_C6") ||
			 MapName == TEXT("SP_JarvisHouse_Small_C7") || MapName == TEXT("SP_JarvisHouse_Small_C8") || MapName == TEXT("SP_Pinehurst_Level") ||
			 MapName == TEXT("SP_Snuggle"))
	{
		FullPresenceString = TEXT("SP_Challenges");
	}
	else
	{
		FullPresenceString = Counselor ? TEXT("InGameCounselor") : TEXT("InGameJason");
	}

	UpdateRichPresence(FVariantData(FullPresenceString));
	
#else
	FText FinalPresenceText;
	bool bSinglePlayer = false;
	if (MapName == TEXT("HigginsHaven"))
		FullPresenceString = TEXT("Higgins Haven");
	else if (MapName == TEXT("Packanack"))
		FullPresenceString = TEXT("Packanack");
	else if (MapName == TEXT("CrystalLake"))
		FullPresenceString = TEXT("Crystal Lake");
	else if (MapName == TEXT("HigginsHaven_small"))
		FullPresenceString = TEXT("Higgins Haven Small");
	else if (MapName == TEXT("Packanack_sm"))
		FullPresenceString = TEXT("Packanack Small");
	else if (MapName == TEXT("CrystalLake_Small"))
		FullPresenceString = TEXT("Crystal Lake Small");
	else if (MapName == TEXT("JarvisHouse_Small"))
		FullPresenceString = TEXT("Jarvis House");
	else if (MapName == TEXT("F13_Pinehurst_Level"))
		FullPresenceString = TEXT("Pinehurst");
	// single player challenge maps
	else if (MapName == TEXT("SP_Broken_Down") || MapName == TEXT("SP_Packanack_sm") || MapName == TEXT("SP_HigginsHaven_Small_C3") ||
			 MapName == TEXT("SP_HigginsHaven_Small_C4") || MapName == TEXT("SP_CrystalLake_Small_C5") || MapName == TEXT("SP_CrystalLake_Small_C6") ||
			 MapName == TEXT("SP_JarvisHouse_Small_C7") || MapName == TEXT("SP_JarvisHouse_Small_C8") || MapName == TEXT("SP_Pinehurst_Level") ||
			 MapName == TEXT("SP_Snuggle"))
	{
		FinalPresenceText = FText(LOCTEXT("RichPresenceInGameChallenges", "Playing Offline Challenges"));
		bSinglePlayer = true;
	}

	if (!bSinglePlayer)
	{
		if (FullPresenceString.IsEmpty())
		{
			if (Counselor)
				FinalPresenceText = FText(LOCTEXT("RichPresenceInGameGenericCounselor", "Playing a Match as a Counselor"));
			else
				FinalPresenceText = FText(LOCTEXT("RichPresenceInGameGenericKiller", "Playing a match as Jason"));
		}
		else
		{
			if (Counselor)
				FinalPresenceText = FText::Format(LOCTEXT("RichPresenceInGameCounselor", "Playing as a Counselor on {0}"), FText::FromString(FullPresenceString));
			else
				FinalPresenceText = FText::Format(LOCTEXT("RichPresenceInGameKiller", "Playing as Jason on {0}"), FText::FromString(FullPresenceString));
		}
	}

	UpdateRichPresence(FVariantData(FinalPresenceText.ToString()));
#endif
}

// TODO: Move to SCLocalPlayer or something more permanent
void ASCPlayerState::UpdateRichPresence(const FVariantData& PresenceData)
{
	if (bIsABot)
		return;

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlinePresencePtr Presence = OnlineSub ? OnlineSub->GetPresenceInterface() : nullptr;

	if (!Presence.IsValid() || !UniqueId.IsValid())
		return;

	FOnlineUserPresenceStatus Status;
	Status.Properties.Add(DefaultPresenceKey, PresenceData);
	Presence->SetPresence(*UniqueId, Status);
}

void ASCPlayerState::SetSpectating(bool Spectating)
{
	check(Role == ROLE_Authority);

	if (bSpectating != Spectating)
	{
		bSpectating = Spectating;
		OnRep_Spectating();
	}
}

bool ASCPlayerState::CanSpawnActiveCharacter() const
{
	if (!ActiveCharacterClass.IsNull())
	{
		// Paranoia check
		ActiveCharacterClass.LoadSynchronous();

		// Only allow them to spawn if they have not already spawned as ActiveCharacterClass
		if (!SpawnedCharacterClass || ActiveCharacterClass.Get() != SpawnedCharacterClass)
			return true;
	}

	return false;
}

void ASCPlayerState::OnRevivePlayer()
{
	bDead = false;
	bSpectating = false;
}

bool ASCPlayerState::IsActiveCharacterKiller() const
{
	return (ActiveCharacterClass && ActiveCharacterClass->IsChildOf(ASCKillerCharacter::StaticClass())); 
}

void ASCPlayerState::SetActiveCharacterClass(TSoftClassPtr<ASCCharacter> NewActiveCharacterClass)
{
	check(Role == ROLE_Authority);
	if (ActiveCharacterClass != NewActiveCharacterClass)
	{
		ActiveCharacterClass = NewActiveCharacterClass;
		OnRep_ActiveCharacterClass();
	}
}

void ASCPlayerState::AsyncSetActiveCharacterClass(TSoftClassPtr<ASCCharacter> NewSoftActiveCharacterClass)
{
	check(Role == ROLE_Authority);

	if (NewSoftActiveCharacterClass.IsNull())
	{
		return;
	}

	SetActiveCharacterClass(NewSoftActiveCharacterClass);

	/*
	// Class is already loaded, set it
	if (NewSoftActiveCharacterClass.Get())
	{
		SetActiveCharacterClass(NewSoftActiveCharacterClass);
	}
	// Try and load the class
	else if (UWorld* World = GetWorld())
	{
		if (USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance()))
		{
			FStreamableDelegate Delegate;
			Delegate.BindUObject(this, &ThisClass::AsyncSetActiveCharacterClass, NewSoftActiveCharacterClass);
			GameInstance->StreamableManager.RequestAsyncLoad(NewSoftActiveCharacterClass.ToSoftObjectPath(), Delegate);
		}
	}
	*/
}

void ASCPlayerState::OnRep_ActiveCharacterClass()
{
	// Lets make sure the class is loaded.
	ActiveCharacterClass.LoadSynchronous();

	PlayerSettingsUpdated();
}

void ASCPlayerState::ClientRequestPerksAndOutfit_Implementation(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass)
{
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOwner()))
	{
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(PC->GetLocalPlayer()))
		{
			if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				ServerReceivePerksAndOutfit(Settings->GetCounselorPerks(CounselorClass), Settings->GetCounselorOutfit(CounselorClass));
			}
			else
			{
				USCGameInstance* GameInstance = Cast<USCGameInstance>(GetGameInstance());
				USCCounselorWardrobe* CounselorWardrobe = GameInstance ? GameInstance->GetCounselorWardrobe() : nullptr;
				if (CounselorWardrobe)
				{
					TArray<FSCPerkBackendData> NoPerks;
					const FSCCounselorOutfit& RandomOutfit = CounselorWardrobe->GetRandomOutfit(CounselorClass, PC);
					ServerReceivePerksAndOutfit(NoPerks, RandomOutfit);
				}
			}
		}
	}
}

void ASCPlayerState::ClientRequestGrabKills_Implementation(const TSoftClassPtr<ASCKillerCharacter>& KillerClass)
{
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOwner()))
	{
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(PC->GetLocalPlayer()))
		{
			if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				ServerReceiveGrabKills(Settings->GetKillerGrabKills(KillerClass));
			}
		}
	}
}

void ASCPlayerState::ClientRequestSelectedWeapon_Implementation(const TSoftClassPtr<ASCKillerCharacter>& KillerClass)
{
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOwner()))
	{
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(PC->GetLocalPlayer()))
		{
			if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				ServerReceiveSelectedWeapon(Settings->GetKillerSelectedWeapon(KillerClass).ToSoftObjectPath());
			}
		}
	}
}

bool ASCPlayerState::ServerReceivePerksAndOutfit_Validate(const TArray<FSCPerkBackendData>& Perks, const FSCCounselorOutfit& Outfit)
{
	return true;
}

void ASCPlayerState::ServerReceivePerksAndOutfit_Implementation(const TArray<FSCPerkBackendData>& Perks, const FSCCounselorOutfit& Outfit)
{
	// Set Active perks
	SetActivePerks(Perks);

	// Set outfit
	SetCounselorOutfit(Outfit);
}

bool ASCPlayerState::ServerReceiveGrabKills_Validate(const TArray<TSubclassOf<ASCContextKillActor>>& GrabKills)
{
	return true;
}

void ASCPlayerState::ServerReceiveGrabKills_Implementation(const TArray<TSubclassOf<ASCContextKillActor>>& GrabKills)
{
	PickedGrabKills = GrabKills;
}

bool ASCPlayerState::ServerReceiveSelectedWeapon_Validate(FSoftObjectPath WeaponClassPath)
{
	return true;
}

void ASCPlayerState::ServerReceiveSelectedWeapon_Implementation(FSoftObjectPath WeaponClassPath)
{
	SetKillerWeapon(TSoftClassPtr<ASCWeapon>(WeaponClassPath));
}

bool ASCPlayerState::ServerSetKillerSkin_Validate(TSubclassOf<USCJasonSkin> Skin)
{
	return true;
}

void ASCPlayerState::ServerSetKillerSkin_Implementation(TSubclassOf<USCJasonSkin> Skin)
{
	SetKillerSkin(Skin);
}

void ASCPlayerState::OnRep_Spectating()
{
	if (ASCPlayerController* OwnerController = Cast<ASCPlayerController>(GetOwner()))
	{
		// Check for character HUD changes
		if (OwnerController->GetSCHUD())
		{
			OwnerController->GetSCHUD()->UpdateCharacterHUD();
		}

		// This seems goofy... but so is bSpectating
		if (OwnerController->IsLocalController())
		{
			if (bSpectating)
			{
				OwnerController->PushSpectateInputComponent();
			}
			else
			{
				OwnerController->PopSpectateInputComponent();
			}
			OwnerController->SetSpectateBlocking(!bSpectating);
		}
	}
}


bool ASCPlayerState::CanChangePickedCharacters() const
{
	UWorld* World = GetWorld();
	if (ASCGameState* GameState = World->GetGameState<ASCGameState>())
	{
		return !GameState->HasMatchStarted();
	}
	else if (ASCGameState_Lobby* LobbyState = World->GetGameState<ASCGameState_Lobby>())
	{
		// Disallow changing your picked characters when past the point of no return
		return !LobbyState->IsTravelCountdownVeryLow();
	}
	else if (ASCGame_Menu* MenuMode = World->GetAuthGameMode<ASCGame_Menu>())
	{
		// Always allow it on the main menu
		return true;
	}

	return false;
}

void ASCPlayerState::RequestCounselorClass(TSoftClassPtr<ASCCounselorCharacter> NewCounselorClass)
{
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOwner())) // IGF 4 Lobby
	{
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(PC->GetLocalPlayer()))
		{
			if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				NewCounselorClass.LoadSynchronous(); // TODO: Make Async
				Settings->CounselorPick = NewCounselorClass.Get();
				Settings->ApplyPlayerSettings(PC);
			}
		}
	}
}

bool ASCPlayerState::ServerRequestCounselor_Validate(const FSCCounselorCharacterProfile& NewCounselorProfile)
{
	return true;
}

void ASCPlayerState::ServerRequestCounselor_Implementation(const FSCCounselorCharacterProfile& NewCounselorProfile)
{
	if (CanChangePickedCharacters())
	{
		/*if (NewCounselorProfile.CounselorCharacterClass)
		{
			const ASCCounselorCharacter* const DefaultCounselor = NewCounselorProfile.CounselorCharacterClass->GetDefaultObject<ASCCounselorCharacter>();
			if (DefaultCounselor->bIsHunter)
				return;
		}*/ //  02/03/2021 - re-enable Hunter Selection

		SetCounselorClass(NewCounselorProfile.CounselorCharacterClass);
		SetCounselorPerks(NewCounselorProfile.Perks);
		SetCounselorOutfit(NewCounselorProfile.Outfit);
	}
}

bool ASCPlayerState::ServerRequestKiller_Validate(const FSCKillerCharacterProfile& NewKillerProfile)
{
	return true;
}

void ASCPlayerState::ServerRequestKiller_Implementation(const FSCKillerCharacterProfile& NewKillerProfile)
{
	if (CanChangePickedCharacters())
	{
		SetKillerClass(NewKillerProfile.KillerCharacterClass);
		SetKillerGrabKills(NewKillerProfile.GrabKills);
		SetKillerSkin(NewKillerProfile.SelectedSkin);
		//SetKillerAbilityUnlockOrder(NewKillerProfile.AbilityUnlockOrder); This was removed at GUNs request, Uncomment this line if you need ability switching again
		SetKillerWeapon(NewKillerProfile.SelectedWeapon);
	}
}

void ASCPlayerState::RequestKillerClass(TSoftClassPtr<ASCKillerCharacter> NewKillerClass)
{
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOwner())) // IGF 4 Lobby
	{
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(PC->GetLocalPlayer()))
		{
			if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				NewKillerClass.LoadSynchronous(); // TODO: Make Async
				// Save locally
				Settings->KillerPick = NewKillerClass.Get();
				Settings->ApplyPlayerSettings(PC);

				// Update the server immediately
				ServerRequestKiller(Settings->GetPickedKillerProfile());
			}
		}
	}
}

bool ASCPlayerState::ServerSetPickedKillerPlayer_Validate(ASCPlayerState* PlayerState)
{
	return true;
}

void ASCPlayerState::ServerSetPickedKillerPlayer_Implementation(ASCPlayerState* NewPickedKiller)
{
	if (!IsHost())
	{
		return;
	}

	UWorld* World = GetWorld();

	// Check if killer picking is allowed
	ASCGameSession* GameSession = Cast<ASCGameSession>(World->GetAuthGameMode()->GameSession);
	if (!GameSession || !GameSession->AllowKillerPicking())
	{
		return;
	}

	if (AGameStateBase* GameState = World->GetGameState())
	{
		// Clear any previously-picked killer players
		bool bWasToggle = false;
		for (APlayerState* Player : GameState->PlayerArray)
		{
			if (ASCPlayerState* PS = Cast<ASCPlayerState>(Player))
			{
				if (PS->IsPickedKiller())
				{
					PS->SetIsPickedKiller(false);
					if (NewPickedKiller && PS == NewPickedKiller)
					{
						bWasToggle = true;
					}
				}
			}
		}

		if (!bWasToggle && NewPickedKiller)
		{
			NewPickedKiller->SetIsPickedKiller(true);
		}
	}
}

void ASCPlayerState::CLIENT_RequestLoadPlayerSettings_Implementation()
{
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOwner()))
	{
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(PC->GetLocalPlayer()))
		{
			if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				FSCKillerCharacterProfile NewKillerProfile = Settings->GetPickedKillerProfile();

				// We don't have a picked killer... nothing to load so STOP IT.
				if (NewKillerProfile.KillerCharacterClass.IsNull())
				{
					SERVER_SettingsLoaded();
					return;
				}

				ServerSetKillerSkin(NewKillerProfile.SelectedSkin);
				ServerReceiveSelectedWeapon(NewKillerProfile.SelectedWeapon.ToSoftObjectPath());
			}
			else
			{
				SERVER_SettingsLoaded();
			}
		}
	}
}

bool ASCPlayerState::SERVER_SettingsLoaded_Validate()
{
	return true;
}

void ASCPlayerState::SERVER_SettingsLoaded_Implementation()
{
	if (ASCGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASCGameState>() : nullptr)
	{
		if (ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GS->GetWorldSettings()))
		{
			// There were no settings... Continue on I suppose.
			WorldSettings->SettingsLoaded();
		}
	}
}

void ASCPlayerState::SetCounselorClass(TSoftClassPtr<ASCCounselorCharacter> NewCounselorClass)
{
	check(Role == ROLE_Authority);

	if (HasReceivedProfile() && !NewCounselorClass.IsNull())
	{
		if (USCGameBlueprintLibrary::IsCharacterLockedForPlayer(this, NewCounselorClass))
		{
			NewCounselorClass = nullptr;
		}
	}

	if (PickedCounselorClass != NewCounselorClass)
	{
		PickedCounselorClass = NewCounselorClass;

		OnRep_PickedCounselorClass(PickedCounselorClass);
	}
}

void ASCPlayerState::SetCounselorPerks(const TArray<FSCPerkBackendData>& NewPerks)
{
	check(Role == ROLE_Authority);

	TArray<FSCPerkBackendData> AllowedPerks;
	if (HasReceivedProfile())
	{
		for (const FSCPerkBackendData& NewPerk : NewPerks)
		{
			// Make sure this player owns this perk on the backend,
			// and they don't have multiple perks with the same class selected
			const FSCPerkBackendData* BackendPerk = OwnedCounselorPerks.FindByKey(NewPerk);
			if (BackendPerk && BackendPerk->PerkClass && !AllowedPerks.ContainsByPredicate([&BackendPerk](const FSCPerkBackendData& Perk) -> bool {	return Perk.PerkClass == BackendPerk->PerkClass; }))
			{
				// Use the perk data from the backend to make sure we have correct perk values
				AllowedPerks.AddUnique(*BackendPerk);
			}
		}
	}
	else
	{
		AllowedPerks = NewPerks;
	}

	if (PickedPerks != AllowedPerks)
	{
		PickedPerks = AllowedPerks;
		OnRep_PickedPerks();
	}
}

void ASCPlayerState::SetCounselorOutfit(const FSCCounselorOutfit& NewOutfit)
{
	check(Role == ROLE_Authority);

	if (NewOutfit != PickedOutfit)
	{
		PickedOutfit = NewOutfit;
		OnRep_PickedOutfit();
	}
}

void ASCPlayerState::SetKillerClass(TSoftClassPtr<ASCKillerCharacter> NewKillerClass)
{
	check(Role == ROLE_Authority);

	if (HasReceivedProfile() && !NewKillerClass.IsNull())
	{
		if (USCGameBlueprintLibrary::IsCharacterLockedForPlayer(this, NewKillerClass))
		{
			NewKillerClass = nullptr;
		}
	}

	if (PickedKillerClass != NewKillerClass)
	{
		PickedKillerClass = NewKillerClass;

		OnRep_PickedKillerClass();
	}
}

void ASCPlayerState::SetKillerGrabKills(const TArray<TSubclassOf<ASCContextKillActor>>& NewKills)
{
	check(Role == ROLE_Authority);

	TArray<TSubclassOf<ASCContextKillActor>> AllowedKills(NewKills);
	if (HasReceivedProfile())
	{
		for (TSubclassOf<ASCContextKillActor> NewKill : NewKills)
		{
			if (!DoesPlayerOwnKill(NewKill))
				AllowedKills.Remove(NewKill);
		}
	}

	if (PickedGrabKills != AllowedKills)
	{
		PickedGrabKills = AllowedKills;
	}
}

void ASCPlayerState::SetKillerSkin(TSubclassOf<USCJasonSkin> Skin)
{
	check(Role == ROLE_Authority);

	if (PickedKillerSkin != Skin)
		PickedKillerSkin = Skin;
}

void ASCPlayerState::SetKillerAbilityUnlockOrder(const TArray<EKillerAbilities>& CustomOrder)
{
	PickedKillerAbilityUnlockOrder = CustomOrder;
}

void ASCPlayerState::SetKillerWeapon(const TSoftClassPtr<ASCWeapon>& Weapon)
{
	check(Role == ROLE_Authority);

	UE_LOG(LogSC, Log, TEXT("SetKillerWeapon %s"), *Weapon.ToString());

	if (PickedKillerWeapon != Weapon)
	{
		PickedKillerWeapon = Weapon;
		OnRep_PickedKillerWeapon();
	}

	if (ASCGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASCGameState>() : nullptr)
	{
		if (ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GS->GetWorldSettings()))
		{
			// There were no settings... Continue on I suppose.
			WorldSettings->SettingsLoaded();
		}
	}
}

void ASCPlayerState::OnRep_PickedCounselorClass(TSoftClassPtr<ASCCounselorCharacter> OldCharacter)
{
	PlayerSettingsUpdated();
}

void ASCPlayerState::OnRep_PickedPerks()
{
	PlayerSettingsUpdated();

	ActivePerks.Empty();
	SetActivePerks(PickedPerks);
}

void ASCPlayerState::OnRep_PickedOutfit()
{
	PlayerSettingsUpdated();
}

void ASCPlayerState::OnRep_PickedKillerClass()
{
	PlayerSettingsUpdated();
}

void ASCPlayerState::OnRep_PickedKillerSkin()
{
	PlayerSettingsUpdated();
}

void ASCPlayerState::OnRep_PickedKillerWeapon()
{
	PickedKillerWeapon.LoadSynchronous();
	PlayerSettingsUpdated();
}

void ASCPlayerState::SetIsPickedKiller(const bool bNewPickedKiller)
{
	check(Role == ROLE_Authority);
	if (bIsPickedKiller != bNewPickedKiller)
	{
		bIsPickedKiller = bNewPickedKiller;

		// Keep the game instance in sync
		USCGameInstance* GI = CastChecked<USCGameInstance>(GetGameInstance());
		if (bIsPickedKiller)
		{
			GI->SetPickedKillerPlayer(AccountId);
		}
		else if (GI->IsPickedKillerPlayer(this))
		{
			GI->ClearPickedKillerPlayer();
		}

		OnRep_bIsPickedKiller();
	}
}

void ASCPlayerState::OnRep_bIsPickedKiller()
{
	PlayerSettingsUpdated();
}

void ASCPlayerState::OnRep_DeadOrEscaped()
{
	if (ASCPlayerController* OwnerController = Cast<ASCPlayerController>(GetOwner()))
	{
		// Check for character HUD changes
		if (OwnerController->GetSCHUD())
		{
			OwnerController->GetSCHUD()->UpdateCharacterHUD();
		}
	}
}

void ASCPlayerState::SetActivePerks(const TArray<FSCPerkBackendData>& PerkData)
{
	ActivePerks.Empty();
	for (const FSCPerkBackendData& Perk : PerkData)
	{
		if (Perk.PerkClass)
		{
			if (USCPerkData* PerkInstance = USCPerkData::CreatePerkFromBackendData(Perk))
			{
				ActivePerks.Add(PerkInstance);
			}
		}
	}
}

void ASCPlayerState::SetActivePerks(const TArray<USCPerkData*>& PerkData)
{
	if (Role < ROLE_Authority)
	{
		SERVER_SetActivePerks(PerkData);
	}
	else
	{
		ActivePerks = PerkData;
		CLIENT_SetActivePerks(PerkData);
	}
}

bool ASCPlayerState::SERVER_SetActivePerks_Validate(const TArray<USCPerkData*>& PerkData)
{
	return true;
}

void ASCPlayerState::SERVER_SetActivePerks_Implementation(const TArray<USCPerkData*>& PerkData)
{
	SetActivePerks(PerkData);
}

void ASCPlayerState::CLIENT_SetActivePerks_Implementation(const TArray<USCPerkData*>& PerkData)
{
	ActivePerks = PerkData;
}

UTexture2D* ASCPlayerState::GetAvatar()
{
	if (!CachedAvatar)
	{
		if (IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get())
	{
			auto IdentityInt = OnlineSub->GetIdentityInterface();
			if (IdentityInt.IsValid() && UniqueId.IsValid())
		{
				// FIXME: pjackson: Refactor
			//	CachedAvatar = IdentityInt->GetAvatar(*UniqueId);
			}
		}
	}

	return CachedAvatar;
}

bool ASCPlayerState::IsPlayerInAttenuationRange(APlayerState* OtherPlayer) const
{
	if (VoiceAudioComponent && OtherPlayer)
	{
		const FVector MyLocation = GetPlayerLocation();
		const FVector TheirLocation = OtherPlayer->GetPlayerLocation();
		if (MyLocation != FVector::ZeroVector && TheirLocation != FVector::ZeroVector)
		{
			const float DistanceToOtherPlayerSq = (MyLocation - TheirLocation).SizeSquared();

			return (DistanceToOtherPlayerSq < FMath::Square(VoiceAudioComponent->AttenuationOverrides.FalloffDistance));
		}
	}

	return false;
}

bool ASCPlayerState::HasWalkieTalkie() const
{
	if (bSpectating)
		return false;

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GS = Cast<ASCGameState>(World->GetGameState()))
		{
			return GS->WalkieTalkiePlayers.Contains(this);
		}
	}

	return false;
}

void ASCPlayerState::AddTapeToInventory(TSubclassOf<USCTapeDataAsset> TapeClass)
{
	check(Role == ROLE_Authority);

#if WITH_EDITOR
	if (GetWorld()->IsPlayInEditor())
		return;
#endif

	if (TapeClass)
	{
		if (AILLPlayerController* OwnerController = Cast<AILLPlayerController>(GetOwner()))
		{
			USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
			USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
			if (RequestManager->CanSendAnyRequests())
			{
				if (ensure(AccountId.IsValid()))
				{
					// Remove class suffix and pass that to the backend
					FString TapeName = TapeClass->GetName();
					TapeName.RemoveFromEnd(TEXT("_C"));

					FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ASCPlayerState::OnAddTapeToInventory);
					if (TapeClass->IsChildOf(USCPamelaTapeDataAsset::StaticClass()))
						RequestManager->AddPamelaTapeToInventory(AccountId, TapeName, Delegate);
					else if (TapeClass->IsChildOf(USCTommyTapeDataAsset::StaticClass()))
						RequestManager->AddTommyTapeToInventory(AccountId, TapeName, Delegate);
				}
				else
				{
					UE_LOG(LogSC, Warning, TEXT("AddTapeToInventory called with blank AccountId!"));
				}
			}
		}
	}
}

void ASCPlayerState::OnAddTapeToInventory(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("OnAddTapeToInventory: ResponseCode %i Reason %s"), ResponseCode, *ResponseContents);
		return;
	}
}

#undef LOCTEXT_NAMESPACE
