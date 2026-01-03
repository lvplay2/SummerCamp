// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSpectatorPawn.h"

#include "ILLGameBlueprintLibrary.h"
#include "SCCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCEscapeVolume.h"
#include "SCGameState.h"
#include "SCHidingSpot.h"
#include "SCInGameHUD.h"
#include "SCPlayerCameraManager.h"
#include "SCPlayerController.h"
#include "SCPlayerState.h"
#include "SCStaticSpectatorCamera.h"

ASCSpectatorPawn::ASCSpectatorPawn(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bToggleShowAll(true)
, ViewingOffset(250.0f)
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"), true);
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraLagSpeed = 28.5f;
	SpringArm->CameraRotationLagSpeed = 10.0f;
	SpringArm->TargetArmLength = ViewingOffset;
	SpringArm->bInheritRoll = false;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bUsePawnControlRotation = true;

	SpectatorCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SpectatorCamera"), true);
	SpectatorCamera->SetupAttachment(SpringArm);

	// Don't block cinematic cameras just because we want to watch people die too
	GetCollisionComponent()->SetCollisionResponseToChannel(ECC_AnimCameraBlocker, ECollisionResponse::ECR_Ignore);

	bToggleHUD = true;
}

void ASCSpectatorPawn::OnRep_Controller()
{
	Super::OnRep_Controller();

	OnReceivedController();
}

void ASCSpectatorPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	OnReceivedController();
}

void ASCSpectatorPawn::SetupPlayerInputComponent(class UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);

	check(InInputComponent);

	// Map toggling the scoreboard
	InInputComponent->BindAction("GLB_ShowScoreboard", IE_Pressed, this, &ASCSpectatorPawn::StartShowScoreboard);
	InInputComponent->BindAction("GLB_ShowScoreboard", IE_Released, this, &ASCSpectatorPawn::StopShowScoreboard);

	// Map toggling the HUD overlay for spectator mode
	InInputComponent->BindAction("SPEC_ToggleHUD", IE_Pressed, this, &ASCSpectatorPawn::ToggleHUD);
}

void ASCSpectatorPawn::MoveForward(float Val)
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		const ESCSpectatorMode SpectatorMode = PC->GetCurrentSpectatingMode();
		if (SpectatorMode == ESCSpectatorMode::FreeCam)
		{
			Super::MoveForward(Val);
		}
		else if (SpectatorMode == ESCSpectatorMode::Player)
		{
			if (Viewing && !bDetachedFromViewingPlayer)
			{
				const float DeltaSeconds = [this]()
				{
					if (UWorld* World = GetWorld())
						return World->GetDeltaSeconds();
					return 0.f;
				}();

				ViewingOffset -= Val * (250.f * DeltaSeconds);
				static const float ClosestViewingOffset = 150.0f;
				static const float FurthestViewingOffset = 350.0f;

				if (ViewingOffset < ClosestViewingOffset)
					ViewingOffset = ClosestViewingOffset;
				else if (ViewingOffset > FurthestViewingOffset)
					ViewingOffset = FurthestViewingOffset;

				SpringArm->TargetArmLength = ViewingOffset;
			}
		}
	}
}

void ASCSpectatorPawn::MoveRight(float Val)
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (PC->GetCurrentSpectatingMode() == ESCSpectatorMode::FreeCam)
		{
			Super::MoveRight(Val);
		}
	}
}

void ASCSpectatorPawn::MoveUp_World(float Val)
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (PC->GetCurrentSpectatingMode() == ESCSpectatorMode::FreeCam)
		{
			Super::MoveUp_World(Val);
		}
	}
}

void ASCSpectatorPawn::AddControllerPitchInput(float Val)
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (PC->GetCurrentSpectatingMode() != ESCSpectatorMode::SpectateIntro)
		{
			FRotator CurrentControlRotation = PC->GetControlRotation();
			ASCCharacter* Spectating = PC->GetSpectatingPlayer();
			if (Spectating && Spectating->bInWater && PC->PlayerCameraManager)
			{
				// Clamp pitch while spectating a player in the water
				CurrentControlRotation.Pitch = FRotator::ClampAxis(FMath::ClampAngle(CurrentControlRotation.Pitch + (Val * PC->InputPitchScale), PC->PlayerCameraManager->ViewPitchMin, 0.0f));
				PC->SetControlRotation(CurrentControlRotation);
			}
			else
			{
				Super::AddControllerPitchInput(Val);
			}
		}
	}
}

void ASCSpectatorPawn::AddControllerYawInput(float Val)
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (PC->GetCurrentSpectatingMode() != ESCSpectatorMode::SpectateIntro)
		{
			Super::AddControllerYawInput(Val);
		}
	}
}

void ASCSpectatorPawn::StartShowScoreboard()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* Hud = PC->GetSCHUD())
		{
			Hud->DisplayScoreboard(true);
		}
	}
}

void ASCSpectatorPawn::StopShowScoreboard()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* Hud = PC->GetSCHUD())
		{
			Hud->DisplayScoreboard(false);
		}
	}
}

void ASCSpectatorPawn::ToggleHUD()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* Hud = PC->GetSCHUD())
		{
			Hud->OnToggleSpecatorHUD(bToggleHUD = !bToggleHUD);
		}
	}
}

void ASCSpectatorPawn::ToggleSpectatorShowAll()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* Hud = PC->GetSCHUD())
		{
			Hud->OnToggleSpecatorShowAll(bToggleShowAll = !bToggleShowAll);
		}
	}
}

void ASCSpectatorPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Stop updating if the match is over
	ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState());
	if (!GS || GS->HasMatchEnded())
	{
		return;
	}

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		// Don't update if we aren't spectating
		ASCPlayerState* PS = Cast<ASCPlayerState>(PC->PlayerState);
		if (!PS || !PS->GetSpectating())
		{
			return;
		}

		PC->BumpIdleTime();
		const AActor* CurrentViewTarget = PC->GetViewTarget();
		if (PC->GetCurrentSpectatingMode() == ESCSpectatorMode::Player)
		{
			// Update spectating player
			if (ASCCharacter* Spectating = PC->GetSpectatingPlayer())
			{
				if (Spectating != Viewing)
				{
					// Clean up old spectating player
					if (Viewing)
					{
						DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
					}

					// View the new player
					if (Spectating)
					{
						Viewing = Spectating;

						FRotator Rotation = Viewing->GetActorRotation();
						Rotation.Pitch = 0.0f;
						Rotation.Roll = 0.0f;

						TeleportTo(Viewing->GetActorLocation(), Rotation);
						AttachToActor(Viewing, FAttachmentTransformRules::SnapToTargetIncludingScale);
						SetActorRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
						bDetachedFromViewingPlayer = false;
						
						if (CurrentViewTarget != this)
						{
							PC->SetViewTarget(this);
						}
						PC->SetControlRotation(Rotation);
					}
				}
				else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Spectating))
				{
					if (Counselor->HasEscaped())
					{
						DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
						bDetachedFromViewingPlayer = true;
						PC->ClientSetSpectatorCamera(GetActorLocation(), GetControlRotation());
						return;
					}

					if (ASCHidingSpot* HidingSpot = Counselor->GetCurrentHidingSpot())
					{
						if (Counselor->IsAlive())
						{
							if (Counselor->IsInContextKill())
							{
								// Move to the spectator viewing location
								if (UArrowComponent* SpecLocation = HidingSpot->GetSpectatorViewingLocation())
								{
									if (CurrentViewTarget != this)
									{
										DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
										SetActorLocation(SpecLocation->GetComponentLocation());
										PC->SetControlRotation(SpecLocation->GetComponentRotation());
										PC->SetViewTarget(this);
									}
								}
							}
							// Change the spectator view to the hide spot camera
							else if (CurrentViewTarget != HidingSpot && IsValid(HidingSpot))
							{
								const float HidingSensitivityMod = Counselor->HidingSensitivityMod;
								PC->ModifyInputSensitivity(EKeys::MouseX, HidingSensitivityMod);
								PC->ModifyInputSensitivity(EKeys::MouseY, HidingSensitivityMod);
								PC->ModifyInputSensitivity(EKeys::Gamepad_RightX, HidingSensitivityMod);
								PC->ModifyInputSensitivity(EKeys::Gamepad_RightY, HidingSensitivityMod);

								// View the hide spot camera
								PC->SetControlRotation(HidingSpot->GetBaseCameraRotation());
								PC->SetViewTarget(HidingSpot);
							}
						}
					}
					else if (CurrentViewTarget != this)
					{
						// Reset input sensitivity
						PC->RestoreInputSensitivity(EKeys::MouseX);
						PC->RestoreInputSensitivity(EKeys::MouseY);
						PC->RestoreInputSensitivity(EKeys::Gamepad_RightX);
						PC->RestoreInputSensitivity(EKeys::Gamepad_RightY);

						// Reset the spectator view back to the spectator camera
						PC->SetViewTarget(this);
						PC->SetControlRotation(GetActorRotation());
					}
				}
				else if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Spectating))
				{
					if (Killer->bUnderWater)
					{
						// We want the distance from the killer to the waters surface since we're doing relative attachment.
						float ZSeparation = Killer->GetWaterZ() - Killer->GetActorLocation().Z;
						SetActorRelativeLocation(FVector(0.0f, 0.0f, ZSeparation + 100.0f));
					}
					else
					{
						SetActorRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
					}
				}
			}
		}
		else
		{
			// Clear specating player
			if (Viewing)
			{
				DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				Viewing = nullptr;
			}
		}
	}
}

void ASCSpectatorPawn::OnReceivedController()
{
	if (IsLocallyControlled())
	{
		if (!ForcedPostProcessVolume)
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner = this;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ForcedPostProcessVolume = GetWorld()->SpawnActor<APostProcessVolume>(SpawnInfo);

			if (ForcedPostProcessVolume)
			{
				ForcedPostProcessVolume->bUnbound = true;
				ForcedPostProcessVolume->Priority = 100.0f;
			}
		}
	}
}