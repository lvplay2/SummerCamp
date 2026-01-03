// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBarricade.h"

#include "ILLCharacterAnimInstance.h"

#include "SCCounselorCharacter.h"
#include "SCDoor.h"
#include "SCGameMode.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"
#include "SCSpecialMoveComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#define BARRICADE_ENABLED_ANGLE -90.0f // Look, zeroes on the end, just for you Dan!
#define BARRICADE_DISABLED_ANGLE 0.0f

static const FName NAME_BarricadeRotation(TEXT("Barricade"));

ASCBarricade::ASCBarricade(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	NetUpdateFrequency = 5.0f;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	DestroyedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestroyedMesh"));
	DestroyedMesh->SetupAttachment(RootComponent);

	CloseBarricade_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CloseBarricade_SpecialMoveHandler"));
	CloseBarricade_SpecialMoveHandler->SetupAttachment(RootComponent);
	CloseBarricade_SpecialMoveHandler->bTakeCameraControl = false;

	OpenBarricade_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("OpenBarricade_SpecialMoveHandler"));
	OpenBarricade_SpecialMoveHandler->SetupAttachment(RootComponent);
	OpenBarricade_SpecialMoveHandler->bTakeCameraControl = false;

	UseBarricadeDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("UseBarricadeDriver"));
	UseBarricadeDriver->SetNotifyName(TEXT("UseBarricade"));
}

void ASCBarricade::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCBarricade, bEnabled);
	DOREPLIFETIME(ASCBarricade, bDestroyed);
	DOREPLIFETIME(ASCBarricade, OriginalRotation);
}

void ASCBarricade::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	CloseBarricade_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCBarricade::OnUseStarted);
	CloseBarricade_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCBarricade::OnUsedBarricade);
	CloseBarricade_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCBarricade::OnBarricadeAborted);
	OpenBarricade_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCBarricade::OnUseStarted);
	OpenBarricade_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCBarricade::OnUsedBarricade);
	OpenBarricade_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCBarricade::OnBarricadeAborted);

	if (UseBarricadeDriver)
	{
		FAnimNotifyEventDelegate TickDelegate;
		TickDelegate.BindDynamic(this, &ASCBarricade::UseBarricadeDriverTick);

		UseBarricadeDriver->InitializeTick(TickDelegate);
	}

	DestroyedMesh->SetVisibility(false);
}

void ASCBarricade::BeginPlay()
{
	Super::BeginPlay();

	// Search for a door in the barricade in case this become relevant after the door
	if (LinkedDoor == nullptr)
	{
		float Distance = 150.f;
		TArray<FOverlapResult> Doors;
		GetWorld()->OverlapMultiByChannel(Doors, GetActorLocation(), FQuat::Identity, ECC_WorldDynamic, FCollisionShape::MakeSphere(Distance));

		// We want to find the nearest door...
		ASCDoor* NearestDoor = nullptr;
		for (FOverlapResult& Result : Doors)
		{
			if (ASCDoor* Door = Cast<ASCDoor>(Result.Actor.Get()))
			{
				float DistToDoor = (Door->GetActorLocation() - GetActorLocation()).Size2D();
				if (DistToDoor < Distance)
				{
					Distance = DistToDoor;
					NearestDoor = Door;
				}
			}
		}

		if (NearestDoor != nullptr)
		{
			LinkedDoor = NearestDoor;
			LinkedDoor->SetLinkedBarricade(this);
		}
	}

	if (Role == ROLE_Authority)
		OriginalRotation = Mesh->RelativeRotation;
}

void ASCBarricade::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (LinkedDoor)
	{
		LinkedDoor->SetLinkedBarricade(nullptr);
	}
}

void ASCBarricade::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	// Apply perk modifiers to the montage play rate
	const float MontagePlayRate = [&Character]()
	{
		float PlayRate = 1.0f;

		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
		{
			if (ASCPlayerState* PS = Counselor->GetPlayerState())
			{
				for (const USCPerkData* Perk : PS->GetActivePerks())
				{
					check(Perk);

					if (!Perk)
						continue;

					PlayRate += Perk->BarricadeSpeedModifier;
				}
			}
		}

		return PlayRate;
	}();

	switch (InteractMethod)
	{
	case EILLInteractMethod::Hold:
		if (bEnabled)
		{
			OpenBarricade_SpecialMoveHandler->ActivateSpecial(Character, MontagePlayRate);
		}
		else
		{
			CloseBarricade_SpecialMoveHandler->ActivateSpecial(Character, MontagePlayRate);

			if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
			{
				GameMode->HandleScoreEvent(Character->PlayerState, BarricadeDoorScoreEvent);
			}
		}
		break;
	}
}

void ASCBarricade::SetEnabled(bool Enable)
{
	if (Role < ROLE_Authority)
		return;

	if (LinkedDoor && LinkedDoor->IsOpen())
		return;

	bEnabled = Enable;
	OnRep_Enabled();
}

void ASCBarricade::OnRep_Enabled()
{
	FRotator NewRot = OriginalRotation;
	NewRot.Pitch = bEnabled ? BARRICADE_ENABLED_ANGLE : BARRICADE_DISABLED_ANGLE;
	Mesh->SetRelativeRotation(NewRot, false, nullptr, ETeleportType::TeleportPhysics);
}

void ASCBarricade::OnRep_Destroyed()
{
	Mesh->SetVisibility(!bDestroyed);
	DestroyedMesh->SetVisibility(bDestroyed);
	if (bDestroyed && DamagedAudio)
		UGameplayStatics::PlaySoundAtLocation(this, DamagedAudio, GetActorLocation());
}

void ASCBarricade::BreakAndDestroy()
{
	if (bEnabled || bIsBarricading)
	{
		bDestroyed = true;
		OnRep_Destroyed();
	}
}

bool ASCBarricade::IsEnabled() const
{
	return bEnabled;
}

void ASCBarricade::OnUseStarted(ASCCharacter* Interactor)
{
	UseBarricadeDriver->SetNotifyOwner(Interactor);

	// When we get to this point these are flipped...
	if (!bEnabled)
	{
		bIsBarricading = true;
		UGameplayStatics::PlaySoundAtLocation(this, EnableBarricadeAudio, GetActorLocation());
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(this, DisableBarricadeAudio, GetActorLocation());
	}
}

void ASCBarricade::OnUsedBarricade(ASCCharacter* Interactor)
{
	UseBarricadeDriver->SetNotifyOwner(nullptr);

	if (Interactor && Interactor->IsLocallyControlled())
	{
		if (LinkedDoor)
		{
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(LinkedDoor->InteractComponent);
		}
	}

	bIsBarricading = false;

	if (Role == ROLE_Authority)
	{
		SetEnabled(!bEnabled);
	}
}

void ASCBarricade::OnBarricadeAborted(ASCCharacter* Interactor)
{
	UseBarricadeDriver->SetNotifyOwner(nullptr);
	bIsBarricading = false;

	// If we're destroyed we'll want the barricade to be lowered, always.
	// Otherwise set the barricade back to where it was before we aborted the move.
	if (!bDestroyed)
	{
		FRotator NewRot = OriginalRotation;
		NewRot.Pitch = bEnabled ? BARRICADE_ENABLED_ANGLE : BARRICADE_DISABLED_ANGLE;
		Mesh->SetRelativeRotation(NewRot, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else if (Role == ROLE_Authority)
	{
		// We're destroyed, force the barricade down all the way
		SetEnabled(true);
	}

	if (Interactor && Interactor->IsLocallyControlled())
	{
		if (LinkedDoor)
		{
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(LinkedDoor->InteractComponent);
		}
	}
}

void ASCBarricade::UseBarricadeDriverTick(FAnimNotifyData NotifyData)
{
	UILLCharacterAnimInstance* AnimInstance = Cast<UILLCharacterAnimInstance>(NotifyData.AnimInstance);
	FRotator NewRot = OriginalRotation;
	NewRot.Pitch = FMath::Lerp(BARRICADE_DISABLED_ANGLE, BARRICADE_ENABLED_ANGLE, FMath::Clamp(AnimInstance->GetUnblendedCurveValue(NAME_BarricadeRotation), 0.0f, 1.0f));
	Mesh->SetRelativeRotation(NewRot, false, nullptr, ETeleportType::TeleportPhysics);
}
