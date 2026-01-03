// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorEmoteMenuWidget.h"

#include "Animation/AnimInstance.h"

#include "SCActorPreview.h"
#include "SCCharacterSelectionsSaveGame.h"
#include "SCCounselorCharacter.h"
#include "SCEmoteData.h"
#include "SCLocalPlayer.h"
#include "SCPlayerController.h"
#include "Image.h"
#include "CanvasPanelSlot.h"
#include "ILLPlayerInput.h"
#include "SCCounselorEmoteSelectWidget.h"
#include "SCSinglePlayerSaveGame.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

USCCounselorEmoteMenuWidget::USCCounselorEmoteMenuWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bBlockMenuInputAxis(true)
{
}

void USCCounselorEmoteMenuWidget::PostInitProperties()
{
	Super::PostInitProperties();

	// Load all of our emote data assets
	if (!EmoteObjectLibrary)
	{
		EmoteObjectLibrary = UObjectLibrary::CreateLibrary(USCEmoteData::StaticClass(), true, GIsEditor);
		EmoteObjectLibrary->AddToRoot();
	}

	EmoteObjectLibrary->LoadBlueprintsFromPath(TEXT("/Game/Blueprints/Emotes"));
}

void USCCounselorEmoteMenuWidget::RemoveFromParent()
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

void USCCounselorEmoteMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (FMath::Abs(PreviewRelativeRotation) >= 0.1f)
	{
		if (ActorPreview && ActorPreview->GetPreviewActor())
		{
			ActorPreview->GetPreviewActor()->AddActorLocalRotation(FRotator(0.0f, 200.0f * PreviewRelativeRotation * InDeltaTime, 0.0f));
		}
	}

	Super::NativeTick(MyGeometry, InDeltaTime);
}

FReply USCCounselorEmoteMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

FReply USCCounselorEmoteMenuWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		bPreviewMouseDown = false;

		if (EmoteSelect && EmoteSelect->IsSelectionHighlighted())
		{
			EmoteSelect->OnConfirmPressed();

			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply USCCounselorEmoteMenuWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

bool USCCounselorEmoteMenuWidget::MenuInputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	if (Key == EKeys::Gamepad_RightX)
	{
		PreviewRelativeRotation = -Delta;
	}

	return bBlockMenuInputAxis ? false : Super::MenuInputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad);
}

TArray<FSCCounselorEmoteData> USCCounselorEmoteMenuWidget::GetAvailableEmotesForCounselor(TSoftClassPtr<ASCCounselorCharacter> CounselorClass) const
{
	TArray<FSCCounselorEmoteData> AllowedEmotes;

	if (CounselorClass)
	{
		if (AILLPlayerController* PlayerController = GetOwningILLPlayer())
		{
			TArray<UClass*> AllEmoteClasses;
			EmoteObjectLibrary->GetObjects(AllEmoteClasses);
			USCSinglePlayerSaveGame* SaveGame = nullptr;

			if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(PlayerController->GetLocalPlayer()))
			{
				SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>();
			}

			for (UClass* Class : AllEmoteClasses)
			{
				// Skip transient classes
				if (Class->HasAnyFlags(RF_Transient))
					continue;

				if (USCEmoteData* Emote = Cast<USCEmoteData>(Class->GetDefaultObject()))
				{
					bool bLocked = false;
					// Check entitlements
					if (!PlayerController->DoesUserOwnEntitlement(Emote->EntitlementID))
					{
						if (bShowLockedEmotes)
						{
							bLocked = true;
						}
						else
						{
							continue;
						}
					}

					// Check Single Player unlocks
					if (!Emote->SinglePlayerEmoteUnlockID.IsNone() && SaveGame && !SaveGame->IsEmoteUnlocked(Emote->SinglePlayerEmoteUnlockID))
					{
						if (bShowLockedEmotes)
						{
							bLocked = true;
						}
						else
						{
							continue;
						}
					}

					// If the list is empty, everyone can use it
					if (Emote->AllowedCounselors.Num() <= 0)
					{
						AllowedEmotes.Add(FSCCounselorEmoteData(Emote, bLocked));
						continue;
					}

					// Check against counselor classes
					if (Emote->AllowedCounselors.Contains(CounselorClass))
					{
						AllowedEmotes.Add(FSCCounselorEmoteData(Emote, bLocked));
					}
				}
			}
		}
	}

	return AllowedEmotes;
}

void USCCounselorEmoteMenuWidget::SetEmote(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, int32 EmoteIndex, TSubclassOf<USCEmoteData> InEmoteData)
{
	if (CounselorCharacterProfiles.Num() <= 0)
		return;

	for (FSCCounselorCharacterProfile& Profile : CounselorCharacterProfiles)
	{
		if (Profile.CounselorCharacterClass == CounselorClass)
		{
			if (EmoteIndex < Profile.Emotes.Num())
				Profile.Emotes[EmoteIndex] = InEmoteData;

			break;
		}
	}
}

UTexture2D* USCCounselorEmoteMenuWidget::GetEmoteIcon(TSubclassOf<USCEmoteData> EmoteData) const
{
	if (EmoteData)
	{
		if (const USCEmoteData* const Emote = Cast<USCEmoteData>(EmoteData->GetDefaultObject()))
		{
			return Emote->EmoteIcon;
		}
	}

	return nullptr;
}

UAnimMontage* USCCounselorEmoteMenuWidget::GetEmoteMontage(TSubclassOf<USCEmoteData> EmoteData, TSoftClassPtr<ASCCounselorCharacter> CounselorClass) const
{
	if (!EmoteData || CounselorClass.IsNull())
		return nullptr;

	CounselorClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCounselorCharacter* const Counselor = Cast<ASCCounselorCharacter>(CounselorClass.Get()->GetDefaultObject()))
	{
		if (const USCEmoteData* const Emote = Cast<USCEmoteData>(EmoteData->GetDefaultObject()))
		{
			return Counselor->GetIsFemale() ? Emote->FemaleMontage : Emote->MaleMontage;
		}
	}

	return nullptr;
}

void USCCounselorEmoteMenuWidget::SetPreviewCounselorClass(TSoftClassPtr<ASCCounselorCharacter> CounselorClass)
{
	if (GetOwningPlayer() && !ActorPreview)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwningPlayer();
		SpawnParams.Instigator = GetOwningPlayer()->GetInstigator();

		PreviewClass.LoadSynchronous(); // TODO: Make Async
		ActorPreview = GetWorld()->SpawnActor<ASCActorPreview>(PreviewClass.Get(), SpawnParams);
	}

	if (ActorPreview)
	{
		TSoftClassPtr<AActor> NewClass = CounselorClass;

		// We already are this class. Fuck off.
		if (ActorPreview->GetPreviewActor() && !NewClass.IsNull() && ActorPreview->GetPreviewActor()->GetClass() == NewClass.Get())
		{
			// Verify our outfit is correct
			if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
			{
				if (FSCCounselorCharacterProfile* Profile = GetProfileData(CounselorClass))
				{
					if (Character->GetOutfit() != Profile->Outfit)
					{
						Character->SetCurrentOutfit(Profile->Outfit, true);
					}
				}
			}
			return;
		}

		// set the new class and spawn it
		ActorPreview->PreviewClass = NewClass;
		ActorPreview->SpawnPreviewActor();

		if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
		{
			// make sure the streaming distance is set high so that all materials always stream in completely
			Character->FaceLimb->StreamingDistanceMultiplier = 1000.0f;
			Character->Hair->StreamingDistanceMultiplier = 1000.0f;
			Character->CounselorOutfitComponent->StreamingDistanceMultiplier = 1000.0f;
			Character->GetMesh()->StreamingDistanceMultiplier = 1000.0f;

			// Init rotation
			Character->SetActorRelativeRotation(FRotator(0.f, 180.f, 0.f));

			// Put some clothes on
			if (FSCCounselorCharacterProfile* Profile = GetProfileData(CounselorClass))
			{
				if (Profile->Outfit.ClothingClass != nullptr)
				{
					Character->SetCurrentOutfit(Profile->Outfit, true);
				}
				else
				{
					Character->RevertToDefaultOutfit();
				}
			}

		}
	}
}

void USCCounselorEmoteMenuWidget::PlayPreviewEmote(TSubclassOf<USCEmoteData> EmoteClass)
{
	if (ActorPreview && ActorPreview->GetPreviewActor())
	{
		if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(ActorPreview->GetPreviewActor()))
		{
			if (EmoteClass)
			{
				if (const USCEmoteData* const Emote = Cast<USCEmoteData>(EmoteClass->GetDefaultObject()))
				{
					Character->PlayAnimMontage(Character->GetIsFemale() ? Emote->FemaleMontage : Emote->MaleMontage);
				}
			}
			else
			{
				Character->GetMesh()->GetAnimInstance()->Montage_Stop(0.25f);
			}
		}
	}
}

FSCCounselorCharacterProfile* USCCounselorEmoteMenuWidget::GetProfileData(TSoftClassPtr<ASCCounselorCharacter> CounselorClass)
{
	for (int32 i(0); i < CounselorCharacterProfiles.Num(); ++i)
	{
		if (CounselorCharacterProfiles[i].CounselorCharacterClass == CounselorClass)
			return &CounselorCharacterProfiles[i];
	}

	CounselorClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCounselorCharacter* const DefaultCharacter = Cast<ASCCounselorCharacter>(CounselorClass.Get()->GetDefaultObject()))
	{
		FSCCounselorCharacterProfile NewProfile;
		NewProfile.CounselorCharacterClass = CounselorClass;
		NewProfile.Outfit.ClothingClass = DefaultCharacter->GetDefaultOutfitClass();
		NewProfile.Emotes = DefaultCharacter->DefaultEmotes;

		int32 id = CounselorCharacterProfiles.Add(NewProfile);
		return &CounselorCharacterProfiles[id];
	}

	return nullptr;
}

ESlateVisibility USCCounselorEmoteMenuWidget::GetGamepadVisibility() const
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

void USCCounselorEmoteMenuWidget::OpenStorePage() const
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
			ShowParams.ProductId = TEXT("2d890456-9878-44e9-b723-93c2b8252843");
#else
			ShowParams.ProductId = TEXT("438740");
#endif
			OnlineExternalUI->ShowStoreUI(LocalPlayer->GetControllerId(), ShowParams, FOnShowStoreUIClosedDelegate());
		}
	}
}