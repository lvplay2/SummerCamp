// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerCharacterWidget.h"

#include "ILLBackendBlueprintLibrary.h"

#include "SCBackendRequestManager.h"
#include "SCBackendPlayer.h"
#include "SCContextKillActor.h"
#include "SCGameInstance.h"
#include "SCKillerCharacter.h"
#include "SCLocalPlayer.h"
#include "SCPlayerController.h"
#include "SCPlayerState.h"
#include "SCWeapon.h"
#include "SCActorPreview.h"

USCKillerCharacterWidget::USCKillerCharacterWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	AmountOfGrabKills = 4;
	CustomUnlockOrderIndex = 0;
}

void USCKillerCharacterWidget::RemoveFromParent()
{
	if (ActorPreview)
	{
		ActorPreview->DestroyPreviewActor();
		ActorPreview->SetActorHiddenInGame(true);
		ActorPreview->Destroy();
		ActorPreview = nullptr;
	}

	Super::RemoveFromParent();
}


void USCKillerCharacterWidget::SaveCharacterProfiles()
{
	if (GetWorld()->IsPlayInEditor())
	{
		OnSaveCharacterProfiles(true);
		ClearDirty();
		return;
	}

	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer()))
	{
		if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
		{
			AILLPlayerController* PC = Cast<AILLPlayerController>(LocalPlayer->GetPlayerController(GetWorld()));
			Settings->KillerCharacterProfiles = KillerCharacterProfiles;
			Settings->ApplyPlayerSettings(PC, true);
			OnSaveCharacterProfiles(true);
			ClearDirty();
			return;
		}
	}

	OnSaveCharacterProfiles(false);
}

void USCKillerCharacterWidget::LoadCharacterProfiles()
{
	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer()))
	{
		if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
		{
			KillerCharacterProfiles = Settings->KillerCharacterProfiles;
		}
	}
}

void USCKillerCharacterWidget::GetKillerWeaponDetails(TSoftClassPtr<ASCWeapon> KillerWeaponClass, FText& WeaponName, UTexture2D*& WeaponImage) const
{
	if (!KillerWeaponClass.IsNull())
	{
		KillerWeaponClass.LoadSynchronous(); // TODO: Make Async
		if (const ASCWeapon* const Weapon = Cast<ASCWeapon>(KillerWeaponClass.Get()->GetDefaultObject()))
		{
			WeaponName = Weapon->GetFriendlyName();
			WeaponImage = Weapon->GetThumbnailImage();
		}
	}
}

bool USCKillerCharacterWidget::GetGrabKillWeaponMatch(TSubclassOf<ASCContextKillActor> GrabKillClass, TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass)
{
	if (KillerCharacterClass && GrabKillClass)
	{
		if (const ASCContextKillActor* const ContextKill = Cast<ASCContextKillActor>(GrabKillClass->GetDefaultObject()))
		{
			// doesn't require a weapon, why are you asking?
			if (ContextKill->GetContextWeaponType() == nullptr)
				return true;

			LoadCharacterProfiles();
			// grab the profile and check if the set weapon is allowed to be used with this kill
			for (int32 iKiller(0); iKiller < KillerCharacterProfiles.Num(); ++iKiller)
			{
				if (KillerCharacterProfiles[iKiller].KillerCharacterClass == KillerCharacterClass)
				{
					if (!KillerCharacterProfiles[iKiller].SelectedWeapon.IsNull())
						return KillerCharacterProfiles[iKiller].SelectedWeapon == ContextKill->GetContextWeaponType();
				}
			}

			KillerCharacterClass.LoadSynchronous(); // TODO: Make Async

			// we did not have a weapon set, check the default weapon instead
			if (const ASCKillerCharacter* const KillerCharacter = Cast<ASCKillerCharacter>(KillerCharacterClass.Get()->GetDefaultObject()))
			{
				if (TSoftClassPtr<ASCWeapon> WeaponClass = KillerCharacter->GetDefaultWeaponClass())
				{
					return ContextKill->GetContextWeaponType() == WeaponClass;
				}
			}
		}
	}

	return false;
}

bool USCKillerCharacterWidget::DoesKillNeedBought(TSubclassOf<class ASCContextKillActor> KillClass)
{
	if (KillClass)
	{
		if (const ASCContextKillActor* const ContextKill = Cast<ASCContextKillActor>(KillClass->GetDefaultObject()))
		{
			// Check to see if the player owns this kill or if its cost is 0. 0 Cost means its a default kill that is auto owned.
			if (ASCPlayerState *PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
			{
				return !PlayerState->DoesPlayerOwnKill(KillClass) && (ContextKill->GetCost() != 0);
			}
		}
	}
	return false;
}

void USCKillerCharacterWidget::PurchaseKill(TSubclassOf<class ASCContextKillActor> KillClass)
{
	if (GetWorld()->IsPlayInEditor())
	{
		OnJasonKillsPurchasedSuccess();
		return;
	}

	if (KillClass)
	{
		USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer());
		if (USCBackendPlayer* BackendPlayer = LocalPlayer ? LocalPlayer->GetBackendPlayer<USCBackendPlayer>() : nullptr)
		{
			// Cut off the "_C" from the end of the class name.
			FString BackendFrienlyClass = KillClass->GetName().Left(KillClass->GetName().Len() - 2);

			USCGameInstance* GI = Cast<USCGameInstance>(LocalPlayer->GetGameInstance());
			USCBackendRequestManager* BRM = GI->GetBackendRequestManager<USCBackendRequestManager>();
			FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &USCKillerCharacterWidget::OnPurchaseKill);
			if (BRM->PurchaseItem(BackendPlayer, TEXT("JasonKills"), BackendFrienlyClass, Delegate))
			{
				return;
			}
		}
	}

	// TODO needs to send more info to the user like did we not reach the backend?
	OnJasonKillsPurchased(false);
}

void USCKillerCharacterWidget::OnPurchaseKill(int32 ResponseCode, const FString& ResponseContents)
{
	// Purchase succeeded. Now we need to refresh this menu.
	if (ResponseCode == 200)
	{
		if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
		{
			PlayerState->OwnedJasonKillsUpdated.AddDynamic(this, &USCKillerCharacterWidget::OnJasonKillsPurchasedSuccess);
			PlayerState->SERVER_RequestBackendProfileUpdate();
		}
	}
	else
	{
		// Handle Error to UI
		OnJasonKillsPurchased(false);
	}
}

void USCKillerCharacterWidget::OnJasonKillsPurchasedSuccess()
{
	if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
	{
		if (PlayerState->OwnedJasonKillsUpdated.IsBound())
		{
			PlayerState->OwnedJasonKillsUpdated.RemoveDynamic(this, &USCKillerCharacterWidget::OnJasonKillsPurchasedSuccess);
		}

		OnJasonKillsPurchased(true);
	}
}

void USCKillerCharacterWidget::SetKillerCharacterProfile(TSoftClassPtr<ASCKillerCharacter> KillerCharacter, const TSubclassOf<ASCContextKillActor> GrabKill, const int32 KillIndex)
{
	bool AlreadyStoredCharacter = false;

	for (int iKiller(0); iKiller < KillerCharacterProfiles.Num(); ++iKiller)
	{
		if (KillerCharacterProfiles[iKiller].KillerCharacterClass == KillerCharacter)
		{
			// Should always have a minimum number of kills (may be null! This is okay!)
			if (KillerCharacterProfiles[iKiller].GrabKills.Num() < AmountOfGrabKills)
				KillerCharacterProfiles[iKiller].GrabKills.AddZeroed(AmountOfGrabKills - KillerCharacterProfiles[iKiller].GrabKills.Num());

			if (KillIndex < KillerCharacterProfiles[iKiller].GrabKills.Num())
			{
				// check to see if its the same data getting saved. if it is skip saving.
				if (KillerCharacterProfiles[iKiller].GrabKills[KillIndex] != GrabKill)
				{
					// clear previously of the same selected out.
					for (int iKill(0); iKill < KillerCharacterProfiles[iKiller].GrabKills.Num(); ++iKill)
					{
						if (KillerCharacterProfiles[iKiller].GrabKills[iKill] == GrabKill)
						{
							KillerCharacterProfiles[iKiller].GrabKills[iKill] = nullptr;
						}
					}

					KillerCharacterProfiles[iKiller].GrabKills[KillIndex] = GrabKill;

					if (USCBackendInventory* BackendInventory = GetInventoryManager())
					{
						BackendInventory->bIsDirty = true;
					}
				}
			}

			AlreadyStoredCharacter = true;
		}
	}

	if (!AlreadyStoredCharacter)
	{
		TArray<TSubclassOf<class ASCContextKillActor>> GrabKills;
		GrabKills.AddZeroed(AmountOfGrabKills);
		GrabKills[KillIndex] = GrabKill;

		KillerCharacterProfiles.Add(FSCKillerCharacterProfile(KillerCharacter, GrabKills));

		if (USCBackendInventory* BackendInventory = GetInventoryManager())
		{
			BackendInventory->bIsDirty = true;
		}
	}
}

FSCKillerCharacterProfile USCKillerCharacterWidget::GetKillerCharacterProfile(TSoftClassPtr<ASCKillerCharacter> KillerCharacter) const
{
	for (int i = 0; i < KillerCharacterProfiles.Num(); ++i)
	{
		if (KillerCharacterProfiles[i].KillerCharacterClass == KillerCharacter)
		{
			return KillerCharacterProfiles[i];
		}
	}

	return FSCKillerCharacterProfile(KillerCharacter, TArray<TSubclassOf<ASCContextKillActor>>());
}

void USCKillerCharacterWidget::SetKillerSkinProfile(const TSoftClassPtr<ASCKillerCharacter> KillerCharacter, const TSubclassOf<class USCJasonSkin> SelectedSkin)
{
	bool AlreadyStoredCharacter = false;

	// attempt to find and set skin on killer profile
	for (int i = 0; i < KillerCharacterProfiles.Num(); ++i)
	{
		// if this is the profile for the provided killer class
		if (KillerCharacterProfiles[i].KillerCharacterClass == KillerCharacter)
		{
			// delta check the skin to make sure we need to set it
			if (KillerCharacterProfiles[i].SelectedSkin != SelectedSkin)
			{
				// set the skin and mark the profile dirty for save
				KillerCharacterProfiles[i].SelectedSkin = SelectedSkin;
				AlreadyStoredCharacter = true;
			}
			break;
		}
	}

	// profile for this killer hasnt been created yet, add it
	if (!AlreadyStoredCharacter)
	{
		TArray<TSubclassOf<ASCContextKillActor>> GrabKills;
		GrabKills.AddZeroed(AmountOfGrabKills);

		FSCKillerCharacterProfile NewProfile = FSCKillerCharacterProfile(KillerCharacter, GrabKills);
		NewProfile.SelectedSkin = SelectedSkin;
		KillerCharacterProfiles.Add(NewProfile);
	}

	// mark the save dirty so it gets saved correctly
	if (USCBackendInventory* BackendInventory = GetInventoryManager())
	{
		BackendInventory->bIsDirty = true;
	}
}

void USCKillerCharacterWidget::SetActorPreviewClass(TSoftClassPtr<ASCKillerCharacter> KillerClass, FTransform Offset)
{
	// if the actor preview hasnt been spawned yet, do it now
	if (GetOwningPlayer() && !ActorPreview)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwningPlayer();
		SpawnParams.Instigator = GetOwningPlayer()->GetInstigator();

		ActorPreview = GetWorld()->SpawnActor<ASCActorPreview>(MenuPreviewActorClass, SpawnParams);
	}

	// the actor preview was spawned
	if (ActorPreview)
	{
		TSoftClassPtr<AActor> NewClass = GetBeautyPoseClass(KillerClass);

		// We already are this class. Fuck off.
		if (ActorPreview->GetPreviewActor() && !NewClass.IsNull() && ActorPreview->GetPreviewActor()->GetClass() == NewClass.Get())
			return;

		// set the new class and spawn it
		ActorPreview->PreviewClass = NewClass;
		ActorPreview->SpawnPreviewActor();

		// get the preview actor and load its clothing
		if (ASCKillerCharacter* Character = Cast<ASCKillerCharacter>(ActorPreview->GetPreviewActor()))
		{
			// make sure the streaming distance is set high so that all materials always stream in completely
			Character->GetMesh()->StreamingDistanceMultiplier = 1000.0f;
			Character->GetKillerMaskMesh()->StreamingDistanceMultiplier = 1000.0f;

			Character->SetActorTransform(Offset, false, nullptr, ETeleportType::TeleportPhysics);

			FSCKillerCharacterProfile KillerProfile = GetKillerCharacterProfile(KillerClass);
			Character->SetSkin(KillerProfile.SelectedSkin);
		}
	}
}

ASCKillerCharacter* USCKillerCharacterWidget::GetPreviewKillerActor() const
{
	if (ActorPreview)
	{
		if (ASCKillerCharacter* Character = Cast<ASCKillerCharacter>(ActorPreview->GetPreviewActor()))
			return Character;
	}

	return nullptr;
}

TSoftClassPtr<ASCKillerCharacter> USCKillerCharacterWidget::GetBeautyPoseClass_Implementation(const TSoftClassPtr<ASCKillerCharacter>& KillerClass)
{
	return KillerClass;
}

ESlateVisibility USCKillerCharacterWidget::GetAbilityUnlockButtonVisibility() const
{
	// feature was disabled
	return ESlateVisibility::Collapsed;
	/*ESlateVisibility BtnVisibility = ESlateVisibility::Collapsed; 

	if (const ASCPlayerState* PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
	{
		if (PlayerState->HasReceivedProfile() && PlayerState->GetPlayerLevel() >= ABILITY_UNLOCKING_LEVEL)
		{
			BtnVisibility = ESlateVisibility::Visible;
		}
	}

	return BtnVisibility;*/
}


void USCKillerCharacterWidget::ResetCurrentAbilitySelectionIndex()
{
	// Reset the index since we're reopening the popup.
	CustomUnlockOrderIndex = 0;
}

void USCKillerCharacterWidget::OnAbilityUnlockSelected(EKillerAbilities SelectedAbility, TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass)
{
	for (FSCKillerCharacterProfile& KillerClassToUpdate : KillerCharacterProfiles)
	{
		// if this is the profile for the provided killer class
		if (KillerClassToUpdate.KillerCharacterClass == KillerCharacterClass)
		{
			// Array starts out empty, so populate it if needed
			if (KillerClassToUpdate.AbilityUnlockOrder.Num() == 0)
			{
				KillerClassToUpdate.AbilityUnlockOrder.Init(EKillerAbilities::Morph, (int32)EKillerAbilities::MAX);
			}

			KillerClassToUpdate.AbilityUnlockOrder[CustomUnlockOrderIndex] = SelectedAbility;
			break;
		}
	}

	CustomUnlockOrderIndex++;
}
