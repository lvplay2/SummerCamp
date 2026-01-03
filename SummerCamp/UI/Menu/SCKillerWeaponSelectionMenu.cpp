// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerCharacter.h"
#include "SCWeapon.h"
#include "SCKillerWeaponSelectionMenu.h"
#include "SCBackendInventory.h"
#include "SCBackendRequestManager.h"
#include "SCLocalPlayer.h"
#include "SCGameInstance.h"

USCKillerWeaponSelectionMenu::USCKillerWeaponSelectionMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

USCBackendInventory* USCKillerWeaponSelectionMenu::GetInventoryManager()
{
	if (!GetPlayerContext().IsValid())
		return nullptr;

	// Setup the Backend Inventory
	if (USCLocalPlayer* const LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer()))
	{
		USCGameInstance* GI = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
		USCBackendRequestManager* SCBackend = GI->GetBackendRequestManager<USCBackendRequestManager>();
		USCBackendPlayer* BackendPlayer = LocalPlayer->GetBackendPlayer<USCBackendPlayer>();
		if (BackendPlayer)
		{
			return BackendPlayer->GetBackendInventory();
		}
	}

	return nullptr;
}

void USCKillerWeaponSelectionMenu::InitalizeMenu(TSoftClassPtr<ASCKillerCharacter> KillerClass)
{
	if (WeaponList == nullptr || KillerClass.IsNull())
		return;

	// set killer class
	SelectedKillerClass = KillerClass;

	SelectedKillerClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCKillerCharacter* const Killer = Cast<ASCKillerCharacter>(SelectedKillerClass.Get()->GetDefaultObject()))
	{
		KillerName = Killer->GetCharacterName();
		KillerTrope = Killer->GetCharacterTropeName();
	}

	// init weapon list
	WeaponList->InitializeList(KillerClass);
}

void USCKillerWeaponSelectionMenu::SaveWeaponProfile(TSoftClassPtr<ASCWeapon> WeaponClass)
{
	// early out for editor
	if (GetWorld()->IsPlayInEditor())
	{
		if (USCBackendInventory* BackendInventory = GetInventoryManager())
		{
			BackendInventory->bIsDirty = false;
			return;
		}
	}

	// load the killer profiles
	TArray<FSCKillerCharacterProfile> KillerCharacterProfiles;
	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer()))
	{
		if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
		{
			// cache off profiles
			KillerCharacterProfiles = Settings->KillerCharacterProfiles;
		}
	}

	// iterate through the saves and attempt to find the profile for the currently selected Killer and set the new weapon
	bool WeaponSet = false;
	for (FSCKillerCharacterProfile& Profile : KillerCharacterProfiles)
	{
		if (Profile.KillerCharacterClass == SelectedKillerClass)
		{
			Profile.SelectedWeapon = WeaponClass;

			// check and clear any bad grab kills
			for (uint8 i(0); i < Profile.GrabKills.Num(); ++i)
			{
				// if no kill is set in this slot then there is no reason to evaluate it
				if (Profile.GrabKills[i] == nullptr)
					continue;

				if (const ASCContextKillActor* const KillDefault = Cast<ASCContextKillActor>(Profile.GrabKills[i]->GetDefaultObject()))
				{
					// the weapon type isnt null and it doesnt match the new set weapon so clear it
					if (KillDefault->GetContextWeaponType() != nullptr && KillDefault->GetContextWeaponType() != WeaponClass)
						Profile.GrabKills[i] = nullptr;
				}
			}
			// we found the profile for this killer in the save list
			WeaponSet = true;
		}
	}

	// we didnt find the profile for this killer, lets make one now and add it
	if (!WeaponSet)
	{
		TArray<TSubclassOf<ASCContextKillActor>> GrabKills;
		GrabKills.AddZeroed(4);// have to add zeroed kills so the save wont crash

		FSCKillerCharacterProfile NewProfile(SelectedKillerClass, GrabKills);
		NewProfile.SelectedWeapon = WeaponClass;
		KillerCharacterProfiles.Add(NewProfile);
	}

	// save the profile
	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer()))
	{
		if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
		{
			AILLPlayerController* PC = Cast<AILLPlayerController>(GetOwningPlayer());
			Settings->KillerCharacterProfiles = KillerCharacterProfiles;
			Settings->ApplyPlayerSettings(PC, true);

			// make sure to clear any dirty flag set
			if (USCBackendInventory* BackendInventory = GetInventoryManager())
			{
				BackendInventory->bIsDirty = false;
			}
			
		}
	}
}