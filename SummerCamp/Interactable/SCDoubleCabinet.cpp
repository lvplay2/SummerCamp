// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCDoubleCabinet.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "SCActorSpawnerComponent.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCGameMode.h"
#include "SCInteractComponent.h"
#include "SCInteractableManagerComponent.h"
#include "SCItem.h"
#include "SCPamelaTape.h"
#include "SCPamelaTapeDataAsset.h"
#include "SCPlayerState.h"
#include "SCSpecialMoveComponent.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCTommyTape.h"
#include "SCTommyTapeDataAsset.h"

/*
				WARNING: THIS CLASS USES NET DORMANCY
	This means that if you need to update any replicated variables,
	logic, RPCs or other networking whatnot, you NEED to update the
	logic handling for dormancy.
*/

ASCDoubleCabinet::ASCDoubleCabinet(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	AnimatedMesh2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnimatedMesh2"));
	AnimatedMesh2->SetIsReplicated(true);
	AnimatedMesh2->bGenerateOverlapEvents = false;
	AnimatedMesh2->SetupAttachment(BaseMesh);

	FollowLocation2 = CreateDefaultSubobject<USceneComponent>(TEXT("FollowLocation2"));
	FollowLocation2->SetupAttachment(AnimatedMesh2);

	ItemAttachLocation2 = CreateDefaultSubobject<USceneComponent>(TEXT("ItemAttachLocation2"));
	ItemAttachLocation2->SetIsReplicated(true);
	ItemAttachLocation2->SetupAttachment(BaseMesh);

	ItemSpawner2 = CreateDefaultSubobject<USCActorSpawnerComponent>(TEXT("ItemSpawner2"));
	ItemSpawner2->SetupAttachment(ItemAttachLocation2);

	InteractComponent2 = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable2"));
	InteractComponent2->InteractMethods = (int32)EILLInteractMethod::Press | (int32)EILLInteractMethod::Hold;
	InteractComponent2->HoldTimeLimit = 1.f;
	InteractComponent2->bCanHighlight = true;
	InteractComponent2->HighlightDistance = 300.f;
	InteractComponent2->SetupAttachment(BaseMesh);

	OpenCloseCabinetDriver2 = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("OpenCloseCabinetDriver2"));
	OpenCloseCabinetDriver2->SetNotifyName(TEXT("OpenCloseCabinet"));

	UseCabinet_SpecialMoveHandler2 = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("UseCabinet_SpecialMoveHandler2"));
	UseCabinet_SpecialMoveHandler2->bTakeCameraControl = true;
	UseCabinet_SpecialMoveHandler2->SetupAttachment(BaseMesh);
}

void ASCDoubleCabinet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCDoubleCabinet, ContainedItem2);
	DOREPLIFETIME(ASCDoubleCabinet, bIsOpen2);
}

void ASCDoubleCabinet::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent2)
	{
		InteractComponent2->OnInteract.AddDynamic(this, &ASCDoubleCabinet::OnInteract2);
		InteractComponent2->OnCanInteractWith.BindDynamic(this, &ASCDoubleCabinet::CanInteractWith2);
		InteractComponent2->OnHoldStateChanged.AddDynamic(this, &ASCDoubleCabinet::OnHoldStateChanged2);
	}

	if (OpenCloseCabinetDriver2)
	{
		FAnimNotifyEventDelegate Begin;
		Begin.BindDynamic(this, &ASCDoubleCabinet::OpenCloseCabinetDriverBegin2);
		FAnimNotifyEventDelegate End;
		End.BindDynamic(this, &ASCDoubleCabinet::OpenCloseCabinetDriverEnd2);
		OpenCloseCabinetDriver2->InitializeBeginEnd(Begin, End);
	}

	if (UseCabinet_SpecialMoveHandler2)
	{
		UseCabinet_SpecialMoveHandler2->SpecialStarted.AddDynamic(this, &ASCDoubleCabinet::UseCabinetStarted2);
		UseCabinet_SpecialMoveHandler2->SpecialAborted.AddDynamic(this, &ASCDoubleCabinet::UseCabinetAborted2);
		UseCabinet_SpecialMoveHandler2->SpecialComplete.AddDynamic(this, &ASCDoubleCabinet::OpenCloseCabinetCompleted2);
	}

	// Default to closed so we can quickly see what transforms are off
	AnimatedMesh2->SetWorldTransform(FTransform::Identity);
	AnimatedMesh2->SetRelativeTransform(CloseTransform2);
}

void ASCDoubleCabinet::BeginPlay()
{
	Super::BeginPlay();

	if (bItemShouldFollowAnimatedMesh)
	{
		ItemAttachLocation2->SetWorldTransform(FollowLocation2->GetComponentTransform());
	}
}

void ASCDoubleCabinet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentInteractor2)
	{
		if (USkeletalMeshComponent* CharacterMesh = CurrentInteractor2->GetMesh())
		{
			FAnimMontageInstance* Montage = CharacterMesh->GetAnimInstance()->GetActiveMontageInstance();

			// Don't move the drawer if we're blending the animation
			if (Montage && Montage->GetWeight() >= 1.f)
			{
				if (InitialTargetBoneTransform2.Equals(FTransform::Identity))
				{
					UAnimInstance* CharacterAnimInstance = CharacterMesh->GetAnimInstance();
					const UAnimMontage* UseMontage = UseCabinet_SpecialMoveHandler2->GetDesiredSpecialMove(CurrentInteractor2).GetDefaultObject()->GetContextMontage();

					FCompactPose Pose;
					FBlendedCurve Curves;

					Pose.SetBoneContainer(&CharacterAnimInstance->GetRequiredBones());
					// TODO: Verfiy we don't need this! Curves.InitFrom(CharacterAnimInstance->GetSkelMeshComponent()->GetCachedAnimCurveMappingNameUids());

					// Get that first animation...
					const UAnimSequenceBase* Sequence = UseMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].AnimReference;
					Sequence->GetAnimationPose(Pose, Curves, FAnimExtractContext(InitialTargetBoneTime));

					// Keep track of this 'inital' position for animating the cabinet over time
					const int32 TargetBoneIndex = CharacterMesh->GetBoneIndex(CabinetTargetBone_NAME);
					InitialTargetBoneTransform2 = Pose.GetBones()[TargetBoneIndex];
				}

				// Otherwise position it based on the 'target_jnt' location
				const FTransform BoneTransform = CharacterMesh->GetSocketTransform(CabinetTargetBone_NAME, ERelativeTransformSpace::RTS_Component);
				const FTransform Delta = BoneTransform.GetRelativeTransform(InitialTargetBoneTransform2);
				const FTransform RelativeOffset = Delta * CloseTransform2;

				AnimatedMesh2->SetRelativeTransform(RelativeOffset);

				if (bItemShouldFollowAnimatedMesh)
				{
					ItemAttachLocation2->SetWorldTransform(FollowLocation2->GetComponentTransform());
				}
			}
		}
	}
	else
	{
		UpdateDrawerTickEnabled();
	}
}

void ASCDoubleCabinet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_CabinetOpen2);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCDoubleCabinet::DebugDraw(ASCCharacter* Character2)
{
	Super::DebugDraw(Character2);

	const FString ItemName2 = ContainedItem2 ? ContainedItem2->GetName() : TEXT("Empty");
	const bool bCanTookIt2 = Character2->CanTakeItem(ContainedItem2);
	const FString InteractorName2 = CurrentInteractor2 ? CurrentInteractor2->GetName() : TEXT("Nobody");
	const AActor* NotifyOwner2 = OpenCloseCabinetDriver2->GetNotifyOwner();

	const float TimeDatLeft2 = GetWorld()->GetTimerManager().GetTimerRemaining(TimerHandle_CabinetOpen2);

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("%s: Open Timer2: %.2f Interactor2: %s (%sDriving)  ContainedItem2: %s  CanTake2: %s  IsOpen2: %s (Fully2: %s)"),
		*GetName(), TimeDatLeft2, *InteractorName2, NotifyOwner2 == CurrentInteractor2 ? TEXT("") : TEXT("NOT "), *ItemName2, bCanTookIt2 ? TEXT("true") : TEXT("false"), bIsOpen2 ? TEXT("true") : TEXT("false"), bIsCabinetFullyOpen2 ? TEXT("true") : TEXT("false")), false);
}

void ASCDoubleCabinet::UpdateDrawerTickEnabled()
{
	if (CurrentInteractor || CurrentInteractor2)
	{
		SetNetDormancy(DORM_Awake);
		SetActorTickEnabled(true);
	}
	else
	{
		SetActorTickEnabled(false);
		SetNetDormancy(DORM_DormantAll);
	}
}

bool ASCDoubleCabinet::SpawnItem(TSubclassOf<ASCItem> ClassOverride /* = nullptr*/)
{
	// randomly try to spawn the item in one of the two drawers instead of always the left first
	if (FMath::Rand() & 0x1)
	{
		if (!Super::SpawnItem(ClassOverride)) // if other drawer fails to spawn item, attempt this drawer
		{
			if (ItemSpawner2->SpawnActor(ClassOverride))
			{
				AddItem(Cast<ASCItem>(ItemSpawner2->GetSpawnedActor()), true);
				return true;
			}

			return false;
		}
		return true;
	}
	else
	{
		if (ItemSpawner2->SpawnActor(ClassOverride)) // attempt to spawn item in this drawer first
		{
			AddItem(Cast<ASCItem>(ItemSpawner2->GetSpawnedActor()), true);
			return true;
		}
	}

	return Super::SpawnItem(ClassOverride); // will only get called if our drawer failed first
}

void ASCDoubleCabinet::AddItem(ASCItem* Item, bool bFromSpawn /*=false*/, ASCCharacter* Interactor/*=nullptr*/)
{
	if (Item)
	{
		if (bFromSpawn || (Interactor != nullptr && Interactor == CurrentInteractor2))
		{
			ForceNetUpdate();
			ASCItem* OldItem = ContainedItem2;
			ContainedItem2 = Item;
			OnRep_ContainedItem2(OldItem);
		}
		else
		{
			// Using the other drawer
			Super::AddItem(Item, bFromSpawn);
		}
	}
}

bool ASCDoubleCabinet::IsCabinetFull() const
{
	return ContainedItem2 != nullptr && Super::IsCabinetFull();
}

void ASCDoubleCabinet::SetAlwaysRelevant(const bool bRelevant)
{
	if (bRelevant || (!CurrentInteractor && !CurrentInteractor2))
	{
		Super::SetAlwaysRelevant(bRelevant);
	}
}

void ASCDoubleCabinet::OnRep_ContainedItem2(ASCItem* OldItem)
{
	if (OldItem && OldItem->GetRootComponent()->GetAttachParent() == ItemAttachLocation2)
	{
		OldItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		OldItem->SetActorRelativeRotation(FRotator::ZeroRotator);
	}

	if (ContainedItem2)
	{
		ContainedItem2->ForceNetUpdate();
		ContainedItem2->SetOwner(this);
		ContainedItem2->AttachToComponent(ItemAttachLocation2, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ContainedItem2->SetActorRelativeRotation(ContainedItem2->CabinetRotation);
		ContainedItem2->GetMesh()->SetRelativeLocation(FVector::ZeroVector);

		ContainedItem2->CurrentCabinet = this;
		ContainedItem2->EnableInteraction(false);
	}
}

void ASCDoubleCabinet::RemoveItem(ASCItem* Item)
{
	if (ContainedItem2 && ContainedItem2 == Item)
	{
		ForceNetUpdate();
		ContainedItem2 = nullptr;
		OnRep_ContainedItem2(Item);
	}
	else
	{
		// Using the other drawer
		Super::RemoveItem(Item);
	}
}

void ASCDoubleCabinet::OnInteract2(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		// Open the drawer
		if (!bIsOpen2)
		{
			if (Character->IsInSpecialMove())
				return;

			UseCabinet_SpecialMoveHandler2->ActivateSpecial(Character);
		}
		// We're looking at the item and don't want it, close the drawer
		else if (bIsCabinetFullyOpen2)
		{
			Close2(false);
		}
		else
		{
			if (!Character->IsInSpecialMove())
				Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent2);
		}
		break;

	case EILLInteractMethod::Hold:
		// Paranoid check
		if (bIsCabinetFullyOpen2 && ContainedItem2)
		{
			// Start special move to pickup item
			ContainedItem2->bIsPicking = true;
			Character->PickingItem = ContainedItem2;

			Close2(true);
		}
		else // Handle our paranoia
		{
			if (bIsOpen2)
				Close2(false);
			else
				Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent2);
		}
		break;
	}
}

int32 ASCDoubleCabinet::CanInteractWith2(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	int32 Flags = 0;
	if (bIsOpen2)
	{
		// Can take the item or close the drawer
		if (ASCCharacter* SCCharacter = Cast<ASCCharacter>(Character))
		{
			if (bIsCabinetFullyOpen2 && SCCharacter->CanTakeItem(ContainedItem2))
			{
				Flags = (int32)EILLInteractMethod::Press | (int32)EILLInteractMethod::Hold;
			}
		}
	}
	else
	{
		// Can only open the drawer
		Flags = (int32)EILLInteractMethod::Press;
	}

	return Flags;
}

void ASCDoubleCabinet::OnHoldStateChanged2(AActor* Interactor, EILLHoldInteractionState NewState)
{
	// LEAVE IT!
}

void ASCDoubleCabinet::OpenCloseCabinetDriverBegin2(FAnimNotifyData NotifyData)
{
	UpdateDrawerTickEnabled();
}

void ASCDoubleCabinet::OpenCloseCabinetDriverEnd2(FAnimNotifyData NotifyData)
{
	UpdateDrawerTickEnabled();
	InitialTargetBoneTransform2 = FTransform::Identity;
}

FVector ASCDoubleCabinet::GetDrawerLocation2() const
{
	return InteractComponent2->GetComponentLocation();
}

void ASCDoubleCabinet::OpenCloseCabinetCompleted2(ASCCharacter* Interactor)
{
	if (GetWorld())
		GetWorldTimerManager().ClearTimer(TimerHandle_CabinetOpen2);

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	// Cleanup our variables
	CurrentInteractor2 = nullptr;
	SetAlwaysRelevant(false);
	UpdateDrawerTickEnabled();
	OpenCloseCabinetDriver2->SetNotifyOwner(nullptr);

	if (Role == ROLE_Authority && ContainedItem2)
	{
		ForceNetUpdate();
		bIsOpen2 = false;

		// Server updates item locations
		AnimatedMesh2->SetWorldTransform(FTransform::Identity);
		AnimatedMesh2->SetRelativeTransform(CloseTransform2);

		if (bItemShouldFollowAnimatedMesh)
		{
			ItemAttachLocation2->SetWorldTransform(FollowLocation2->GetComponentTransform());
		}
	}
}

void ASCDoubleCabinet::UseCabinetStarted2(ASCCharacter* Interactor)
{
	if (!Interactor)
		return;

	// Stay relevant so long as someone is interacting
	SetAlwaysRelevant(true);

	if (Role == ROLE_Authority)
	{
		ForceNetUpdate();
		bIsOpen2 = true;
	}

	// Just caching our interactor
	CurrentInteractor2 = Interactor;
	OpenCloseCabinetDriver2->SetNotifyOwner(Interactor);
	UpdateDrawerTickEnabled();

	// We're managing this montage now
	USCSpecialMove_SoloMontage* UseCabinetSpecialMove = Cast<USCSpecialMove_SoloMontage>(Interactor->GetActiveSpecialMove());
	UAnimMontage* Montage = UseCabinetSpecialMove ? UseCabinetSpecialMove->GetContextMontage() : nullptr;
	if (Montage == nullptr) // This isn't a good thing to happen, but could due to relevancy
		return;

	const bool bKeepOpen = CurrentInteractor2->CanTakeItem(ContainedItem2);
	if (bKeepOpen)
	{
		// We have an item, we're going to control this montage by hand
		UseCabinetSpecialMove->ResetTimer(-1.f);
	}
	GetWorldTimerManager().SetTimer(TimerHandle_CabinetOpen2, this, &ASCDoubleCabinet::FullyOpenedCabinet2, Montage->GetSectionLength(Montage->GetSectionIndex(CabinetOpen_NAME)), false);
}

void ASCDoubleCabinet::FullyOpenedCabinet2()
{
	bIsCabinetFullyOpen2 = true;
	if (!CurrentInteractor2->CanTakeItem(ContainedItem2))
		Close2(false);

	if (Role == ROLE_Authority && CurrentInteractor2)
	{
		if (AController* Controller = CurrentInteractor2->GetController())
		{
			if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(Controller->PlayerState))
			{
				// Check if we're looking at a Tommy or Pamela tape. If we've collected all of them then just close the drawer.
				bool collectedAll = true;
				if (ASCPamelaTape* PammyTape = Cast<ASCPamelaTape>(ContainedItem2))
				{
					if (ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>())
					{
						TArray<TSubclassOf<USCPamelaTapeDataAsset>> ActivePamelaTapes = GM->GetActivePamelaTapes();
						int TapeID = 0;
						while (collectedAll && TapeID < ActivePamelaTapes.Num())
						{
							collectedAll = PlayerState->UnlockedPamelaTapes.Contains(ActivePamelaTapes[TapeID++]);
						}
					}
				}
				else if (ASCTommyTape* TommyTape = Cast<ASCTommyTape>(ContainedItem2))
				{
					if (ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>())
					{
						TArray<TSubclassOf<USCTommyTapeDataAsset>> ActiveTommyTapes = GM->GetActiveTommyTapes();
						int TapeID = 0;
						while (collectedAll && TapeID < ActiveTommyTapes.Num())
						{
							collectedAll = PlayerState->UnlockedTommyTapes.Contains(ActiveTommyTapes[TapeID++]);
						}
					}
				}
				else
				{
					return;
				}

				if (collectedAll)
					Close2(false);
			}
		}
	}
}

void ASCDoubleCabinet::UseCabinetAborted2(ASCCharacter* Interactor)
{
	if (Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent2);

	OpenCloseCabinetCompleted2(Interactor);

	if (Role == ROLE_Authority)
	{
		ForceNetUpdate();
		bIsOpen2 = false;

		// Server updates item locations
		AnimatedMesh2->SetWorldTransform(FTransform::Identity);
		AnimatedMesh2->SetRelativeTransform(CloseTransform2);

		if (bItemShouldFollowAnimatedMesh)
		{
			ItemAttachLocation2->SetWorldTransform(FollowLocation2->GetComponentTransform());
		}
	}
}

void ASCDoubleCabinet::Close2(bool bTakeItem)
{
	if (Role != ROLE_Authority)
		return;

	// Make sure AI know they have searched this cabinet
	if (ASCCounselorAIController* AIInteractor = CurrentInteractor2 ? Cast<ASCCounselorAIController>(CurrentInteractor2->Controller) : nullptr)
	{
		AIInteractor->SetCabinetSearched(InteractComponent2);
	}

	MULTICAST_Close2(bTakeItem);
}

void ASCDoubleCabinet::MULTICAST_Close2_Implementation(bool bTakeItem)
{
	// This would be bad
	if (!CurrentInteractor2)
		return;

	if (CurrentInteractor2->IsLocallyControlled())
		CurrentInteractor2->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent2);

	// No longer interacting, allow net culling
	SetAlwaysRelevant(false);
	bIsCabinetFullyOpen2 = false;

	// This is also not great
	USCSpecialMove_SoloMontage* UseCabinetSpecialMove = Cast<USCSpecialMove_SoloMontage>(CurrentInteractor2->GetActiveSpecialMove());
	UAnimMontage* Montage = UseCabinetSpecialMove ? UseCabinetSpecialMove->GetContextMontage() : nullptr;
	UAnimInstance* CharacterAnimInstance = CurrentInteractor2->GetMesh()->GetAnimInstance();
	if (!Montage || !CharacterAnimInstance)
		return;

	// Should we close that drawer or leave it open like we were raised in a barn (or you know, distracted by a psychopathic murderer)
	float PlayTime = 0.f;
	if (bTakeItem)
	{
		CharacterAnimInstance->Montage_JumpToSection(CabinetTake_NAME, Montage);
		PlayTime = Montage->GetSectionLength(Montage->GetSectionIndex(CabinetTake_NAME));

		ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(CurrentInteractor2);
		const bool bWillSwap = ContainedItem2 && !ContainedItem2->bIsSpecial && Counselor && Counselor->IsSmallInventoryFull();
		if (bWillSwap)
		{
			CharacterAnimInstance->Montage_SetNextSection(CabinetTake_NAME, CabinetTakeClose_NAME, Montage);
			PlayTime += Montage->GetSectionLength(Montage->GetSectionIndex(CabinetTakeClose_NAME));
		}
	}
	else if (ContainedItem2)
	{
		CharacterAnimInstance->Montage_JumpToSection(CabinetClose_NAME, Montage);
		PlayTime = Montage->GetSectionLength(Montage->GetSectionIndex(CabinetClose_NAME));
	}

	// If bTakeItem is false, PlayTime will be 0 meaning we need to peace out of this special move RIGHT NOW
	if (PlayTime > 0.f)
	{
		UseCabinetSpecialMove->ResetTimer(PlayTime);
	}
	else
	{
		UseCabinetSpecialMove->ForceDestroy();
		CharacterAnimInstance->Montage_Stop(0.2f, Montage);
	}
}
