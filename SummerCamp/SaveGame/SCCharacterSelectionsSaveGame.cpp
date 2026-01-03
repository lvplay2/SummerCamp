// Copyright (C) 2015-2018 IllFonic, LLC.

#include "SummerCamp.h"
#include "SCCharacterSelectionsSaveGame.h"
#include "SCSinglePlayerSaveGame.h"

#include "SCCharacter.h"
#include "SCLocalPlayer.h"
#include "SCPlayerController.h"
#include "SCPlayerController_Lobby.h"
#include "SCPlayerController_Menu.h"

USCCharacterSelectionsSaveGame::USCCharacterSelectionsSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCCharacterSelectionsSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave)
{
	Super::ApplyPlayerSettings(PlayerController, bAndSave);

	VerifySettingOwnership(PlayerController, false);

	if (UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(PlayerController))
		{
			PC->bSentPreferencesToServer = false;
			World->GetTimerManager().SetTimerForNextTick(PC, &ASCPlayerController::SendPreferencesToServer);
		}
		else if (ASCPlayerController_Lobby* LobbyPC = Cast<ASCPlayerController_Lobby>(PlayerController))
		{
			LobbyPC->bSentPreferencesToServer = false;
			World->GetTimerManager().SetTimerForNextTick(LobbyPC, &ASCPlayerController_Lobby::SendPreferencesToServer);
		}
		else if (ASCPlayerController_Menu* MenuPC = Cast<ASCPlayerController_Menu>(PlayerController))
		{
			MenuPC->bSentPreferencesToServer = false;
			World->GetTimerManager().SetTimerForNextTick(MenuPC, &ASCPlayerController_Menu::SendPreferencesToServer);
		}
	}
}

bool USCCharacterSelectionsSaveGame::HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const
{
	USCCharacterSelectionsSaveGame* Other = CastChecked<USCCharacterSelectionsSaveGame>(OtherSave);
	return (CounselorPick != Other->CounselorPick
		|| CounselorCharacterProfiles != Other->CounselorCharacterProfiles
		|| KillerPick != Other->KillerPick
		|| KillerCharacterProfiles != Other->KillerCharacterProfiles);
}

void USCCharacterSelectionsSaveGame::SaveGameLoaded(AILLPlayerController* PlayerController)
{
	Super::SaveGameLoaded(PlayerController);

	VerifySettingOwnership(PlayerController, true);
}

void USCCharacterSelectionsSaveGame::VerifySettingOwnership(AILLPlayerController* PlayerController, const bool bAndSave)
{
	USCLocalPlayer* LocalPlayer = CastChecked<USCLocalPlayer>(PlayerController->GetLocalPlayer());
	if (!LocalPlayer)
		return;

	bool bDirtied = false;

	ASCPlayerState* PlayerState = Cast<ASCPlayerState>(PlayerController->PlayerState);
	USCSinglePlayerSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>();
	const bool bRecievedBackendProfile = PlayerState ? PlayerState->HasReceivedProfile() : false;

	// Wait to receive entitlements and our profile
	if (LocalPlayer->HasReceivedEntitlements() && bRecievedBackendProfile)
	{
		// Clear character picks for characters we do not have entitlements for
		auto DoesUserOwnCharacter = [&](TSubclassOf<ASCCharacter> CharacterClass) -> bool
		{
			if (ASCCharacter* DefaultCharacter = CharacterClass ? CharacterClass.GetDefaultObject() : nullptr)
			{
				/*if (ASCCounselorCharacter* DefaultCounselor = Cast<ASCCounselorCharacter>(DefaultCharacter))
				{
					if (DefaultCounselor->bIsHunter)
						return false;
				}*/ // 02/03/2021 - re-enable Hunter Selection

				return PlayerController->DoesUserOwnEntitlement(DefaultCharacter->UnlockEntitlement);
			}

			return false;
		};

		if (CounselorPick && !DoesUserOwnCharacter(CounselorPick))
		{
			UE_LOG(LogSC, Log, TEXT("No Entitlement Owned for %s"), *CounselorPick->GetName());
			CounselorPick = nullptr;
			bDirtied = true;
		}

		if (KillerPick && !DoesUserOwnCharacter(KillerPick))
		{
			UE_LOG(LogSC, Log, TEXT("No Entitlement Owned for %s"), *KillerPick->GetName());
			KillerPick = nullptr;
			bDirtied = true;
		}

		// make sure we own the dlc for the jason skins
		for (FSCKillerCharacterProfile& KillerProfile : KillerCharacterProfiles)
		{
			if (KillerProfile.SelectedSkin != nullptr)
			{
				if (USCJasonSkin* Skin = Cast<USCJasonSkin>(KillerProfile.SelectedSkin->GetDefaultObject()))
				{
					if (!PlayerController->DoesUserOwnEntitlement(Skin->DLCUnlock))
					{
						UE_LOG(LogSC, Log, TEXT("No Entitlement Owned for %s"), *Skin->GetName());
						KillerProfile.SelectedSkin = nullptr;
						bDirtied = true;
					}
				}
			}

			// make sure we validate the grab kills for dlc
			for (uint8 iKill(0); iKill < KillerProfile.GrabKills.Num(); ++iKill)
			{
				if (KillerProfile.GrabKills[iKill] != nullptr)
				{
					if (!PlayerState->DoesPlayerOwnKill(KillerProfile.GrabKills[iKill]))
					{
						UE_LOG(LogSC, Log, TEXT("No Entitlement Owned for %s"), *KillerProfile.GrabKills[iKill]->GetName());
						KillerProfile.GrabKills[iKill] = nullptr;
						bDirtied = true;
					}
				}
			}

			// Make sure we have the entitlement for the weapon selected.
			if (!KillerProfile.SelectedWeapon.IsNull())
			{
				KillerProfile.SelectedWeapon.LoadSynchronous();
				if (ASCWeapon* Weapon = Cast<ASCWeapon>(KillerProfile.SelectedWeapon->GetDefaultObject()))
				{
					if (!PlayerController->DoesUserOwnEntitlement(Weapon->DLCEntitlement))
					{
						UE_LOG(LogSC, Log, TEXT("No Entitlement Owned for %s"), *KillerProfile.SelectedWeapon->GetName());
						KillerProfile.SelectedWeapon = nullptr;
						bDirtied = true;
					}
				}
			}
		}

		for (FSCCounselorCharacterProfile& CounselorProfile : CounselorCharacterProfiles)
		{
			if (CounselorProfile.CounselorCharacterClass.IsNull())
				continue;

			CounselorProfile.CounselorCharacterClass.LoadSynchronous();

			// validate clothing options
			if (CounselorProfile.Outfit.ClothingClass != nullptr)
			{
				if (USCClothingData* ClothingData = Cast<USCClothingData>(CounselorProfile.Outfit.ClothingClass->GetDefaultObject()))
				{
					if (!PlayerController->DoesUserOwnEntitlement(ClothingData->UnlockEntitlement))
					{
						UE_LOG(LogSC, Log, TEXT("No Entitlement Owned for %s"), *ClothingData->GetName());
						CounselorProfile.Outfit.ClothingClass = nullptr;
						bDirtied = true;
					}

					// validate all of the swatch options for the outfit
					for (int i(0); i < CounselorProfile.Outfit.MaterialPairs.Num(); ++i)
					{
						if (CounselorProfile.Outfit.MaterialPairs[i].SelectedSwatch)
						{
							// make sure this swatch is valid for this outfit
							if (!ClothingData->IsClothingSwatchValidForOutfit(CounselorProfile.Outfit.MaterialPairs[i].SelectedSwatch))
							{
								CounselorProfile.Outfit.MaterialPairs.RemoveAt(i);
								bDirtied = true;
								continue;
							}

							// make sure if this is dlc they own the entitlement
							if (USCClothingSlotMaterialOption* SlotOption = Cast<USCClothingSlotMaterialOption>(CounselorProfile.Outfit.MaterialPairs[i].SelectedSwatch->GetDefaultObject()))
							{
								if (!PlayerController->DoesUserOwnEntitlement(SlotOption->UnlockEntitlement))
								{
									UE_LOG(LogSC, Log, TEXT("No Entitlement Owned for %s"), *SlotOption->GetName());
									CounselorProfile.Outfit.MaterialPairs.RemoveAt(i);
									bDirtied = true;
								}
							}
						}
					}
				}
			}

			ASCCounselorCharacter* DefaultCounselor = Cast<ASCCounselorCharacter>(CounselorProfile.CounselorCharacterClass->GetDefaultObject());
			// Load default emotes if none are saved yet
			if (CounselorProfile.Emotes.Num() == 0)
			{
				if (DefaultCounselor)
				{
					for (const auto& DefaultEmoteData : DefaultCounselor->DefaultEmotes)
					{
						CounselorProfile.Emotes.Add(DefaultEmoteData);
					}
				}
			}

			// Pointer guard. Always pointer guard.
			if (SaveGame && SaveGame->HasReceivedProfile())
			{
				// make sure we own the dlc or unlock for emotes and they are equipped on a valid counselor class
				for (int i(0); i < CounselorProfile.Emotes.Num(); ++i)
				{
					if (CounselorProfile.Emotes[i] == nullptr)
						continue;

					if (USCEmoteData* Emote = Cast<USCEmoteData>(CounselorProfile.Emotes[i]->GetDefaultObject()))
					{
						if (!PlayerController->DoesUserOwnEntitlement(Emote->EntitlementID))
						{
							UE_LOG(LogSC, Log, TEXT("No Entitlement Owned for %s"), *Emote->GetName());
							// Naughty naughty
							if (DefaultCounselor && i < DefaultCounselor->DefaultEmotes.Num())
								CounselorProfile.Emotes[i] = DefaultCounselor->DefaultEmotes[i];
							else
								CounselorProfile.Emotes[i] = nullptr;

							bDirtied = true;
							continue;
						}

						if (!Emote->SinglePlayerEmoteUnlockID.IsNone() && SaveGame && !SaveGame->IsEmoteUnlocked(Emote->SinglePlayerEmoteUnlockID))
						{
							// Naughty naughty
							if (DefaultCounselor && i < DefaultCounselor->DefaultEmotes.Num())
								CounselorProfile.Emotes[i] = DefaultCounselor->DefaultEmotes[i];
							else
								CounselorProfile.Emotes[i] = nullptr;

							bDirtied = true;
							continue;
						}

						if (Emote->AllowedCounselors.Num() <= 0)
							continue;

						bool bEmoteAllowed = false;
						for (TSoftClassPtr<ASCCounselorCharacter> AllowedCounselor : Emote->AllowedCounselors)
						{
							if (AllowedCounselor == CounselorProfile.CounselorCharacterClass)
							{
								bEmoteAllowed = true;
								break;
							}
						}

						if (!bEmoteAllowed)
						{
							// Somehow we have an emote equipped for a counselor that isn't allowed to use it
							if (DefaultCounselor && i < DefaultCounselor->DefaultEmotes.Num())
								CounselorProfile.Emotes[i] = DefaultCounselor->DefaultEmotes[i];
							else
								CounselorProfile.Emotes[i] = nullptr;

							bDirtied = true;
						}
					}
				}
			}
		}
	}

	// Save out changes to the player
	if (bDirtied && bAndSave)
	{
		ApplyPlayerSettings(PlayerController, true);
	}
}

const FSCCounselorCharacterProfile& USCCharacterSelectionsSaveGame::GetCounselorProfile(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass) const
{
	for (const FSCCounselorCharacterProfile& CounselorProfile : CounselorCharacterProfiles)
	{
		if (CounselorProfile.CounselorCharacterClass == CounselorClass)
		{
			return CounselorProfile;
		}
	}

	static FSCCounselorCharacterProfile Default;
	Default.CounselorCharacterClass = CounselorClass;
	return Default;
}

const TArray<FSCPerkBackendData>& USCCharacterSelectionsSaveGame::GetCounselorPerks(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass) const
{
	return GetCounselorProfile(CounselorClass).Perks;
}

const FSCCounselorOutfit& USCCharacterSelectionsSaveGame::GetCounselorOutfit(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass) const
{
	return GetCounselorProfile(CounselorClass).Outfit;
}

const TArray<TSubclassOf<USCEmoteData>>& USCCharacterSelectionsSaveGame::GetCounselorEmotes(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass) const
{
	return GetCounselorProfile(CounselorClass).Emotes;
}

const FSCKillerCharacterProfile& USCCharacterSelectionsSaveGame::GetKillerProfile(const TSoftClassPtr<ASCKillerCharacter>& KillerClass) const
{
	for (const FSCKillerCharacterProfile& KillerProfile : KillerCharacterProfiles)
	{
		if (KillerProfile.KillerCharacterClass == KillerClass)
		{
			return KillerProfile;
		}
	}

	static FSCKillerCharacterProfile Default;
	Default.KillerCharacterClass = KillerClass;
	return Default;
}

const TArray<TSubclassOf<ASCContextKillActor>>& USCCharacterSelectionsSaveGame::GetKillerGrabKills(const TSoftClassPtr<ASCKillerCharacter>& KillerClass) const
{
	return GetKillerProfile(KillerClass).GrabKills;
}

TSoftClassPtr<ASCWeapon> USCCharacterSelectionsSaveGame::GetKillerSelectedWeapon(const TSoftClassPtr<ASCKillerCharacter>& KillerClass) const
{
	return GetKillerProfile(KillerClass).SelectedWeapon;
}