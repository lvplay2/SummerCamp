// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGVCCharacter.h"
#include "SCGVCItem.h"
#include "SCGVCPlayerController.h"
#include "GenericPlatformMath.h"
#include "SCGVCHUD.h"
#include "OnlineKeyValuePair.h"
//#include "SCGVCGreenScreen.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCVCCharacter"

//debug log
#define VCprint(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::White,text)

const FString	ASCGVCCharacter::CorrectDebugCode = FString("UUDDLRLRBA");
const FName	ASCGVCCharacter::VC_ACHIEVEMENT_NAME = FName("ACH_VIRTUALCABIN_COMPLETE");

// Sets default values
ASCGVCCharacter::ASCGVCCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	m_FirstPersonCameraComponent = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FirstPersonCamera"));
	m_FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, BaseEyeHeight);
	m_FirstPersonCameraComponent->bUsePawnControlRotation = true;
	m_FirstPersonCameraComponent->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);

	m_InteractibleWidgetManager = ObjectInitializer.CreateDefaultSubobject<USCGVCInteractibleWidgetManager>(this, TEXT("InteractibleWidgetManager"));
}

// Called to bind functionality to input
void ASCGVCCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAxis("VC_MoveForward", this, &ASCGVCCharacter::MoveForward);
	InputComponent->BindAxis("VC_MoveHorizontal", this, &ASCGVCCharacter::MoveHorizontal);

	InputComponent->BindAxis("VC_LookHorizontal", this, &ASCGVCCharacter::AddControllerYawInput);
	InputComponent->BindAxis("VC_LookVertical", this, &ASCGVCCharacter::AddControllerPitchInput);

	InputComponent->BindAction("VC_Crouch", IE_Pressed, this, &ASCGVCCharacter::ToggleCrouch);
	//InputComponent->BindAction("VC_Crouch", IE_Released, this, &ASCGVCCharacter::OnVCStopCrouch);

	InputComponent->BindAction("VC_DoAction", IE_Pressed, this, &ASCGVCCharacter::PerformAction);
	InputComponent->BindAction("VC_CancelAction", IE_Pressed, this, &ASCGVCCharacter::CancelAction);

	InputComponent->BindAction("VC_InventoryUp", IE_Pressed, this, &ASCGVCCharacter::DoInventoryUp);
	InputComponent->BindAction("VC_InventoryDown", IE_Pressed, this, &ASCGVCCharacter::DoInventoryDown);
	InputComponent->BindAction("VC_InventoryLeft", IE_Pressed, this, &ASCGVCCharacter::DoInventoryLeft);
	InputComponent->BindAction("VC_InventoryRight", IE_Pressed, this, &ASCGVCCharacter::DoInventoryRight);
	InputComponent->BindAction("VC_Pause", IE_Released, this, &ASCGVCCharacter::OnPause);

	InputComponent->BindAction("VC_DebugCodeA", IE_Pressed, this, &ASCGVCCharacter::DoDebugCodeInput<'A'>);
	InputComponent->BindAction("VC_DebugCodeB", IE_Pressed, this, &ASCGVCCharacter::DoDebugCodeInput<'B'>);
	InputComponent->BindAction("VC_DebugCodeUp", IE_Pressed, this, &ASCGVCCharacter::DoDebugCodeInput<'U'>);
	InputComponent->BindAction("VC_DebugCodeDown", IE_Pressed, this, &ASCGVCCharacter::DoDebugCodeInput<'D'>);
	InputComponent->BindAction("VC_DebugCodeLeft", IE_Pressed, this, &ASCGVCCharacter::DoDebugCodeInput<'L'>);
	InputComponent->BindAction("VC_DebugCodeRight", IE_Pressed, this, &ASCGVCCharacter::DoDebugCodeInput<'R'>);

	FInputActionBinding InventoryBinding;
	//to bind it to execute on pause state
	InventoryBinding.bExecuteWhenPaused = true;
	InventoryBinding.ActionDelegate.BindDelegate(this, FName("ToggleInventory"));
	InventoryBinding.ActionName = FName("VC_InventoryToggle");
	InventoryBinding.KeyEvent = IE_Pressed;

	FInputActionBinding DescrTextBinding;
	//to bind it to execute on pause state
	DescrTextBinding.bExecuteWhenPaused = true;
	DescrTextBinding.ActionDelegate.BindDelegate(this, FName("ToggleDescription"));
	DescrTextBinding.ActionName = FName("VC_DescriptionToggle");
	DescrTextBinding.KeyEvent = IE_Released;

	InputComponent->AddActionBinding(InventoryBinding);
	InputComponent->AddActionBinding(DescrTextBinding);
}

void ASCGVCCharacter::OnReceivedPlayerState()
{
	Super::OnReceivedPlayerState();

	if (ASCPlayerState* PS = GetWorld() ? GetPlayerState() : nullptr)
	{
#if PLATFORM_XBOXONE
		FString FullPresenseText = TEXT("VirtualCabin");
		PS->UpdateRichPresence(FVariantData(FullPresenseText));
#else
		FText FullPresenseText = FText(LOCTEXT("RichPresenseInGameVirtualCabin", "Visiting the Virtual Cabin"));
		PS->UpdateRichPresence(FVariantData(FullPresenseText.ToString()));
#endif
	}
}

// Called when the game starts or when spawned
void ASCGVCCharacter::BeginPlay()
{
	Super::BeginPlay();
	m_FirstPersonCameraComponent->bUsePawnControlRotation = true;

	m_Inventory.SetNum(VC_MAX_INVENTORY_ITEMS);
	m_InventoryLength = 0;
	SetControlState(EVCPlayerCharacterControlState::EVCPC_Walk);
	m_curDebugCode = FString("");
}

// Called every frame
void ASCGVCCharacter::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_ExamineItem:
		break;
	case EVCPlayerCharacterControlState::EVCPC_Walk:
	{
		m_ExaminedItem = nullptr;

		FHitResult OutHit;
		bool LookingAtAnInteractible = false;
		if ( TraceFromCurrentCamera(OutHit) )
		{
			if (OutHit.Actor.Get() != nullptr && OutHit.Actor.Get()->IsA(ASCGVCInteractible::StaticClass()))
			{
				ASCGVCInteractible* currLookedAtInteractible = Cast<ASCGVCInteractible>(OutHit.Actor.Get());

				LookingAtAnInteractible = true;
				if (m_LookedAtInteractible != currLookedAtInteractible)
				{
					if (m_LookedAtInteractible != nullptr)
					{
						m_LookedAtInteractible->ToggleLookedAtEffect(false);
					}
					
					m_LookedAtInteractible = currLookedAtInteractible;
					m_LookedAtInteractible->ToggleLookedAtEffect(true);
				}
			}
		}

		if (!LookingAtAnInteractible && m_LookedAtInteractible != nullptr)
		{
			m_LookedAtInteractible->ToggleLookedAtEffect(false);
			m_LookedAtInteractible = nullptr;
		}

		OnTryFootstepSound(DeltaTime);
	}
	break;
	}
}

ASCGVCPlayerState* ASCGVCCharacter::GetPlayerState() const
{
	return Cast<ASCGVCPlayerState>(PlayerState);
}

void ASCGVCCharacter::MoveForward(float Amt)
{
	if (FMath::Abs(Amt) > 0)
	{
		switch (m_ControlState)
		{
		case EVCPlayerCharacterControlState::EVCPC_Walk:
		{
			FRotator PlayerRotation = this->GetActorRotation();
			PlayerRotation.Pitch = 0;
			if (m_WalkSpeed > 1)
				m_WalkSpeed = 1;
			AddMovementInput(PlayerRotation.Vector(), Amt * m_WalkSpeed);
		}
		break;
		default:
			break;
		}
	}
}

void ASCGVCCharacter::MoveHorizontal(float Amt)
{
	if (Controller != nullptr && FMath::Abs(Amt) > 0)
	{
		switch (m_ControlState)
		{
		case EVCPlayerCharacterControlState::EVCPC_Walk:
		{
			FRotator PlayerRotation = Controller->GetControlRotation();
			PlayerRotation.Pitch = 0;
			FVector RightVector = FRotationMatrix(PlayerRotation).GetScaledAxis(EAxis::Y);
			if (m_WalkSpeed > 1)
				m_WalkSpeed = 1;
			AddMovementInput(RightVector, Amt * m_WalkSpeed);
		}
		break;
		default:
			break;
		}
	}
}

void ASCGVCCharacter::ToggleCrouch()
{
	if (!b_isCrouching)
	{
		OnVCStartCrouch();
	}
	else
	{
		OnVCStopCrouch();
	}
}

void ASCGVCCharacter::OnVCStartCrouch_Implementation ()
{
	VCprint("onstartcrouch from C++");
	if (m_ControlState == EVCPlayerCharacterControlState::EVCPC_Walk)
	{
		Crouch();
	}
}

void ASCGVCCharacter::OnVCStopCrouch_Implementation()
{
	VCprint("onstopcrouch from C++");
	if (m_ControlState == EVCPlayerCharacterControlState::EVCPC_Walk)
	{
		UnCrouch();
	}
}

void ASCGVCCharacter::OnTryFootstepSound_Implementation(float DeltaTime)
{
	VCprint("ontryfootstepsound from C++");
}

void ASCGVCCharacter::AddControllerPitchInput(float Amt)
{
	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_Walk:
	case EVCPlayerCharacterControlState::EVCPC_CameraOnly:
		Super::AddControllerPitchInput(Amt * m_LookSpeed);
		break;
	case EVCPlayerCharacterControlState::EVCPC_ExamineItem:
		if (FMath::Abs(Amt) > m_RotationDeadZone)
			RotateExaminedItemPitch(Amt * -m_PitchSpeed);
		break;
	case EVCPlayerCharacterControlState::EVCPC_InInventory:
		if (FMath::Abs(Amt) > m_RotationDeadZone)
			RotateEquippedItemPitch(Amt * -m_PitchSpeed);
		break;
	default:
		break;
	}
}

void ASCGVCCharacter::AddControllerYawInput(float Amt)
{
	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_Walk:
	case EVCPlayerCharacterControlState::EVCPC_CameraOnly:
		Super::AddControllerYawInput(Amt * m_LookSpeed);
		break;
	case EVCPlayerCharacterControlState::EVCPC_ExamineItem:
		if (FMath::Abs(Amt) > m_RotationDeadZone)
			RotateExaminedItemYaw(Amt * -m_YawSpeed);
		break;
	case EVCPlayerCharacterControlState::EVCPC_InInventory:
		if (FMath::Abs(Amt) > m_RotationDeadZone)
			RotateEquippedItemYaw(Amt * -m_YawSpeed);
		break;
	default:
		break;
	}
}

void ASCGVCCharacter::RotateExaminedItemYaw(float Amt)
{
	if (m_ExaminedItem != nullptr && m_FirstPersonCameraComponent != nullptr)
	{
		m_ExaminedItem->GetRotationPivot()->AddWorldRotation(FQuat(m_FirstPersonCameraComponent->GetUpVector(), Amt));
	}
}
 
void ASCGVCCharacter::RotateExaminedItemPitch(float Amt)
{
	if (m_ExaminedItem != nullptr && m_FirstPersonCameraComponent != nullptr)
	{
		m_ExaminedItem->GetRotationPivot()->AddWorldRotation(FQuat(m_FirstPersonCameraComponent->GetRightVector(), Amt));
	}
}

void ASCGVCCharacter::RotateEquippedItemYaw(float Amt)
{
	if (m_EquippedItem != nullptr && m_FirstPersonCameraComponent != nullptr)
	{
		m_EquippedItem->GetRotationPivot()->AddWorldRotation(FQuat(m_FirstPersonCameraComponent->GetUpVector(), Amt));
	}
}

void ASCGVCCharacter::RotateEquippedItemPitch(float Amt)
{
	if (m_EquippedItem != nullptr && m_FirstPersonCameraComponent != nullptr)
	{
		m_EquippedItem->GetRotationPivot()->AddWorldRotation(FQuat(m_FirstPersonCameraComponent->GetRightVector(), Amt));
	}
}

void ASCGVCCharacter::PerformAction()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr)
		return;

	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_Walk:
		{
			FHitResult OutHit;
			if (TraceFromCurrentCamera(OutHit))
			{
				if (OutHit.Actor.Get() != nullptr && OutHit.Actor.Get()->IsA(ASCGVCInteractible::StaticClass()))
				{
					//is this an item?
					if (OutHit.Actor.Get()->IsA(ASCGVCItem::StaticClass()))
					{
						if (m_ExaminedItem == nullptr)
						{
							m_ExaminedItem = Cast<ASCGVCItem>(OutHit.Actor.Get());
							SetControlState(EVCPlayerCharacterControlState::EVCPC_ExamineItem);
							
							m_ExaminedItem->ToggleCollision(false);
							m_ExaminedItem->ToggleLookedAtEffect(false, true);

							m_ExaminedItem->AttachToComponent(m_FirstPersonCameraComponent, FAttachmentTransformRules::KeepWorldTransform);
							
							m_ExaminedItem->SetActorRelativeLocation(m_ExaminedItem->m_ExaminePosition);
							m_ExaminedItem->SetActorRelativeRotation(FQuat(m_ExaminedItem->m_ExamineRotation));

							m_ExaminedItem->OnStartInteracting(this, PC);
							m_ExaminedItem->SetToForwardRendering();
							m_ExaminedItem->SetItemToExamineRotationPivot();
							m_ExaminedItem->OnInvokeEnteredExamineModeCue();
							if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
							{
								HUD->SetUseAndBackInfoButtonsText(m_ExaminedItem->m_UseButtonInfoText, m_ExaminedItem->m_BackButtonInfoText);
								if (m_ExaminedItem->m_IsInventoryItem && m_InventoryEnabled)
								{
									HUD->SetUseAndBackInfoButtonsText(m_ExaminedItem->m_UseButtonInfoText, m_ExaminedItem->m_BackButtonInfoText);
									HUD->ShowButtonInfo(true, true, true, true);
								}
								else
									HUD->ShowButtonInfo(false, true, true, true);
							}
							ToggleDescription();
						}
					}
					//if an interactible
					else
					{
						m_ExaminedInteractible = Cast<ASCGVCInteractible>(OutHit.Actor.Get());
						if (m_ExaminedInteractible != nullptr && m_ExaminedInteractible->m_isExaminable)
						{

							// if interactible needs to open inventory
							if (m_ExaminedInteractible->b_ItemUsableOnInteractible && GetInventoryLength() > 0)
							{
								m_LookedAtInteractible = m_ExaminedInteractible;
								m_ExaminedInteractible->ToggleLookedAtEffect(false, true);
								ToggleInventory();
								if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
								{
									HUD->ShowButtonInfo(true, true, true, true);
								}
							}
							else
							{
								SetControlState(EVCPlayerCharacterControlState::EVCPC_InteractingWithObject);	
								m_ExaminedInteractible->ToggleCollision(false);
								m_ExaminedInteractible->ToggleLookedAtEffect(false, true);
								
								if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
								{
									HUD->SetUseAndBackInfoButtonsText(m_ExaminedInteractible->m_UseButtonInfoText, m_ExaminedInteractible->m_BackButtonInfoText);
									HUD->ShowButtonInfo(m_ExaminedInteractible->b_isUsableWhileExamining, false, false, m_ExaminedInteractible->b_ShowBackButtonInfo);
								}

								m_ExaminedInteractible->OnStartInteracting(this, PC);
								ToggleDescription();
							}
						}
					}
				}
			}
		}
		break;
	case EVCPlayerCharacterControlState::EVCPC_ExamineItem:
		if (m_ExaminedItem != nullptr)
		{
			m_ExaminedItem->OnActionPressedWhenExamined(this, PC);
			if (m_InventoryEnabled == false)
				return;

			if (m_ExaminedItem->m_IsInventoryItem)
			{
				PickUpItem(m_ExaminedItem);
				
				m_ExaminedItem = nullptr;
				SetControlState(EVCPlayerCharacterControlState::EVCPC_Walk);
				
				if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
				{
					if (HUD->IsDescriptionVisible())
						HUD->HideDescriptionText();

					HUD->HideButtoninfo();
				}
			}
		}
		break;
	case EVCPlayerCharacterControlState::EVCPC_InteractingWithObject:
		if (m_ExaminedInteractible != nullptr)
			m_ExaminedInteractible->OnActionPressedWhenExamined(this, PC);
		/*if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			HUD->HideButtoninfo();*/
		break;
	case EVCPlayerCharacterControlState::EVCPC_InInventory:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			HUD->HandleInventoryAction();
		break;
	case EVCPlayerCharacterControlState::EVCPC_InDebugMenu:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			HUD->HandleDebugMenuAction();
		break;
	default:
		break;
	}
}

void ASCGVCCharacter::CancelAction()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr)
		return;

	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_ExamineItem:
		SetControlState(EVCPlayerCharacterControlState::EVCPC_Walk);
		
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
		{
			if (HUD->IsDescriptionVisible())
				HUD->HideDescriptionText(true);
		}
		if (m_ExaminedItem != nullptr)
		{
			m_ExaminedItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			

			m_ExaminedItem->SetToWorldRendering();
			m_ExaminedItem->SetItemToNormalLocationAndPivot();
			m_ExaminedItem->SnapToAnchor();
			m_ExaminedItem->ToggleCollision(true);
			m_ExaminedItem->ToggleLookedAtEffect(true, true);
			m_ExaminedItem->OnStopInteracting(this, PC);
			m_ExaminedItem = nullptr;
			if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
				HUD->HideButtoninfo();
		}
		break;
	case EVCPlayerCharacterControlState::EVCPC_InteractingWithObject:
		SetControlState(EVCPlayerCharacterControlState::EVCPC_Walk);
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
		{
			if (HUD->IsDescriptionVisible())
				HUD->HideDescriptionText();
		}

		if (m_ExaminedInteractible != nullptr)
		{
			m_ExaminedInteractible->OnStopInteracting(this, PC);
			m_ExaminedInteractible->ToggleCollision(true);
			m_ExaminedInteractible->ToggleLookedAtEffect(true);
			m_ExaminedInteractible = nullptr;
		}
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			HUD->HideButtoninfo();
		break;
	case EVCPlayerCharacterControlState::EVCPC_Walk:
		break;
	case EVCPlayerCharacterControlState::EVCPC_InInventory:
		ToggleInventory();
	default:
		break;
	}
}

void ASCGVCCharacter::PickUpItem(ASCGVCItem* NewItem)
{
	if (this->OnPickUpItem.IsBound())
		this->OnPickUpItem.Broadcast(NewItem->GetItemID());
	NewItem->OnPickedUp();
	NewItem->OnInvokePickUpCue();
	NewItem->SetInteractibleInWorld(false);
	if (NewItem->OnItemPickedDelegate.IsBound())
		NewItem->OnItemPickedDelegate.Broadcast();

	NewItem->ToggleCollision(false);
	NewItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	NewItem->SetItemToNormalLocationAndPivot();

	int32 firstAvailableSlot = m_Inventory.Find(nullptr);

	if (firstAvailableSlot != INDEX_NONE)
	{
		m_Inventory[firstAvailableSlot] = NewItem;
		NewItem->ToggleHidden(true);

		m_InventoryLength++;
	}
	else
	{
		VCprint("Inventory is full");
	}
}

void ASCGVCCharacter::DropItem(ASCGVCItem* DroppedItem)
{
	if (DroppedItem != nullptr)
	{
		int32 index;
		if (m_Inventory.Find(DroppedItem, index))
		{
			m_Inventory[index] = nullptr;

			DroppedItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			DroppedItem->SetInteractibleInWorld(true);
			DroppedItem->SetItemToNormalLocationAndPivot();
			DroppedItem->OnDropped();
			DroppedItem->OnInvokeDropCue();
			if (DroppedItem->OnItemDroppedDelegate.IsBound())
				DroppedItem->OnItemDroppedDelegate.Broadcast();

			m_InventoryLength--;
		}
	}
}

//use VC_InventoryUp and VC_InventoryDown for the Debug menu as well
void ASCGVCCharacter::DoInventoryUp()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr)
		return;

	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_InInventory:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			HUD->HandleInventoryUp();
		break;
	case EVCPlayerCharacterControlState::EVCPC_InDebugMenu:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			HUD->HandleDebugMenuUp();
		break;
	case EVCPlayerCharacterControlState::EVCPC_Walk:
		break;
	default:
		break;
	}
}

void ASCGVCCharacter::DoInventoryDown()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr)
		return;
	
	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_InInventory:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			HUD->HandleInventoryDown();
		break;
	case EVCPlayerCharacterControlState::EVCPC_InDebugMenu:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			HUD->HandleDebugMenuDown();
		break;
	case EVCPlayerCharacterControlState::EVCPC_Walk:
		break;
	default:
		break;
	}
}

void ASCGVCCharacter::DoInventoryLeft()
{
	if (m_ControlState == EVCPlayerCharacterControlState::EVCPC_InInventory)
	{
		ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
		if (PC == nullptr)
			return;

		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			HUD->HandleInventoryLeft();
	}
}

void ASCGVCCharacter::DoInventoryRight()
{
	if (m_ControlState == EVCPlayerCharacterControlState::EVCPC_InInventory)
	{
		ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
		if (PC == nullptr)
			return;

		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			HUD->HandleInventoryRight();
	}
}
 
bool ASCGVCCharacter::HasItemInInventory(const FString& itemID)
{
	for (int i = 0; i < m_Inventory.Num(); i++)
	{
		if (m_Inventory[i] != nullptr)
		{
			if (m_Inventory[i]->GetItemID() == itemID)
				return true;
		}
	}
	return false;
}

TArray<ASCGVCItem*> ASCGVCCharacter::GetInventory()
{
	return m_Inventory;
}

void ASCGVCCharacter::ToggleInventory()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr || m_InventoryEnabled == false)
		return;
	
	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_Walk:
		if (GetInventoryLength() > 0)
		{
			if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
			{
				HUD->ResetUseAndBackInfoButtonsText();
			}
			if (m_LookedAtInteractible != nullptr)
			{
				m_LookedAtInteractible->ToggleLookedAtEffect(false, true);
			}

			SetControlState(EVCPlayerCharacterControlState::EVCPC_InInventory);
			PC->ShowInventory();
		}
		break;
	case EVCPlayerCharacterControlState::EVCPC_ExamineItem:
		break;
	case EVCPlayerCharacterControlState::EVCPC_InteractingWithObject:
		break;
	case EVCPlayerCharacterControlState::EVCPC_InInventory:
		SetControlState(EVCPlayerCharacterControlState::EVCPC_Walk);
		if (m_EquippedItem != nullptr)
		{
			m_EquippedItem->ToggleHidden(true);
			m_EquippedItem->SetToWorldRendering();
			m_EquippedItem->SetItemToNormalLocationAndPivot();
			m_EquippedItem = nullptr;
		}
		PC->HideInventory();
		if (m_LookedAtInteractible != nullptr)
		{
			m_LookedAtInteractible->ToggleLookedAtEffect(true, false);
		}
		break;
	}
}

void ASCGVCCharacter::ToggleDebugMenu()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr || m_DebugMenuEnabled == false)
		return;

	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_Walk:
		SetControlState(EVCPlayerCharacterControlState::EVCPC_InDebugMenu);
		PC->ShowDebugMenu();
		break;
	case EVCPlayerCharacterControlState::EVCPC_InDebugMenu:
		SetControlState(EVCPlayerCharacterControlState::EVCPC_Walk);
		PC->HideDebugMenu();
		break;
	default:
		break;
	}
}

void ASCGVCCharacter::OnPause()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr)
		return;

	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_Walk:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
		{
			//SetControlState(EVCPlayerCharacterControlState::EVCPC_InPauseMenu);
			HUD->ShowPauseMenu();
		}
		break;
	case EVCPlayerCharacterControlState::EVCPC_InPauseMenu:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
		{
			//SetControlState(EVCPlayerCharacterControlState::EVCPC_Walk);
			HUD->HidePauseMenu();
		}
		break;
	default:
		break;
	}
}

void ASCGVCCharacter::ToggleDescription()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr)
		return;

	switch (m_ControlState)
	{
	case EVCPlayerCharacterControlState::EVCPC_Walk:
		break;
	case EVCPlayerCharacterControlState::EVCPC_ExamineItem:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
		{
			if (m_ExaminedItem != nullptr && HUD->IsDescriptionVisible() == false)
				HUD->ShowDescriptionText(m_ExaminedItem->GetName().ToString(), m_ExaminedItem->m_DescriptionText.ToString());
			else if (HUD->IsDescriptionVisible())
				HUD->HideDescriptionText();
		}
		break;
	case EVCPlayerCharacterControlState::EVCPC_InteractingWithObject:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
		{
			if (m_ExaminedInteractible != nullptr && HUD->IsDescriptionVisible() == false)
				HUD->ShowDescriptionText(m_ExaminedInteractible->GetName().ToString(), m_ExaminedInteractible->m_DescriptionText.ToString());
			else if (HUD->IsDescriptionVisible())
				HUD->HideDescriptionText();
		}
		break;
	case EVCPlayerCharacterControlState::EVCPC_InInventory:
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
		{
			if (m_EquippedItem != nullptr && HUD->IsDescriptionVisible() == false)
				HUD->ShowDescriptionText(m_EquippedItem->GetName().ToString(), m_EquippedItem->m_DescriptionText.ToString());
			else if (HUD->IsDescriptionVisible())
				HUD->HideDescriptionText();
		}
		break;
	default:
		break;
	}
}

void ASCGVCCharacter::SetEquippedItem(ASCGVCItem* newItem)
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr)
		return;

	//hide the previous equipped item
	if (m_EquippedItem != nullptr)
	{
		m_EquippedItem->ToggleHidden(true);

		//if not green screen
		m_EquippedItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		
		m_EquippedItem->SetToWorldRendering();
		m_EquippedItem->SetItemToNormalLocationAndPivot();
		m_EquippedItem = nullptr;
	}

	m_EquippedItem = newItem;
	
	if (m_EquippedItem != nullptr)
	{
		if (m_ControlState == EVCPlayerCharacterControlState::EVCPC_InInventory)
		{
			m_EquippedItem->ToggleCollision(false);
			m_EquippedItem->AttachToComponent(m_FirstPersonCameraComponent, FAttachmentTransformRules::KeepWorldTransform);
			m_EquippedItem->SetActorRelativeLocation(m_EquippedItem->m_ExaminePosition);
			m_EquippedItem->SetActorRelativeRotation(FQuat(m_EquippedItem->m_ExamineRotation));
			m_EquippedItem->ToggleLookedAtEffect(false, true);
			m_EquippedItem->ToggleHidden(false);
			m_EquippedItem->SetToForwardRendering();
			m_EquippedItem->SetItemToExamineRotationPivot();
		}
	}
}

void ASCGVCCharacter::SendActionOnEquippedItem()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());

	if (m_EquippedItem != nullptr && m_LookedAtInteractible != nullptr && PC != nullptr)
	{
		PC->HideInventory();

		m_EquippedItem->ToggleHidden(true);
		m_EquippedItem->SetToWorldRendering();
		m_EquippedItem->SetItemToNormalLocationAndPivot();
		
		m_LookedAtInteractible->OnItemUsed(this, Cast<ASCGVCPlayerController>(GetController()), m_EquippedItem);
		
		SetControlState(EVCPlayerCharacterControlState::EVCPC_Walk);
		m_EquippedItem = nullptr;
	}
}

void ASCGVCCharacter::ClosedPauseMenuCallback()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr)
		return;
	if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
	{
		SetControlState(EVCPlayerCharacterControlState::EVCPC_Walk);
		HUD->HidePauseMenu();
	}
}

void ASCGVCCharacter::SetControlState(EVCPlayerCharacterControlState newstate)
{
	m_ControlState = newstate;
	//turn on the inventory button info if walk or ininventory, or turn it off if else
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());

	if (PC != nullptr)
	{
		if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
		{
			if (m_ControlState == EVCPlayerCharacterControlState::EVCPC_Walk)
			{
				//OnVCStopCrouch();
				if (m_InventoryEnabled  && GetInventoryLength() > 0)
				{
					HUD->ShowInventoryButtonInfo();
				}
				else
				{
					HUD->HideInventoryButtoninfo();
				}
			}
			else
			{
				HUD->HideInventoryButtoninfo();
			}
		}
	}

	if (this->OnChangedState.IsBound())
		this->OnChangedState.Broadcast();
}

void ASCGVCCharacter::DoDebugCodeInput(TCHAR charentered)
{
	CheckIfDebugCodeString(charentered);
}

void ASCGVCCharacter::CheckIfDebugCodeString(TCHAR newchar)
{
	if (m_DebugMenuEnabled == false)
		return;

	m_curDebugCode.AppendChar(newchar);

	//VCprint(m_curDebugCode);
	if (m_curDebugCode.Len() >= CorrectDebugCode.Len())
	{
		if (m_curDebugCode.Equals(CorrectDebugCode, ESearchCase::IgnoreCase))
		{
			ToggleDebugMenu();
		}
		m_curDebugCode = FString("");
	}
	else
	{
		for (int i = 0; i < m_curDebugCode.Len(); i++)
		{
			if (m_curDebugCode[i] != CorrectDebugCode[i])
			{
				m_curDebugCode = FString("");
				break;
			}
		}
	}
}

bool ASCGVCCharacter::CanInteractAtAll() const
{
	return (m_ControlState == EVCPlayerCharacterControlState::EVCPC_Walk);
}

void ASCGVCCharacter::SetVCAchievementProgress()
{
	VCprint("In Code, SetAchievementProgress");
	if (ASCGVCPlayerState* PS = GetPlayerState())
		PS->UpdateAchievement(VC_ACHIEVEMENT_NAME, 100.0f);
}

void ASCGVCCharacter::PlayUISaveAnimation()
{
	ASCGVCPlayerController* PC = Cast<ASCGVCPlayerController>(GetController());
	if (PC == nullptr)
		return;

	if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(PC->GetHUD()))
	{
		HUD->PlayUISaveAnimation();
	}
}

bool ASCGVCCharacter::TraceFromCurrentCamera(FHitResult& OutHitResult)
{
	// Get the camera transform
	FVector CameraLoc;
	FRotator CameraRot;
	GetActorEyesViewPoint(CameraLoc, CameraRot);
	UWorld* const World = GetWorld();
	if (World)
	{
		const FVector TraceStart = CameraLoc;
		const FVector TraceEnd = TraceStart + CameraRot.Vector() * m_TraceReach;
		ECollisionChannel TraceChannel = ECC_WorldDynamic;
		FCollisionQueryParams TraceParams(NAME_None, false, this);

		if (World->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, TraceChannel, TraceParams))
			return true;
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
