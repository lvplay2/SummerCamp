// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCabinet.h"

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
#include "SCSpecialItem.h"
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

ASCCabinet::ASCCabinet(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, InitialTargetBoneTime(0.1f)
{
	bReplicates = true;
	bReplicateMovement = false;
	bAlwaysRelevant = false;
	NetUpdateFrequency = 30.0f;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.f;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PostPhysics;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->bGenerateOverlapEvents = false;
	RootComponent = BaseMesh;

	AnimatedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnimatedMesh"));
	AnimatedMesh->SetIsReplicated(true);
	AnimatedMesh->bGenerateOverlapEvents = false;
	AnimatedMesh->SetupAttachment(BaseMesh);

	FollowLocation = CreateDefaultSubobject<USceneComponent>(TEXT("FollowLocation"));
	FollowLocation->SetupAttachment(AnimatedMesh);

	ItemAttachLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ItemAttachLocation"));
	ItemAttachLocation->SetIsReplicated(true);
	ItemAttachLocation->SetupAttachment(BaseMesh);

	ItemSpawner = CreateDefaultSubobject<USCActorSpawnerComponent>(TEXT("ItemSpawner"));
	ItemSpawner->SetupAttachment(ItemAttachLocation);

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable"));
	InteractComponent->InteractMethods = (int32)EILLInteractMethod::Press | (int32)EILLInteractMethod::Hold;
	InteractComponent->HoldTimeLimit = 1.f;
	InteractComponent->bCanHighlight = true;
	InteractComponent->HighlightDistance = 300.f;
	InteractComponent->SetupAttachment(BaseMesh);

	OpenCloseCabinetDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("OpenCloseCabinetDriver"));
	OpenCloseCabinetDriver->SetNotifyName(TEXT("OpenCloseCabinet"));

	UseCabinet_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("UseCabinet_SpecialMoveHandler"));
	UseCabinet_SpecialMoveHandler->bTakeCameraControl = true;
	UseCabinet_SpecialMoveHandler->SetupAttachment(BaseMesh);
}

void ASCCabinet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCCabinet, ContainedItem);
	DOREPLIFETIME(ASCCabinet, bIsOpen);
}

void ASCCabinet::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent)
	{
		InteractComponent->OnInteract.AddDynamic(this, &ASCCabinet::OnInteract);
		InteractComponent->OnCanInteractWith.BindDynamic(this, &ASCCabinet::CanInteractWith);
		InteractComponent->OnHoldStateChanged.AddDynamic(this, &ASCCabinet::OnHoldStateChanged);
	}

	if (OpenCloseCabinetDriver)
	{
		FAnimNotifyEventDelegate Begin;
		Begin.BindDynamic(this, &ASCCabinet::OpenCloseCabinetDriverBegin);
		FAnimNotifyEventDelegate End;
		End.BindDynamic(this, &ASCCabinet::OpenCloseCabinetDriverEnd);
		OpenCloseCabinetDriver->InitializeBeginEnd(Begin, End);
	}

	if (UseCabinet_SpecialMoveHandler)
	{
		UseCabinet_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCCabinet::UseCabinetStarted);
		UseCabinet_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCCabinet::UseCabinetAborted);
		UseCabinet_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCCabinet::OpenCloseCabinetCompleted);
	}

	// Default to closed so we can quickly see what transforms are off
	if (!bIsOpen)
	{
		AnimatedMesh->SetWorldTransform(FTransform::Identity);
		AnimatedMesh->SetRelativeTransform(CloseTransform);
	}

	SetActorTickEnabled(false);
}

void ASCCabinet::BeginPlay()
{
	Super::BeginPlay();

	if (bItemShouldFollowAnimatedMesh)
	{
		ItemAttachLocation->SetWorldTransform(FollowLocation->GetComponentTransform());
	}

	// Start out asleep
	SetNetDormancy(DORM_DormantAll);
}

void ASCCabinet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentInteractor)
	{
		if (USkeletalMeshComponent* CharacterMesh = CurrentInteractor->GetMesh())
		{
			FAnimMontageInstance* Montage = CharacterMesh->GetAnimInstance()->GetActiveMontageInstance();

			// Don't move the drawer if we're blending the animation
			if (Montage && Montage->GetWeight() >= 1.f)
			{
				if (InitialTargetBoneTransform.Equals(FTransform::Identity))
				{
					UAnimInstance* CharacterAnimInstance = CharacterMesh->GetAnimInstance();
					const UAnimMontage* UseMontage = UseCabinet_SpecialMoveHandler->GetDesiredSpecialMove(CurrentInteractor).GetDefaultObject()->GetContextMontage();

					FCompactPose Pose;
					FBlendedCurve Curves;

					Pose.SetBoneContainer(&CharacterAnimInstance->GetRequiredBones());
					// TODO: Verfiy we don't need this! Curves.InitFrom(CharacterAnimInstance->GetSkelMeshComponent()->SkeletalMesh->Skeleton->GetDefaultCurveUIDList());

					// Get that first animation...
					const UAnimSequenceBase* Sequence = UseMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].AnimReference;
					Sequence->GetAnimationPose(Pose, Curves, FAnimExtractContext(InitialTargetBoneTime));

					// Keep track of this 'inital' position for animating the cabinet over time
					const int32 TargetBoneIndex = CharacterMesh->GetBoneIndex(CabinetTargetBone_NAME);
					InitialTargetBoneTransform = Pose.GetBones()[TargetBoneIndex];
				}

				// Otherwise position it based on the 'target_jnt' location
				const FTransform BoneTransform = CharacterMesh->GetSocketTransform(CabinetTargetBone_NAME, ERelativeTransformSpace::RTS_Component);
				const FTransform Delta = BoneTransform.GetRelativeTransform(InitialTargetBoneTransform);
				const FTransform RelativeOffset = Delta * CloseTransform;

				AnimatedMesh->SetRelativeTransform(RelativeOffset);

				if (bItemShouldFollowAnimatedMesh)
				{
					ItemAttachLocation->SetWorldTransform(FollowLocation->GetComponentTransform());
				}
			}
		}
	}
	else
	{
		UpdateDrawerTickEnabled();
	}
}

void ASCCabinet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_CabinetOpen);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCCabinet::DebugDraw(ASCCharacter* Character)
{
	const FString ItemName = ContainedItem ? ContainedItem->GetName() : TEXT("Empty");
	const bool bCanTookIt = Character->CanTakeItem(ContainedItem);
	const FString InteractorName = CurrentInteractor ? CurrentInteractor->GetName() : TEXT("Nobody");
	const AActor* NotifyOwner = OpenCloseCabinetDriver->GetNotifyOwner();

	const float TimeDatLeft = GetWorld()->GetTimerManager().GetTimerRemaining(TimerHandle_CabinetOpen);

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("%s: Open Timer: %.2f Interactor: %s (%sDriving)  ContainedItem: %s  CanTake: %s  IsOpen: %s (Fully: %s)"),
		*GetName(), TimeDatLeft, *InteractorName, NotifyOwner == CurrentInteractor ? TEXT("") : TEXT("NOT "), *ItemName, bCanTookIt ? TEXT("true") : TEXT("false"), bIsOpen ? TEXT("true") : TEXT("false"), bIsCabinetFullyOpen ? TEXT("true") : TEXT("false")), false);
}

void ASCCabinet::UpdateDrawerTickEnabled()
{
	if (CurrentInteractor)
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

bool ASCCabinet::SpawnItem(TSubclassOf<ASCItem> ClassOverride /* = nullptr*/)
{
	if (ItemSpawner->SpawnActor(ClassOverride))
	{
		AddItem(Cast<ASCItem>(ItemSpawner->GetSpawnedActor()));
		return true;
	}

	return false;
}

void ASCCabinet::AddItem(ASCItem* Item, bool bFromSpawn /*=false*/, ASCCharacter* Interactor /*=nullptr*/)
{
	if (Item)
	{
		ForceNetUpdate();
		ASCItem* OldItem = ContainedItem;
		ContainedItem = Item;
		OnRep_ContainedItem(OldItem);
	}
}

bool ASCCabinet::IsCabinetFull() const
{
	return ContainedItem != nullptr;
}

FVector ASCCabinet::GetDrawerLocation() const
{
	return InteractComponent->GetComponentLocation();
}

void ASCCabinet::OnRep_ContainedItem(ASCItem* OldItem)
{
	if (OldItem && OldItem->GetRootComponent()->GetAttachParent() == ItemAttachLocation)
	{
		OldItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		OldItem->SetActorRelativeRotation(FRotator::ZeroRotator);
	}

	if (ContainedItem)
	{
		ContainedItem->ForceNetUpdate();
		ContainedItem->SetOwner(this);
		ContainedItem->AttachToComponent(ItemAttachLocation, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ContainedItem->SetActorRelativeRotation(ContainedItem->CabinetRotation);
		ContainedItem->GetMesh()->SetRelativeLocation(FVector::ZeroVector);

		ContainedItem->CurrentCabinet = this;
		ContainedItem->EnableInteraction(false);
	}
}

void ASCCabinet::RemoveItem(ASCItem* Item)
{
	if (ContainedItem && ContainedItem == Item)
	{
		ForceNetUpdate();
		ContainedItem = nullptr;
		OnRep_ContainedItem(Item);
	}
}

void ASCCabinet::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		// Open the drawer
		if (!bIsOpen)
		{
			// Make sure we're not in a special move (Client or Server) before closing
			if (Character->IsInSpecialMove())
				return;

			UseCabinet_SpecialMoveHandler->ActivateSpecial(Character);
		}
		// We're looking at the item and don't want it, close the drawer
		else if (bIsCabinetFullyOpen)
		{
			Close(false);
		}
		else
		{
			if (!Character->IsInSpecialMove())
				Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
		}
		break;

	case EILLInteractMethod::Hold:
		// Paranoid check
		if (bIsCabinetFullyOpen && ContainedItem)
		{
			// Start special move to pickup item
			ContainedItem->bIsPicking = true;
			Character->PickingItem = ContainedItem;

			Close(true);
		}
		else // Handle our paranoia
		{
			if (bIsOpen)
				Close(false);
			else
				Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
		}

		break;
	}
}

int32 ASCCabinet::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	int32 Flags = 0;
	if (bIsOpen)
	{
		// Can take the item or close the drawer
		if (ASCCharacter* SCCharacter = Cast<ASCCharacter>(Character))
		{
			if (bIsCabinetFullyOpen && SCCharacter->CanTakeItem(ContainedItem))
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

void ASCCabinet::OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState NewState)
{
	// LEAVE THIS HERE PLEASE
}

void ASCCabinet::OpenCloseCabinetDriverBegin(FAnimNotifyData NotifyData)
{
	UpdateDrawerTickEnabled();
}

void ASCCabinet::OpenCloseCabinetDriverEnd(FAnimNotifyData NotifyData)
{
	UpdateDrawerTickEnabled();
	InitialTargetBoneTransform = FTransform::Identity;
}

void ASCCabinet::OpenCloseCabinetCompleted(ASCCharacter* Interactor)
{
	if (GetWorld())
		GetWorldTimerManager().ClearTimer(TimerHandle_CabinetOpen);

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	// Cleanup our variables
	CurrentInteractor = nullptr;
	SetAlwaysRelevant(false);
	UpdateDrawerTickEnabled();
	OpenCloseCabinetDriver->SetNotifyOwner(nullptr);

	if (Role == ROLE_Authority && ContainedItem)
	{
		ForceNetUpdate();
		bIsOpen = false;

		// Server updates item locations
		AnimatedMesh->SetWorldTransform(FTransform::Identity);
		AnimatedMesh->SetRelativeTransform(CloseTransform);

		if (bItemShouldFollowAnimatedMesh)
		{
			ItemAttachLocation->SetWorldTransform(FollowLocation->GetComponentTransform());
		}
	}
}

void ASCCabinet::UseCabinetStarted(ASCCharacter* Interactor)
{
	if (!Interactor)
		return;

	// Stay relevant so long as someone is interacting
	SetAlwaysRelevant(true);

	if (Role == ROLE_Authority)
	{
		ForceNetUpdate();
		bIsOpen = true;
	}

	// Just caching our interactor
	CurrentInteractor = Interactor;
	OpenCloseCabinetDriver->SetNotifyOwner(Interactor);
	UpdateDrawerTickEnabled();

	// We're managing this montage now
	USCSpecialMove_SoloMontage* UseCabinetSpecialMove = Cast<USCSpecialMove_SoloMontage>(Interactor->GetActiveSpecialMove());
	UAnimMontage* Montage = UseCabinetSpecialMove ? UseCabinetSpecialMove->GetContextMontage() : nullptr;
	if (Montage == nullptr) // This isn't a good thing to happen, but could due to relevancy
		return;
	
	const bool bKeepOpen = Interactor->CanTakeItem(ContainedItem);
	if (bKeepOpen)
	{
		// We have an item, we're going to control this montage by hand
		UseCabinetSpecialMove->ResetTimer(-1.f);
	}
	GetWorldTimerManager().SetTimer(TimerHandle_CabinetOpen, this, &ASCCabinet::FullyOpenedCabinet, Montage->GetSectionLength(Montage->GetSectionIndex(CabinetOpen_NAME)), false);
}

void ASCCabinet::FullyOpenedCabinet()
{
	bIsCabinetFullyOpen = true;
	if (!CurrentInteractor->CanTakeItem(ContainedItem))
		Close(false);

	if (Role == ROLE_Authority && CurrentInteractor)
	{
		if (AController* Controller = CurrentInteractor->GetController())
		{
			if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(Controller->PlayerState))
			{
				// Check if we're looking at a Tommy or Pamela tape. If we've collected all of them then just close the drawer.
				bool collectedAll = true;
				if (ASCPamelaTape* PammyTape = Cast<ASCPamelaTape>(ContainedItem))
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
				else if (ASCTommyTape* TommyTape = Cast<ASCTommyTape>(ContainedItem))
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
					Close(false);
			}
		}
	}
}

void ASCCabinet::UseCabinetAborted(ASCCharacter* Interactor)
{
	if (Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	OpenCloseCabinetCompleted(Interactor);

	if (Role == ROLE_Authority)
	{
		ForceNetUpdate();
		bIsOpen = false;

		// Server updates item locations
		AnimatedMesh->SetWorldTransform(FTransform::Identity);
		AnimatedMesh->SetRelativeTransform(CloseTransform);

		if (bItemShouldFollowAnimatedMesh)
		{
			ItemAttachLocation->SetWorldTransform(FollowLocation->GetComponentTransform());
		}
	}
}

void ASCCabinet::Close(bool bTakeItem)
{
	if (Role != ROLE_Authority)
		return;

	// Make sure AI know they have searched this cabinet
	if (ASCCounselorAIController* AIInteractor = CurrentInteractor ? Cast<ASCCounselorAIController>(CurrentInteractor->Controller) : nullptr)
	{
		AIInteractor->SetCabinetSearched(InteractComponent);
	}

	MULTICAST_Close(bTakeItem);
}

void ASCCabinet::MULTICAST_Close_Implementation(bool bTakeItem)
{
	// This would be bad
	if (!CurrentInteractor)
		return;

	if (CurrentInteractor->IsLocallyControlled())
		CurrentInteractor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	// No longer interacting, allow net culling
	SetAlwaysRelevant(false);
	bIsCabinetFullyOpen = false;

	// This is also not great
	USCSpecialMove_SoloMontage* UseCabinetSpecialMove = Cast<USCSpecialMove_SoloMontage>(CurrentInteractor->GetActiveSpecialMove());
	UAnimMontage* Montage = UseCabinetSpecialMove ? UseCabinetSpecialMove->GetContextMontage() : nullptr;
	UAnimInstance* CharacterAnimInstance = CurrentInteractor->GetMesh()->GetAnimInstance();
	if (!Montage || !CharacterAnimInstance)
		return;

	// Should we close that drawer or leave it open like we were raised in a barn (or you know, distracted by a psychopathic murderer)
	float PlayTime = 0.f;
	if (bTakeItem)
	{
		CharacterAnimInstance->Montage_JumpToSection(CabinetTake_NAME, Montage);
		PlayTime = Montage->GetSectionLength(Montage->GetSectionIndex(CabinetTake_NAME));

		ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(CurrentInteractor);
		const bool bWillSwap = ContainedItem && !ContainedItem->bIsSpecial && Counselor && Counselor->IsSmallInventoryFull();
		if (bWillSwap)
		{
			CharacterAnimInstance->Montage_SetNextSection(CabinetTake_NAME, CabinetTakeClose_NAME, Montage);
			PlayTime += Montage->GetSectionLength(Montage->GetSectionIndex(CabinetTakeClose_NAME));
		}
	}
	else if (ContainedItem)
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
		CharacterAnimInstance->Montage_Stop(0.2f, Montage);
		UseCabinetSpecialMove->ForceDestroy();
	}
}

void ASCCabinet::SetAlwaysRelevant(const bool bRelevant)
{
	SetNetDormancy(bRelevant ? DORM_Awake : DORM_DormantAll);
	bAlwaysRelevant = bRelevant;
}
