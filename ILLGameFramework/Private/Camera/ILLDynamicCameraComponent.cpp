// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "Camera/ILLDynamicCameraComponent.h"

FCameraBehaviorSettings::FCameraBehaviorSettings()
// Blend
: MinimumCharacterSpeed(0.f)
, BlendExponent(2.f)
, BlendFunc(VTBlend_EaseInOut)
, BlendInDelay(1.f)
, BlendInTime(1.f)
// SpringArm
, TargetArmLength(300.f)
, SocketOffset(FVector::ZeroVector)
, TargetOffset(FVector::ZeroVector)
, bUsePawnControlRotation(true)
, bInheritPitch(true)
, bInheritYaw(true)
, bInheritRoll(false)
, bEnableCameraLag(true)
, bEnableCameraRotationLag(true)
, bUseCameraLagSubstepping(true)
, CameraLagSpeed(10.f)
, CameraRotationLagSpeed(10.f)
, CameraLagMaxTimeStep(1.f / 60.f)
, CameraLagMaxDistance(0.f)
// Camera
, FieldOfView(90.f)
, OrthoWidth(512.f)
, OrthoNearClipPlane(0.f)
, OrthoFarClipPlane(WORLD_MAX)
, AspectRatio(1.777778f)
, bUseFieldOfViewForLOD(true)
{
}

void FCameraBehaviorSettings::Apply(UILLDynamicCameraComponent* DynamicCamera) const
{
	USpringArmComponent* SpringArm = DynamicCamera->SpringArm;
	SpringArm->TargetArmLength = TargetArmLength;
	SpringArm->SocketOffset = SocketOffset;
	SpringArm->TargetOffset = TargetOffset;
	SpringArm->bUsePawnControlRotation = bUsePawnControlRotation;
	SpringArm->bInheritPitch = bInheritPitch;
	SpringArm->bInheritYaw = bInheritYaw;
	SpringArm->bInheritRoll = bInheritRoll;
	SpringArm->bEnableCameraLag = bEnableCameraLag;
	SpringArm->bEnableCameraRotationLag = bEnableCameraRotationLag;
	SpringArm->bUseCameraLagSubstepping = bUseCameraLagSubstepping;
	SpringArm->CameraLagSpeed = CameraLagSpeed;
	SpringArm->CameraRotationLagSpeed = CameraRotationLagSpeed;
	SpringArm->CameraLagMaxTimeStep = CameraLagMaxTimeStep;
	SpringArm->CameraLagMaxDistance = CameraLagMaxDistance;

	UCameraComponent* Camera = DynamicCamera->Camera;
	Camera->FieldOfView = FieldOfView;
	Camera->OrthoWidth = OrthoWidth;
	Camera->OrthoNearClipPlane = OrthoNearClipPlane;
	Camera->OrthoFarClipPlane = OrthoFarClipPlane;
	Camera->AspectRatio = AspectRatio;
	Camera->bConstrainAspectRatio = bOverride_AspectRatio;
	Camera->bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
}

FCameraBehaviorSettings FCameraBehaviorSettings::Lerp(const FCameraBehaviorSettings& A, const FCameraBehaviorSettings& B, float Alpha)
{
	// Early out for non-blending values (common)
	Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
	if (Alpha == 0.f)
		return A;
	else if (Alpha == 1.f)
		return B;

	// Blend our alpha first and then use that blended alpha to lerp all the values
	switch (B.BlendFunc)
	{
	case VTBlend_Linear:
		Alpha = FMath::Lerp(0.f, 1.f, Alpha);
		break;
	case VTBlend_Cubic:
		Alpha = FMath::CubicInterp(0.f, 0.f, 1.f, 0.f, Alpha);
		break;
	case VTBlend_EaseIn:
		Alpha = FMath::Lerp(0.f, 1.f, FMath::Pow(Alpha, B.BlendExponent));
		break;
	case VTBlend_EaseOut:
		Alpha = FMath::Lerp(0.f, 1.f, FMath::Pow(Alpha, 1.f / B.BlendExponent));
		break;
	case VTBlend_EaseInOut:
		// Fall through
	default:
		Alpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, B.BlendExponent);
		break;
	}

	// For now use the camera flags for the camera we are transitioning to. Doing this to avoid a camera pop half way through the transition.
	FCameraBehaviorSettings result(/*(Alpha < 0.5f) ? A :*/ B);

	// Spring arm blends
	result.TargetArmLength = FMath::Lerp(A.TargetArmLength, B.TargetArmLength, Alpha);
	result.SocketOffset = FMath::Lerp(A.SocketOffset, B.SocketOffset, Alpha);
	result.TargetOffset = FMath::Lerp(A.TargetOffset, B.TargetOffset, Alpha);
	result.CameraLagSpeed = FMath::Lerp(A.CameraLagSpeed, B.CameraLagSpeed, Alpha);
	result.CameraRotationLagSpeed = FMath::Lerp(A.CameraRotationLagSpeed, B.CameraRotationLagSpeed, Alpha);
	result.CameraLagMaxTimeStep = FMath::Lerp(A.CameraLagMaxTimeStep, B.CameraLagMaxTimeStep, Alpha);
	result.CameraLagMaxDistance = FMath::Lerp(A.CameraLagMaxDistance, B.CameraLagMaxDistance, Alpha);

	// Camera blends
	result.FieldOfView = FMath::Lerp(A.FieldOfView, B.FieldOfView, Alpha);
	result.OrthoWidth = FMath::Lerp(A.OrthoWidth, B.OrthoWidth, Alpha);
	result.OrthoNearClipPlane = FMath::Lerp(A.OrthoNearClipPlane, B.OrthoNearClipPlane, Alpha);
	result.OrthoFarClipPlane = FMath::Lerp(A.OrthoFarClipPlane, B.OrthoFarClipPlane, Alpha);
	result.AspectRatio = FMath::Lerp(A.AspectRatio, B.AspectRatio, Alpha);

	return result;
}

// ----------------------------------------------------------------------------
FName UILLDynamicCameraComponent::SpringArmComponentName = TEXT("SpringArm");
FName UILLDynamicCameraComponent::CameraComponentName = TEXT("Camera");

UILLDynamicCameraComponent::UILLDynamicCameraComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bUse2DVelocity(true)
, CurrentOffsetIndex(0)
{
	bWantsInitializeComponent = true;
	bAutoActivate = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	/*SpringArm = CreateDefaultSubobject<USpringArmComponent>(SpringArmComponentName);
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(CameraComponentName);
	Camera->bUsePawnControlRotation = false;
	Camera->FieldOfView = 90.f;
	Camera->SetupAttachment(SpringArm);*/
}

void UILLDynamicCameraComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
	if (Camera)
	{
		Camera->DestroyComponent();
		Camera = nullptr;
	}

	if (SpringArm)
	{
		SpringArm->DestroyComponent();
		SpringArm = nullptr;
	}

	SpringArm = NewObject<USpringArmComponent>(this);
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritRoll = false;
	SpringArm->RegisterComponent();

	Camera = NewObject<UCameraComponent>(this);
	Camera->bUsePawnControlRotation = false;
	Camera->FieldOfView = 90.f;
	Camera->SetupAttachment(SpringArm);
	Camera->RegisterComponent();

	DefaultSpringArmLength = SpringArm->TargetArmLength;

	if (MovementBasedSettings.Num() > 0)
	{
		MovementBasedSettings.Sort([](const FCameraBehaviorSettings& A, const FCameraBehaviorSettings& B) { return A.MinimumCharacterSpeed < B.MinimumCharacterSpeed; });
		PreviousSettings = MovementBasedSettings[0];
		DefaultSpringArmLength = PreviousSettings.TargetArmLength;
	}

	SetComponentTickEnabled(IsActive());
}

void UILLDynamicCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	SpringArm->AttachToComponent(GetAttachmentRootActor()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
}

void UILLDynamicCameraComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* Owner = GetAttachmentRootActor();

	const FVector Velocity = Owner->GetVelocity();
	const float Speed = bUse2DVelocity ? Velocity.Size2D() : Velocity.Size();
	float ArmLength = DefaultSpringArmLength;

	// Adjust all settings based on speed
	if (MovementBasedSettings.Num() > 0)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();

		int32 TargetOffsetIndex = 0;
		for (; TargetOffsetIndex < MovementBasedSettings.Num(); ++TargetOffsetIndex)
		{
			if (Speed < MovementBasedSettings[TargetOffsetIndex].MinimumCharacterSpeed)
			{
				break;
			}
		}

		// Remove last increment from for loop (minimum of 0)
		TargetOffsetIndex = FMath::Max(0, TargetOffsetIndex - 1);

		if (CurrentOffsetIndex != TargetOffsetIndex)
		{
			// Store our last camera settings before moving to the new settings
			const float OldDelta = FMath::Clamp((CurrentTime - CurrentOffsetStartTimestamp - MovementBasedSettings[CurrentOffsetIndex].BlendInDelay) / MovementBasedSettings[CurrentOffsetIndex].BlendInTime, 0.f, 1.f);
			PreviousSettings = FCameraBehaviorSettings::Lerp(PreviousSettings, MovementBasedSettings[CurrentOffsetIndex], OldDelta);
			CurrentOffsetStartTimestamp = CurrentTime;
			CurrentOffsetIndex = TargetOffsetIndex;
		}

		// Blend into new camera settings
		const float Delta = FMath::Clamp((CurrentTime - CurrentOffsetStartTimestamp - MovementBasedSettings[CurrentOffsetIndex].BlendInDelay) / MovementBasedSettings[CurrentOffsetIndex].BlendInTime, 0.f, 1.f);
		const FCameraBehaviorSettings TargetOffset = FCameraBehaviorSettings::Lerp(PreviousSettings, MovementBasedSettings[CurrentOffsetIndex], Delta);
		TargetOffset.Apply(this);

		ArmLength = TargetOffset.TargetArmLength;
	}

	// Adjust arm length based on pitch allowing for ellipsoidal rotation
	if (SpringArmAdjustPitchCurve != nullptr)
	{
		if (AController* Controller = GetAttachmentRootActor()->GetInstigatorController())
		{
			const FRotator ControllerRotation = Controller->GetControlRotation();
			const float NormalizedPitch = FRotator::NormalizeAxis(ControllerRotation.Pitch);
			const float ArmAdjust = SpringArmAdjustPitchCurve->GetFloatValue(NormalizedPitch);
			ArmLength *= ArmAdjust;
		}
	}

	// Adjust arm length based on velocity
	if (SpringArmAdjustSpeedCurve != nullptr)
	{
		const float ArmAdjust = SpringArmAdjustSpeedCurve->GetFloatValue(Speed);
		ArmLength *= ArmAdjust;
	}

	SpringArm->TargetArmLength = ArmLength;
}

void UILLDynamicCameraComponent::Activate(bool bReset /*= false*/)
{
	Super::Activate(bReset);

	SpringArm->Activate(bReset);
	SpringArm->SetComponentTickEnabled(true);
	Camera->Activate(bReset);
	Camera->SetComponentTickEnabled(true);

	// Re-init the spring arm
	const uint32 PrevSettings = SpringArm->bDoCollisionTest | (SpringArm->bEnableCameraLag << 1) | (SpringArm->bEnableCameraRotationLag << 2);
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;
	if (SpringArm->IsRegistered()) // Can't tick if we haven't registered yet. Please don't crash.
		SpringArm->TickComponent(0.f, ELevelTick::LEVELTICK_TimeOnly, nullptr);
	SpringArm->bDoCollisionTest = PrevSettings & 0x1;
	SpringArm->bEnableCameraLag = PrevSettings & 0x2;
	SpringArm->bEnableCameraRotationLag = PrevSettings & 0x4;

	// Re-init the Camera
	FMinimalViewInfo ViewInfo;
	Camera->GetCameraView(0.f, ViewInfo);

	SetComponentTickEnabled(true);
}

void UILLDynamicCameraComponent::Deactivate()
{
	Super::Deactivate();

	SpringArm->Deactivate();
	SpringArm->SetComponentTickEnabled(false);
	Camera->Deactivate();
	Camera->SetComponentTickEnabled(false);

	SetComponentTickEnabled(false);
}

void UILLDynamicCameraComponent::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	if (MovementBasedSettings.Num() == 0)
		return;

	const UFont* SmallFont = GEngine->GetSmallFont();

	const float Delta = FMath::Clamp((GetWorld()->GetTimeSeconds() - CurrentOffsetStartTimestamp - MovementBasedSettings[CurrentOffsetIndex].BlendInDelay) / MovementBasedSettings[CurrentOffsetIndex].BlendInTime, 0.f, 1.f);
	YPos += YL;
	// MovementBasedSettings.Num(), blending CurrentOffsetIndex (Delta)
	Canvas->DrawText(SmallFont, FString::Printf(TEXT("%d settings, blending %d (%.2f)"), MovementBasedSettings.Num(), CurrentOffsetIndex, Delta), 16.f, YPos);

	const auto DisplaySettings = [&](const FCameraBehaviorSettings& CameraSettings, bool bIsBeingBlended)
	{
		FString Lag;
		if (CameraSettings.bEnableCameraLag)
		{
			Lag += FString::Printf(TEXT(" Lag (%.2f:%.2f) "), CameraSettings.CameraLagSpeed, CameraSettings.CameraLagMaxDistance);
		}

		if (CameraSettings.bEnableCameraRotationLag)
		{
			Lag.TrimEndInline();
			Lag += FString::Printf(TEXT(" RotLag (%.2f) "), CameraSettings.CameraRotationLagSpeed);
		}

		Lag.TrimEndInline();

		YPos += YL;
		// > MinSpeed: BlendInDelay + BlendInTime, Arm: TargetArmLength Lag? (LagSpeed:MaxLagDistance) RotLag? (RotLagSpeed) Cam: FoV @ Ratio
		Canvas->DrawText(SmallFont, FString::Printf(TEXT("%s%.1f: %.1f + %.1f; Arm: %.2f %s Cam: %.2f @ %.4f"),
			bIsBeingBlended ? TEXT("> ") : TEXT(""), CameraSettings.MinimumCharacterSpeed, CameraSettings.BlendInDelay, CameraSettings.BlendInTime,
			CameraSettings.TargetArmLength, *Lag, CameraSettings.FieldOfView, CameraSettings.AspectRatio),
			32.f, YPos);
	};

	if (Delta < 1.f)
	{
		Canvas->SetDrawColor(FColor::Red);
		DisplaySettings(PreviousSettings, true);
	}

	for (int32 SettingsIndex = 0; SettingsIndex < MovementBasedSettings.Num(); ++SettingsIndex)
	{
		const bool bIsBeingBlended = SettingsIndex == CurrentOffsetIndex;
		Canvas->SetDrawColor(bIsBeingBlended ? FColor::Yellow : FColor::Cyan);
		DisplaySettings(MovementBasedSettings[SettingsIndex], bIsBeingBlended);
	}
}
