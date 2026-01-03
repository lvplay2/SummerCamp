// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCHidingSpot.h"

#include "SCContextKillComponent.h"
#include "SCCounselorAnimInstance.h"
#include "SCCounselorCharacter.h"
#include "SCGameState.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"
#include "SCSpacerCapsuleComponent.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCSpecialMoveComponent.h"
#include "SCStatusIconComponent.h"
#include "SCWeapon.h"
#include "SCImposterOutfit.h"

static const FName NAME_KillMoveTag(TEXT("KillMove"));
static const FName NAME_SearchMoveTag(TEXT("SearchMove"));
static const FName NAME_HideMoveTag(TEXT("HideMove"));

#define CAMERA_BLEND_BACK_TIME 0.1f

// Sets default values
ASCHidingSpot::ASCHidingSpot(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, VignetteIntensityMax(1.5f)
, bIgnoreCollisionWhileHiding(true)
, bAllowCameraRotation(true)
, bDestroyOnSearch(true)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("skeletalMesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	Mesh->bEnableUpdateRateOptimizations = true;
	Mesh->bNoSkeletonUpdate = true;

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable"));
	InteractComponent->SetupAttachment(Mesh);

	KillerInteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("KillerInteractable"));
	KillerInteractComponent->SetupAttachment(Mesh);

	ContextKillComponent = CreateDefaultSubobject<USCContextKillComponent>(TEXT("HidingSpotKill"));
	ContextKillComponent->ComponentTags.Add(NAME_KillMoveTag);
	ContextKillComponent->SetupAttachment(RootComponent);

	SearchComponent = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("SearchComponent"));
	SearchComponent->ComponentTags.Add(NAME_SearchMoveTag);
	SearchComponent->SetupAttachment(RootComponent);

	EnterSpecialMove = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("EnterSpecialMove"));
	EnterSpecialMove->ComponentTags.Add(NAME_HideMoveTag);
	EnterSpecialMove->SetupAttachment(RootComponent);

	DestructionFXDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("DestructionFXDriver"));
	DestructionFXDriver->SetNotifyName(TEXT("HidingDestructionDriver"));

	HidingCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Hiding Camera"));
	HidingCamera->SetupAttachment(RootComponent);

	HidingCameraPivot = CreateDefaultSubobject<USceneComponent>(TEXT("Hiding Camera Pivot"));
	HidingCameraPivot->SetupAttachment(RootComponent);

	SpectatorViewingLocation = CreateDefaultSubobject<UArrowComponent>(TEXT("Spectator Viewing"));
	SpectatorViewingLocation->SetupAttachment(RootComponent); 

	OnExitControlRotation = CreateDefaultSubobject<UArrowComponent>(TEXT("On Exit Facing"));
	OnExitControlRotation->SetupAttachment(RootComponent);

	OccupiedIcon = CreateDefaultSubobject<USCStatusIconComponent>(TEXT("OccupiedIcon"));
	OccupiedIcon->SetupAttachment(RootComponent);
	OccupiedIcon->bUseDistanceNotify = true;
	OccupiedIcon->DistanceToNotify = 300.0f;
	OccupiedIcon->bOutOfRangeWhenMatchNotInProgress = true;

	CounselorHidingSpotLocation = CreateDefaultSubobject<USceneComponent>(TEXT("CounselorHidingSpotLocation"));
	CounselorHidingSpotLocation->SetupAttachment(Mesh);

	ItemDropLocationLarge = CreateDefaultSubobject<USceneComponent>(TEXT("ItemDropLarge"));
	ItemDropLocationLarge->SetupAttachment(RootComponent);

	ItemDropLocationSmall01 = CreateDefaultSubobject<USceneComponent>(TEXT("ItemDropSmall01"));
	ItemDropLocationSmall01->SetupAttachment(RootComponent);

	ItemDropLocationSmall02 = CreateDefaultSubobject<USceneComponent>(TEXT("ItemDropSmall02"));
	ItemDropLocationSmall02->SetupAttachment(RootComponent);

	ItemDropLocationSmall03 = CreateDefaultSubobject<USceneComponent>(TEXT("ItemDropSmall03"));
	ItemDropLocationSmall03->SetupAttachment(RootComponent);

	bReplicates = true;
	bAlwaysRelevant = false;
	NetUpdateFrequency = 40.0f;
	//NetCullDistanceSquared = FMath::Square(1000.f); // Comment in to test relevance locally
}

void ASCHidingSpot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCHidingSpot, bDestroyed);
	DOREPLIFETIME_CONDITION(ASCHidingSpot, CounselorInHiding, COND_InitialOnly);
}

void ASCHidingSpot::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent)
	{
		InteractComponent->OnCanInteractWith.BindDynamic(this, &ASCHidingSpot::CanCounselorInteractWith);
		InteractComponent->OnInteract.AddDynamic(this, &ASCHidingSpot::OnCounselorInteract);
	}

	if (KillerInteractComponent)
	{
		KillerInteractComponent->OnCanInteractWith.BindDynamic(this, &ASCHidingSpot::CanKillerInteractWith);
		KillerInteractComponent->OnInteract.AddDynamic(this, &ASCHidingSpot::OnKillerInteract);
	}

	FAnimNotifyEventDelegate DestructionDelegate;
	DestructionDelegate.BindDynamic(this, &ASCHidingSpot::NativeDestroyHidingSpot);

	if (DestructionFXDriver)
	{
		DestructionFXDriver->InitializeBegin(DestructionDelegate);
	}

	// Killer doing the murderin'
	TArray<UActorComponent*> KillMoves = GetComponentsByTag(USCContextKillComponent::StaticClass(), NAME_KillMoveTag);
	for (auto& Move : KillMoves)
	{
		USCContextKillComponent* ContextKill = Cast<USCContextKillComponent>(Move);
		ContextKill->KillStarted.AddDynamic(this, &ASCHidingSpot::ContextKillDestinationReached);
		ContextKill->KillComplete.AddDynamic(this, &ASCHidingSpot::ContextKillCompleted);
	}

	// Killer looking for counselors
	TArray<UActorComponent*> SearchMoves = GetComponentsByTag(USCSpecialMoveComponent::StaticClass(), NAME_SearchMoveTag);
	for (auto& Move : SearchMoves)
	{
		USCSpecialMoveComponent* SearchMove = Cast<USCSpecialMoveComponent>(Move);
		SearchMove->DestinationReached.AddDynamic(this, &ASCHidingSpot::EnableTick);
		SearchMove->SpecialStarted.AddDynamic(this, &ASCHidingSpot::HideSpotSearchStart);
		SearchMove->SpecialComplete.AddDynamic(this, &ASCHidingSpot::HideSpotSearchCompleted);
	}

	// Counselors hiding from the Killer
	TArray<UActorComponent*> HideMoves = GetComponentsByTag(USCSpecialMoveComponent::StaticClass(), NAME_HideMoveTag);
	for (auto& Move : HideMoves)
	{
		USCSpecialMoveComponent* HideMove = Cast<USCSpecialMoveComponent>(Move);
		HideMove->SpecialStarted.AddDynamic(this, &ASCHidingSpot::EnterStarted);
		HideMove->SpecialComplete.AddDynamic(this, &ASCHidingSpot::EnterCompleted);
		HideMove->SpecialAborted.AddDynamic(this, &ASCHidingSpot::EnterAborted);
	}

	HidingCameraTransform = HidingCamera->GetRelativeTransform();

	HidingCameraBaseTransform = HidingCameraTransform;

	// Begin Math 'n' shit to mirror the camera about the Y axis

	// cache the pitch and rotate without it.
	FRotator RotatorWithoutPitch = HidingCameraTransform.GetRotation().Rotator();
	const float CachedPitch = RotatorWithoutPitch.Pitch;
	RotatorWithoutPitch.Pitch = 0.f;

	// Get our new camera forward.
	const FVector HidingCamDefaultForward = RotatorWithoutPitch.Vector();
	const FVector MirrorVector = FVector::RightVector;

	// Mirror across Y Axis
	const FVector NewForward = HidingCamDefaultForward.RotateAngleAxis(180.f, MirrorVector);
	RotatorWithoutPitch = NewForward.ToOrientationRotator();
	RotatorWithoutPitch.Pitch = CachedPitch;

	// Put the pieces together for our mirrored transform.
	HidingCameraTransformMirrored = HidingCamera->GetRelativeTransform();
	HidingCameraTransformMirrored.SetRotation(FQuat(RotatorWithoutPitch));

	FVector MirroredLocation = HidingCameraTransformMirrored.GetLocation();
	MirroredLocation.X = -MirroredLocation.X;
	HidingCameraTransformMirrored.SetLocation(MirroredLocation);
}

void ASCHidingSpot::EnableTick(ASCCharacter* Interactor)
{
	SetActorTickEnabled(true);
}

void ASCHidingSpot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bAllowCameraRotation && CounselorInHiding && CounselorInHiding->IsInHiding() && !CounselorInHiding->IsInSpecialMove())
	{
		// If CounselorInHiding isn't the local player, use the local player's controller to calculate camera position (for spectating)
		ASCPlayerController* Controller = [this]() -> ASCPlayerController*
		{
			if (CounselorInHiding->IsLocallyControlled() == false)
			{
				UWorld* World = GetWorld();
				ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController();
				APlayerController* LocalPC = LocalPlayer ? LocalPlayer->GetPlayerController(World) : nullptr;

				if (ASCPlayerController* LocalSCPC = Cast<ASCPlayerController>(LocalPC))
				{
					if (LocalSCPC->GetSpectatingPlayer() == CounselorInHiding)
						return LocalSCPC;
				}
				return nullptr;
			}

			return Cast<ASCPlayerController>(CounselorInHiding->Controller);
		}();
		const FRotator ControlRotation = Controller ? Controller->GetControlRotation().GetNormalized() : FRotator::ZeroRotator;
		const FRotator HidingRootRotation = HidingCameraBaseTransform.Rotator().GetNormalized();

		const FRotator NewRotation = FRotator(FMath::Clamp(ControlRotation.Pitch, HidingRootRotation.Pitch - MaxHidingRotationOffset.X, HidingRootRotation.Pitch + MaxHidingRotationOffset.X),
											FMath::Clamp(ControlRotation.Yaw, HidingRootRotation.Yaw - MaxHidingRotationOffset.Y, HidingRootRotation.Yaw + MaxHidingRotationOffset.Y),
											ControlRotation.Roll);

		// Force the control rotation within the bounds so there's no feeling of lag in the controls.
		if (ControlRotation != NewRotation)
			ForceControlRotation(Controller, NewRotation);

		// Rotate the camera and offset it's position from the pivot.
		HidingCamera->SetRelativeRotation(NewRotation);
		FVector PivotLocation = HidingCameraPivot->RelativeLocation;
		if (HidingCameraBaseTransform.Equals(HidingCameraTransformMirrored))
		{
			PivotLocation.X = -PivotLocation.X;
		}

		const FVector NewLocation = PivotLocation - (NewRotation.Vector() * HidingCamDistFromPivot);
		HidingCamera->SetRelativeLocation(NewLocation);
	}
}

void ASCHidingSpot::BeginPlay()
{
	Super::BeginPlay();

	SetSenseEffects(false);

	if (bIgnoreCollisionWhileHiding && CounselorInHiding)
	{
		CounselorInHiding->MoveIgnoreActorAdd(this);
	}
}

void ASCHidingSpot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Exit);
		GetWorldTimerManager().ClearTimer(TimerHandle_ReturnCamera);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCHidingSpot::ContextKillDestinationReached(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	// If there's no interactor that kill isn't active, abort.
	if (Interactor == nullptr || CounselorInHiding == nullptr)
		return;

	Mesh->bNoSkeletonUpdate = false;

	if (USkeletalMeshComponent* SkelMesh = CounselorInHiding->GetMesh())
	{
		SkelMesh->SetOwnerNoSee(false);

		TArray<AActor*> AttachedActors;
		CounselorInHiding->GetAttachedActors(AttachedActors);
		for (AActor* Actor : AttachedActors)
		{
			ASCItem * Item = Cast<ASCItem>(Actor);
			if (Item && Item->bIsSpecial)
			{
				// Don't show the Map (it's currently the only "special" item) item when you die in a hiding spot. 
				// It'll show at the base of the ragdoll counselor's feet otherwise.
				continue;
			}
			
			Actor->SetActorHiddenInGame(false);
		}

		TArray<USceneComponent*> AttachedComponents;
		CounselorInHiding->GetMesh()->GetChildrenComponents(true, AttachedComponents);
		for (USceneComponent* Comp : AttachedComponents)
		{
			if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Comp))
			{
				MeshComp->SetOwnerNoSee(false);
			}
		}
	}

	InteractComponent->SetDrawAtLocation(false);
	if (CounselorInHiding && CounselorInHiding->IsLocallyControlled())
	{
		CounselorInHiding->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
	}

	if (Interactor && Interactor->IsLocallyControlled())
	{
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(KillerInteractComponent);
	}

	if (UAnimSequence* KillAnim = GetWeaponKillAnim(Interactor->GetCurrentWeapon()))
		Mesh->PlayAnimation(KillAnim, false);

	bBeginDestruction = true;
}

void ASCHidingSpot::ContextKillCompleted(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	if (Interactor && Interactor->IsLocallyControlled())
	{
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(KillerInteractComponent);
	}

	if (Role == ROLE_Authority)
	{
		// We may have aborted rather than finishing the kill
		if (!Victim->IsAlive())
		{
			// Person hiding inside is dead. Feel free to net cull!
			bAlwaysRelevant = false;
			bDestroyed = bDestructable;
			bBeginDestruction = false;
			OnRep_Destroyed();
		}
	}

	Mesh->bNoSkeletonUpdate = true;
}

USCSpecialMoveComponent* ASCHidingSpot::GetCorrectSpecialMoveComponent_Implementation(AActor* Interactor)
{
	return EnterSpecialMove;
}

USCSpecialMoveComponent* ASCHidingSpot::GetCorrectSearchMoveComponent_Implementation(AActor* Interactor)
{
	return SearchComponent;
}

USCContextKillComponent* ASCHidingSpot::GetCorrectContextKillComponent_Implementation(AActor* Interactor)
{
	return ContextKillComponent;
}

UAnimSequence* ASCHidingSpot::GetWeaponKillAnim(ASCWeapon* KillerWeapon)
{
	for (FWeaponSpecificAnim WeaponType : WeaponSpecificAnims)
	{
		if (!WeaponType.WeaponClass || !WeaponType.AnimSequence)
			continue;

		if (KillerWeapon->IsA(WeaponType.WeaponClass))
		{
			return WeaponType.AnimSequence;
		}
	}

	return nullptr;
}

UAnimSequence* ASCHidingSpot::GetWeaponSearchAnim(ASCWeapon* KillerWeapon)
{
	for (FWeaponSpecificAnim WeaponType : WeaponSpecificSearchAnims)
	{
		if (!WeaponType.WeaponClass || !WeaponType.AnimSequence)
			continue;

		if (KillerWeapon->IsA(WeaponType.WeaponClass))
		{
			return WeaponType.AnimSequence;
		}
	}

	return nullptr;
}

void ASCHidingSpot::OnRep_Destroyed()
{
	if (bDestroyed)
	{
		if (!OccupiedIcon->IsHidden())
			OccupiedIcon->HideIcon();

		InteractComponent->SetActive(false);
		DestroyHidingSpot();

		// Let the game state know this actor is dead
		UWorld* World = GetWorld();
		if (ASCGameState* GameState = Cast<ASCGameState>(World ? World->GetGameState() : nullptr))
			GameState->ActorBroken(this);
	}
}

void ASCHidingSpot::HideSpotSearchStart(ASCCharacter* Interactor)
{
	// Unlikely!
	if (!Mesh)
		return;

	// The BP sometimes does it's own shit like an asshole!
	Mesh->bNoSkeletonUpdate = false;

	if (UAnimationAsset* AnimToPlay = GetWeaponSearchAnim(Interactor->GetCurrentWeapon()))
		Mesh->PlayAnimation(AnimToPlay, false);
}

void ASCHidingSpot::HideSpotSearchCompleted(ASCCharacter* Interactor)
{
	SetActorTickEnabled(false);
	bBeginDestruction = false;
	Mesh->bNoSkeletonUpdate = true;

	if (Role == ROLE_Authority && bDestroyOnSearch)
	{
		bDestroyed = bDestructable;
		OnRep_Destroyed();
	}

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(KillerInteractComponent);
}

void ASCHidingSpot::HideSpotSearchAborted(ASCCharacter* Interactor)
{
	SetActorTickEnabled(false);
	bBeginDestruction = false;

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(KillerInteractComponent);
}

int32 ASCHidingSpot::CanCounselorInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (bDestroyed || bBeginDestruction || Character->IsInSpecialMove())
		return 0;

	if (CounselorInHiding && CounselorInHiding != Character)
		return 0;

	// people with impostor outfits cant use hiding spots
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
	{
		if (Counselor->HasSpecialItem(ASCImposterOutfit::StaticClass()))
			return 0;
	}

	return InteractComponent->InteractMethods;
}

void ASCHidingSpot::OnCounselorInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor);
	check(Counselor);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		if (CounselorInHiding == Counselor)
		{
			if (TestBlockers(Counselor))
			{
				ResetBlockers();
				break;
			}
			MULTICAST_ExitStarted(CounselorInHiding);
		}
		else if (!CounselorInHiding)
		{
			if (TestBlockers(Counselor))
			{
				Counselor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
				ResetBlockers();
				break;
			}

			// Default fallback
			USCSpecialMoveComponent* SpecialMove = GetCorrectSpecialMoveComponent(Counselor);
			if (!SpecialMove)
			{
				SpecialMove = EnterSpecialMove;
			}

			// Apply perk modifiers to the montage play rate
			const float MontagePlayRate = [&Counselor]()
			{
				float PlayRate = 1.0f;

				if (ASCPlayerState* PS = Counselor->GetPlayerState())
				{
					for (const USCPerkData* Perk : PS->GetActivePerks())
					{
						check(Perk);

						if (!Perk)
							continue;

						PlayRate += Perk->WindowHidespotInteractionSpeedModifier;
					}
				}

				return PlayRate;
			}();
			Counselor->SetStance(ESCCharacterStance::Standing);
			SpecialMove->ActivateSpecial(Counselor, MontagePlayRate);
			SetCounselorInHiding(Counselor); // Make sure we set this immediately on the server so that Jason doesn't break stuff if he tried to interact with the bed
		}
		else
		{
			Counselor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
		}
		break;
	}
}

int32 ASCHidingSpot::CanKillerInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character);
	if (!Killer)
		return 0;

	// It's broke or going to be broke
	if (bDestroyed || bBeginDestruction)
		return 0;

	// You're busy!
	if (Killer->IsInSpecialMove() || Killer->GetGrabbedCounselor())
		return 0;

	// Counselor is getting in or out
	if (bExiting || (InteractComponent->IsLockedInUse() && !CounselorInHiding))
		return 0;

	return InteractComponent->InteractMethods;
}

void ASCHidingSpot::OnKillerInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor);
	check(Killer);

	if (bExiting)
	{
		Killer->GetInteractableManagerComponent()->UnlockInteraction(KillerInteractComponent);
		return;
	}

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		if (CounselorInHiding)
		{
			GetCorrectContextKillComponent(Killer)->SERVER_ActivateContextKill(Killer, CounselorInHiding);
			Killer->GetInteractableManagerComponent()->UnlockInteraction(KillerInteractComponent);
			MULTICAST_SetVignetteIntensity(0.f);
		}
		else
		{
			// Play search animation
			if (ASCWeapon* KillerWeapon = Killer->GetCurrentWeapon())
			{
				GetCorrectSearchMoveComponent(Killer)->ActivateSpecial(Killer);
				bBeginDestruction = true;
			}
			else
			{
				Killer->GetInteractableManagerComponent()->UnlockInteraction(KillerInteractComponent);
			}
		}
		break;
	}
}

void ASCHidingSpot::CancelHiding()
{
	if (!CounselorInHiding)
		return;

	// Try to set our end location
	if (Role == ROLE_Authority && !CounselorInHiding->bIsGrabbed)
	{
		if (USCSpacerCapsuleComponent* SpacerComponent = GetExitSpacer(CounselorInHiding))
			CounselorInHiding->SetActorLocation(SpacerComponent->GetComponentLocation());
	}

	if (GetWorld() && GetWorldTimerManager().IsTimerActive(TimerHandle_Exit))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Exit);
	}

	bExiting = true;
	ExitCompleted();
}

// Enter Delegates ------------------------------------------------

void ASCHidingSpot::EnterStarted(ASCCharacter* Interactor)
{
	// Stay relevant so long as someone is inside
	bAlwaysRelevant = true;
	Mesh->bNoSkeletonUpdate = false;

	if (bIgnoreCollisionWhileHiding && Interactor)
		Interactor->MoveIgnoreActorAdd(this);
}

void ASCHidingSpot::EnterCompleted(ASCCharacter* Interactor)
{
	ResetBlockers();

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		SetCounselorInHiding(Counselor);

	if (!CounselorInHiding)
		return;

	CounselorInHiding->EnterHidingSpot(this);
	if (CounselorInHiding->IsLocallyControlled())
	{
		InteractComponent->SetDrawAtLocation(true, FVector2D(0.08f, 0.85f));
		ForceControlRotation(Cast<ASCPlayerController>(CounselorInHiding->Controller), HidingCameraBaseTransform.Rotator());
	}

	if (bEnteredFromMirrorSide)
	{
		HidingCameraBaseTransform = HidingCameraTransformMirrored;

		if (!bHasUpdatedItemDropLocations)
		{
			// Move item drops to the "mirrored" side
			auto MoveToMirroredSide = [](USceneComponent* ItemDropLocationComponent) -> void
			{
				if (!ItemDropLocationComponent)
					return;

				FVector RelativeLocation = ItemDropLocationComponent->RelativeLocation;
				RelativeLocation.X = -RelativeLocation.X;

				ItemDropLocationComponent->SetRelativeLocation(RelativeLocation, false, nullptr, ETeleportType::None);
			};

			MoveToMirroredSide(ItemDropLocationLarge);
			MoveToMirroredSide(ItemDropLocationSmall01);
			MoveToMirroredSide(ItemDropLocationSmall02);
			MoveToMirroredSide(ItemDropLocationSmall03);

			bHasUpdatedItemDropLocations = true;
		}
	}
	else
	{
		HidingCameraBaseTransform = HidingCameraTransform;
	}

	SetActorTickEnabled(true);

	TArray<AActor*> AttachedActors;
	CounselorInHiding->GetAttachedActors(AttachedActors);
	for (AActor* Actor : AttachedActors)
	{
		Actor->SetActorHiddenInGame(true);
	}

	TArray<USceneComponent*> AttachedComponents;
	CounselorInHiding->GetMesh()->GetChildrenComponents(true, AttachedComponents);
	for (USceneComponent* Comp : AttachedComponents)
	{
		if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Comp))
		{
			MeshComp->SetOwnerNoSee(true);
		}
	}

	MakeOnlyActiveCamera(this, HidingCamera);
	CounselorInHiding->TakeCameraControl(this, 0.f);
	HidingCamera->SetRelativeTransform(HidingCameraBaseTransform);
	SetVignetteIntensity(VignetteIntensityMax);

	if (!CounselorInHiding->IsLocallyControlled())
		OccupiedIcon->ShowIcon();

	// Give this counselor a badge!
	if (Role == ROLE_Authority)
	{
		if (ASCPlayerState* PS = CounselorInHiding->GetPlayerState())
			PS->EarnedBadge(UseHideSpotBadge);
	}

	// Call to BP
	OnEnterCompleted();
}

void ASCHidingSpot::EnterAborted(ASCCharacter* Interactor)
{
	// Enter failed, allow net culling again
	bAlwaysRelevant = false;
	Mesh->bNoSkeletonUpdate = true;

	ResetBlockers();

	// So Safe. So Comfortable. So Shoney's.
	if (Interactor)
	{
		FTransform DesiredTransform(FTransform::Identity);

		if (Interactor->GetActiveSCSpecialMove())
			DesiredTransform = Interactor->GetActiveSCSpecialMove()->GetDesiredTransform();

		// If the desired transform is set, return our interacting actor to it
		if (DesiredTransform.GetLocation() != FVector::ZeroVector)
			Interactor->SetActorTransform(DesiredTransform);

		if (Interactor->IsLocallyControlled())
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
	}
}

// Exit RPCs -------------------------------------------------

void ASCHidingSpot::MULTICAST_ExitStarted_Implementation(ASCCharacter* Interactor)
{
	if (!CounselorInHiding || bExiting)
		return;

	// Reset our exit position
	if (!CounselorHidingSpotLocation->GetRelativeTransform().GetTranslation().IsNearlyZero())
		CounselorInHiding->SetActorLocation(CounselorHidingSpotLocation->GetComponentLocation());

	InteractComponent->SetDrawAtLocation(false);

	BlueprintExitStarted();
	CounselorInHiding->MoveIgnoreActorAdd(this);
	CounselorInHiding->ExitHidingSpot(this);

	TArray<AActor*> AttachedActors;
	CounselorInHiding->GetAttachedActors(AttachedActors);
	for (AActor* Actor : AttachedActors)
	{
		// Items will be handled separately
		if (const ASCItem* Item = Cast<ASCItem>(Actor))
			continue;

		Actor->SetActorHiddenInGame(false);
	}

	// Items will be handled now! (small items stay hidden)
	if (ASCItem* Item = CounselorInHiding->GetEquippedItem())
		Item->SetActorHiddenInGame(false);

	USkeletalMeshComponent* CounselorMesh = CounselorInHiding->GetMesh();

	if (CounselorMesh)
	{
		TArray<USceneComponent*> AttachedComponents;
		CounselorMesh->GetChildrenComponents(true, AttachedComponents);
		for (USceneComponent* Comp : AttachedComponents)
		{
			if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Comp))
			{
				MeshComp->SetOwnerNoSee(false);
			}
		}
	}

	SetVignetteIntensity(0.f);

	if (!CounselorInHiding->IsLocallyControlled())
	{
		OccupiedIcon->HideIcon();
	}

	UWorld* World = GetWorld();

	if (!CounselorInHiding->bIsGrabbed)
	{
		bExiting = true;
		if (CounselorMesh)
		{
			if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(CounselorMesh->GetAnimInstance()))
			{
				static const FName NAME_HideExit(TEXT("HideExit"));
				AnimInst->Montage_JumpToSection(NAME_HideExit);
				if (UAnimMontage* CurrentMontage = AnimInst->GetCurrentActiveMontage())
				{
					const FAnimMontageInstance* MontageInstance = AnimInst->GetActiveInstanceForMontage(CurrentMontage);
					const float ExitLength = CurrentMontage->GetSectionLength(CurrentMontage->GetSectionIndex(NAME_HideExit)) / MontageInstance->GetPlayRate();
					if (World)
					{
						World->GetTimerManager().SetTimer(TimerHandle_Exit, FTimerDelegate::CreateUObject(this, &ThisClass::ExitCompleted), ExitLength, false);

						// Make sure we get our camera back when our movement returns
						const float BlendTime = CurrentMontage->BlendOut.GetBlendTime();
						World->GetTimerManager().SetTimer(TimerHandle_ReturnCamera,
							FTimerDelegate::CreateUObject(this, &ThisClass::ReturnCameraToCounselorOnExit, CounselorInHiding),
							ExitLength - BlendTime - CAMERA_BLEND_BACK_TIME,
							false);
					}
				}
			}
		}

		if (World)
		{
			// We failed to set the timer, make sure we don't get stuck in the hide spot
			if (!World->GetTimerManager().IsTimerActive(TimerHandle_Exit))
			{
				// Try to set our end location
				if (Role == ROLE_Authority)
				{
					if (USCSpacerCapsuleComponent* SpacerComponent = GetExitSpacer(Interactor))
						CounselorInHiding->SetActorLocation(SpacerComponent->GetComponentLocation());
				}

				// Make sure we're not left playing an animation
				if (CounselorMesh)
				{
					if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(CounselorMesh->GetAnimInstance()))
					{
						AnimInst->Montage_Stop(0.f);
					}
				}

				// We out!
				ExitCompleted();
			}
		}
	}
	else
	{
		CancelHiding();
	}
}

void ASCHidingSpot::ExitCompleted()
{
	// Don't exit twice
	if (!bExiting)
		return;

	bExiting = false;

	// No longer hiding in here, allow net culling
	bAlwaysRelevant = false;
	Mesh->bNoSkeletonUpdate = true;

	ResetBlockers();

	SetActorTickEnabled(false);

	bEnteredFromMirrorSide = false;

	if (!CounselorInHiding)
		return;

	if (CounselorInHiding->IsLocallyControlled())
		CounselorInHiding->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	if (CounselorInHiding->Controller && bForceExitCameraControlRotation)
	{
		FRotator NewControlRot = OnExitControlRotation->GetComponentRotation();
		NewControlRot.Roll = 0.f;
		CounselorInHiding->Controller->SetControlRotation(NewControlRot);
	}

	CounselorInHiding->MoveIgnoreActorRemove(this);

	CounselorInHiding->FinishedExitingHiding();

	SetCounselorInHiding(nullptr);
}

void ASCHidingSpot::ReturnCameraToCounselorOnExit(ASCCounselorCharacter* Counselor)
{
	if (IsValid(Counselor))
	{
		if (!Counselor->bIsGrabbed)
			Counselor->ReturnCameraToCharacter(CAMERA_BLEND_BACK_TIME, VTBlend_EaseInOut, 2.f, true, !bForceExitCameraControlRotation);

		// Done specifically here so that we don't update our control rotation after starting to return the camera to the player
		SetActorTickEnabled(false);
	}
}

void ASCHidingSpot::ForceControlRotation(ASCPlayerController* LocalController, FRotator InRotation)
{
	if (LocalController)
	{
		LocalController->SetControlRotation(InRotation);
	}
}

void ASCHidingSpot::SetVignetteIntensity(float InIntensity)
{
	HidingCamera->PostProcessSettings.VignetteIntensity = InIntensity;
}

void ASCHidingSpot::MULTICAST_SetVignetteIntensity_Implementation(float InIntensity)
{
	SetVignetteIntensity(InIntensity);
}

FVector ASCHidingSpot::GetItemDropLocation(int32 Index /*= -1*/, float ZOffset /*= 0.f*/)
{
	FVector Location = ItemDropLocationLarge->GetComponentLocation();

	switch(Index)
	{
	case 0:
		Location = ItemDropLocationSmall01->GetComponentLocation();
		break;

	case 1:
		Location = ItemDropLocationSmall02->GetComponentLocation();
		break;

	case 2:
		Location = ItemDropLocationSmall03->GetComponentLocation();
		break;

	default:
		return Location;
	}

	if (UWorld* World = GetWorld())
	{
		FCollisionQueryParams CollisionParams(FName(TEXT("ItemDropTrace")));
		CollisionParams.IgnoreMask = ECC_Pawn | ECC_PhysicsBody;
		CollisionParams.AddIgnoredActor(this);
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Location + FVector::UpVector * 250.f, Location + FVector::UpVector * -250.f, ECollisionChannel::ECC_WorldStatic, CollisionParams))
		{
			return Hit.Location + FVector::UpVector * ZOffset;
		}
	}

	return Location;
}

FVector ASCHidingSpot::GetEntranceLocation() const
{
	return EnterSpecialMove->GetComponentLocation();
}

void ASCHidingSpot::NativeDestroyHidingSpot(FAnimNotifyData NotifyData)
{
	bDestroyed = true;
	OnRep_Destroyed();
}

void ASCHidingSpot::OnRep_CounselorInHiding()
{
	// OnRep should only happen on initial only (this is to handle mid-game joining)
	SetCounselorInHiding(CounselorInHiding);
}

void ASCHidingSpot::SetCounselorInHiding(ASCCounselorCharacter* InCounselor)
{
	CounselorInHiding = InCounselor;
	if (CounselorInHiding)
	{
		SetActorTickEnabled(true);

		if (bIgnoreCollisionWhileHiding)
			CounselorInHiding->MoveIgnoreActorAdd(this);

		// Root motion doesn't always get us where we want to be, so make sure we snap into our hiding spot (if set)
		if (!CounselorHidingSpotLocation->GetRelativeTransform().GetTranslation().IsNearlyZero())
			CounselorInHiding->SetActorLocation(CounselorHidingSpotLocation->GetComponentLocation());
	}
}

bool ASCHidingSpot::TestBlockers_Implementation(ASCCounselorCharacter* Counselor)
{
	return false;
}

void ASCHidingSpot::ResetBlockers_Implementation()
{
}
