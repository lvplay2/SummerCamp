// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLCharacter.h"

#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "AudioThread.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "DisplayDebugHelpers.h"

#include "ILLCharacterMovement.h"
#include "ILLDynamicCameraComponent.h"
#include "ILLGameState.h"
#include "ILLInteractableManagerComponent.h"
#include "ILLInventoryComponent.h"
#include "ILLInventoryItem.h"
#include "ILLPlayerController.h"
#include "ILLPlayerInput.h"
#include "ILLSpecialMoveBase.h"
#include "SoundNodeLocalPlayer.h"

const FName AILLCharacter::Mesh1PComponentName(TEXT("PawnMesh1P"));

bool FILLSkeletonIndependentMontage::CanPlayOnCharacter(const AILLCharacter* Character, const bool bExactMatchOnly/* = false*/, const int32 Index/* = 0*/) const
{
	if (!Class.IsNull() && Character->GetClass() == Class.Get())
	{
		// Exact match
		return true;
	}
	else if (!bExactMatchOnly && (Class.Get() == nullptr || Character->GetClass()->IsChildOf(Class.Get())))
	{
		// Partial match
		return CanPlayOnSkeleton(Character, Index);
	}

	return false;
}

bool FILLSkeletonIndependentMontage::CanPlayOnSkeleton(const AILLCharacter* Character, const int32 Index/* = 0*/) const
{
	if (Index >= Montages.Num())
	{
		return false;
	}

	USkeleton* Skeleton = Character->IsFirstPerson() ? Montages[Index].FirstPerson->GetSkeleton() : Montages[Index].ThirdPerson->GetSkeleton();
	if (Skeleton == Character->GetPawnMesh()->SkeletalMesh->Skeleton)
	{
		return true;
	}

	return false;
}

AILLCharacter::AILLCharacter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UILLCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	Mesh1P = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(Mesh1PComponentName);
	if (Mesh1P)
	{
		Mesh1P->SetupAttachment(GetCapsuleComponent());
		Mesh1P->bReceivesDecals = false;
		Mesh1P->bSelfShadowOnly = true;
		Mesh1P->bLightAttachmentsAsGroup = true;
		Mesh1P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
		Mesh1P->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Mesh1P->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Mesh1P->SetCollisionObjectType(ECC_Pawn);
		Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	GetMesh()->bReceivesDecals = false;
	GetMesh()->bLightAttachmentsAsGroup = true;

	bThirdPersonShadowInFirst = true;
	AspectRatioOverride = -1.f;
	TimeBetweenFidgets = 20.f;
	FidgetTimeRange = 5.f;
	bFidgetPreferItems = true;
	NextFidget =  -1;

	CrouchEyeHeightLerpSpeed = 10.f;
	UnCrouchEyeHeightLerpSpeed = 20.f;
}

void AILLCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Update the USoundNodeLocalPlayer
	if (!GExitPurge)
	{
		const uint32 UniqueID = GetUniqueID();
		FAudioThread::RunCommandOnAudioThread([UniqueID]()
		{
			USoundNodeLocalPlayer::GetLocallyControlledActorCache().Remove(UniqueID);
		});
	}

	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Fidget);
	}

	Super::EndPlay(EndPlayReason);
}

void AILLCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (UWorld* World = GetWorld())
	{
		// Set initial mesh visibility (3rd person view)
		if (!bSkipPostInitializeUpdateVisiblity)
		{
			UpdateCharacterMeshVisibility();
		}

		// Reset the fidget timer
		ResetFidgetTimer();

		// Tell the people
		if (AILLGameState* GameState = World->GetGameState<AILLGameState>())
		{
			GameState->CharacterInitialized(this);
		}
	}
}

void AILLCharacter::Destroyed()
{
	Super::Destroyed();
	
	// Tell the people
	if (UWorld* World = GetWorld())
	{
		if (AILLGameState* GameState = World->GetGameState<AILLGameState>())
		{
			GameState->CharacterDestroyed(this);
		}
	}

	// Destroy the ActiveSpecialMove
	if (ActiveSpecialMove)
	{
		ActiveSpecialMove->MarkPendingKill();
		ActiveSpecialMove = nullptr;
	}
}

void AILLCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Blend our BaseEyeHeight towards the DesiredEyeHeight
	if (!FMath::IsNearlyEqual(BaseEyeHeight, DesiredEyeHeight))
	{
		if (EyeHeightLerpSpeed <= 0.f)
		{
			BaseEyeHeight = DesiredEyeHeight;
		}
		else
		{
			BaseEyeHeight = FMath::LerpStable(BaseEyeHeight, DesiredEyeHeight, FMath::Clamp(DeltaSeconds*EyeHeightLerpSpeed, 0.f, 1.f));
			if (FMath::IsNearlyEqual(BaseEyeHeight, DesiredEyeHeight))
			{
				// Reset this so the next eye height change does not have the exact same lerp
				EyeHeightLerpSpeed = 0.f;
			}
		}
	}

	// Process our current special move
	if (ActiveSpecialMove)
	{
		if (ActiveSpecialMove->IsPendingKill())
		{
			ActiveSpecialMove = nullptr;
		}
		else
		{
			ActiveSpecialMove->TickMove(DeltaSeconds);
		}
	}

	// Update the USoundNodeLocalPlayer
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bLocallyControlled = (PC ? PC->IsLocalController() : false) || IsLocalPlayerViewTarget();
	const uint32 UniqueID = GetUniqueID();
	FAudioThread::RunCommandOnAudioThread([UniqueID, bLocallyControlled]()
	{
	    USoundNodeLocalPlayer::GetLocallyControlledActorCache().Add(UniqueID, bLocallyControlled);
	});
}

void AILLCharacter::CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult)
{
	for (int32 iCamera = 0; iCamera < ActiveCameraStack.Num(); ++iCamera)
	{
		auto& Camera = ActiveCameraStack[iCamera];

		// Pop the camera if the component is not enabled.
		if (!Camera.DynamicCamera->IsActive())
		{
			ActiveCameraStack.RemoveAt(iCamera);
			--iCamera;
		}
	}

	// We're not using Dynamic Cameras, fall back to AActor defaults
	if (ActiveCameraStack.Num() == 0)
	{
		Super::CalcCamera(DeltaTime, OutResult);
	}
	else
	{
		if (ActiveCameraStack.Num() > 1)
		{
			const float CurrentTime = GetWorld()->GetTimeSeconds();
			float TotalWeight = 0.f;

			// Calculate blend deltas
			for (int32 iCamera = 0; iCamera < ActiveCameraStack.Num(); ++iCamera)
			{
				auto& Camera = ActiveCameraStack[iCamera];
				Camera.BlendDelta = FMath::Clamp((CurrentTime - Camera.StartTimestamp) / Camera.BlendLength, 0.f, 1.f);

				// Last camera needs a blend weight
				if (iCamera == ActiveCameraStack.Num() - 1)
				{
					Camera.BlendWeight = Camera.BlendDelta;
					TotalWeight += Camera.BlendWeight;
				}

				// Other camera blend weights are calculated by the next camera up
				if (iCamera > 0)
				{
					auto& PrevCameraBlend = ActiveCameraStack[iCamera - 1];
					PrevCameraBlend.BlendWeight = PrevCameraBlend.BlendDelta - Camera.BlendDelta;

					// Remove dead cameras
					if (PrevCameraBlend.BlendDelta > 0.1f && PrevCameraBlend.BlendWeight <= 0.01f)
					{
						ActiveCameraStack[iCamera - 1].DynamicCamera->Deactivate();
						ActiveCameraStack.RemoveAt(iCamera - 1);
						--iCamera;
					}
					else
					{
						TotalWeight += PrevCameraBlend.BlendWeight;
					}
				}
			}

			// Normalize blend deltas
			for (auto& Camera : ActiveCameraStack)
			{
				Camera.BlendWeightNormalized = FMath::Max(Camera.BlendWeight / TotalWeight, KINDA_SMALL_NUMBER);
			}

			if (ActiveCameraStack.Num() == 1)
			{
				ActiveCameraStack[0].BlendDelta = ActiveCameraStack[0].BlendWeight = ActiveCameraStack[0].BlendWeightNormalized = 1.f;
				ActiveCameraStack[0].DynamicCamera->Camera->GetCameraView(DeltaTime, OutResult);
			}
			else
			{
				// View info intilizes some values, we need all 0s to support blending
				FMinimalViewInfo FullBlendInfo;
				memset(&FullBlendInfo, 0, sizeof(FullBlendInfo));

				for (auto& CameraData : ActiveCameraStack)
				{
					// Update the cache if we still have a camera, otherwise we'll just blend from the cache
					if (CameraData.DynamicCamera)
					{
						CameraData.DynamicCamera->Camera->GetCameraView(DeltaTime, CameraData.CameraInfoCache);
					}

					// Blend all MinimalViewInfo values based on normalized weight
					const float Weight = CameraData.BlendWeightNormalized;
					FullBlendInfo.Location += CameraData.CameraInfoCache.Location * Weight;
					FullBlendInfo.Rotation += CameraData.CameraInfoCache.Rotation * Weight;
					FullBlendInfo.FOV += CameraData.CameraInfoCache.FOV * Weight;
					FullBlendInfo.OrthoWidth += CameraData.CameraInfoCache.OrthoWidth * Weight;
					FullBlendInfo.OrthoNearClipPlane += CameraData.CameraInfoCache.OrthoNearClipPlane * Weight;
					FullBlendInfo.OrthoFarClipPlane += CameraData.CameraInfoCache.OrthoFarClipPlane * Weight;

					FullBlendInfo.AspectRatio += CameraData.CameraInfoCache.AspectRatio * Weight;
					FullBlendInfo.bConstrainAspectRatio |= CameraData.CameraInfoCache.bConstrainAspectRatio;
					FullBlendInfo.bUseFieldOfViewForLOD |= CameraData.CameraInfoCache.bUseFieldOfViewForLOD;
				}

				OutResult = FullBlendInfo;
			}
		}
		else
		{
			// Only one camera, just use it
			ActiveCameraStack[0].DynamicCamera->Camera->GetCameraView(DeltaTime, OutResult);
		}
	}

	if (bOverrideAspectRatio)
	{
		OutResult.bConstrainAspectRatio = bConstrainAspectRatioOverride;

		if (AspectRatioOverride > 0.f)
		{
			OutResult.AspectRatio = AspectRatioOverride;
		}
	}

	CachedCameraInfo = OutResult;
}

void AILLCharacter::SetActiveCamera(UILLDynamicCameraComponent* NewTargetCamera)
{
	if (ActiveCameraStack.Num())
	{
		if (ActiveCameraStack[0].DynamicCamera == NewTargetCamera)
			return;
	}

	// Expecting a smash cut to the new camera, so clear out our blending stack
	for (auto& Camera : ActiveCameraStack)
	{
		Camera.DynamicCamera->Deactivate();
	}

	ActiveCameraStack.Empty();

	if (NewTargetCamera)
	{
		NewTargetCamera->Activate(true);
		FILLCameraBlendInfo Data(NewTargetCamera, GetWorld()->GetTimeSeconds(), 0.f, 1.f);
		Data.BlendWeightNormalized = 1.f;
		ActiveCameraStack.Add(Data);
	}
}

void AILLCharacter::SetActiveCameraWithBlend(UILLDynamicCameraComponent* NewTargetCamera, float BlendTime)
{
	if (ActiveCameraStack.Num() == 1)
	{
		if (ActiveCameraStack[0].DynamicCamera == NewTargetCamera)
		{
			return;
		}
	}

	// No blend time or a NULL camera results in a smash cut
	if (BlendTime <= 0.f || NewTargetCamera == nullptr)
	{
		SetActiveCamera(NewTargetCamera);
		return;
	}

	if (ActiveCameraStack.Num() == 0)
	{
		// Look for a replacement active camera
		UILLDynamicCameraComponent* ActiveCamera = nullptr;
		if (bFindCameraComponentWhenViewTarget)
		{
			TInlineComponentArray<UILLDynamicCameraComponent*> Cameras;
			GetComponents<UILLDynamicCameraComponent>(/*out*/ Cameras);

			for (UILLDynamicCameraComponent* CameraComponent : Cameras)
			{
				if (CameraComponent->bIsActive)
				{
					ActiveCamera = CameraComponent;
					break;
				}
			}
		}

		// No replacement found, results in a hard snap rather than requested blend
		if (ActiveCamera == nullptr)
		{
			FMessageLog("PIE").Warning(FText::FromString(TEXT("SetActiveCameraWithBlend called before SetActiveCamera, snapping to target camera. From: ")))
				->AddToken(FUObjectToken::Create(GetClass()));
			SetActiveCamera(NewTargetCamera);

			return; // <-- Early out
		}

		// We found a replacement, only warn
		FMessageLog("PIE").Info(FText::FromString(FString::Printf(TEXT("SetActiveCameraWithBlend called before SetActiveCamera, using first camera (%s) as active. From: "), *ActiveCamera->GetName())))
			->AddToken(FUObjectToken::Create(GetClass()));
	}

	NewTargetCamera->Activate(true);
	FILLCameraBlendInfo Data(NewTargetCamera, GetWorld()->GetTimeSeconds(), BlendTime);

	// If we already have this camera switch the old reference to just use the cache until it blends out
	for (int32 iCamera = 0; iCamera < ActiveCameraStack.Num(); ++iCamera)
	{
		if (ActiveCameraStack[iCamera].DynamicCamera == NewTargetCamera)
		{
			// We found the camera, reverse the blends for this camera and everything after it.
			TArray<FILLCameraBlendInfo> RevisedCameraStack;

			// Add all cameras that come before the found camera in their proper form.
			for (int32 jCamera = 0; jCamera < iCamera; ++jCamera)
			{
				RevisedCameraStack.Add(ActiveCameraStack[jCamera]);
			}

			RevisedCameraStack.Add(FILLCameraBlendInfo(ActiveCameraStack[ActiveCameraStack.Num() - 1].DynamicCamera, 0.f, 0.f, 1.f));

			// We've already added the final camera to our stack so skip that one.
			for (int32 kCamera = ActiveCameraStack.Num() - 2; kCamera >= iCamera; --kCamera)
			{
				float newTime = GetWorld()->GetTimeSeconds() - ActiveCameraStack[kCamera + 1].BlendLength * (1 - ActiveCameraStack[kCamera + 1].BlendDelta);

				FILLCameraBlendInfo Reverse(ActiveCameraStack[kCamera].DynamicCamera, newTime, ActiveCameraStack[kCamera + 1].BlendLength);

				RevisedCameraStack.Add(Reverse);
			}

			ActiveCameraStack = RevisedCameraStack;
			return;
		}
	}

	ActiveCameraStack.Add(Data);
}

void AILLCharacter::RecalculateBaseEyeHeight()
{
	const float OldBaseEyeHeight = BaseEyeHeight;

	Super::RecalculateBaseEyeHeight();

	// Store off the BaseEyeHeight as the DesiredEyeHeight for interpolation
	DesiredEyeHeight = BaseEyeHeight;
	BaseEyeHeight = OldBaseEyeHeight;
}

void AILLCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	OnReceivedController();
}

void AILLCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	OnReceivedPlayerState();
}

void AILLCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	OnReceivedController();
	OnReceivedPlayerState();
}

void AILLCharacter::OnReceivedController()
{
}

void AILLCharacter::OnReceivedPlayerState()
{
}

bool AILLCharacter::IsUsingGamepad() const
{
	if (AILLPlayerController* PlayerController = Cast<AILLPlayerController>(Controller))
	{
		// Only local players can be using a gamepad (may be AI)
		if (!PlayerController->IsLocalController())
			return false;

		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PlayerController->PlayerInput))
			return Input->IsUsingGamepad();
	}

	return false;
}

void AILLCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("ToggleThirdPerson", IE_Pressed, this, &AILLCharacter::OnToggleThirdPerson);
}

void AILLCharacter::AddControllerYawInput(float Val)
{
	if (!IsInSpecialMove() || (ActiveSpecialMove && ActiveSpecialMove->CanRotateYaw()))
		Super::AddControllerYawInput(Val);
}

void AILLCharacter::AddControllerPitchInput(float Val)
{
	if (!IsInSpecialMove() || (ActiveSpecialMove && ActiveSpecialMove->CanRotatePitch()))
		Super::AddControllerPitchInput(Val);
}

float AILLCharacter::PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance)
	{
		return UseMesh->AnimScriptInstance->Montage_Play(AnimMontage, InPlayRate);
	}

	return 0.0f;
}

float AILLCharacter::PlayAnimMontage(FILLCharacterMontage* AnimMontage, float InPlayRate, FName StartSectionName)
{
	float Length = 0.f;

	// First Person
	if (GetFirstPersonMesh() && AnimMontage->FirstPerson)
	{
		const float Duration = GetFirstPersonMesh()->AnimScriptInstance->Montage_Play(AnimMontage->FirstPerson, InPlayRate);
		if (Duration > 0.f)
		{
			// Start at a given Section.
			if (StartSectionName != NAME_None)
			{
				GetFirstPersonMesh()->AnimScriptInstance->Montage_JumpToSection(StartSectionName, AnimMontage->FirstPerson);
			}

			Length = Duration;
		}
	}

	// Third Person
	if (GetMesh() && AnimMontage->ThirdPerson)
	{
		const float Duration = GetMesh()->AnimScriptInstance->Montage_Play(AnimMontage->ThirdPerson, InPlayRate);
		if (Duration > 0.f)
		{
			// Start at a given Section.
			if (StartSectionName != NAME_None)
			{
				GetMesh()->AnimScriptInstance->Montage_JumpToSection(StartSectionName, AnimMontage->ThirdPerson);
			}

			Length = FMath::Max(Length, Duration);
		}
	}

	return Length;
}

bool AILLCharacter::IsAnimMontagePlaying(UAnimMontage* AnimMontage) const
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		return UseMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage);
	}

	return false;
}

void AILLCharacter::StopAnimMontage(class UAnimMontage* AnimMontage)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance && UseMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage))
	{
		UseMesh->AnimScriptInstance->Montage_Stop(AnimMontage->BlendOut.GetBlendTime());
	}
}

void AILLCharacter::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	static const FName NAME_DebugCameraStack = TEXT("CameraStack");
	if (DebugDisplay.IsDisplayOn(NAME_DebugCameraStack))
	{
		const UFont* SmallFont = GEngine->GetSmallFont();

		const auto PrintViewInfo = [&](const FMinimalViewInfo& Info, const UFont* Font, const float XPos = 32.f)
		{
			// Location: (X, Y, Z) Rotation: (Yaw, Pitch, Roll)
			// FOV: #.# Ortho: (Width, Near Clip, Far Clip) Aspect: #.#
			YPos += YL;
			Canvas->DrawText(Font, FString::Printf(TEXT("Location: (%s) Rotation: (%s)"), *Info.Location.ToString(), *Info.Rotation.ToString()), XPos, YPos);
			YPos += YL;
			Canvas->DrawText(Font, FString::Printf(TEXT("FOV: %.1f Ortho: (W=%.1f NC=%.1f FC=%.1f) Aspect: %.3f %s"), Info.FOV, Info.OrthoWidth, Info.OrthoNearClipPlane, Info.OrthoFarClipPlane, Info.AspectRatio, Info.bConstrainAspectRatio ? TEXT("Locked") : TEXT("")), XPos, YPos);
		};

		Canvas->SetDrawColor(FColor::Cyan);
		float SumOfBlendWeights = 0.f;
		for (const auto& Camera : ActiveCameraStack)
		{
			// Name [Class] (Normalized Weight):
			//      Delta #.# Length #.# Weight #.#
			YPos += YL;
			if (Camera.DynamicCamera)
			{
				Canvas->DrawText(SmallFont, FString::Printf(TEXT("%s [%s] (%.2f):"), *Camera.DynamicCamera->GetName(), *Camera.DynamicCamera->GetClass()->GetName(), Camera.BlendWeightNormalized), 4.f, YPos);
				Camera.DynamicCamera->DisplayDebug(Canvas, DebugDisplay, YL, YPos);
				Canvas->SetDrawColor(FColor::Cyan);
			}
			else
			{
				Canvas->DrawText(SmallFont, FString::Printf(TEXT("None [Cache Info] (%.2f):"), Camera.BlendWeightNormalized), 4.f, YPos);
			}
			YPos += YL;
			Canvas->DrawText(SmallFont, FString::Printf(TEXT("D %.2f L %.2f W %.2f"), Camera.BlendDelta, Camera.BlendLength, Camera.BlendWeight), 32.f, YPos);

			PrintViewInfo(Camera.CameraInfoCache, SmallFont);

			SumOfBlendWeights += Camera.BlendWeightNormalized;
		}

		FMinimalViewInfo ViewInfo;
		CalcCamera(0.f, ViewInfo);

		Canvas->SetDrawColor(FColor::Green);
		YPos += YL;
		YPos += YL;
		Canvas->DrawText(SmallFont, FString::Printf(TEXT("Resulting Camera (%.2f):"), SumOfBlendWeights), 4.f, YPos);
		PrintViewInfo(ViewInfo, SmallFont);
	}
}

void AILLCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	EyeHeightLerpSpeed = UnCrouchEyeHeightLerpSpeed;

	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// Super call above will call to RecalculateBaseEyeHeight, which will maintain BaseEyeHeight for the sake of interpolation
	BaseEyeHeight -= HalfHeightAdjust;
}

void AILLCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	EyeHeightLerpSpeed = CrouchEyeHeightLerpSpeed;

	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// Super call above will call to RecalculateBaseEyeHeight, which will maintain BaseEyeHeight for the sake of interpolation
	BaseEyeHeight += HalfHeightAdjust;
}

void AILLCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Notify listeners
	OnCharacterLanded.Broadcast(Hit);
}

USkeletalMeshComponent* AILLCharacter::GetPawnMesh() const
{
	return IsFirstPerson() ? Mesh1P : GetMesh();
}

void AILLCharacter::SetCharacterMeshes(USkeletalMesh* SkelMesh1P, USkeletalMesh* SkelMesh3P, const bool bUpdateVisibility/* = true*/)
{
	if (Mesh1P)
	{
		Mesh1P->SetSkeletalMesh(SkelMesh1P);
	}

	if (GetMesh())
	{
		GetMesh()->SetSkeletalMesh(SkelMesh3P);
	}

	if (bUpdateVisibility)
	{
		UpdateCharacterMeshVisibility();
	}
}

void AILLCharacter::SetAnimInstanceClass(UClass* AnimInstance)
{
	if (GetMesh() && AnimInstance)
	{
		GetMesh()->SetAnimInstanceClass(AnimInstance);
	}
}

void AILLCharacter::UpdateCharacterMeshVisibility()
{
	SetActorHiddenInGame(!ShouldShowCharacter());

	// Selectively animate the first person mesh
	const bool bFirstPerson = IsFirstPerson();

	if (Mesh1P)
	{
		Mesh1P->SetHiddenInGame(!bFirstPerson);
		Mesh1P->MeshComponentUpdateFlag = bFirstPerson ? EMeshComponentUpdateFlag::AlwaysTickPose : EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	}

	// Always animate the third person mesh when it is the view target so that things like footstep sounds will process
	GetMesh()->SetHiddenInGame(bFirstPerson);
	GetMesh()->MeshComponentUpdateFlag = bFirstPerson ? EMeshComponentUpdateFlag::AlwaysTickPose : EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;

	// Optionally cast hidden shadows for first person
	GetMesh()->bCastHiddenShadow = bFirstPerson && bThirdPersonShadowInFirst;

	// Have the inventory update mesh visibility too
	UILLInventoryComponent* InventoryComponent = Cast<UILLInventoryComponent>(GetComponentByClass(UILLInventoryComponent::StaticClass()));
	if (InventoryComponent)
	{
		InventoryComponent->UpdateMeshVisibility(bFirstPerson);
	}
}

float AILLCharacter::PlayAnimMontageEx(class UAnimMontage* AnimMontage1P, class UAnimMontage* AnimMontage3P, float InPlayRate/* = 1.f*/, FName StartSectionName/* = NAME_None*/)
{
	FILLCharacterMontage Montage = { AnimMontage1P, AnimMontage3P };
	return PlayAnimMontageEx(Montage);
}

float AILLCharacter::PlayAnimMontageEx(FILLCharacterMontage Montage, float InPlayRate /*= 1.f*/, FName StartSectionName /*= NAME_None*/)
{
	float Result = 0.f;
	if (Montage.FirstPerson && GetFirstPersonMesh() && GetFirstPersonMesh()->AnimScriptInstance)
	{
		Result = GetFirstPersonMesh()->AnimScriptInstance->Montage_Play(Montage.FirstPerson, InPlayRate);
	}

	if (Montage.ThirdPerson && GetThirdPersonMesh() && GetThirdPersonMesh()->AnimScriptInstance)
	{
		Result = FMath::Max(Result, GetThirdPersonMesh()->AnimScriptInstance->Montage_Play(Montage.ThirdPerson, InPlayRate));
	}
	return Result;
}

void AILLCharacter::StopAnimMontageEx(class UAnimMontage* AnimMontage1P, class UAnimMontage* AnimMontage3P)
{
	FILLCharacterMontage Montage = { AnimMontage1P, AnimMontage3P };
	StopAnimMontageEx(Montage);
}

void AILLCharacter::StopAnimMontageEx(FILLCharacterMontage Montage)
{
	if (Montage.FirstPerson && GetFirstPersonMesh() && GetFirstPersonMesh()->AnimScriptInstance && GetFirstPersonMesh()->AnimScriptInstance->Montage_IsPlaying(Montage.FirstPerson))
	{
		GetFirstPersonMesh()->AnimScriptInstance->Montage_Stop(Montage.FirstPerson->BlendOut.GetBlendTime());
	}

	if (Montage.ThirdPerson && GetThirdPersonMesh() && GetThirdPersonMesh()->AnimScriptInstance && GetThirdPersonMesh()->AnimScriptInstance->Montage_IsPlaying(Montage.ThirdPerson))
	{
		GetThirdPersonMesh()->AnimScriptInstance->Montage_Stop(Montage.ThirdPerson->BlendOut.GetBlendTime());
	}
}

void AILLCharacter::StopAllAnimMontages()
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		UseMesh->AnimScriptInstance->Montage_Stop(0.0f);
	}
}

void AILLCharacter::OnToggleThirdPerson()
{
	AILLPlayerController* MyPC = Cast<AILLPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		SetThirdPerson(!bInThirdPerson);
	}
}

void AILLCharacter::PlayCameraShake(TSubclassOf<class UCameraShake> Shake, const float Scale, const ECameraAnimPlaySpace::Type PlaySpace, const FRotator UserPlaySpaceRot, const bool bPlayForLocalViewers/* = true*/)
{
	// Only play camera shakes for local controllers, this should be simulated
	AILLPlayerController* MyPC = Cast<AILLPlayerController>(Controller);
	if (MyPC && MyPC->IsLocalController() && MyPC->PlayerCameraManager)
	{
		MyPC->PlayerCameraManager->PlayCameraShake(Shake, Scale, PlaySpace, UserPlaySpaceRot);
	}

	if (bPlayForLocalViewers)
	{
		// Handle this character being a view target for a local player too
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if ((*It)->IsLocalController() && (*It)->GetViewTarget() == this)
			{
				(*It)->PlayerCameraManager->PlayCameraShake(Shake, Scale, PlaySpace, UserPlaySpaceRot);
			}
		}
	}
}

void AILLCharacter::StopCameraShakeInstances(TSubclassOf<class UCameraShake> Shake)
{
	// Only play camera shakes for local controllers, this should be simulated
	AILLPlayerController* MyPC = Cast<AILLPlayerController>(Controller);
	if (MyPC && MyPC->IsLocalController() && MyPC->PlayerCameraManager)
	{
		MyPC->PlayerCameraManager->StopAllInstancesOfCameraShake(Shake);
	}

	// Handle this character being a view target for a local player too
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if ((*It)->IsLocalController() && (*It)->GetViewTarget() == this)
		{
			(*It)->PlayerCameraManager->StopAllInstancesOfCameraShake(Shake);
		}
	}
}

const FMinimalViewInfo& AILLCharacter::GetCachedCameraInfo() const
{
	check(IsLocallyControlled());
	check(Cast<AAIController>(Controller) == nullptr);
	return CachedCameraInfo;
}

bool AILLCharacter::IsFirstPerson() const
{
	if (GetNetMode() == NM_DedicatedServer)
		return false;

	if (bInThirdPerson)
	{
		return false;
	}

	if (!Controller || !Controller->IsLocalPlayerController())
	{
		// For remote-controlled characters use first person if they are a view target
		if (IsLocalPlayerViewTarget())
		{
			return true;
		}
	}
	else if (AILLPlayerController* PC = Cast<AILLPlayerController>(Controller))
	{
		// For locally-controlled characters, use first person if that is our camera style too
		static FName NAME_Default(TEXT("Default"));
		if (PC->PlayerCameraManager->CameraStyle == NAME_Default)
		{
			return true;
		}
	}

	return false;
}

bool AILLCharacter::IsLocalPlayerViewTarget() const
{
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if ((*It)->IsLocalController() && (*It)->GetViewTarget() == this)
			{
				return true;
			}
		}
	}

	return false;
}

void AILLCharacter::SetThirdPerson(bool bNewThirdPerson)
{
	bInThirdPerson = bNewThirdPerson;
	UpdateCharacterMeshVisibility();
}

bool AILLCharacter::CanInteractAtAll() const
{
	if (IsInSpecialMove())
	{
		return false;
	}

	if (AILLPlayerController* PC = Cast<AILLPlayerController>(Controller))
	{
		if (!PC->IsGameInputAllowed())
			return false;
	}

	return true;
}

bool AILLCharacter::CanAbortCurrentMoveFor(TSubclassOf<UILLSpecialMoveBase> SpecialMove, AActor* TargetActor/* = nullptr*/) const
{
	return false;
}

bool AILLCharacter::CanStartSpecialMove(TSubclassOf<UILLSpecialMoveBase> SpecialMove, AActor* TargetActor/* = nullptr*/) const
{
	return (!IsInSpecialMove() || CanAbortCurrentMoveFor(SpecialMove, TargetActor));
}

void AILLCharacter::StartSpecialMove(TSubclassOf<UILLSpecialMoveBase> SpecialMove, AActor* TargetActor/* = nullptr*/, const bool bViaReplication/* = false*/)
{
	if (!bViaReplication)
	{
		if (CanStartSpecialMove(SpecialMove, TargetActor))
			ServerStartSpecialMove(SpecialMove, TargetActor);
		return;
	}

	TSubclassOf<UILLSpecialMoveBase> MatchingMove = nullptr;
	for (TSubclassOf<UILLSpecialMoveBase> MoveClass : AllowedSpecialMoves)
	{
		if (MoveClass == SpecialMove->GetClass())
		{
			MatchingMove = MoveClass;
			break;
		}
		else if (MoveClass->IsChildOf(SpecialMove))
		{
			MatchingMove = MoveClass;
		}
	}

	if (MatchingMove)
	{
		ActiveSpecialMove = NewObject<UILLSpecialMoveBase>(this, MatchingMove);
		ActiveSpecialMove->BeginMove(TargetActor, FOnILLSpecialMoveCompleted::FDelegate::CreateUObject(this, &AILLCharacter::OnSpecialMoveCompleted));

		OnSpecialMoveStarted();
	}
}

void AILLCharacter::OnSpecialMoveStarted()
{
}

void AILLCharacter::OnSpecialMoveCompleted()
{
	ActiveSpecialMove = nullptr;
}

void AILLCharacter::ServerStartSpecialMove_Implementation(TSubclassOf<class UILLSpecialMoveBase> SpecialMove, AActor* TargetActor)
{
	ClientsStartSpecialMove(SpecialMove, TargetActor);
}

bool AILLCharacter::ServerStartSpecialMove_Validate(TSubclassOf<class UILLSpecialMoveBase> SpecialMove, AActor* TargetActor)
{
	return true;
}

void AILLCharacter::ClientsStartSpecialMove_Implementation(TSubclassOf<class UILLSpecialMoveBase> SpecialMove, AActor* TargetActor)
{
	StartSpecialMove(SpecialMove, TargetActor, true);
}

bool AILLCharacter::CanFidget_Implementation() const
{
	return true;
}

void AILLCharacter::ResetFidgetTimer(float ExtraTime /* = 0.f */, bool bCancelActiveFidget /* = true */)
{
	if (bCancelActiveFidget)
	{
		StopAnimMontageEx(ActiveFidget);
		ActiveFidget.FirstPerson = nullptr;
		ActiveFidget.ThirdPerson = nullptr;
	}

	// TODO: Add support to optionally network these values
	if (NextFidget == -1)
	{
		bFidgetFromItem = bFidgetPreferItems ? true : FMath::RandBool();

		// We might have an item with no fidgets
		if (auto Fidgets = GetFidgetList())
		{
			if (Fidgets->Num() > 0)
				NextFidget = FMath::RandRange(0, Fidgets->Num() - 1);
		}
	}

	ExtraTime = FMath::Max(ExtraTime, 0.f) + FMath::FRandRange(-FidgetTimeRange, FidgetTimeRange);
	GetWorldTimerManager().ClearTimer(TimerHandle_Fidget);
	GetWorldTimerManager().SetTimer(TimerHandle_Fidget, this, &AILLCharacter::TryFidget, TimeBetweenFidgets + ExtraTime, false); // Will loop manually
}

const TArray<FILLCharacterMontage>* AILLCharacter::GetFidgetList() const
{
	const TArray<FILLCharacterMontage>* Fidgets = &FidgetAnimations;

	// Equipped item fidgets override base fidgets
	const UILLInventoryComponent* Inventory = Cast<UILLInventoryComponent>(GetComponentByClass(UILLInventoryComponent::StaticClass()));
	if (Inventory)
	{
		AILLInventoryItem* CurrentItem = Inventory->GetEquippedItem();
		if (bFidgetFromItem && CurrentItem)
		{
			if (const TArray<FILLCharacterMontage>* ItemFidgets = CurrentItem->GetFidgets(this))
			{
				Fidgets = ItemFidgets;
			}
		}
	}

	return Fidgets;
}

void AILLCharacter::TryFidget()
{
	auto Fidgets = GetFidgetList();
	if (CanFidget() == false || NextFidget == -1 || Fidgets == nullptr || NextFidget >= Fidgets->Num())
	{
		ResetFidgetTimer();
		return;
	}

	ActiveFidget = (*Fidgets)[NextFidget];
	float MontageTime = PlayAnimMontageEx(ActiveFidget);
	NextFidget = -1; // Fidget played, grab a new one
	ResetFidgetTimer(MontageTime, false);
}
