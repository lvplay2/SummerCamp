// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "Camera/ILLVistaCamera.h"

#include "MessageLog.h"
#include "UObjectToken.h"

FBlendSettings::FBlendSettings()
: bLockOutgoingView(false)
, ConstrainAspectRatioBlendType(EConstrainAspectRatioActivationType::PostRatio)
, BlendTime(5.f)
, BlendFunc(EViewTargetBlendFunction::VTBlend_EaseInOut)
, BlendExponent(2.f)
{
}

// -------------------------------------------------------------------------------
AILLVistaCamera::AILLVistaCamera(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, TimeLimit(0.f)
, YawInputName(NAME_None)
, EllipsoidalYawCurve(nullptr)
, bClampYaw(false)
, MinRelativeYaw(-180.f)
, MaxRelativeYaw(180.f)
, PitchInputName(NAME_None)
, EllipsoidalPitchCurve(nullptr)
, bClampPitch(false)
, MinRelativePitch(-89.f)
, MaxRelativePitch(89.f)
, bBlendingIn(true)
, BlendStartTimestamp(-1.f)
, DefaultSpringArmRotation(0.f, 0.f, 0.f)
, DefaultSpringArmLength(0.f)
, PitchOffset(0.f)
, YawOffset(0.f)
, ActivePlayerController(nullptr)
, ActivePlayerCharacter(nullptr)
{
	// Increase our input priority so we can override the player character input
	InputPriority = 5;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->RelativeLocation = FVector(0.f, 0.f, 100.f);
	SpringArm->TargetArmLength = 500;
	SpringArm->bDoCollisionTest = false; // TODO: Add support for custom collision channel rather than just turning it off (don't want to collide with the player)
	SpringArm->SetupAttachment(SceneRoot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	// Tick disabled by default, turned on based on blending values
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AILLVistaCamera::BeginPlay()
{
	Super::BeginPlay();

	Camera->GetCameraView(0.f, DefaultCameraValues);
	DefaultSpringArmRotation = SpringArm->GetComponentRotation();
	DefaultSpringArmLength = SpringArm->TargetArmLength;
}

void AILLVistaCamera::EnableInput(class APlayerController* PlayerController)
{
	Super::EnableInput(PlayerController);

	// Bind rotation control
	if (YawInputName.IsNone() == false)
		InputComponent->BindAxis(YawInputName, this, &AILLVistaCamera::AddYaw);

	if (PitchInputName.IsNone() == false)
		InputComponent->BindAxis(PitchInputName, this, &AILLVistaCamera::AddPitch);
}

void AILLVistaCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Pre calculate what we're blending between
	FBlendSettings CurrentBlendSettings;
	FMinimalViewInfo TargetCamera, SourceCamera;
	if (bBlendingIn)
	{
		CurrentBlendSettings = BlendIn;
		ActivePlayerCharacter->CalcCamera(0.f, SourceCamera);
		Camera->GetCameraView(0.f, TargetCamera);
	}
	else
	{
		CurrentBlendSettings = BlendOut;
		ActivePlayerCharacter->CalcCamera(0.f, TargetCamera);
		Camera->GetCameraView(0.f, SourceCamera);
	}

	const float Delta = (GetWorld()->GetTimeSeconds() - BlendStartTimestamp) / CurrentBlendSettings.BlendTime;

	// Done blending, stop ticking
	if (Delta >= 1.f)
	{
		PrimaryActorTick.SetTickFunctionEnable(false);
		ActivePlayerCharacter->SetAspectRatioOverride(false, false); // Disable

		// Revert to default when done blending unless aspect ratio is explicitly disabled
		if (CurrentBlendSettings.ConstrainAspectRatioBlendType != EConstrainAspectRatioActivationType::None)
		{
			Camera->bConstrainAspectRatio = DefaultCameraValues.bConstrainAspectRatio;
		}

		// Clean up if we were blending out
		if (bBlendingIn == false)
		{
			ResetCameraSettings();
		}

		return;
	}

	// PostRatio on sets constrained once our ratio is past the window ratio (horizontal letter box)
	// It also unsets if we go below our window ratio to prevent verticle letter box
	if (CurrentBlendSettings.ConstrainAspectRatioBlendType == EConstrainAspectRatioActivationType::PostRatio)
	{
		FMinimalViewInfo CurrentCamera = SourceCamera;
		CurrentCamera.BlendViewInfo(TargetCamera, Delta);

		const float WindowAspectRatio = GEngine->GameViewport->Viewport->GetDesiredAspectRatio();
		if (CurrentCamera.AspectRatio > WindowAspectRatio)
		{
			Camera->bConstrainAspectRatio = true;
			ActivePlayerCharacter->SetAspectRatioOverride(true, true);
		}
		else
		{
			Camera->bConstrainAspectRatio = false;
			ActivePlayerCharacter->SetAspectRatioOverride(true, false);
		}
	}
}

void AILLVistaCamera::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(VistaAutoExitTimer);
	}

	Super::EndPlay(EndPlayReason);
}

float AILLVistaCamera::StartBlendIn(AILLCharacter* Player)
{
	APlayerController* PlayerController = Cast<APlayerController>(Player->GetInstigatorController());
	if (PlayerController == NULL)
	{
#if WITH_EDITOR
		// 'Player' doesn't have a PlayerController associated with it! We can't active the vista camera 'this' as a result.
		FMessageLog("PIE").Error()
			->AddToken(FUObjectToken::Create(Player))
			->AddToken(FTextToken::Create(FText::FromString(TEXT(" doesn't have a PlayerController associated with it! We can't active the vista camera "))))
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::FromString(TEXT(" as a result."))));
#endif

		return 0.f;
	}

	// Only adjust the camera for the local player
	if (PlayerController->IsLocalController() == false)
	{
#if WITH_EDITOR
		// 'Player' isn't a local player! 'this' only supports local players.
		FMessageLog("PIE").Error()
			->AddToken(FUObjectToken::Create(PlayerController->GetCharacter()))
			->AddToken(FTextToken::Create(FText::FromString(TEXT(" isn't a local player! "))))
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::FromString(TEXT(" only supports local players."))));
#endif

		return 0.f;
	}

	// Reverts camera settings to default
	ResetCameraSettings();

	ActivePlayerCharacter = Player;
	ActivePlayerController = PlayerController;

	// Start blend
	bBlendingIn = true;
	BlendStartTimestamp = GetWorld()->GetTimeSeconds();
	ActivePlayerController->SetViewTargetWithBlend(this, BlendIn.BlendTime, BlendIn.BlendFunc, BlendIn.BlendExponent, BlendIn.bLockOutgoingView);
	
	// If we have any range to move enable input
	if (MinRelativeYaw != MaxRelativeYaw && MinRelativePitch != MaxRelativePitch)
	{
		EnableInput(ActivePlayerController);
	}

	// Update ticking based on our blend in style
	FMinimalViewInfo SourceCameraInfo;
	ActivePlayerCharacter->CalcCamera(0.f, SourceCameraInfo);
	UpdateTickEnabledFromBlendType(SourceCameraInfo.bConstrainAspectRatio, Camera->bConstrainAspectRatio, BlendIn.ConstrainAspectRatioBlendType);

	// Auto-exit
	if (TimeLimit > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(VistaAutoExitTimer, FTimerDelegate::CreateUObject(this, &AILLVistaCamera::AbortVistaCamera), TimeLimit, false);
	}

	return BlendIn.BlendTime;
}

float AILLVistaCamera::StartBlendOut()
{
	// Make sure the we were active
	if (ActivePlayerController == NULL)
	{
		return 0.f;
	}

	// Don't re-trigger a blend out
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float BlendOutDelta = (CurrentTime - BlendStartTimestamp);
	if (bBlendingIn == false && BlendOutDelta < BlendOut.BlendTime)
	{
		return BlendOut.BlendTime - BlendOutDelta;
	}

	DisableInput(ActivePlayerController);

	ActivePlayerController->SetViewTargetWithBlend(ActivePlayerCharacter, BlendOut.BlendTime, BlendOut.BlendFunc, BlendOut.BlendExponent, BlendOut.bLockOutgoingView);
	BlendStartTimestamp = CurrentTime;
	bBlendingIn = false;

	// Update ticking based on our blend in style
	FMinimalViewInfo TargetCameraInfo;
	ActivePlayerCharacter->CalcCamera(0.f, TargetCameraInfo);
	UpdateTickEnabledFromBlendType(Camera->bConstrainAspectRatio, TargetCameraInfo.bConstrainAspectRatio, BlendOut.ConstrainAspectRatioBlendType);

	return BlendOut.BlendTime;
}

void AILLVistaCamera::ResetCameraSettings()
{
	// Reset camera/spring position to default
	SpringArm->SetWorldRotation(DefaultSpringArmRotation);
	SpringArm->TargetArmLength = DefaultSpringArmLength;
	Camera->bConstrainAspectRatio = DefaultCameraValues.bConstrainAspectRatio;
	PitchOffset = 0.f;
	YawOffset = 0.f;

	ActivePlayerController = NULL;
	ActivePlayerCharacter = NULL;

	if (VistaAutoExitTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(VistaAutoExitTimer);
	}
}

void AILLVistaCamera::UpdateTickEnabledFromBlendType(const bool& bSourceCameraConstrained, const bool& bTargetCameraConstrained, const EConstrainAspectRatioActivationType& BlendType)
{
	// If we want unique blending function for the aspect ratio we need to update
	// None and OverTime do not need to update
	switch (BlendType)
	{
	case EConstrainAspectRatioActivationType::None:
		{
			ActivePlayerCharacter->SetAspectRatioOverride(true, false);
			Camera->bConstrainAspectRatio = false;
		}
		break;
	case EConstrainAspectRatioActivationType::Imediate:
		{
			ActivePlayerCharacter->SetAspectRatioOverride(true, bTargetCameraConstrained, Camera->AspectRatio);
			PrimaryActorTick.SetTickFunctionEnable(true);
		}
		break;
	case EConstrainAspectRatioActivationType::PostRatio: // Pass through
	case EConstrainAspectRatioActivationType::PostBlend:
		{
			bool bForceOn = bSourceCameraConstrained & bTargetCameraConstrained;

			// If both cameras are set to constrain, don't turn it off
			ActivePlayerCharacter->SetAspectRatioOverride(true, bForceOn);

			if (bForceOn == false)
			{
				PrimaryActorTick.SetTickFunctionEnable(true);
			}
		}
		break;

	case EConstrainAspectRatioActivationType::OverTime: // Pass through
	default:
		break;
	}
}

void AILLVistaCamera::AddPitch(float Delta)
{
	// Make sure we can't gimbal lock
	float MinPitch = -89.f, MaxPitch = 89.f;
	if (bClampPitch)
	{
		MinPitch = MinRelativePitch;
		MaxPitch = MaxRelativePitch;
	}

	// Invert pitch by default to match player input
	PitchOffset = FMath::Clamp(PitchOffset - Delta, MinPitch, MaxPitch);

	// Rebuild rotation every update
	const FRotator NewRot = DefaultSpringArmRotation + FRotator(PitchOffset, YawOffset, 0.f);
	SpringArm->SetWorldRotation(NewRot);

	UpdateArmLength();
}

void AILLVistaCamera::AddYaw(float Delta)
{
	if (bClampYaw)
	{
		YawOffset = FMath::Clamp(YawOffset + Delta, MinRelativeYaw, MaxRelativeYaw);
	}
	else
	{
		YawOffset = FRotator::ClampAxis(YawOffset + Delta);
	}

	// Rebuild rotation every update
	const FRotator NewRot = DefaultSpringArmRotation + FRotator(PitchOffset, YawOffset, 0.f);
	SpringArm->SetWorldRotation(NewRot);

	UpdateArmLength();
}

void AILLVistaCamera::UpdateArmLength()
{
	float Length = 0.f;
	float Denominator = 0.f;
	if (EllipsoidalYawCurve)
	{
		Length += EllipsoidalYawCurve->GetFloatValue(YawOffset);
		Denominator += 1.f;
	}

	if (EllipsoidalPitchCurve)
	{
		Length += EllipsoidalPitchCurve->GetFloatValue(PitchOffset);
		Denominator += 1.f;
	}

	// Might not have either curve set
	if (Denominator > 0.f)
	{
		SpringArm->TargetArmLength = Length / Denominator;
	}
}

void AILLVistaCamera::AbortVistaCamera()
{
	// Won't do anything if PlayerCharacter is NULL
	StartBlendOut();
}
