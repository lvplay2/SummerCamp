// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCContextKillComponent.h"

#include "Animation/AnimMontage.h"

#include "ILLGameBlueprintLibrary.h"

#include "SCAfterimage.h"
#include "SCContextKillActor.h"
#include "SCContextKillDamageType.h"
#include "SCCounselorCharacter.h"
#include "SCGameMode.h"
#include "SCInGameHUD.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"
#include "SCPlayerState.h"
#include "SCSpectatorPawn.h"
#include "SCWeapon.h"

#include "MapErrors.h"
#include "MessageLog.h"
#include "UObjectToken.h"

USCContextKillComponent::USCContextKillComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bIsGrabKill(true)
, bIsEnabled(true)
, bSnapToGround(true)
, SnapToGroundDistance(50.f)
, bDropCounselor(true)
, bDropCounselorAtEnd(true)
, AttachVictimToKiller(true)
, bAttachToHand(false)
{
	SetIsReplicated(true);
}

void USCContextKillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USCContextKillComponent, bIsKillActive);
}

#if WITH_EDITOR
void USCContextKillComponent::CheckForErrors()
{
	Super::CheckForErrors();

	// Not enabled, don't bother error checking
	if (!bIsEnabled)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	UClass* KillerClass = FindObject<UClass>(ANY_PACKAGE, TEXT("/Game/Characters/Killers/Jason/Base/Jason_Base.Jason_Base_C"));
	ASCKillerCharacter* DefaultKiller = KillerClass ? KillerClass->GetDefaultObject<ASCKillerCharacter>() : nullptr;
	if (!DefaultKiller)
		return;

	// Helpers
	const AActor* OwningActor = GetOwner();
	const float Epsilon = 0.15f;

	// No capsule? No special move.
	if (const UCapsuleComponent* PlayerCapsule = DefaultKiller->GetCapsuleComponent())
	{
		// Adjust transform to match the capsule size
		FTransform ComponentTransform = GetComponentTransform();
		ComponentTransform.SetLocation(GetDesiredLocation() + FVector::UpVector * (PlayerCapsule->GetScaledCapsuleHalfHeight() + 5.f)); // Snap location + half capsule height + 5 cm

		static const FName NAME_SpecialMoveEncroached(TEXT("SpecialMoveEncroached"));
		FCollisionQueryParams Params(NAME_SpecialMoveEncroached, false, OwningActor);
		FCollisionResponseParams ResponseParams;
		PlayerCapsule->InitSweepCollisionParams(Params, ResponseParams);
		const ECollisionChannel BlockingChannel = PlayerCapsule->GetCollisionObjectType();
		const FCollisionShape CollisionShape = PlayerCapsule->GetCollisionShape(-Epsilon);

		// Check for overlaps
		if (World->OverlapBlockingTestByChannel(ComponentTransform.GetLocation(), ComponentTransform.GetRotation(), BlockingChannel, CollisionShape, Params, ResponseParams))
		{
			// Someone is blocking this
			static const FName NAME_ContextKillBlocked(TEXT("ContextKillBlocked"));
			FMessageLog MapCheck("MapCheck");
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("ActorName"), FText::FromString(OwningActor->GetName() + TEXT(" : ") + GetName()));
			MapCheck.Warning()
				->AddToken(FUObjectToken::Create(OwningActor))
				->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("Context kill move {ActorName} is encroached and cannot be performed.")), Arguments)))
				->AddToken(FMapErrorToken::Create(NAME_ContextKillBlocked));
		}
	}
}
#endif // WITH_EDITOR

void USCContextKillComponent::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	if (bSimulateInEditor)
	{
		BeginSimulation();
	}
#endif
}

ASCCharacter* USCContextKillComponent::GetLocalInteractor() const
{
	if (Killer && Killer->IsLocallyControlled())
	{
		return Killer;
	}

	if (Victim && Victim->IsLocallyControlled())
	{
		return Victim;
	}

	return nullptr;
}

int32 USCContextKillComponent::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (bIsKillActive || !bIsEnabled)
		return 0;

	if (WeaponRequirements.Num() > 0)
	{
		bool bWeaponFound = false;
		if (ASCCharacter* SCCharacter = Cast<ASCCharacter>(Character))
		{
			if (ASCWeapon* CharacterWeapon = SCCharacter->GetCurrentWeapon())
			{
				for (TSubclassOf<ASCWeapon> Weapon : WeaponRequirements)
				{
					if (CharacterWeapon->IsA(Weapon))
					{
						bWeaponFound = true;
						break;
					}
				}
			}
		}

		if (!bWeaponFound)
			return 0;
	}

	if (!bIsGrabKill)
	{
		// if this is a counselor only the hunter can do kill stuff.
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
		{
			if (!Counselor->bIsHunter)
			{
				return 0;
			}
		}
		return (int32)EILLInteractMethod::Press;
	}

	if (ASCKillerCharacter* KillerCharacter = Cast<ASCKillerCharacter>(Character))
	{
		if (KillerCharacter->GetGrabbedCounselor() && KillerCharacter->CanGrabKill())
		{
			return (int32)EILLInteractMethod::Press;
		}
	}

	return 0;
}

bool USCContextKillComponent::SERVER_ActivateContextKill_Validate(ASCCharacter* InKiller, ASCCharacter* InVictim)
{
	return true;
}

void USCContextKillComponent::SERVER_ActivateContextKill_Implementation(ASCCharacter* InKiller, ASCCharacter* InVictim)
{
	// Crashes rarely on dedicated servers
	if (!IsValid(InKiller) || !IsValid(InVictim))
		return;

	// if either one is in a special move we can't start a new one. ESCAPE!
	if (InKiller->IsInSpecialMove() || InVictim->IsInSpecialMove())
	{
		UE_LOG(LogSC, Warning, TEXT("[SERVER_ActivateContextKill] FAILED - KillerSM? %d - VictimSM? %d"), InKiller->IsInSpecialMove(), InVictim->IsInSpecialMove());
		return;
	}

	MULTICAST_ActivateContextKill(InKiller, InVictim);
}

void USCContextKillComponent::MULTICAST_ActivateContextKill_Implementation(ASCCharacter* InKiller, ASCCharacter* InVictim)
{
	if (!InKiller || !InVictim)
		return;

	// Begin the killer special move locally only and set Our references and delegates
	InKiller->StartSpecialMove(GetKillerSpecialMoveClass(InKiller), InKiller, true);
	if (USCSpecialMove_SoloMontage* SM = Cast<USCSpecialMove_SoloMontage>(InKiller->GetActiveSpecialMove()))
		SetKillerAndSpecialMove(InKiller, SM);

	// Begin the victim's special move locally only and set our references and delegates
	InVictim->StartSpecialMove(GetVictimSpecialMoveClass(InKiller), InVictim, true);
	if (USCSpecialMove_SoloMontage* SM = Cast<USCSpecialMove_SoloMontage>(InVictim->GetActiveSpecialMove()))
		SetVictimAndSpecialMove(InVictim, SM);
}

void USCContextKillComponent::ActivateGrabKill(ASCKillerCharacter* InKiller)
{
	if (InKiller && InKiller->GetGrabbedCounselor())
	{
		SERVER_ActivateContextKill(InKiller, InKiller->GetGrabbedCounselor());
	}
}

TSubclassOf<USCSpecialMove_SoloMontage> USCContextKillComponent::GetVictimSpecialMoveClass(const ASCCharacter* InWeaponHolder /* = nullptr*/) const
{
	if (InWeaponHolder != nullptr && bPlayWeaponSpecificKills)
	{
		if (const ASCWeapon* Weapon = InWeaponHolder->GetCurrentWeapon())
		{
			for (const FWeaponSpecificSpecial& WeaponSpecial : VictimWeaponKillClasses)
			{
				if (Weapon->IsA(WeaponSpecial.WeaponClass))
				{
					return WeaponSpecial.SpecialMove;
				}
			}
		}
	}

	return VictimSpecialClass;
}

TSubclassOf<USCSpecialMove_SoloMontage> USCContextKillComponent::GetKillerSpecialMoveClass(const ASCCharacter* InWeaponHolder /* = nullptr*/) const
{
	if (InWeaponHolder != nullptr && bPlayWeaponSpecificKills)
	{
		if (ASCWeapon* Weapon = InWeaponHolder->GetCurrentWeapon())
		{
			for (FWeaponSpecificSpecial WeaponSpecial : KillerWeaponKillClasses)
			{
				if (Weapon->IsA(WeaponSpecial.WeaponClass))
				{
					return WeaponSpecial.SpecialMove;
				}
			}
		}
	}

	return InteractorSpecialClass;
}

void USCContextKillComponent::BeginAction()
{
	if (++InteractorsInPosition > 1)
	{
		SyncBeginAction();
	}
}

void USCContextKillComponent::EndAction()
{
	// handle killer end
	if (Killer)
	{
		if (bUseSpecificCollisionSettings)
			Killer->GetCapsuleComponent()->SetCollisionResponseToChannels(StoredKillerCollision);

		if (bIgnoreGravity)
			Killer->SetGravityEnabled(true);

		if (ASCKillerCharacter* KillerCharacter = Cast<ASCKillerCharacter>(Killer))
		{
			if (bDropCounselorAtEnd || !Victim || !Victim->IsAlive()) // Victim could have disconnected during a vehicle grab
			{
				// Only need to drop on the server
				if (GetOwnerRole() == ROLE_Authority)
					KillerCharacter->MULTICAST_DropCounselor(false);
			}
			else
			{
				// HACK HACK HACK!!!
				KillerCharacter->GrabValue = 1.f;
				// HACK HACK HACK!!!
			}
		}

		if (bHideWeapon && Killer->GetCurrentWeapon())
		{
			Killer->GetCurrentWeapon()->SetActorHiddenInGame(false);
		}

		Killer->ToggleInput(true);
		if (Killer->Controller)
			Killer->Controller->SetControlRotation(Killer->GetActorRotation());
		Killer->ReturnCameraToCharacter(0.75f, VTBlend_EaseInOut, 2.f);
		Killer->MoveIgnoreActorRemove(GetOwner());
		Killer->HideHUD(false);

		// Make sure we unlock the interaction
		if (Killer->GetInteractableManagerComponent())
			Killer->GetInteractableManagerComponent()->UnlockInteraction(Killer->GetInteractableManagerComponent()->GetLockedInteractable());

		Killer->ContextKillEnded();
	}

	// Handle Counselor end... KILL THEM ALL!!!
	if (Victim)
	{
		if (bUseSpecificCollisionSettings)
			Victim->GetCapsuleComponent()->SetCollisionResponseToChannels(StoredVictimCollision);

		if (bIgnoreGravity)
			Victim->SetGravityEnabled(true);

		if (GetOwnerRole() == ROLE_Authority)
		{
			Victim->CLIENT_SetSkeletonToGameplayMode();
		}

		if (const USCSpecialMove_SoloMontage* const SM = Cast<USCSpecialMove_SoloMontage>(GetVictimSpecialMoveClass(Killer)->GetDefaultObject()))
		{
			if (SM->DieOnFinish())
			{
				if (GetOwnerRole() == ROLE_Authority)
				{
					Victim->ForceKillCharacter(Killer, FDamageEvent(KillInfo));
				}
			}
			else
			{
				if (Victim->IsLocallyControlled())
				{
					Victim->ReturnCameraToCharacter(2.5f, VTBlend_Cubic, 2.f);
				}

				Victim->ContextKillEnded();
			}
		}

		Victim->ToggleInput(true);
		Victim->HideHUD(false);
	}

	// If we're always relevant and set to play for everyone we need to tell them they're no longer in a context kill.
	if (GetOwner()->bAlwaysRelevant && bPlayForEveryone && !GetLocalInteractor())
	{
		if (UWorld* World = GetWorld())
		{
			if (ASCPlayerController* LocalController = Cast<ASCPlayerController>(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(World)))
			{
				ASCCharacter* LocalInteractor = Cast<ASCCharacter>(LocalController->GetPawn());

				// Don't double return our camera
				if (LocalInteractor != Killer && LocalInteractor != Victim)
				{
					// Safety check for dedicated servers
					if (LocalInteractor && LocalInteractor->IsLocallyControlled())
					{
						LocalInteractor->HideHUD(false);
						LocalInteractor->ReturnCameraToCharacter(2.5f, VTBlend_Cubic, 2.f);
						LocalInteractor->ContextKillEnded();
					}
				}
			}
		}
	}

	// Don't double complete
	if (KillerSpecialMove)
	{
		KillerSpecialMove->RemoveCompletionDelegates(this);
		KillerSpecialMove = nullptr;
	}

	if (VictimSpecialMove)
	{
		VictimSpecialMove->RemoveCompletionDelegates(this);
		VictimSpecialMove = nullptr;
	}

	InteractorsInPosition = 0;

	if (Killer && Victim)
		KillComplete.Broadcast(Killer, Victim);

	Killer = nullptr;
	Victim = nullptr;

	bIsKillActive = false;

	if (ASCContextKillActor* ContextKillActor = Cast<ASCContextKillActor>(GetOwner()))
	{
		ContextKillActor->Tick(0.f);
	}

	// Turn off our cameras, the kill is over, nothing to see here anymore
	DisableChildCameras();
}

void USCContextKillComponent::SyncBeginAction()
{
	// Snap actor locations to anim root.
	if (!Killer || !Victim)
	{
		UE_LOG(LogSC, Warning, TEXT("[SyncBeginAction] FAILED - Killer: %s - Victim: %s"), Killer ? *Killer->GetName() : TEXT("NULL"), Victim ? *Victim->GetName() : TEXT("NULL"));
		return;
	}

	bIsKillActive = true;

	KillStarted.Broadcast(Killer, Victim);

	// Setup collision responses
	if (bUseSpecificCollisionSettings)
	{
		StoredKillerCollision = Killer->GetCapsuleComponent()->GetCollisionResponseToChannels();
		StoredVictimCollision = Victim->GetCapsuleComponent()->GetCollisionResponseToChannels();

		Killer->GetCapsuleComponent()->SetCollisionResponseToChannels(InKillCollisionResponses);
		Victim->GetCapsuleComponent()->SetCollisionResponseToChannels(InKillCollisionResponses);
	}

	if (bIgnoreGravity)
	{
		Killer->SetGravityEnabled(false);
		Victim->SetGravityEnabled(false);
	}

	Killer->HideHUD(true);
	Victim->HideHUD(true);

	// Hide the wiggle minigame
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Victim->GetCharacterController()))
	{
		if (ASCInGameHUD* HUD = PC->GetSCHUD())
		{
			HUD->OnEndWiggleMinigame();
		}
	}

	Killer->StopCameraShakeInstances(UCameraShake::StaticClass());
	Victim->StopCameraShakeInstances(UCameraShake::StaticClass());
	Victim->CLIENT_SetSkeletonToCinematicMode();

	if (bHideWeapon && Killer->GetCurrentWeapon())
	{
		Killer->GetCurrentWeapon()->SetActorHiddenInGame(true);
	}

	Killer->SetActorLocationAndRotation(SnapLocation + FVector::UpVector * Killer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), GetComponentRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	
	ASCKillerCharacter* KillerCharacter = Cast<ASCKillerCharacter>(Killer);
	if (KillerCharacter)
	{
		if (bDropCounselor)
		{
			KillerCharacter->DropCounselor();
			Victim->SetActorLocationAndRotation(SnapLocation + FVector::UpVector * Victim->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), GetComponentRotation(), false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			if (KillerCharacter->GetGrabbedCounselor())
				KillerCharacter->UpdateGrabCapsule(false);
		}
	}

	if (AttachVictimToKiller)
	{
		// We need to handle Counselor's attaching to Jason as a special case.
		if (KillerCharacter)
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Victim))
			{
				if (KillerCharacter->GetGrabbedCounselor() == Counselor)
					Counselor->AttachToKiller(bAttachToHand);
				else
					KillerCharacter->GrabCounselor(Counselor);
			}
		}
		else
		{
			Victim->AttachToCharacter(Killer);
		}
	}

	// Activate camera.
	ActivateKillCam();

	KillDestinationReached.Broadcast(Killer, Victim);

	// resume special move.
	KillerSpecialMove->BeginAction();
	VictimSpecialMove->BeginAction();

	// ApplyScoring
	if (GetOwnerRole() == ROLE_Authority && KillInfo != nullptr)
	{
		if (ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>())
		{
			GM->ContextKillScored(Killer->GetController(), Victim->GetController(), KillInfo.GetDefaultObject());
		}
	}
}

void USCContextKillComponent::SetKillerAndSpecialMove(ASCCharacter* InKiller, USCSpecialMove_SoloMontage* InSpecialMove)
{
	if (!InKiller)
	{
		UE_LOG(LogSC, Warning, TEXT("SetKillerAndSpecialMove : InKiller is nullptr"));
		return;
	}

	if (!InSpecialMove)
	{
		UE_LOG(LogSC, Warning, TEXT("SetKillerAndSpecialMove : InSpecialMove is nullptr"));
		return;
	}

	Killer = InKiller;
	KillerSpecialMove = InSpecialMove;
	Killer->MoveIgnoreActorAdd(GetOwner());
	Killer->MoveIgnoreActorAdd(Victim);

	Killer->ToggleInput(false);
	Killer->ContextKillStarted();

	KillerSpecialMove->DestinationReachedDelegate = DestinationReachedDelegate;
	KillerSpecialMove->CancelSpecialMoveDelegate.AddDynamic(this, &USCContextKillComponent::KillAborted);
	KillerSpecialMove->AddCompletionDelegate(FOnILLSpecialMoveCompleted::FDelegate::CreateUObject(this, &USCContextKillComponent::EndAction));

	FVector Location;
	FRotator Rotation;
	const bool bNeedsToMove = GetKillTransform(Location, Rotation, true);
	KillerSpecialMove->SetDesiredTransform(Location, Rotation, bNeedsToMove);
}

void USCContextKillComponent::SetVictimAndSpecialMove(ASCCharacter* InVictim, USCSpecialMove_SoloMontage* InSpecialMove)
{
	if (!InVictim)
	{
		UE_LOG(LogSC, Warning, TEXT("SetVictimAndSpecialMove : InVictim is nullptr"));
		return;
	}

	if (!InSpecialMove)
	{
		UE_LOG(LogSC, Warning, TEXT("SetVictimAndSpecialMove : InSpecialMove is nullptr"));
		return;
	}

	Victim = InVictim;
	VictimSpecialMove = InSpecialMove;
	Victim->MoveIgnoreActorAdd(GetOwner());
	Victim->MoveIgnoreActorAdd(Killer);

	Victim->ToggleInput(false);
	Victim->ContextKillStarted();

	VictimSpecialMove->DestinationReachedDelegate = DestinationReachedDelegate;
	VictimSpecialMove->CancelSpecialMoveDelegate.AddDynamic(this, &USCContextKillComponent::KillAborted);
	VictimSpecialMove->AddCompletionDelegate(FOnILLSpecialMoveCompleted::FDelegate::CreateUObject(this, &USCContextKillComponent::EndAction));

	FVector Location;
	FRotator Rotation;
	bool NeedsToMove = GetKillTransform(Location, Rotation, false);
	VictimSpecialMove->SetDesiredTransform(Location, Rotation, NeedsToMove);
}

void USCContextKillComponent::DisableChildCameras()
{
	TArray<USceneComponent*> ChildrenComponents;
	GetChildrenComponents(true, ChildrenComponents);
	for (USceneComponent* Comp : ChildrenComponents)
	{
		if (UCameraComponent* Camera = Cast<UCameraComponent>(Comp))
		{
			Camera->SetActive(false);
		}
	}
}

USCSplineCamera* USCContextKillComponent::GetSplineCamera() const
{
	TArray<USceneComponent*> ChildComponents;
	GetChildrenComponents(true, ChildComponents);

	TArray<USCSplineCamera*> FoundSplineCameras;

	// Find all of the spline cameras on the kill
	for (USceneComponent* Component : ChildComponents)
	{
		if (USCSplineCamera* SplineCam = Cast<USCSplineCamera>(Component))
		{
				FoundSplineCameras.Add(SplineCam);
		}
	}

	// See if we should be playing a specific camera set
	TArray<USCSplineCamera*> WeaponSpecificCameras;
	if (Killer != nullptr && bPlayWeaponSpecificKills)
	{
		if (const ASCWeapon* Weapon = Killer->GetCurrentWeapon())
		{
			for (const FWeaponSpecificSpecial& WeaponSpecial : KillerWeaponKillClasses)
			{
				if (Weapon->IsA(WeaponSpecial.WeaponClass))
				{
					if (WeaponSpecial.KillCameraNames.Num() == 0)
						break;

					for (const FName& CamName : WeaponSpecial.KillCameraNames)
					{
						for (USCSplineCamera* Camera : FoundSplineCameras)
						{
							const FName SplineCamName = Camera->GetFName();
							if (SplineCamName == CamName)
								WeaponSpecificCameras.Add(Camera);
						}
					}
					break;
				}
			}
		}

		if (WeaponSpecificCameras.Num() > 0)
			FoundSplineCameras = WeaponSpecificCameras;
	}

	// Cameras sorted by best match to worst <dot product, camera>
	TMap<float, USCSplineCamera*> SortedSplineCameras;
	if (const ASCCharacter* LocalInteractor = GetLocalInteractor())
	{
		const FRotator ControlRotation = LocalInteractor->GetControlRotation();
		for (USCSplineCamera* Camera : FoundSplineCameras)
		{
			float dot = ControlRotation.Vector() | Camera->GetFirstFrameRotation().Vector();
			SortedSplineCameras.Add(dot, Camera);
		}
	}
	// We have no local interactor, doesn't matter what angle so send our first camera and call it good.
	else
	{
		return FoundSplineCameras.Num() > 0 ? FoundSplineCameras[0] : nullptr;
	}

	// Sort by dot product descending
	SortedSplineCameras.KeySort([](float A, float B) -> bool { return B < A; });

	// Store off the absolute best angle just in case no cameras return valid.
	USCSplineCamera* DefaultCamera = nullptr;

	// Test validity of best case first
	for (const auto& CameraPair : SortedSplineCameras)
	{
		if (!DefaultCamera)
			DefaultCamera = CameraPair.Value;

		if (CameraPair.Value->IsCameraValid(nullptr))
			return CameraPair.Value;
	}

	// No valid cameras found just send the best rotation match possible
	return DefaultCamera;
}

void USCContextKillComponent::ActivateKillCam()
{
	// Disable the cameras and activate the chosen cam.
	DisableChildCameras();
	ASCCharacter* LocalInteractor = GetLocalInteractor();
	ASCSpectatorPawn* LocalSpectator = nullptr;

	// If we're always relevant and want to play for everyone and there's no local interactor that means this is someone else's controller, GRAB 'EM!
	if (GetOwner()->bAlwaysRelevant && bPlayForEveryone && !LocalInteractor)
	{
		if (ASCPlayerController* LocalController = Cast<ASCPlayerController>(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld())))
		{
			LocalInteractor = Cast<ASCCharacter>(LocalController->GetPawn());

			// Safety check for dedicated servers
			if (LocalInteractor && !LocalInteractor->IsLocallyControlled())
				return;

			// Clear any current interaction and mark that the context kill is started.
			if (LocalInteractor)
			{
				LocalInteractor->ClearAnyInteractions();
				LocalInteractor->HideHUD(true);
				LocalInteractor->ContextKillStarted();
			}

			// Make sure spectators are grabbed as well
			if (!LocalInteractor && LocalController->IsLocalController())
			{
				LocalController->SetViewTarget(GetOwner());
				if (USCSplineCamera* SplineCam = GetSplineCamera())
				{
					if (bDisableAllActorCams)
						MakeOnlyActiveCamera(GetOwner(), SplineCam);
					else
						SplineCam->SetActive(true);

					if (GetContextCameraFocus.IsBound())
						SplineCam->ActivateCamera(GetContextCameraFocus.Execute(), LocalInteractor);
					else
						SplineCam->ActivateCamera(nullptr, LocalInteractor);
				}
				return;
			}
		}
	}

	if (LocalInteractor)
	{
		if (USCSplineCamera* SplineCam = GetSplineCamera())
		{
			if (bDisableAllActorCams)
				MakeOnlyActiveCamera(GetOwner(), SplineCam);
			else
				SplineCam->SetActive(true);

			if (GetContextCameraFocus.IsBound())
				SplineCam->ActivateCamera(GetContextCameraFocus.Execute(), LocalInteractor);
			else
				SplineCam->ActivateCamera(nullptr, LocalInteractor);
		}

		LocalInteractor->TakeCameraControl(GetOwner());
	}
}

bool USCContextKillComponent::GetKillTransform(FVector& Location, FRotator& Rotation, const bool bForKiller)
{
	SnapLocation = GetDesiredLocation();

	bool bNeedsLocation = false;
	if (bForKiller)
	{
		switch (InteractorPositionRequirement)
		{
		case EContextPositionReqs::LocationOnly:
			Location = SnapLocation;
			Rotation = Killer->GetActorRotation();
			bNeedsLocation = true;
			break;
		case EContextPositionReqs::RotationOnly:
			Location = Killer->GetActorLocation();
			Rotation = GetComponentRotation();
			bNeedsLocation = true;
			break;
		case EContextPositionReqs::LocationAndRotation:
			Location = SnapLocation;
			Rotation = GetComponentRotation();
			bNeedsLocation = true;
			break;
		}
	}
	else
	{
		switch (VictimPositionRequirement)
		{
		case EContextPositionReqs::LocationOnly:
			Location = SnapLocation;
			Rotation = Victim->GetActorRotation();
			bNeedsLocation = true;
			break;
		case EContextPositionReqs::RotationOnly:
			Location = Victim->GetActorLocation();
			Rotation = GetComponentRotation();
			bNeedsLocation = true;
			break;
		case EContextPositionReqs::LocationAndRotation:
			Location = SnapLocation;
			Rotation = GetComponentRotation();
			bNeedsLocation = true;
			break;
		}
	}

	return bNeedsLocation;
}

FVector USCContextKillComponent::GetDesiredLocation() const
{
	FVector DesiredLocation = GetComponentLocation();
	if (bSnapToGround)
	{
		if (UWorld* World = GetWorld())
		{
			static const FName NAME_SnapToGround(TEXT("SpecialMoveSnapToGround"));
			FCollisionQueryParams QueryParams(NAME_SnapToGround);
			QueryParams.AddIgnoredActor(Killer);

			int32 Iterations = 0;
			FHitResult Hit;
			while (World->LineTraceSingleByChannel(Hit, DesiredLocation + FVector::UpVector * SnapToGroundDistance, DesiredLocation - FVector::UpVector * SnapToGroundDistance, ECollisionChannel::ECC_WorldStatic, QueryParams))
			{
				// Don't loop forever
				if (Iterations++ >= 16)
					break;

				if (AActor* HitActor = Hit.Actor.Get())
				{
					if (HitActor->IsA(APawn::StaticClass()))
					{
						QueryParams.AddIgnoredActor(HitActor);
						continue;
					}
				}

				DesiredLocation = Hit.Location;
				break;
			}

#if WITH_EDITOR
			if (!Hit.bBlockingHit || DesiredLocation != Hit.Location)
			{
				FMessageLog("PIE").Warning()
					->AddToken(FUObjectToken::Create(GetOwner()))
					->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT(" failed to snap to ground, consider a larger range than %.2f"), SnapToGroundDistance))));
			}
#endif
		}
	}

	return DesiredLocation;
}

void USCContextKillComponent::KillAborted()
{
	// Clean up whatever is set when a kill starts
	if (bIsKillActive)
	{
		if (Killer)
		{
			if (bUseSpecificCollisionSettings)
				Killer->GetCapsuleComponent()->SetCollisionResponseToChannels(StoredKillerCollision);

			if (bIgnoreGravity)
				Killer->SetGravityEnabled(true);

			if (bHideWeapon && Killer->GetCurrentWeapon())
			{
				Killer->GetCurrentWeapon()->SetActorHiddenInGame(false);
			}

			if (Killer->Controller)
				Killer->Controller->SetControlRotation(Killer->GetActorRotation());
			Killer->ReturnCameraToCharacter(0.75f, VTBlend_EaseInOut, 2.f);
			Killer->HideHUD(false);

			// Make sure we unlock the interaction
			Killer->GetInteractableManagerComponent()->UnlockInteraction(Killer->GetInteractableManagerComponent()->GetLockedInteractable());
		}

		if (Victim)
		{
			if (bUseSpecificCollisionSettings)
				Victim->GetCapsuleComponent()->SetCollisionResponseToChannels(StoredVictimCollision);

			if (bIgnoreGravity)
				Victim->SetGravityEnabled(true);

			Victim->CLIENT_SetSkeletonToGameplayMode();
			Victim->HideHUD(false);

			if (Victim->IsLocallyControlled())
			{
				Victim->ReturnCameraToCharacter(2.5f, VTBlend_Cubic, 2.f);
			}
		}
	}

	// Cleanup things that get set, regardless of if the kill starts
	if (Killer)
	{
		Killer->MoveIgnoreActorRemove(GetOwner());

		// If we dropped the character, turn collision back on
		if (bDropCounselor)
			Killer->MoveIgnoreActorRemove(Victim);

		Killer->ToggleInput(true);
		Killer->ContextKillEnded();
	}

	if (Victim)
	{
		Victim->MoveIgnoreActorRemove(GetOwner());

		// If we dropped the character, turn collision back on
		if (bDropCounselor)
			Victim->MoveIgnoreActorRemove(Killer);

		Victim->ToggleInput(true);
		Victim->ContextKillEnded();
	}

	// If we're always relevant and set to play for everyone we need to tell them thay're no longer in a context kill.
	if (GetOwner()->bAlwaysRelevant && bPlayForEveryone && !GetLocalInteractor())
	{
		if (ASCPlayerController* LocalController = Cast<ASCPlayerController>(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld())))
		{
			ASCCharacter* LocalInteractor = Cast<ASCCharacter>(LocalController->GetPawn());

			// Don't double return our camera
			if (LocalInteractor != Killer && LocalInteractor != Victim)
			{
				// Safety check for dedicated servers
				if (LocalInteractor && LocalInteractor->IsLocallyControlled())
				{
					LocalInteractor->ReturnCameraToCharacter(2.5f, VTBlend_Cubic, 2.f);
					LocalInteractor->ContextKillEnded();
				}
			}
		}
	}

	if (KillerSpecialMove)
	{
		// Prevent recursively calling this function
		KillerSpecialMove->CancelSpecialMoveDelegate.RemoveAll(this);
		KillerSpecialMove->CancelSpecialMove();
		KillerSpecialMove = nullptr;
	}

	if (VictimSpecialMove)
	{
		// Prevent recursively calling this function
		VictimSpecialMove->CancelSpecialMoveDelegate.RemoveAll(this);
		VictimSpecialMove->CancelSpecialMove();
		VictimSpecialMove = nullptr;
	}

	InteractorsInPosition = 0;

	if (Killer && Victim)
	{
		if (KillAbort.IsBound())
			KillAbort.Broadcast(Killer, Victim);
		else
			KillComplete.Broadcast(Killer, Victim);
	}

	Killer = nullptr;
	Victim = nullptr;

	bIsKillActive = false;

	if (ASCContextKillActor* ContextKillActor = Cast<ASCContextKillActor>(GetOwner()))
	{
		ContextKillActor->Tick(0.f);
	}
}

// Simulation ----------------------------------------------------------
void USCContextKillComponent::BeginSimulation()
{
	if (!GetOwner()->GetWorldTimerManager().IsTimerActive(TimerHandle_Simulate))
	{
		GetOwner()->GetWorldTimerManager().SetTimer(TimerHandle_Simulate, this, &USCContextKillComponent::BeginSimulation, 2.f, false);
		return;
	}

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		KillerMesh = NewObject<USkeletalMeshComponent>(GetOwner(), USkeletalMeshComponent::StaticClass(), "killerPreview");
		VictimMesh = NewObject<USkeletalMeshComponent>(GetOwner(), USkeletalMeshComponent::StaticClass(), "VictimPreview");

		KillerMesh->RegisterComponent();
		VictimMesh->RegisterComponent();

		KillerMesh->SetSkeletalMesh(KillerMeshClass);
		VictimMesh->SetSkeletalMesh(VictimMeshClass);
		KillerSpecialMove = Cast<USCSpecialMove_SoloMontage>(GetKillerSpecialMoveClass()->GetDefaultObject());
		VictimSpecialMove = Cast<USCSpecialMove_SoloMontage>(GetVictimSpecialMoveClass()->GetDefaultObject());
		KillerMesh->SetHiddenInGame(false);
		VictimMesh->SetHiddenInGame(false);
		KillerMesh->SetWorldLocationAndRotation(GetComponentLocation(), GetComponentRotation());
		KillerMesh->AddRelativeRotation(FRotator(0.f, -90.f, 0.f));

		UAnimMontage* KillerMontage = KillerSpecialMove->GetContextMontage();
		KillerMesh->PlayAnimation(KillerMontage, false);

		UAnimMontage* VictimMontage = VictimSpecialMove->GetContextMontage();
		VictimMesh->PlayAnimation(VictimMontage, false);

		VictimMesh->AttachToComponent(KillerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("GrabSocket"));
		VictimMesh->AddRelativeRotation(FRotator(0.f, -90.f, 0.f));
		PC->SetViewTargetWithBlend(GetOwner());

		GetOwner()->GetWorldTimerManager().SetTimer(TimerHandle_Simulate, this, &USCContextKillComponent::Simulate, KillerMontage->GetPlayLength(), false);
	}
}

void USCContextKillComponent::EndSimulation()
{
	KillerSpecialMove = nullptr;
	VictimSpecialMove = nullptr;

	KillerMesh->DestroyComponent();
	KillerMesh = nullptr;

	VictimMesh->DestroyComponent();
	VictimMesh = nullptr;

	GetOwner()->GetWorldTimerManager().ClearTimer(TimerHandle_Simulate);
}

void USCContextKillComponent::Simulate()
{
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		UAnimMontage* KillerMontage = KillerSpecialMove->GetContextMontage();
		KillerMesh->PlayAnimation(KillerMontage, false);
		VictimMesh->PlayAnimation(VictimSpecialMove->GetContextMontage(), false);
		Controller->SetViewTargetWithBlend(GetOwner());
		TArray<USceneComponent*> ChildComponents;
		GetChildrenComponents(true, ChildComponents);

		GetOwner()->GetWorldTimerManager().SetTimer(TimerHandle_Simulate, this, &USCContextKillComponent::Simulate, KillerMontage->GetPlayLength(), false);
	}
}
