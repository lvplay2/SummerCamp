// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorWardrobeWidget.h"

#include "Image.h"
#include "CanvasPanelSlot.h"
#include "OnlineExternalUIInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "ILLPlayerInput.h"

#include "SCActorPreview.h"
#include "SCAnimInstance.h"
#include "SCBackendInventory.h"
#include "SCBackendRequestManager.h"
#include "SCBackendPlayer.h"
#include "SCCounselorCharacter.h"
#include "SCCounselorWardrobe.h"
#include "SCGameInstance.h"
#include "SCLocalPlayer.h"
#include "SCOutfitListWidget.h"

#define OUTFIT_SET_WAIT_TIME 0.35f

USCCounselorWardrobeWidget::USCCounselorWardrobeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ActorPreview(nullptr)
	, OutfitSelectedTimer(0.0f)
{
}

void USCCounselorWardrobeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (FMath::Abs(ControllerInputDelta) >= 0.1f)
	{
		if (ActorPreview && ActorPreview->GetPreviewActor())
		{
			ActorPreview->GetPreviewActor()->AddActorLocalRotation(FRotator(0.0f, 200.0f * ControllerInputDelta * InDeltaTime, 0.0f));
		}
	}

	if (OutfitSelectedTimer > 0.0f)
		OutfitSelectedTimer -= InDeltaTime;

	Super::NativeTick(MyGeometry, InDeltaTime);
}

FReply USCCounselorWardrobeWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (PreviewImage)
		{
			FVector2D Location = Cast<UCanvasPanelSlot>(PreviewImage->Slot)->GetPosition();
			FVector2D Size = Location + PreviewImage->Brush.ImageSize;

			FVector2D MouseLoc = InMouseEvent.GetScreenSpacePosition();

			if (Location.X < MouseLoc.X && Size.X > MouseLoc.X && Location.Y < MouseLoc.Y && Size.Y > MouseLoc.Y)
			{
				bPreviewMouseDown = true;
			}

			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USCCounselorWardrobeWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		bPreviewMouseDown = false;

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply USCCounselorWardrobeWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPreviewMouseDown)
	{
		if (ActorPreview && ActorPreview->GetPreviewActor())
		{
			ActorPreview->GetPreviewActor()->AddActorLocalRotation(FRotator(0.0f, -InMouseEvent.GetCursorDelta().X, 0.0f));
		}
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

bool USCCounselorWardrobeWidget::MenuInputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	if (Key == EKeys::Gamepad_RightX)
	{
		ControllerInputDelta = -Delta;
		return true;
	}

	return Super::MenuInputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad);
}

void USCCounselorWardrobeWidget::RemoveFromParent()
{
	if (ActorPreview)
	{
		// make sure skin textures and such get cleaned up
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

USCBackendInventory* USCCounselorWardrobeWidget::GetInventoryManager() const
{
	if (!GetPlayerContext().IsValid())
		return nullptr;

	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer()))
	{
		USCGameInstance* GI = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
		USCBackendRequestManager* SCBackend = GI->GetBackendRequestManager<USCBackendRequestManager>();
		if (USCBackendPlayer* BackendPlayer = LocalPlayer->GetBackendPlayer<USCBackendPlayer>())
		{
			return BackendPlayer->GetBackendInventory();
		}
	}

	return nullptr;
}

void USCCounselorWardrobeWidget::SaveCharacterProfiles()
{
	if (GetWorld()->IsPlayInEditor())
	{
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
			ClearDirty();
		}
	}
}

void USCCounselorWardrobeWidget::LoadCharacterProfiles()
{
	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer()))
	{
		if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
		{
			CounselorCharacterProfiles = Settings->CounselorCharacterProfiles;
		}
	}
}

FSCCounselorCharacterProfile* USCCounselorWardrobeWidget::GetProfileData()
{
	for (int i = 0; i < CounselorCharacterProfiles.Num(); ++i)
	{
		if (CounselorCharacterProfiles[i].CounselorCharacterClass.Get() == CounselorClass)
			return &CounselorCharacterProfiles[i];
	}

	if (const ASCCounselorCharacter* const DefaultCharacter = Cast<ASCCounselorCharacter>(CounselorClass->GetDefaultObject()))
	{
		FSCCounselorCharacterProfile NewProfile;
		NewProfile.CounselorCharacterClass = CounselorClass;
		NewProfile.Outfit.ClothingClass = DefaultCharacter->GetDefaultOutfitClass();
		NewProfile.Emotes = DefaultCharacter->DefaultEmotes;

		int id = CounselorCharacterProfiles.Add(NewProfile);

		if (USCBackendInventory* BackendInventory = GetInventoryManager())
			BackendInventory->bIsDirty = true;

		return &CounselorCharacterProfiles[id];
	}

	return nullptr;
}

bool USCCounselorWardrobeWidget::AreSettingsDirty() const
{
	if (USCBackendInventory* BackendInventory = GetInventoryManager())
	{
		return BackendInventory->bIsDirty;
	}

	return false;
}

void USCCounselorWardrobeWidget::ClearDirty()
{
	if (USCBackendInventory* BackendInventory = GetInventoryManager())
	{
		BackendInventory->bIsDirty = false;
	}
}

void USCCounselorWardrobeWidget::SetPreviewClass(TSoftClassPtr<ASCCounselorCharacter> CharacterClass)
{
	// if preview actor isnt currently spawned, spawn it here
	if (GetOwningPlayer() && !ActorPreview)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwningPlayer();
		SpawnParams.Instigator = GetOwningPlayer()->GetInstigator();

		ActorPreview = GetWorld()->SpawnActor<ASCActorPreview>(PreviewClass, SpawnParams);
	}
	
	// set selected character class
	CounselorClass = CharacterClass;
	FSCCounselorOutfit Outfit;

	if (ActorPreview)
	{
		// We already are this class. Fuck off.
		if (ActorPreview->GetPreviewActor() && !CounselorClass.IsNull() && ActorPreview->GetPreviewActor()->GetClass() == CounselorClass.Get())
		{
			// Verify our outfit is correct
			if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
			{
				if (FSCCounselorCharacterProfile* Profile = GetProfileData())
				{
					if (Character->GetOutfit() != Profile->Outfit)
					{
						Character->SetCurrentOutfit(Profile->Outfit, true);
						Outfit = Profile->Outfit;
					}
				}
			}

			return;
		}

		// set the new class and spawn it
		ActorPreview->PreviewClass = CounselorClass;
		ActorPreview->SpawnPreviewActor();

		if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
		{
			// make sure the streaming distance is set high so that all materials always stream in completely
			Character->FaceLimb->StreamingDistanceMultiplier = 1000.0f;
			Character->Hair->StreamingDistanceMultiplier = 1000.0f;
			Character->CounselorOutfitComponent->StreamingDistanceMultiplier = 1000.0f;
			Character->GetMesh()->StreamingDistanceMultiplier = 1000.0f;

			// init preview rotation
			Character->SetActorRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

			if (FSCCounselorCharacterProfile* Profile = GetProfileData())
			{
				if (Profile->Outfit.ClothingClass != nullptr)
				{
					Character->SetCurrentOutfit(Profile->Outfit, true);
					Outfit = Profile->Outfit;
				}
				else
				{
					Character->RevertToDefaultOutfit();
					Outfit.ClothingClass = Character->GetDefaultOutfitClass();
				}
			}

			// grab name from counselor class and set it for the menu
			TArray<FString> Names;
			Character->GetCharacterName().ToString().ParseIntoArray(Names, TEXT(" "), 1);

			if (Names.Num() > 0)
			{
				PreviewClassFirstName = Names[0];

				if (Names.Num() > 1)
					PreviewClassLastName = Names[1];

				if (Names.Num() > 2)
					PreviewClassLastName += TEXT(" ") + Names[2];
			}
		}
	}

	// init outfit list widget
	if (OutfitListWidget)
	{
		OutfitListWidget->OnItemSelected.AddDynamic(this, &USCCounselorWardrobeWidget::SetPreviewOutfit);
		OutfitListWidget->InitializeList(CounselorClass, Outfit.ClothingClass, this);
	}
}

void USCCounselorWardrobeWidget::SetPreviewOutfit(TSubclassOf<USCClothingData> OutfitClass, bool bFromClick)
{
	if (OutfitSelectedTimer > 0.0f)
		return;

	FSCCounselorOutfit Outfit;
	Outfit.ClothingClass = OutfitClass;

	if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
	{
		if (GetProfileData()->Outfit.ClothingClass != OutfitClass)
			GetProfileData()->Outfit.MaterialPairs.Empty();
		
		GetProfileData()->Outfit.ClothingClass = OutfitClass;

		Outfit = GetProfileData()->Outfit;

		if (USCBackendInventory* BackendInventory = GetInventoryManager())
			BackendInventory->bIsDirty = true;

		// set new outfit on preview
		Character->SetCurrentOutfit(Outfit, true);

		OutfitSelectedTimer = OUTFIT_SET_WAIT_TIME;
	}
}

void USCCounselorWardrobeWidget::SetPreviewSwatch(int32 SlotIndex, TSubclassOf<USCClothingSlotMaterialOption> Swatch)
{
	if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
	{
		FSCCounselorOutfit Outfit;

		// attempt to replace the slot swatch
		bool bFoundSwatch = false;
		for (FSCClothingMaterialPair& ClothingPair : GetProfileData()->Outfit.MaterialPairs)
		{
			if (ClothingPair.SlotIndex == SlotIndex)
			{
				ClothingPair.SelectedSwatch = Swatch;
				bFoundSwatch = true;
			}
		}

		// if no swatch was set, add a new one
		if (!bFoundSwatch)
		{
			FSCClothingMaterialPair NewPair;
			NewPair.SelectedSwatch = Swatch;
			NewPair.SlotIndex = SlotIndex;

			GetProfileData()->Outfit.MaterialPairs.Add(NewPair);
		}

		// grab outfit to apply to preview
		Outfit = GetProfileData()->Outfit;

		// mark save dirty
		if (USCBackendInventory* BackendInventory = GetInventoryManager())
			BackendInventory->bIsDirty = true;

		// apply the new outfit to the preview character
		Character->SetCurrentOutfit(Outfit);
	}
}

ESlateVisibility USCCounselorWardrobeWidget::GetGamepadVisibility() const
{
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOwningPlayer()))
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			return Input->IsUsingGamepad() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		}
	}

	return ESlateVisibility::Collapsed;
}

FSCClothingMaterialPair USCCounselorWardrobeWidget::GetSelectedSwatchFromSave(int32 SwatchSlot) const
{
	for (int i = 0; i < CounselorCharacterProfiles.Num(); ++i)
	{
		if (CounselorCharacterProfiles[i].CounselorCharacterClass == CounselorClass)
		{
			for (FSCClothingMaterialPair Swatch : CounselorCharacterProfiles[i].Outfit.MaterialPairs)
			{
				if (Swatch.SlotIndex == SwatchSlot)
					return Swatch;
			}
		}
	}

	return FSCClothingMaterialPair();
}

void USCCounselorWardrobeWidget::OpenStorePage() const
{
	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetOwningLocalPlayer()))
	{
		IOnlineExternalUIPtr OnlineExternalUI = Online::GetExternalUIInterface();
		if (OnlineExternalUI.IsValid())
		{
			FShowStoreParams ShowParams;
#if PLATFORM_PS4
			IOnlineSubsystem* OSS = Online::GetSubsystem(GetOwningPlayer()->GetWorld());
			if (OSS->GetAppId() == TEXT("CUSA10007_00")) // FREEDOM
				ShowParams.ProductId = TEXT("UP2165-CUSA07878_00");
			else if (ensure(OSS->GetAppId() == TEXT("CUSA09960_00"))) // EURO
				ShowParams.ProductId = TEXT("EP2165-CUSA06846_00");
#elif PLATFORM_XBOXONE
			ShowParams.ProductId = TEXT("f0030100-2958-4bba-b512-799c058cac71");
#else
			ShowParams.ProductId = TEXT("438740");
#endif
			OnlineExternalUI->ShowStoreUI(LocalPlayer->GetControllerId(), ShowParams, FOnShowStoreUIClosedDelegate());
		}
	}
}

void USCCounselorWardrobeWidget::RandomizeOutfit()
{
	if (OutfitSelectedTimer > 0.0f)
		return;

	USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
	USCCounselorWardrobe* CounselorWardrobe = GameInstance ? GameInstance->GetCounselorWardrobe() : nullptr;

	FSCCounselorOutfit RandomOutfit = CounselorWardrobe->GetRandomOutfit(CounselorClass, GetOwningILLPlayer());

	if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
	{
		GetProfileData()->Outfit = RandomOutfit;

		if (USCBackendInventory* BackendInventory = GetInventoryManager())
			BackendInventory->bIsDirty = true;

		// set new outfit on preview
		Character->SetCurrentOutfit(RandomOutfit, true);
	}

	// update outfit selection buttons
	if (OutfitListWidget)
		OutfitListWidget->SelectByOutfitClass(RandomOutfit.ClothingClass);

	OutfitSelectedTimer = OUTFIT_SET_WAIT_TIME;
}