// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorCharacterWidget.h"

#include "ILLBackendBlueprintLibrary.h"

#include "SCBackendRequestManager.h"
#include "SCBackendPlayer.h"
#include "SCCounselorActiveAbility.h"
#include "SCEmoteData.h"
#include "SCGameInstance.h"
#include "SCLocalPlayer.h"
#include "SCPerkData.h"
#include "SCPlayerController.h"
#include "SCPlayerState.h"
#include "SCActorPreview.h"

USCCounselorCharacterWidget::USCCounselorCharacterWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCCounselorCharacterWidget::RemoveFromParent()
{
	if (ActorPreview)
	{
		if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
		{
			Character->CleanupOutfit();
		}

		ActorPreview->DestroyPreviewActor();
		ActorPreview->SetActorHiddenInGame(true);
		ActorPreview->Destroy();
		ActorPreview = nullptr;
	}

	Super::RemoveFromParent();
}

void USCCounselorCharacterWidget::SaveCharacterProfiles()
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
			Settings->CounselorCharacterProfiles = CounselorCharacterProfiles;
			Settings->ApplyPlayerSettings(PC, true);
			OnSaveCharacterProfiles(true);
			ClearDirty();
			return;
		}
	}

	OnSaveCharacterProfiles(false);
}

void USCCounselorCharacterWidget::LoadCharacterProfiles()
{
	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer()))
	{
		if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
		{
			CounselorCharacterProfiles = Settings->CounselorCharacterProfiles;
		}
	}
}

bool USCCounselorCharacterWidget::IsPerkSlotEmpty(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, int32 PerkIndex) const
{
	if (CounselorClass)
	{
		for (int i = 0; i < CounselorCharacterProfiles.Num(); ++i)
		{
			if (CounselorCharacterProfiles[i].CounselorCharacterClass == CounselorClass)
			{
				if (CounselorCharacterProfiles[i].Perks.Num() > PerkIndex)
				{
					return CounselorCharacterProfiles[i].Perks[PerkIndex].InstanceID.IsEmpty();
				}
				else
				{
					return true;
				}
			}
		}
	}

	return true;
}

bool USCCounselorCharacterWidget::IsEmoteSlotEmpty(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, int32 EmoteIndex) const
{
	if (CounselorClass)
	{
		FSCCounselorCharacterProfile CounselorProfile = GetCounselorCharacterProfile(CounselorClass);
		if (CounselorProfile.Emotes.Num() > EmoteIndex)
			return CounselorProfile.Emotes[EmoteIndex] == nullptr;
	}

	return true;
}

void USCCounselorCharacterWidget::SetUnsavedCounselorProfile(TArray<FSCCounselorCharacterProfile> _CounselorCharacterProfiles)
{
	bool bDirty = false;
	if (CounselorCharacterProfiles.Num() != _CounselorCharacterProfiles.Num())
	{
		CounselorCharacterProfiles = _CounselorCharacterProfiles;
		bDirty = true;
	}
	else
	{
		for (int i(0); i < _CounselorCharacterProfiles.Num(); ++i)
		{
			if (CounselorCharacterProfiles[i].CounselorCharacterClass != _CounselorCharacterProfiles[i].CounselorCharacterClass
				||CounselorCharacterProfiles[i].Perks != _CounselorCharacterProfiles[i].Perks
				|| CounselorCharacterProfiles[i].Outfit != _CounselorCharacterProfiles[i].Outfit
				|| CounselorCharacterProfiles[i].Emotes != _CounselorCharacterProfiles[i].Emotes)
			{
				CounselorCharacterProfiles = _CounselorCharacterProfiles;
				bDirty = true;
				break;
			}
		}
	}

	if (USCBackendInventory* BackendInventory = GetInventoryManager())
	{
		BackendInventory->bIsDirty = bDirty;
	}
}

void USCCounselorCharacterWidget::PurchasePerkRoll()
{
	USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer());
	if (USCBackendPlayer* BackendPlayer = LocalPlayer ? LocalPlayer->GetBackendPlayer<USCBackendPlayer>() : nullptr)
	{
		USCGameInstance* GI = Cast<USCGameInstance>(LocalPlayer->GetGameInstance());
		USCBackendRequestManager* BRM = GI->GetBackendRequestManager<USCBackendRequestManager>();
		FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &USCCounselorCharacterWidget::OnPurchasePerkRoll);
		if (BRM->PurchasePerkRoll(BackendPlayer, Delegate))
		{
			return;
		}
	}

	// TODO needs to send more info to the user like did we not reach the backend?
	OnPerkRollPurchased(false);
}

void USCCounselorCharacterWidget::OnPurchasePerkRoll(int32 ResponseCode, const FString& ResponseContents)
{
	// Purchase succeeded. Now we need to refresh this menu.
	if (ResponseCode == 200)
	{
		// Parse the response from the backend that contains the new perk that
		// was just rolled.
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContents);
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
		{
			FString InstanceID;
			JsonObject->TryGetStringField(TEXT("InstanceID"), InstanceID);

			FString ClassName;
			JsonObject->TryGetStringField(TEXT("ClassName"), ClassName);
				
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
			if (JsonObject->TryGetArrayField(TEXT("Modifiers"), JsonPerkModifiers))
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

				// Set the modifiers into our backend struct.
				PerkBackendData.PositivePerkModifiers = PositiveModifier;
				PerkBackendData.NegativePerkModifiers = NegativeModifier;
				PerkBackendData.LegendaryPerkModifiers = LegendaryModifier;

				// Grab the cash in value.
				JsonObject->TryGetNumberField(TEXT("CashInValue"), PerkBackendData.BuyBackCP);

				// Grab the bonus cash in value (for legendary perks).
				JsonObject->TryGetNumberField(TEXT("CashInBonus"), PerkBackendData.CashInBonusCP);

				// Update the backend profile with the new perk.
				if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
				{
					PlayerState->OwnedPerksUpdated.AddDynamic(this, &USCCounselorCharacterWidget::OnPerkRollPurchasedSuccess);
					PlayerState->SERVER_RequestBackendProfileUpdate();
				}

				OnPerkRollPurchased(true, PerkBackendData);
				return;
			}
		}
	}

	// Handle Error to UI
	OnPerkRollPurchasedDone(false);
}

void USCCounselorCharacterWidget::OnPerkRollPurchasedSuccess()
{
	if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
	{
		if (PlayerState->OwnedPerksUpdated.IsBound())
		{
			PlayerState->OwnedPerksUpdated.RemoveDynamic(this, &USCCounselorCharacterWidget::OnPerkRollPurchasedSuccess);
		}
	}

	OnPerkRollPurchasedDone(true);
}

void USCCounselorCharacterWidget::SellBackPerk(FSCPerkBackendData PerkClass)
{
	USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer());
	if (USCBackendPlayer* BackendPlayer = LocalPlayer ? LocalPlayer->GetBackendPlayer<USCBackendPlayer>() : nullptr)
	{
		USCGameInstance* GI = Cast<USCGameInstance>(LocalPlayer->GetGameInstance());
		USCBackendRequestManager* BRM = GI->GetBackendRequestManager<USCBackendRequestManager>();
		FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &USCCounselorCharacterWidget::OnSellBackPerk);
		CurrentInstanceID = PerkClass.InstanceID;
		if (BRM->SellBackPerk(BackendPlayer, CurrentInstanceID, Delegate))
		{
			return;
		}
	}

	// TODO needs to send more info to the user like did we not reach the backend?
	OnSellBackPerkDone(false);
}

void USCCounselorCharacterWidget::OnSellBackPerk(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode == 200)
	{
		// We need to clear the perk if it was assigned to any loadouts and update the player loadout save.
		for (int i = 0; i < CounselorCharacterProfiles.Num(); ++i)
		{
			for (int j = 0; j < CounselorCharacterProfiles[i].Perks.Num(); ++j)
			{
				if (CounselorCharacterProfiles[i].Perks[j].InstanceID == CurrentInstanceID)
				{
					CounselorCharacterProfiles[i].Perks[j] = FSCPerkBackendData();
				}
			}
		}

		// Update the backend profile with the new perk.
		if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
		{
			PlayerState->OwnedPerksUpdated.AddDynamic(this, &USCCounselorCharacterWidget::OnSellBackPerkSuccess);
			PlayerState->SERVER_RequestBackendProfileUpdate();
		}
	}
}

void USCCounselorCharacterWidget::OnSellBackPerkSuccess()
{
	if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(GetPlayerContext().GetPlayerState()))
	{
		if (PlayerState->OwnedPerksUpdated.IsBound())
		{
			PlayerState->OwnedPerksUpdated.RemoveDynamic(this, &USCCounselorCharacterWidget::OnSellBackPerkSuccess);
		}
	}

	// Done!
	if (GetWorld()->IsPlayInEditor())
	{
		OnSellBackPerkDone(true);
		ClearDirty();
		return;
	}

	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer()))
	{
		if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
		{
			AILLPlayerController* PC = Cast<AILLPlayerController>(LocalPlayer->GetPlayerController(GetWorld()));
			Settings->CounselorCharacterProfiles = CounselorCharacterProfiles;
			Settings->ApplyPlayerSettings(PC, true);
			OnSellBackPerkDone(true);
			ClearDirty();
			return;
		}
	}

	OnSellBackPerkDone(false);
}

void USCCounselorCharacterWidget::SetCounselorCharacterProfile(TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter, TArray<FSCPerkBackendData> Perks, const FSCCounselorOutfit Outfit, const TArray<TSubclassOf<USCEmoteData>> Emotes)
{
	int AlreadyStoredCharacter = false;

	for (int i = 0; i < CounselorCharacterProfiles.Num(); ++i)
	{
		if (CounselorCharacterProfiles[i].CounselorCharacterClass == CounselorCharacter)
		{
			// check to see if this was the same data coming in or not.
			if (CounselorCharacterProfiles[i].Perks != Perks || CounselorCharacterProfiles[i].Outfit != Outfit || CounselorCharacterProfiles[i].Emotes != Emotes)
			{
				// not the same data lets update it and mark it dirty so we know to save it.
				CounselorCharacterProfiles[i].Perks = Perks;
				CounselorCharacterProfiles[i].Outfit = Outfit;
				CounselorCharacterProfiles[i].Emotes = Emotes;

				if (USCBackendInventory* BackendInventory = GetInventoryManager())
				{
					BackendInventory->bIsDirty = true;
				}
			}

			AlreadyStoredCharacter = true;
			break;
		}
	}

	if (!AlreadyStoredCharacter)
	{
		CounselorCharacterProfiles.Add(FSCCounselorCharacterProfile(CounselorCharacter, Perks, Outfit, Emotes));

		if (USCBackendInventory* BackendInventory = GetInventoryManager())
		{
			BackendInventory->bIsDirty = true;
		}
	}
}

FSCCounselorCharacterProfile USCCounselorCharacterWidget::GetCounselorCharacterProfile(TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter) const
{
	for (int i = 0; i < CounselorCharacterProfiles.Num(); ++i)
	{
		if (CounselorCharacterProfiles[i].CounselorCharacterClass == CounselorCharacter)
		{
			return CounselorCharacterProfiles[i];
		}
	}

	FSCCounselorCharacterProfile Default;
	Default.CounselorCharacterClass = CounselorCharacter;
	return Default;
}

TArray<FSCPerkBackendData> USCCounselorCharacterWidget::GetCounselorCharacterPerks(const TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter) const
{
	return GetCounselorCharacterProfile(CounselorCharacter).Perks;
}

bool USCCounselorCharacterWidget::GetCounselorPerkEquipped(const TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter, const FSCPerkBackendData PerkCompare) const
{
	for (FSCPerkBackendData PerkData : GetCounselorCharacterProfile(CounselorCharacter).Perks)
	{
		if (PerkData == PerkCompare)
			return true;
	}

	return false;
}

void USCCounselorCharacterWidget::GetCounselorCharacterClothes(const TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter, FSCCounselorOutfit& Outfit) const
{
	Outfit = GetCounselorCharacterProfile(CounselorCharacter).Outfit;
}

TArray<TSubclassOf<USCEmoteData>> USCCounselorCharacterWidget::GetCounselorCharacterEmotes(const TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter) const
{
	return GetCounselorCharacterProfile(CounselorCharacter).Emotes;
}

void USCCounselorCharacterWidget::SetActorPreviewClass(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, FTransform Offset)
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
		TSoftClassPtr<AActor> NewClass = GetBeautyPoseClass(CounselorClass);

		// We already are this class. Fuck off.
		if (ActorPreview->GetPreviewActor() && !NewClass.IsNull() && ActorPreview->GetPreviewActor()->GetClass() == NewClass.Get())
		{
			// Verify our outfit is correct
			if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
			{
				FSCCounselorCharacterProfile Profile = GetCounselorCharacterProfile(CounselorClass);
				if (Character->GetOutfit() != Profile.Outfit)
				{
					Character->SetCurrentOutfit(Profile.Outfit, true);
				}
			}

			return;
		}

		// set the new class and spawn it
		ActorPreview->PreviewClass = NewClass;
		ActorPreview->SpawnPreviewActor();

		// get the preview actor and load its clothing
		if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
		{
			// make sure the streaming distance is set high so that all materials always stream in completely
			Character->FaceLimb->StreamingDistanceMultiplier = 1000.0f;
			Character->Hair->StreamingDistanceMultiplier = 1000.0f;
			Character->CounselorOutfitComponent->StreamingDistanceMultiplier = 1000.0f;
			Character->GetMesh()->StreamingDistanceMultiplier = 1000.0f;

			Character->SetActorTransform(Offset, false, nullptr, ETeleportType::TeleportPhysics);

			FSCCounselorCharacterProfile Profile = GetCounselorCharacterProfile(CounselorClass);

			if (Profile.Outfit.ClothingClass != nullptr)
			{
				Character->SetCurrentOutfit(Profile.Outfit, true);
			}
			else
			{
				Character->RevertToDefaultOutfit();
			}
		}
	}
}

TSoftClassPtr<ASCCounselorCharacter> USCCounselorCharacterWidget::GetBeautyPoseClass_Implementation(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass)
{
	return CounselorClass;
}