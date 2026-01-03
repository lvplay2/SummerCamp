// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSpecialMoveComponent.h"

#include "Animation/AnimMontage.h"

#include "SCContextComponent.h"
#include "SCCounselorCharacter.h"
#include "SCKillerCharacter.h"
#include "SCSplineCamera.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCWeapon.h"

#include "MapErrors.h"
#include "MessageLog.h"
#include "UObjectToken.h"

USCSpecialMoveComponent::USCSpecialMoveComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bClearDelegatesOnFinish(false)
, bTakeCameraControl(true)
, bEnabled(true)
, CurrentInteractor(nullptr)
, CurrentSpecialMove(nullptr)
, BlendInTime(0.f)
, BlendBackTime(0.25f)
, bForceNewCameraRotation(true)
, SnapToGroundDistance(50.f)
{

}

#if WITH_EDITOR
void USCSpecialMoveComponent::CheckForErrors()
{
	Super::CheckForErrors();

	// No move! Nothing to check!
	if (!SpecialMove)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	// Utility to see if this move is even for this character class
	auto CanPlayOnClass = [](const TSubclassOf<UILLSpecialMoveBase>& TestMove, const TSoftClassPtr<ASCCharacter>& CharacterClass) -> UCapsuleComponent*
	{
		CharacterClass.LoadSynchronous(); // TODO: Make Async -- maybe, this is editor only map check code

		ASCCharacter* TestCharacter = Cast<ASCCharacter>(CharacterClass.Get()->GetDefaultObject());
		if (!TestCharacter)
			return nullptr;

		TSubclassOf<UILLSpecialMoveBase> MatchingMove = nullptr;
		for (TSubclassOf<UILLSpecialMoveBase> MoveClass : TestCharacter->GetAllowedSpecialMoves())
		{
			if (MoveClass == TestMove->GetClass())
			{
				MatchingMove = MoveClass;
				break;
			}
			else if (MoveClass->IsChildOf(TestMove))
			{
				MatchingMove = MoveClass;
			}
			else if (TestMove->IsChildOf(MoveClass))
			{
				MatchingMove = TestMove;
				break;
			}
		}

		if (MatchingMove)
			return TestCharacter->GetCapsuleComponent();

		return nullptr;
	};

	// Add any additional character classes here
	TSoftClassPtr<ASCCharacter> TestClasses[] = {
		TSoftClassPtr<ASCCharacter>(FSoftObjectPath(TEXT("/Game/Characters/Counselors/Base/Base_Counselor.Base_Counselor_C"))),
		TSoftClassPtr<ASCCharacter>(FSoftObjectPath(TEXT("/Game/Characters/Killers/Jason/Base/Jason_Base.Jason_Base_C"))),
	};

	// Helpers
	const AActor* OwningActor = GetOwner();
	const float Epsilon = 0.15f;
	FMessageLog MapCheck("MapCheck");
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("ActorName"), FText::FromString(OwningActor->GetName() + TEXT(" : ") + GetName()));

	// Test all classes to make sure they have space
	for (const TSoftClassPtr<ASCCharacter>& TestCharacter : TestClasses)
	{
		// No capsule? No special move.
		if (const UCapsuleComponent* PlayerCapsule = CanPlayOnClass(SpecialMove, TestCharacter))
		{
			// Adjust transform to match the capsule size
			FTransform ComponentTransform = GetComponentTransform();
			ComponentTransform.SetLocation(GetDesiredLocation(nullptr) + FVector::UpVector * (PlayerCapsule->GetScaledCapsuleHalfHeight() + 5.f)); // SnapToGround location + half capsule height + 5cm

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
				static const FName NAME_SpecialMoveBlocked(TEXT("SpecialMoveBlocked"));
				TestCharacter.LoadSynchronous(); // TODO: Make Async -- maybe, this is editor only map check code
				Arguments.Add(TEXT("CharacterClass"), FText::FromString(TestCharacter.Get()->GetName()));
				MapCheck.Warning()
					->AddToken(FUObjectToken::Create(OwningActor))
					->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("Special move {ActorName} is encroached for class {CharacterClass}")), Arguments)))
					->AddToken(FMapErrorToken::Create(NAME_SpecialMoveBlocked));
			}
		}
	}
}
#endif // WITH_EDITOR

void USCSpecialMoveComponent::BeginPlay()
{
	Super::BeginPlay();

	AttachedSplineCameras.Empty();

	TArray<USceneComponent*> ChildComponents;
	GetChildrenComponents(true, ChildComponents);

	// Find all of the spline cameras
	for (USceneComponent* comp : ChildComponents)
	{
		if (USCSplineCamera* SplineCam = Cast<USCSplineCamera>(comp))
		{
			AttachedSplineCameras.Add(SplineCam);
		}
	}
}

void USCSpecialMoveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (CurrentSpecialMove)
	{
		CurrentSpecialMove->DestinationReachedDelegate.RemoveAll(this);
		CurrentSpecialMove->RemoveCompletionDelegates(this);
		CurrentSpecialMove->CancelSpecialMoveDelegate.RemoveAll(this);
		CurrentSpecialMove->SpeedUpAppliedDelegate.RemoveAll(this);
		CurrentSpecialMove = nullptr;
	}
}

void USCSpecialMoveComponent::ActivateSpecial(ASCCharacter* Interactor, const float MontagePlayRate /*= 1.f*/)
{
	BeginSpecial(Interactor, MontagePlayRate);
}

void USCSpecialMoveComponent::BeginSpecial(ASCCharacter* Interactor, const float MontagePlayRate)
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		SERVER_BeginSpecial(Interactor, MontagePlayRate);
	}
	else
	{
		MULTICAST_BeginSpecial(Interactor, MontagePlayRate);
	}
}

bool USCSpecialMoveComponent::SERVER_BeginSpecial_Validate(ASCCharacter* Interactor, const float MontagePlayRate)
{
	return true;
}

void USCSpecialMoveComponent::SERVER_BeginSpecial_Implementation(ASCCharacter* Interactor, const float MontagePlayRate)
{
	BeginSpecial(Interactor, MontagePlayRate);
}

void USCSpecialMoveComponent::MULTICAST_BeginSpecial_Implementation(ASCCharacter* Interactor, const float MontagePlayRate)
{
	if (!Interactor)
		return;

	Interactor->StartSpecialMove(GetDesiredSpecialMove(Interactor), Interactor, true);

	if (USCSpecialMove_SoloMontage* SoloMontage = Cast<USCSpecialMove_SoloMontage>(Interactor->GetActiveSpecialMove()))
	{
		if (CurrentSpecialMove)
		{
			CurrentSpecialMove->DestinationReachedDelegate.RemoveAll(this);
			CurrentSpecialMove->RemoveCompletionDelegates(this);
			CurrentSpecialMove->CancelSpecialMoveDelegate.RemoveAll(this);
			CurrentSpecialMove->SpeedUpAppliedDelegate.RemoveAll(this);
			CurrentSpecialMove = nullptr;
		}

		SpecialCreated.Broadcast(Interactor);
		CurrentSpecialMove = SoloMontage;
		CurrentInteractor = Interactor;
		CurrentInteractor->MoveIgnoreActorAdd(GetOwner());

		SnapLocation = GetDesiredLocation(Interactor);

		CurrentSpecialMove->SetMontagePlayRate(MontagePlayRate);
		CurrentSpecialMove->SetDesiredTransform(SnapLocation, GetComponentRotation(), CurrentSpecialMove->GetRequiresLocation());
		CurrentSpecialMove->DestinationReachedDelegate.AddDynamic(this, &USCSpecialMoveComponent::NativeDestinationReached);
		CurrentSpecialMove->AddCompletionDelegate(FOnILLSpecialMoveCompleted::FDelegate::CreateUObject(this, &USCSpecialMoveComponent::NativeSpecialComplete));
		CurrentSpecialMove->CancelSpecialMoveDelegate.AddDynamic(this, &USCSpecialMoveComponent::NativeSpecialAborted);
		CurrentSpecialMove->SpeedUpAppliedDelegate.AddDynamic(this, &USCSpecialMoveComponent::NativeSpecialSpedUp);
	}
}

void USCSpecialMoveComponent::NativeDestinationReached()
{
	if (!CurrentSpecialMove)
	{
#if WITH_EDITOR
		FMessageLog("PIE").Error()
			->AddToken(FTextToken::Create(FText::FromString(TEXT("No special move active on "))))
			->AddToken(FUObjectToken::Create(GetOwner()));
#endif
		return;
	}

	if (!CurrentInteractor)
	{
#if WITH_EDITOR
		FMessageLog("PIE").Error()
			->AddToken(FTextToken::Create(FText::FromString(TEXT("Destination reached without an interactor on "))))
			->AddToken(FUObjectToken::Create(GetOwner()));
#endif
		return;
	}

	if (bTakeCameraControl)
		PlayInteractionCamera(CurrentInteractor);

	DestinationReached.Broadcast(CurrentInteractor);

	if (CurrentSpecialMove->GetRequiresLocation())
	{
		const FVector BaseLocation = bSnapToGround ? SnapLocation : GetComponentLocation();
		CurrentInteractor->SetActorLocationAndRotation(BaseLocation + FVector::UpVector * CurrentInteractor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), GetComponentRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (bTakeCameraControl)
		CurrentInteractor->TakeCameraControl(GetOwner(), 0.5, EViewTargetBlendFunction::VTBlend_EaseInOut, 2.0f, true);

	CurrentSpecialMove->BeginAction();
	if (CurrentSpecialMove && !CurrentSpecialMove->IsPendingKill())
		SpecialStarted.Broadcast(CurrentInteractor);
}

void USCSpecialMoveComponent::NativeSpecialComplete()
{
	if (CurrentInteractor)
	{
		if (bTakeCameraControl)
			StopInteractionCamera(CurrentInteractor);

		CurrentInteractor->MoveIgnoreActorRemove(GetOwner());

		// restore mevemont after finishing the special move.
		if (CurrentInteractor->GetController())
			CurrentInteractor->GetController()->SetIgnoreMoveInput(false);
	}

	SpecialComplete.Broadcast(CurrentInteractor);

	CurrentSpecialMove = nullptr;
	CurrentInteractor = nullptr;

	if (bClearDelegatesOnFinish)
	{
		DestinationReached.Clear();
		SpecialStarted.Clear();
		SpecialComplete.Clear();
		SpecialAborted.Clear();
		SpecialSpedUp.Clear();
	}
}

void USCSpecialMoveComponent::NativeSpecialAborted()
{
	if (CurrentInteractor)
	{
		if (bTakeCameraControl)
			StopInteractionCamera(CurrentInteractor);

		CurrentInteractor->MoveIgnoreActorRemove(GetOwner());
	}

	if (SpecialAborted.IsBound())
		SpecialAborted.Broadcast(CurrentInteractor);
	else
		SpecialComplete.Broadcast(CurrentInteractor);

	if (bClearDelegatesOnFinish)
	{
		DestinationReached.Clear();
		SpecialStarted.Clear();
		SpecialComplete.Clear();
		SpecialAborted.Clear();
		SpecialSpedUp.Clear();
	}
}

void USCSpecialMoveComponent::NativeSpecialSpedUp()
{
	SpecialSpedUp.Broadcast(CurrentInteractor);
}

float USCSpecialMoveComponent::GetMontageLength(const ASCCharacter* Interactor) const
{
	if (const USCSpecialMove_SoloMontage* Move = GetDesiredSpecialMove(Interactor).GetDefaultObject())
		if (UAnimMontage* Montage = Move->GetContextMontage())
			return Montage->GetPlayLength();

	return 0.f;
}

void USCSpecialMoveComponent::PlayInteractionCamera(ASCCharacter* Interactor)
{
	if (AttachedSplineCameras.Num() <= 0 || !Interactor->IsLocallyControlled())
		return;

	// Cameras sorted by best match to worst <dot product, camera>
	TMap<float, USCSplineCamera*> SortedSplineCameras;
	FRotator ControlRotation = Interactor->GetControlRotation();
	for (USCSplineCamera* Cam : AttachedSplineCameras)
	{
		if (!IsValid(Cam))
			continue;

		float dot = ControlRotation.Vector().GetSafeNormal() | Cam->GetFirstFrameRotation().Vector().GetSafeNormal();
		SortedSplineCameras.Add(dot, Cam);
	}

	// Sort by dot product descending
	SortedSplineCameras.KeySort([](float A, float B) { return B < A; } );

	// Store off the absolute best angle just in case no cameras return valid.
	PlayingCamera = nullptr;

	// Test validity of best case first
	for (auto& Elem : SortedSplineCameras)
	{
		if (!PlayingCamera)
			PlayingCamera = Elem.Value;

		if (Elem.Value->IsCameraValid(nullptr))
		{
			PlayingCamera = Elem.Value;
			break;
		}
	}

	// No valid cameras found just send the best rotation match possible
	if (PlayingCamera)
	{
		MakeOnlyActiveCamera(this->GetOwner(), PlayingCamera);
		PlayingCamera->ActivateCamera(nullptr, Interactor);
		Interactor->TakeCameraControl(this->GetOwner(), BlendInTime, VTBlend_EaseInOut, 2.f, bForceNewCameraRotation);
	}
}

void USCSpecialMoveComponent::StopInteractionCamera(ASCCharacter* Interactor)
{
	Interactor->ReturnCameraToCharacter(BlendBackTime, VTBlend_EaseInOut, 2.0f, true, bForceNewCameraRotation);
	
	// Stop camera anim
	if (PlayingCamera)
	{
		PlayingCamera->DeactivateCamera();
		PlayingCamera = nullptr;
	}
}

TSubclassOf<USCSpecialMove_SoloMontage> USCSpecialMoveComponent::GetDesiredSpecialMove(const ASCCharacter* Interactor) const
{
	// Just want the default or didn't offer up an interactor
	if (bUseWeaponSpecificMoves == false || Interactor == nullptr)
		return SpecialMove;

	// Find our weapon specific move
	if (ASCWeapon* Weapon = Interactor->GetCurrentWeapon())
	{
		for (FWeaponSpecificSpecial WeaponMove : WeaponSpecificMoves)
		{
			if (Weapon->IsA(WeaponMove.WeaponClass))
			{
				return WeaponMove.SpecialMove;
			}
		}
	}

	// No specific weapon animation available, just return the default
	return SpecialMove;
}

FVector USCSpecialMoveComponent::GetDesiredLocation(const AActor* Interactor) const
{
	FVector DesiredLocation = GetComponentLocation();
	if (bSnapToGround)
	{
		if (UWorld* World = GetWorld())
		{
			static const FName NAME_SnapToGround(TEXT("SpecialMoveSnapToGround"));
			FCollisionQueryParams QueryParams(NAME_SnapToGround);
			QueryParams.AddIgnoredActor(Interactor);

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
			if (!Hit.bBlockingHit || SnapLocation != Hit.Location)
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
