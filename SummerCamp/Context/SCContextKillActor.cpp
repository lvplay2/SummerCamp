// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCContextKillActor.h"

#include "Animation/AnimMontage.h"

#include "SCContextKillComponent.h"
#include "SCCounselorCharacter.h"
#include "SCGameMode_Hunt.h"
#include "SCGameState.h"
#include "SCInteractComponent.h"
#include "SCInteractableManagerComponent.h"
#include "SCKillerCharacter.h"
#include "SCSpecialMove_SoloMontage.h"

/*
				WARNING: THIS CLASS USES NET DORMANCY
	This means that if you need to update any replicated variables,
	logic, RPCs or other networking whatnot, you NEED to update the
	logic handling for dormancy.
*/

ASCContextKillActor::ASCContextKillActor(const FObjectInitializer& ObjectInitialzier)
: Super(ObjectInitialzier)
, bOnlyUseableOnce(false)
, bUsed(false)
, bUseFocalPointForKillCam(true)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	bReplicates = true;
	bAlwaysRelevant = false;
	NetUpdateFrequency = 10.0f;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	ContextKillComponent = CreateDefaultSubobject<USCContextKillComponent>(TEXT("ContextKillComponent"));
	ContextKillComponent->SetupAttachment(RootComponent);
	ContextKillComponent->bIsGrabKill = true;

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable"));
	InteractComponent->SetupAttachment(RootComponent);
	InteractComponent->ActorIgnoreList.Add(ASCCounselorCharacter::StaticClass());

	CameraFocalPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ContextCameraFocalPoint"));
	CameraFocalPoint->SetupAttachment(ContextKillComponent);
	CameraFocalPoint->RelativeLocation = FVector(0.f, 0.f, 100.f);

	FXDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("FXDriver"));
	FXDriver->SetNotifyName(TEXT("FXDriver"));
}

void ASCContextKillActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCContextKillActor, bUsed);
}

void ASCContextKillActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent)
	{
		InteractComponent->OnInteract.AddDynamic(this, &ASCContextKillActor::ActivateKill);
		InteractComponent->OnCanInteractWith.BindDynamic(this, &ASCContextKillActor::CanInteractWith);
	}

	FXDriverBeginDelegate.BindDynamic(this, &ASCContextKillActor::FXDriverBegin);
	FXDriverTickDelegate.BindDynamic(this, &ASCContextKillActor::FXDriverTick);
	FXDriverEndDelegate.BindDynamic(this, &ASCContextKillActor::FXDriverEnd);

	if (FXDriver)
	{
		FXDriver->InitializeNotifies(FXDriverBeginDelegate, FXDriverTickDelegate, FXDriverEndDelegate);
	}

	// Register focal point to ContextKillComponent
	ContextKillComponent->GetContextCameraFocus.BindDynamic(this, &ASCContextKillActor::GetCameraFocalPoint);
	ContextKillComponent->KillStarted.AddDynamic(this, &ASCContextKillActor::KillStarted);
	ContextKillComponent->KillComplete.AddDynamic(this, &ASCContextKillActor::KillCompleted);

	SetActorTickEnabled(bShouldOrientTowardKiller);
}

void ASCContextKillActor::BeginPlay()
{
	Super::BeginPlay();

	SetNetDormancy(DORM_DormantAll); // Start out asleep
}

void ASCContextKillActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_AnimationPlaying);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCContextKillActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Only rotate when we want to!
	if (bShouldOrientTowardKiller)
	{
		// Gotta have a killer who isn't killing, but has someone they want to kill... if we're going to update
		ASCKillerCharacter* Killer = TryGetKiller();
		if (!Killer || Killer->IsInContextKill() || !Killer->GetGrabbedCounselor())
			return;

		// Too far to need updating
		FVector FromKiller = GetActorLocation() - Killer->GetActorLocation();
		if (FromKiller.SizeSquared2D() > FMath::Square(InteractComponent->HighlightDistance))
			return;

		FromKiller.Z = 0.f;

		// Rotate our actor
		const FRotator FromKillerRot = FromKiller.ToOrientationRotator();
		SetActorRotation(FromKillerRot, ETeleportType::TeleportPhysics);

		// Unrotate scene components that don't want the update
		if (IgnoredRotationComponents.Num())
		{
			const FRotator InvRot = FromKillerRot.GetInverse();
			for (USceneComponent* SceneComponent : IgnoredRotationComponents)
			{
				SceneComponent->SetRelativeRotation(InvRot, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}
}

int32 ASCContextKillActor::CanInteractWith(class AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (bUsed)
	{
		return 0;
	}

	// shouldnt be able to use a context kill if you cant grab kill
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
	{
		if (!Killer->CanGrabKill())
		{
			return 0;
		}
	}

	return ContextKillComponent->CanInteractWith(Character, ViewLocation, ViewRotation);
}

USceneComponent* ASCContextKillActor::GetCameraFocalPoint()
{
	if (bUseFocalPointForKillCam)
		return CameraFocalPoint;

	return nullptr;
}

void ASCContextKillActor::ActivateKill(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	if (Character->IsInSpecialMove())
	{
		Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
		return;
	}

	ForceNetUpdate(); // Make sure the interaction replicated
	switch (InteractMethod)
	{
		case EILLInteractMethod::Press:
			// May not be a killer, the hunter can perform a context kill as well (killing Jason)
			if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor))
			{
				ContextKillComponent->ActivateGrabKill(Killer);
			}
			else // code for the hunter kill
			{
				if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
				{
					if (GS->CurrentKiller)
					{
						if (GS->CurrentKiller->HasActiveKill(this))
						{
							ContextKillComponent->SERVER_ActivateContextKill(Character, GS->CurrentKiller);
						}
					}
				}
			}
			break;
	}
	Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
}

void ASCContextKillActor::SetAnimatedKillMesh(USkeletalMeshComponent* InKillMesh, UAnimSequenceBase* InKillAnimation)
{
	KillMesh = InKillMesh;
	KillMeshAnimation = InKillAnimation;
	KillMesh->bNoSkeletonUpdate = true; // Default the mesh to asleep
}

void ASCContextKillActor::KillStarted(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	if (Role == ROLE_Authority)
	{
		ForceNetUpdate();
		// Turn relevance on while the kill is in progress so that net updates aren't lost on characters far away.
		bAlwaysRelevant = true;
		if (bOnlyUseableOnce)
			bUsed = true;
	}

	if (KillMesh && KillMeshAnimation)
	{
		// Start updating the skeletal mesh again and play the kill animation on it
		KillMesh->bNoSkeletonUpdate = false;
		KillMesh->PlayAnimation(KillMeshAnimation, false);

		// When the animation is done playing, stop updating the mesh again
		FTimerDelegate Delegate;
		Delegate.BindLambda([](USkeletalMeshComponent* SkeleMesh)
		{
			if (IsValid(SkeleMesh))
				SkeleMesh->bNoSkeletonUpdate = true;
		}, KillMesh);

		GetWorldTimerManager().SetTimer(TimerHandle_AnimationPlaying, Delegate, KillMeshAnimation->SequenceLength, false);
	}

	OnKillStarted.Broadcast(Interactor, Victim);
}

void ASCContextKillActor::KillCompleted(ASCCharacter* Killer, ASCCharacter* Victim)
{
	// Turn relevance back off once the kill is complete.
	bAlwaysRelevant = false;
	OnKillCompleted.Broadcast(Killer, Victim);
}

void ASCContextKillActor::FXDriverBegin(FAnimNotifyData NotifyData)
{
	FXBegin(NotifyData);
}

void ASCContextKillActor::FXDriverTick(FAnimNotifyData NotifyData)
{
	FXTick(NotifyData);
}

void ASCContextKillActor::FXDriverEnd(FAnimNotifyData NotifyData)
{
	FXEnd(NotifyData);
}

void ASCContextKillActor::AddIgnoredRotationComponents(TArray<USceneComponent*> Components)
{
	for (USceneComponent* SceneComponent : Components)
	{
		if (SceneComponent->GetOwner() == this)
			IgnoredRotationComponents.Add(SceneComponent);
	}
}

ASCKillerCharacter* ASCContextKillActor::TryGetKiller() const
{
	UWorld* World = GetWorld();
	ASCGameState* GameState = World ? Cast<ASCGameState>(World->GetGameState()) : nullptr;
	if (!GameState)
		return nullptr;

	// TODO: Support multiple killers (return the closest?)
	return GameState->CurrentKiller;
}
