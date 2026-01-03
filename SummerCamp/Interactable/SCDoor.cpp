// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCDoor.h"

#include "AI/Navigation/NavAreas/NavArea_Default.h"
#include "AI/Navigation/NavModifierComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "NavAreas/SCNavArea_LockedDoorMultiAgent.h"
#include "NavAreas/SCNavArea_UnlockedDoor.h"
#if WITH_APEX
# include "DestructibleMesh.h"
#endif

#include "ILLCharacterAnimInstance.h"
#include "ILLPlayerInput.h"

#include "SCBarricade.h"
#include "SCContextKillComponent.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCGameState_Hunt.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"
#include "SCPlayerState.h"
#include "SCSpecialMoveComponent.h"
#include "SCStatusIconComponent.h"
#include "SCWeapon.h"
#include "SCGameState_Paranoia.h"

#define OPEN_DOOR_ANGLE -90.f
#define CLOSE_DOOR_ANGLE 0.f

static const FName NAME_DoorRotation(TEXT("DoorRotation"));

ASCDoor::ASCDoor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)

// Components

// Config
, LightDestructionMesh(nullptr)
, LightDestructionThreshold( 0.8f)
, MediumDestructionMesh(nullptr)
, MediumDestructionThreshold( 0.6f)
, HeavyDestructionMesh(nullptr)
, HeavyDestructionThreshold(0.4f)
, bIsLockable(true)
, BurstExplosionForce(5000.f)
, Health(INT_MAX) // Set to force OnRep regardless of health
, BaseDoorHealth(100)
, BreakExplosionForce(500.f)
, OpenDoorSound(nullptr)
, CloseDoorSound(nullptr)
, LockedDoorSound(nullptr)

// Interaction

// State
, bIsOpen(false)
, bIsLocked(false)
, bIsDestroyed(false)
, LifespanAfterDestruction(10.f)

// Utilites
, StartAngle(CLOSE_DOOR_ANGLE)
, TargetAngle(CLOSE_DOOR_ANGLE)
{
	// Undo settings changed by ANavLinkProxy
	SetActorEnableCollision(true);
	bCanBeDamaged = true;
	bHidden = false;

	bReplicates = true;
	bAlwaysRelevant = false;
	NetUpdateFrequency = 40.0f;
	//NetCullDistanceSquared = FMath::Square(500.f); // Comment in to test relevance locally

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(RootComponent);
	Pivot->SetIsReplicated(false);

	Mesh = CreateDefaultSubobject<UDestructibleComponent>(TEXT("StaticMesh"));
	Mesh->SetupAttachment(Pivot);
	Mesh->bGenerateOverlapEvents = false;

	DoorCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorCollision"));
	DoorCollision->SetupAttachment(Mesh);
	DoorCollision->bGenerateOverlapEvents = false;

	VehicleCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("VehicleCollision"));
	VehicleCollision->SetupAttachment(RootComponent);
	VehicleCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VehicleCollision->SetCollisionResponseToChannels(FCollisionResponseContainer(ECR_Ignore));
	VehicleCollision->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	VehicleCollision->SetCanEverAffectNavigation(false);
	VehicleCollision->bGenerateOverlapEvents = false;

	AIPassthroughVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("AIPassthroughVolume"));
	AIPassthroughVolume->SetupAttachment(RootComponent);

	NavigationModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavigationModifier"));

	OutArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Facing"));
	OutArrow->SetupAttachment(RootComponent);

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("InteractComponent"));
	InteractComponent->InteractMethods = (int32)EILLInteractMethod::Press | (int32)EILLInteractMethod::Hold;
	InteractComponent->HoldTimeLimit = 2.f;
	InteractComponent->bCanHighlight = true;
	InteractComponent->HighlightDistance = 300.f;
	InteractComponent->SetupAttachment(Pivot);

	OpenCloseDoorDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("OpenCloseDoorDriver"));
	OpenCloseDoorDriver->SetNotifyName(TEXT("OpenCloseDoor"));

	BreakDownDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("BreakDownDriver"));
	BreakDownDriver->SetNotifyName(TEXT("BreakDownSync"));

	BreakDownLoopDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("BreakDownLoopDriver"));
	BreakDownLoopDriver->SetNotifyName(TEXT("BreakDownLoop"));

	KillerOpenDoorInside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("KillerOpenDoorInside_SpecialMoveHandler"));
	KillerOpenDoorInside_SpecialMoveHandler->SetupAttachment(RootComponent);
	KillerOpenDoorInside_SpecialMoveHandler->bTakeCameraControl = false;

	KillerOpenDoorOutside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("KillerOpenDoorOutside_SpecialMoveHandler"));
	KillerOpenDoorOutside_SpecialMoveHandler->SetupAttachment(RootComponent);
	KillerOpenDoorOutside_SpecialMoveHandler->bTakeCameraControl = false;

	LockDoorInside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("LockDoorInside_SpecialMoveHandler"));
	LockDoorInside_SpecialMoveHandler->SetupAttachment(RootComponent);
	LockDoorInside_SpecialMoveHandler->bTakeCameraControl = false;

	UnlockDoorInside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("UnlockDoorInside_SpecialMoveHandler"));
	UnlockDoorInside_SpecialMoveHandler->SetupAttachment(RootComponent);
	UnlockDoorInside_SpecialMoveHandler->bTakeCameraControl = false;

	UnlockDoorOutside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("UnlockDoorOutside_SpecialMoveHandler"));
	UnlockDoorOutside_SpecialMoveHandler->SetupAttachment(RootComponent);
	UnlockDoorOutside_SpecialMoveHandler->bTakeCameraControl = false;

	OpenDoorInside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("OpenDoorInside_SpecialMoveHandler"));
	OpenDoorInside_SpecialMoveHandler->SetupAttachment(RootComponent);
	OpenDoorInside_SpecialMoveHandler->bTakeCameraControl = false;

	OpenDoorOutside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("OpenDoorOutside_SpecialMoveHandler"));
	OpenDoorOutside_SpecialMoveHandler->SetupAttachment(RootComponent);
	OpenDoorOutside_SpecialMoveHandler->bTakeCameraControl = false;

	OpenDoorWheelchairOutside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("OpenDoorWheelchairOutside_SpecialMoveHandler"));
	OpenDoorWheelchairOutside_SpecialMoveHandler->SetupAttachment(RootComponent);
	OpenDoorWheelchairOutside_SpecialMoveHandler->bTakeCameraControl = false;

	CloseDoorInsideForward_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CloseDoorInsideForward_SpecialMoveHandler"));
	CloseDoorInsideForward_SpecialMoveHandler->SetupAttachment(RootComponent);
	CloseDoorInsideForward_SpecialMoveHandler->bTakeCameraControl = false;

	CloseDoorInsideBackward_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CloseDoorInsideBackward_SpecialMoveHandler"));
	CloseDoorInsideBackward_SpecialMoveHandler->SetupAttachment(RootComponent);
	CloseDoorInsideBackward_SpecialMoveHandler->bTakeCameraControl = false;

	CloseDoorOutsideForward_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CloseDoorOutsideForward_SpecialMoveHandler"));
	CloseDoorOutsideForward_SpecialMoveHandler->SetupAttachment(RootComponent);
	CloseDoorOutsideForward_SpecialMoveHandler->bTakeCameraControl = false;

	CloseDoorOutsideBackward_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CloseDoorOutsideBackward_SpecialMoveHandler"));
	CloseDoorOutsideBackward_SpecialMoveHandler->SetupAttachment(RootComponent);
	CloseDoorOutsideBackward_SpecialMoveHandler->bTakeCameraControl = false;

	BurstDoorFromInside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("BurstDoorFromInside_SpecialMoveHandler"));
	BurstDoorFromInside_SpecialMoveHandler->SetupAttachment(RootComponent);
	BurstDoorFromInside_SpecialMoveHandler->bTakeCameraControl = false;

	BurstDoorFromOutside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("BurstDoorFromOutside_SpecialMoveHandler"));
	BurstDoorFromOutside_SpecialMoveHandler->SetupAttachment(RootComponent);
	BurstDoorFromOutside_SpecialMoveHandler->bTakeCameraControl = false;

	BurstDoorDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("BurstDoorDriver"));
	BurstDoorDriver->SetNotifyName(TEXT("WallSmashDriver"));

	BreakDownFromInside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("BreakDownFromInside_SpecialMoveHandler"));
	BreakDownFromInside_SpecialMoveHandler->SetupAttachment(RootComponent);
	BreakDownFromInside_SpecialMoveHandler->bTakeCameraControl = false;

	BreakDownFromOutside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("BreakDownFromOutside_SpecialMoveHandler"));
	BreakDownFromOutside_SpecialMoveHandler->SetupAttachment(RootComponent);
	BreakDownFromOutside_SpecialMoveHandler->bTakeCameraControl = false;

	ContextKillComponent = CreateDefaultSubobject<USCContextKillComponent>(TEXT("ContextKillComponent"));
	ContextKillComponent->SetupAttachment(RootComponent);

	DoorKillDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("DoorKillDriver"));
	DoorKillDriver->SetNotifyName(TEXT("DoorKillDriver"));

	LockedIcon = CreateDefaultSubobject<USCStatusIconComponent>(TEXT("LockedIcon"));
	LockedIcon->SetupAttachment(RootComponent);
}

void ASCDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCDoor, bIsOpen);
	DOREPLIFETIME(ASCDoor, bIsLocked);
	DOREPLIFETIME(ASCDoor, Health);
}

//////////////////////////////////////////////////////////////////////////////////
// AActor Interface
void ASCDoor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Set up all interact components on this object. This is used to enable having 2 interact locations for the grendel doors.
	TArray<USceneComponent*> ChildComponents;
	RootComponent->GetChildrenComponents(true, ChildComponents);

	for (USceneComponent* ChildComp : ChildComponents)
	{
		if (USCInteractComponent* Interact = Cast<USCInteractComponent>(ChildComp))
		{
			InteractComponents.Add(Interact);
			Interact->OnInteract.AddDynamic(this, &ASCDoor::OnInteract);
			Interact->OnCanInteractWith.BindDynamic(this, &ASCDoor::CanInteractWith);
			Interact->OnHoldStateChanged.AddDynamic(this, &ASCDoor::OnHoldStateChanged);
		}
	}

	if (AIPassthroughVolume)
	{
		AIPassthroughVolume->OnComponentEndOverlap.AddDynamic(this, &ASCDoor::OnActorPassedThrough);
	}

	if (OpenCloseDoorDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		FAnimNotifyEventDelegate TickDelegate;
		FAnimNotifyEventDelegate EndDelegate;
		BeginDelegate.BindDynamic(this, &ASCDoor::OpenCloseDoorDriverBegin);
		TickDelegate.BindDynamic(this, &ASCDoor::OpenCloseDoorDriverTick);
		EndDelegate.BindDynamic(this, &ASCDoor::OpenCloseDoorDriverEnd);
		OpenCloseDoorDriver->InitializeAnimDriver(nullptr, BeginDelegate, TickDelegate, EndDelegate);
		OpenCloseDoorDriver->SetAutoClear(false);
	}

	if (BreakDownDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		FAnimNotifyEventDelegate EndDelegate;
		BeginDelegate.BindDynamic(this, &ASCDoor::BreakDownSync);
		EndDelegate.BindDynamic(this, &ASCDoor::BreakDownDamage);
		BreakDownDriver->InitializeBeginEnd(BeginDelegate, EndDelegate);
		BreakDownDriver->SetAutoClear(false);
	}

	if (BreakDownLoopDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCDoor::BreakDownLoop);
		BreakDownLoopDriver->InitializeBegin(BeginDelegate);
		BreakDownLoopDriver->SetAutoClear(false);
	}

	if (BurstDoorDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCDoor::BurstDestroyDoor);
		BurstDoorDriver->InitializeBegin(BeginDelegate);
	}

	if (DoorKillDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		FAnimNotifyEventDelegate TickDelegate;
		FAnimNotifyEventDelegate EndDelegate;
		BeginDelegate.BindDynamic(this, &ASCDoor::OpenCloseDoorDriverBegin);
		TickDelegate.BindDynamic(this, &ASCDoor::DoorKillDriverTick);
		EndDelegate.BindDynamic(this, &ASCDoor::DoorKillDriverEnd);
		DoorKillDriver->InitializeAnimDriver(nullptr, BeginDelegate, TickDelegate, EndDelegate);
	}

	LockDoorInside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::ToggleLock);
	UnlockDoorInside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::ToggleLock);
	UnlockDoorOutside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::ToggleLock);

	// Killer Open
	KillerOpenDoorInside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OpenCloseDoorCompleted);
	KillerOpenDoorInside_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDoor::OpenCloseDoorAborted);
	KillerOpenDoorOutside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OpenCloseDoorCompleted);
	KillerOpenDoorOutside_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDoor::OpenCloseDoorAborted);

	// Counselor Open
	OpenDoorInside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OpenCloseDoorCompleted);
	OpenDoorInside_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDoor::OpenCloseDoorAborted);
	OpenDoorInside_SpecialMoveHandler->SpecialSpedUp.AddDynamic(this, &ASCDoor::OpenDoorSpedUp);
	OpenDoorOutside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OpenCloseDoorCompleted);
	OpenDoorOutside_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDoor::OpenCloseDoorAborted);
	OpenDoorOutside_SpecialMoveHandler->SpecialSpedUp.AddDynamic(this, &ASCDoor::OpenDoorSpedUp);
	OpenDoorWheelchairOutside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OpenCloseDoorCompleted);
	OpenDoorWheelchairOutside_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDoor::OpenCloseDoorAborted);
	OpenDoorWheelchairOutside_SpecialMoveHandler->SpecialSpedUp.AddDynamic(this, &ASCDoor::OpenDoorSpedUp);

	// Counselor Close
	CloseDoorInsideForward_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OpenCloseDoorCompleted);
	CloseDoorInsideForward_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDoor::OpenCloseDoorAborted);
	CloseDoorInsideBackward_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OpenCloseDoorCompleted);
	CloseDoorInsideBackward_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDoor::OpenCloseDoorAborted);
	CloseDoorOutsideForward_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OpenCloseDoorCompleted);
	CloseDoorOutsideForward_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDoor::OpenCloseDoorAborted);
	CloseDoorOutsideBackward_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OpenCloseDoorCompleted);
	CloseDoorOutsideBackward_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDoor::OpenCloseDoorAborted);

	// Rage
	BurstDoorFromInside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OnBurstComplete);
	BurstDoorFromOutside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OnBurstComplete);

	// Breakdown
	BreakDownFromInside_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCDoor::OnBreakDownStart);
	BreakDownFromInside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OnBreakDownStop);

	BreakDownFromOutside_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCDoor::OnBreakDownStart);
	BreakDownFromOutside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OnBreakDownStop);

	// Murder
	ContextKillComponent->KillComplete.AddDynamic(this, &ASCDoor::KillComplete);

	if (Role == ROLE_Authority)
	{
		Health = BaseDoorHealth;
	}
}

void ASCDoor::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize the door's rotation
	StartAngle = Pivot->GetRelativeTransform().GetRotation().Rotator().Yaw;

	// Force ticking off at game start (will turn on when we need it)
	SetActorTickEnabled(false);

	// Search for a barricade in the door in case this become relevant after the barricade
	if (LinkedBarricade == nullptr)
	{
		const float Distance = 150.f;
		TArray<FOverlapResult> Barricades;
		GetWorld()->OverlapMultiByChannel(Barricades, GetActorLocation(), FQuat::Identity, ECC_WorldDynamic, FCollisionShape::MakeSphere(Distance));

		for (FOverlapResult& Result : Barricades)
		{
			if (ASCBarricade* Barricade = Cast<ASCBarricade>(Result.Actor.Get()))
			{
				SetLinkedBarricade(Barricade);
				LinkedBarricade->LinkedDoor = this;
				break;
			}
		}
	}
}

float ASCDoor::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	check(EventInstigator && EventInstigator->Role == ROLE_Authority);

	// Only the killer should be able to hurt the door
	ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(DamageCauser);
	if (!Killer)
		return 0.f;

	int32 OldHealth = Health;
	const float PreviousDestructionRatio = (float)Health / (float)BaseDoorHealth;
	Health -= IsBarricaded() ? DamageAmount * 0.5f : DamageAmount;

	if (Health <= 0)
	{
		if (!bIsDestroyed)
		{
			Killer->SERVER_DestroyDoor(this, BreakExplosionForce);
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), DoorDestroySoundCue, GetActorLocation());
		}
	}
	else
	{
		const float DestructionRatio = (float)Health / (float)BaseDoorHealth;
		if (DestructionRatio <= HeavyDestructionThreshold && PreviousDestructionRatio > HeavyDestructionThreshold)
		{
			UpdateDestructionState(HeavyDestructionMesh);
		}
		else if (DestructionRatio <= MediumDestructionThreshold && PreviousDestructionRatio > MediumDestructionThreshold)
		{
			UpdateDestructionState(MediumDestructionMesh);
		}
		else if (DestructionRatio <= LightDestructionThreshold && PreviousDestructionRatio > LightDestructionThreshold)
		{
			UpdateDestructionState(LightDestructionMesh);
		}

		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DoorHitSoundCue, GetActorLocation());
	}

	// Make AI noise
	if (ASCCharacter* DestroyingCharacter = Cast<ASCCharacter>(DamageCauser))
		MakeNoise(1.f, DestroyingCharacter);

	return DamageAmount;
}

void ASCDoor::OnRep_Health(int32 OldHealth)
{
	if (Health > 0)
	{
		const float PreviousDestructionRatio = (float)OldHealth / (float)BaseDoorHealth;
		const float DestructionRatio = (float)Health / (float)BaseDoorHealth;
		if (DestructionRatio <= HeavyDestructionThreshold && PreviousDestructionRatio > HeavyDestructionThreshold)
		{
			UpdateDestructionState(HeavyDestructionMesh);
		}
		else if (DestructionRatio <= MediumDestructionThreshold && PreviousDestructionRatio > MediumDestructionThreshold)
		{
			UpdateDestructionState(MediumDestructionMesh);
		}
		else if (DestructionRatio <= LightDestructionThreshold && PreviousDestructionRatio > LightDestructionThreshold)
		{
			UpdateDestructionState(LightDestructionMesh);
		}

		// Don't play the sound if the door is healing. A.K.A. the initial on rep at the beginning of the match.
		if (HasActorBegunPlay() && Health != BaseDoorHealth)
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), DoorHitSoundCue, GetActorLocation());
	}
	else if (bIsDestroyed == false)
	{
		// This happened due to relevancy change, destroy the door anew!
		Local_DestroyDoor(nullptr, BreakExplosionForce);
	}
}

void ASCDoor::KillComplete(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	if (Interactor && Interactor->IsLocallyControlled())
	{
		for (USCInteractComponent* Interact : InteractComponents)
		{
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(Interact);
		}
	}

	if (Role == ROLE_Authority)
		Interactor->SERVER_OpenDoor(this, true);
}

void ASCDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Force us out of breaking down the door (if we have someone doing that)
	TryAbortBreakDown(true);

	SetLinkedBarricade(nullptr);

	if (TimerHandle_DestroyMesh.IsValid())
		GetWorldTimerManager().ClearTimer(TimerHandle_DestroyMesh);
}

// ~AActor Interface
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Components

void ASCDoor::OnActorPassedThrough(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->IsA<ASCCounselorCharacter>())
	{
		if (ASCCounselorAIController* CounselorAIController = Cast<ASCCounselorAIController>(OtherActor->GetInstigatorController()))
		{
			CounselorAIController->OnPassedThroughDoor(this);
		}
	}
}

// ~Components
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Interaction

int32 ASCDoor::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	// Can't interact if we're destroyed or a killer is breaking down the door
	if (bIsDestroyed || IsBeingBrokenDown())
		return 0;

	if (Character->IsInSpecialMove())
		return 0;

	int32 Flags = 0;

	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
	{
		// No closing doors for Jason!
		if (!bIsOpen && !Killer->GetGrabbedCounselor())
		{
			// Jason should be able to rage through doors or break them down, regardless of if they're barricaded or locked... so... always?
			Flags |= (int32)EILLInteractMethod::Press;
		}

		// We can try and kill some bitches though.
		if (Killer->GetGrabbedCounselor() && IsInteractorInside(Killer) && !IsLockedOrBarricaded())
		{
			Flags |= ContextKillComponent->CanInteractWith(Character, ViewLocation, ViewRotation);
		}
	}
	else
	{
		// Inside the door, we can lock/unlock it
		if (IsInteractorInside(Character))
		{
			bool bCanLock = bIsLockable;
			// make sure you cant lock doors that dont have barricades in paranoia mode
			if (ASCGameState_Paranoia* GS = Cast<ASCGameState_Paranoia>(GetWorld()->GetGameState()))
			{
				bCanLock = LinkedBarricade != nullptr;
			}

			if (!bIsOpen && bCanLock)
			{
				Flags |= (int32)EILLInteractMethod::Hold;
			}

			// Is it unlocked?
			if (!IsLockedOrBarricaded())
			{
				Flags |= (int32)EILLInteractMethod::Press;
			}
		}
		else // Outside the door
		{
			// Is it unlocked?
			if (!IsLockedOrBarricaded())
			{
				Flags |= (int32)EILLInteractMethod::Press;
			}
			else if (bIsLocked) // Counselors can unlock doors from outside (prevents griefing)
			{
				Flags |= (int32)EILLInteractMethod::Hold;
			}
		}
	}

	return Flags;
}

void ASCDoor::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	const bool bIsInside = IsInteractorInside(Interactor);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
		{
			// Head slam
			if (Killer->GetGrabbedCounselor() != nullptr)
			{
				if (bIsInside)
				{
					ContextKillComponent->ActivateGrabKill(Killer);
					MULTICAST_SetKillDriverNotifyOwner(Killer);
				}
				else
				{
					for (USCInteractComponent* Interact : InteractComponents)
					{
						Killer->GetInteractableManagerComponent()->UnlockInteraction(Interact);
					}
				}

				return;
			}

			// Door burst
			if (Killer->IsRageActive())
			{
				if (IsInteractorInside(Killer))
				{
					BurstDoorFromInside_SpecialMoveHandler->ActivateSpecial(Killer);
				}
				else
				{
					BurstDoorFromOutside_SpecialMoveHandler->ActivateSpecial(Killer);
				}

				MULTICAST_SetBurstNotifyOwner(Killer);
				return;
			}

			// Can't open a locked door
			if (!CanOpenDoor())
			{
				for (USCInteractComponent* Interact : InteractComponents)
				{
					Killer->GetInteractableManagerComponent()->UnlockInteraction(Interact);
				}
				MULTICAST_PlaySound(LockedDoorSound);
				return;
			}

			// Killer can't close doors
			if (bIsOpen)
			{
				for (USCInteractComponent* Interact : InteractComponents)
				{
					Killer->GetInteractableManagerComponent()->UnlockInteraction(Interact);
				}
				return;
			}

			// Open door
			if (bIsInside)
			{
				KillerOpenDoorInside_SpecialMoveHandler->ActivateSpecial(Killer);
			}
			else
			{
				KillerOpenDoorOutside_SpecialMoveHandler->ActivateSpecial(Killer);
			}

			// Set owner of the driver
			OpenCloseDoorDriver->SetNotifyOwner(Killer);
			MULTICAST_SetDoorOpenCloseDriverOwner(Killer);
		}
		else // If it's a counselor, do some fancy animation shit
		{
			// Can't open a locked door
			if (!CanOpenDoor())
			{
				for (USCInteractComponent* Interact : InteractComponents)
				{
					Character->GetInteractableManagerComponent()->UnlockInteraction(Interact);
				}
				MULTICAST_PlaySound(LockedDoorSound);
				return;
			}

			USCSpecialMoveComponent* SpecialMoveComp = nullptr;
			if (bIsOpen)
			{
				// We're going to play different animations based on what direction the player wants to go when closing the door
				// and what direction they're facing when they close the door.

				// We're going outside if we're on the door side of the knob, we're staying inside if the door is on the otherside
				// of the knob from us.
				const FVector OutDirection = OutArrow->GetComponentRotation().RotateVector(FVector::ForwardVector);
				const FVector DoorDirection = FRotator(0.f, OPEN_DOOR_ANGLE, 0.f).RotateVector(OutDirection);
				const FVector PlayerToKnob = InteractComponent->GetComponentLocation() - Character->GetActorLocation();
				const bool MovingOutside = (PlayerToKnob | OutDirection) < 0.f && (DoorDirection | PlayerToKnob) < 0.f;
				const bool FacingOutside = (Character->GetActorForwardVector() | OutDirection) > 0.f;

				// Orientation found, pick that animation!
				if (MovingOutside)
				{
					if (FacingOutside)
					{
						SpecialMoveComp = CloseDoorOutsideBackward_SpecialMoveHandler;
					}
					else
					{
						SpecialMoveComp = CloseDoorOutsideForward_SpecialMoveHandler;
					}
				}
				else
				{
					if (FacingOutside)
					{
						SpecialMoveComp = CloseDoorInsideForward_SpecialMoveHandler;
					}
					else
					{
						SpecialMoveComp = CloseDoorInsideBackward_SpecialMoveHandler;
					}
				}
			}
			else
			{
				SpecialMoveComp = bIsInside ? OpenDoorInside_SpecialMoveHandler : Character->IsInWheelchair() ? OpenDoorWheelchairOutside_SpecialMoveHandler : OpenDoorOutside_SpecialMoveHandler;
			}

			SpecialMoveComp->ActivateSpecial(Character);

			// Set owner of the driver
			OpenCloseDoorDriver->SetNotifyOwner(Character);
			MULTICAST_SetDoorOpenCloseDriverOwner(Interactor);
		}
		break;

	case EILLInteractMethod::Hold:

		if (LinkedBarricade)
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			{
				if (IsBeingBrokenDown())// just in case the door was being broken down when 
				{
					for (USCInteractComponent* Interact : InteractComponents)
					{
						Counselor->GetInteractableManagerComponent()->UnlockInteraction(Interact);
					}
					break;
				}
			}
			LinkedBarricade->OnInteract(Interactor, InteractMethod);
		}
		else
		{
			if (bIsLocked)
			{
				// the killer should always use the default version
				if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
				{
					UnlockDoorInside_SpecialMoveHandler->ActivateSpecial(Character);
					break;
				}
				else if (bIsInside)
				{
					UnlockDoorInside_SpecialMoveHandler->ActivateSpecial(Character);
				}
				else
				{
					UnlockDoorOutside_SpecialMoveHandler->ActivateSpecial(Character);
				}
			}
			else
			{
				LockDoorInside_SpecialMoveHandler->ActivateSpecial(Character);
			}
		}
		break;
	}
}

void ASCDoor::OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState NewState)
{
	if (NewState == EILLHoldInteractionState::Canceled)
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
		{
			for (USCInteractComponent* Interact : InteractComponents)
			{
				Character->GetInteractableManagerComponent()->UnlockInteraction(Interact);
			}
		}
	}
}

bool ASCDoor::HasInteractComponent(const USCInteractComponent* Interactable) const
{
	for (USCInteractComponent* InteractComp : InteractComponents)
	{
		if (InteractComp == Interactable)
			return true;
	}

	return false;
}

bool ASCDoor::IsInteractionLockedInUse() const
{
	for (USCInteractComponent* InteractComp : InteractComponents)
	{
		if (InteractComp->IsLockedInUse())
			return true;
	}

	return false;
}

USCInteractComponent* ASCDoor::GetClosestInteractComponent(const AILLCharacter* Character) const
{
	USCInteractComponent* ClosestInteract = nullptr;
	const FVector CharacterLocation = Character->GetActorLocation();
	float ClosestDistanceSq = FLT_MAX;

	for (USCInteractComponent* InteractComp : InteractComponents)
	{
		const float Distance = (CharacterLocation - InteractComp->GetComponentLocation()).SizeSquared();
		if (Distance < ClosestDistanceSq)
		{
			ClosestDistanceSq = Distance;
			ClosestInteract = InteractComp;
		}
	}

	return ClosestInteract;
}

void ASCDoor::SetLinkedBarricade(ASCBarricade* Barricade)
{
	if (LinkedBarricade == Barricade)
		return;

	if (LinkedBarricade)
	{
		LinkedBarricade->CloseBarricade_SpecialMoveHandler->SpecialComplete.RemoveDynamic(this, &ASCDoor::OnBarricadeFinished);
		LinkedBarricade->OpenBarricade_SpecialMoveHandler->SpecialComplete.RemoveDynamic(this, &ASCDoor::OnBarricadeFinished);
	}

	if (Barricade)
	{
		Barricade->CloseBarricade_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OnBarricadeFinished);
		Barricade->OpenBarricade_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDoor::OnBarricadeFinished);
	}

	LinkedBarricade = Barricade;
}

void ASCDoor::OnBarricadeFinished(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
	{
		for (USCInteractComponent* Interact : InteractComponents)
		{
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(Interact);
		}
	}

	// Make sure we are using the correct nav area for AI
	UpdateNavArea();
}

bool ASCDoor::CanOpenDoor() const
{
	if (IsLockedOrBarricaded())
		return false;

	return true;
}

// ~Interaction
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// State

void ASCDoor::OnRep_IsOpen()
{
	// Stop animating the door and set the final orientation
	bIsDoorDriverActive = false;
	Pivot->SetRelativeRotation(FRotator(0.f, bIsOpen ? OPEN_DOOR_ANGLE : CLOSE_DOOR_ANGLE, 0.f));
}

void ASCDoor::OnDestroyDoorMesh()
{
	TimerHandle_DestroyMesh.Invalidate();

	if (Mesh)
	{
		Mesh->DestroyComponent();
		Mesh = nullptr;
	}
}

// ~State
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Break Down
void ASCDoor::OnBurstComplete(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
	{
		for (USCInteractComponent* Interact : InteractComponents)
		{
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(Interact);
		}
	}

	if (Interactor)
	{
		Interactor->SERVER_DestroyDoor(this, BurstExplosionForce);
	}
}

void ASCDoor::OnBreakDownStart(ASCCharacter* Interactor)
{
	if (KillerBreakingDown == nullptr)
		KillerBreakingDown = Cast<ASCKillerCharacter>(Interactor);

	if (KillerBreakingDown == nullptr)
		return;

	USCSpecialMove_SoloMontage* BreakDoorSpecialMove = Cast<USCSpecialMove_SoloMontage>(KillerBreakingDown->GetActiveSpecialMove());
	BreakDoorSpecialMove->ResetTimer(-1.f);

	BreakDownDriver->SetNotifyOwner(KillerBreakingDown);
	BreakDownLoopDriver->SetNotifyOwner(KillerBreakingDown);

	bAlwaysRelevant = true;
}

void ASCDoor::OnBreakDownStop(ASCCharacter* Interactor)
{
	if (KillerBreakingDown)
	{
		if (KillerBreakingDown->IsLocallyControlled())
		{
			for (USCInteractComponent* Interact : InteractComponents)
			{
				KillerBreakingDown->GetInteractableManagerComponent()->UnlockInteraction(Interact);
			}
			KillerBreakingDown->ClearBreakDownActor();
		}

		KillerBreakingDown = nullptr;
	}
	else
	{
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor))
		{
			if (Killer->IsLocallyControlled())
			{
				for (USCInteractComponent* Interact : InteractComponents)
				{
					Killer->GetInteractableManagerComponent()->UnlockInteraction(Interact);
				}
				Killer->ClearBreakDownActor();
			}
		}
	}

	bBreakDownSucceeding = false;
	BreakDownDriver->SetNotifyOwner(nullptr);
	BreakDownLoopDriver->SetNotifyOwner(nullptr);

	bAlwaysRelevant = false;
}

void ASCDoor::BreakDownSync(FAnimNotifyData NotifyData)
{
	if (KillerBreakingDown == nullptr)
		return;

	// Is the next swing going to destroy the door?
	ASCWeapon* Weapon = KillerBreakingDown->GetCurrentWeapon();
	const float WeaponDamage = Weapon->WeaponStats.Damage * KillerBreakingDown->DamageWorldModifier;
	const float Damage = IsBarricaded() ? WeaponDamage * 0.5f : WeaponDamage;
	if (Damage >= Health)
	{
		static const FName BreakSuccess_NAME(TEXT("Success"));
		bBreakDownSucceeding = true;

		// Go to the success state
		NotifyData.AnimInstance->Montage_JumpToSection(BreakSuccess_NAME);

		if (USCSpecialMove_SoloMontage* BreakDownSpecialMove = Cast<USCSpecialMove_SoloMontage>(KillerBreakingDown->GetActiveSpecialMove()))
		{
			const UAnimMontage* Montage = BreakDownSpecialMove->GetContextMontage();
			BreakDownSpecialMove->ResetTimer(Montage->GetSectionLength(Montage->GetSectionIndex(BreakSuccess_NAME)));
		}
	}
}

void ASCDoor::BreakDownDamage(FAnimNotifyData NotifyData)
{
	if (KillerBreakingDown == nullptr || KillerBreakingDown->Role != ROLE_Authority)
		return;

	// Is the next swing going to destroy the door?
	ASCWeapon* Weapon = KillerBreakingDown->GetCurrentWeapon();
	const float WeaponDamage = Weapon->WeaponStats.Damage * KillerBreakingDown->DamageWorldModifier;
	TakeDamage(WeaponDamage, FDamageEvent(), KillerBreakingDown->GetCharacterController(), KillerBreakingDown);
}

void ASCDoor::BreakDownLoop(FAnimNotifyData NotifyData)
{
	TryAbortBreakDown();
}

bool ASCDoor::TryAbortBreakDown(bool bForce /*= false*/)
{
	if (KillerBreakingDown == nullptr)
		return false;

	if ((KillerBreakingDown->ShouldAbortBreakingDown() && !bBreakDownSucceeding) || bForce)
	{
		UAnimInstance* KillerAnimInstance = KillerBreakingDown->GetMesh()->GetAnimInstance();
		USCSpecialMove_SoloMontage* SpecialMove = Cast<USCSpecialMove_SoloMontage>(KillerBreakingDown->GetActiveSpecialMove());
		UAnimMontage* Montage = SpecialMove ? SpecialMove->GetContextMontage() : nullptr;

		if (KillerAnimInstance && Montage)
		{
			static const FName BreakAbort_NAME(TEXT("Abort"));

			// Set our next section
			const FName CurrentSectionName = KillerAnimInstance->Montage_GetCurrentSection(Montage);
			KillerAnimInstance->Montage_SetNextSection(CurrentSectionName, BreakAbort_NAME, Montage);

			// Figure out when we'll be done playing
			const float CurrentPosition = KillerAnimInstance->Montage_GetPosition(Montage);
			const float TimeLeftForSection = Montage->GetSectionTimeLeftFromPos(CurrentPosition);
			const float NewSectionTime = Montage->GetSectionLength(Montage->GetSectionIndex(BreakAbort_NAME));
			const float TotalTimeLeft = TimeLeftForSection + NewSectionTime;

			// Set that new timeout!
			SpecialMove->ResetTimer(TotalTimeLeft);

			return true;
		}
	}

	return false;
}
// ~Break Down
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Utilities

bool ASCDoor::IsInteractorInside(const AActor* Interactor) const
{
	return ((Interactor->GetActorLocation() - GetActorLocation()).GetSafeNormal() | GetActorForwardVector()) < 0;
}

void ASCDoor::OpenCloseDoorDriverTick(FAnimNotifyData NotifyData)
{
	if (bIsDoorDriverActive)
	{
		const UILLCharacterAnimInstance* AnimInstance = Cast<UILLCharacterAnimInstance>(NotifyData.AnimInstance);
		const float LerpAlpha = FMath::Clamp(AnimInstance->GetUnblendedCurveValue(NAME_DoorRotation), 0.f, 1.f);
		const FRotator Rot(0.f, FMath::Lerp(StartAngle, TargetAngle, LerpAlpha), 0.f);

		Pivot->SetRelativeRotation(Rot);
	}
}

void ASCDoor::OpenCloseDoorDriverBegin(FAnimNotifyData NotifyData)
{
	if (bIsOpen)
	{
		TargetAngle = CLOSE_DOOR_ANGLE;
		StartAngle = OPEN_DOOR_ANGLE;
	}
	else
	{
		TargetAngle = OPEN_DOOR_ANGLE;
		StartAngle = CLOSE_DOOR_ANGLE;
	}

	bIsDoorDriverActive = true;
}

void ASCDoor::OpenCloseDoorDriverEnd(FAnimNotifyData NotifyData)
{
	bIsDoorDriverActive = false;
}

void ASCDoor::DoorKillDriverTick(FAnimNotifyData NotifyData)
{
	// Can be set false by aborting a special move
	if (bIsDoorDriverActive)
	{
		UILLCharacterAnimInstance* AnimInstance = Cast<UILLCharacterAnimInstance>(NotifyData.AnimInstance);
		const FRotator Rot(0.f, AnimInstance->GetUnblendedCurveValue(NAME_DoorRotation), 0.f);
		Pivot->SetRelativeRotation(Rot);
	}
}

void ASCDoor::DoorKillDriverEnd(FAnimNotifyData NotifyData)
{
	if (Role == ROLE_Authority)
	{
		MULTICAST_SetKillDriverNotifyOwner(nullptr);
	}

	bIsDoorDriverActive = false;
}

void ASCDoor::BurstDestroyDoor(FAnimNotifyData NotifyData)
{
	if (Role == ROLE_Authority)
	{
		if (ASCCharacter* Destroyer = Cast<ASCCharacter>(NotifyData.AnimInstance->GetOwningActor()))
			Destroyer->SERVER_DestroyDoor(this, BurstExplosionForce);
		Health = 0;
		MULTICAST_SetBurstNotifyOwner(nullptr);
	}
}

void ASCDoor::MULTICAST_DestroyDoor_Implementation(AActor* DamageCauser, float Impulse)
{
	if (DamageCauser == nullptr)
		return;

	Local_DestroyDoor(DamageCauser, Impulse);
}

void ASCDoor::Local_DestroyDoor(AActor* DamageCauser, float Impulse)
{
	if (bIsDestroyed)
		return;

	bIsDestroyed = true;

	// Set a timer to destroy our mesh
	FTimerDelegate DestroyMeshDelegate = FTimerDelegate::CreateUObject(this, &ASCDoor::OnDestroyDoorMesh);
	GetWorldTimerManager().SetTimer(TimerHandle_DestroyMesh, DestroyMeshDelegate, LifespanAfterDestruction, false);

	const FVector Facing = [this, DamageCauser]()
	{
		if (DamageCauser)
			return (DamageCauser->GetActorLocation() - InteractComponent->GetComponentLocation());

		return OutArrow->GetComponentRotation().Vector();
	}().GetSafeNormal2D();

	Mesh->SetSimulatePhysics(true);
	Mesh->ApplyRadiusDamage(100.f, OutArrow->GetComponentLocation() + (Facing * 5.f), 3000.f, Impulse, true);
	Mesh->CastShadow = false;
	DoorCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OnDestroyDoor(DamageCauser, Impulse);

	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	ForceNetUpdate();
	InteractComponent->bIsEnabled = false;

	if (LinkedBarricade)
	{
		LinkedBarricade->BreakAndDestroy();
	}

	if (LockedIcon)
	{
		LockedIcon->DestroyComponent();
		LockedIcon = nullptr;
	}

	if (HasActorBegunPlay())
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DoorDestroySoundCue, GetActorLocation());

	// Make we are using the correct nav area for AI
	UpdateNavArea();

	// Let the game state know this actor is dead
	UWorld* World = GetWorld();
	if (ASCGameState* GameState = Cast<ASCGameState>(World ? World->GetGameState() : nullptr))
		GameState->ActorBroken(this);
}

void ASCDoor::OpenCloseDoorCompleted(ASCCharacter* Interactor)
{
	if (Interactor)
	{
		for (USCInteractComponent* Interact : InteractComponents)
		{
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(Interact);
		}

		if (Interactor->Role == ROLE_Authority)
		{
			Interactor->SERVER_OpenDoor(this, !bIsOpen);
		}
	}

	if (bIsDoorDriverActive)
	{
		// Snap the door to open or close (inverse of bIsOpen)
		Pivot->SetRelativeRotation(FRotator(0.f, bIsOpen ? CLOSE_DOOR_ANGLE : OPEN_DOOR_ANGLE, 0.f));
	}

	OpenCloseDoorDriver->SetNotifyOwner(nullptr);
}

void ASCDoor::OpenCloseDoorAborted(ASCCharacter* Interactor)
{
	if (Interactor)
	{
		for (USCInteractComponent* Interact : InteractComponents)
		{
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(Interact);
		}

		// Did we rotate far enough to toggle being open/closed?
		if (Interactor->Role == ROLE_Authority)
		{
			if (USCSpecialMove_SoloMontage* SCMove = Interactor->GetActiveSCSpecialMove())
			{
				if (SCMove->GetTimePercentage() > 0.5f)
					Interactor->SERVER_OpenDoor(this, !bIsOpen);
			}
		}
	}

	OpenCloseDoorDriver->SetNotifyOwner(nullptr);
}

void ASCDoor::OpenDoorSpedUp(ASCCharacter* Interactor)
{
	OpenCloseDoorDriver->SetNotifyOwner(Interactor);
}

void ASCDoor::ToggleLock(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
	{
		for (USCInteractComponent* Interact : InteractComponents)
		{
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(Interact);
		}
	}

	// Called by everyone, only set the bool on the server
	if (Role == ROLE_Authority)
	{
		bIsLocked = !bIsLocked;
	}

	// Make we are using the correct nav area for AI
	UpdateNavArea();
}

void ASCDoor::MULTICAST_PlaySound_Implementation(USoundCue* SoundToPlay)
{
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundToPlay, GetActorLocation());
}

void ASCDoor::MULTICAST_SetDoorOpenCloseDriverOwner_Implementation(AActor* InOwner)
{
	OpenCloseDoorDriver->SetNotifyOwner(InOwner);
	if (ASCCharacter* Character = Cast<ASCCharacter>(InOwner))
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->Controller))
		{
			const FVector OutDirection = OutArrow->GetComponentRotation().RotateVector(FVector::ForwardVector);
			const FVector PlayerToKnob = InteractComponent->GetComponentLocation() - Character->GetActorLocation();
			const bool MovingOutside = (PlayerToKnob | OutDirection) < 0.f;
			FRotator Rotation = MovingOutside ? OutArrow->GetComponentRotation().Add(0.f, 180.f, 0.f) : OutArrow->GetComponentRotation();
			Rotation.Roll = 0.f;

			PC->SetControlRotation(Rotation);
		}
	}
}

void ASCDoor::MULTICAST_SetBurstNotifyOwner_Implementation(AActor* InOwner)
{
	BurstDoorDriver->SetNotifyOwner(InOwner);
}

void ASCDoor::MULTICAST_SetKillDriverNotifyOwner_Implementation(AActor* InOwner)
{
	DoorKillDriver->SetNotifyOwner(InOwner);
}

void ASCDoor::UpdateDestructionState(UDestructibleMesh* NewDestructionMesh)
{
	if (NewDestructionMesh
#if WITH_APEX
		&& NewDestructionMesh->GetApexDestructibleAsset()
#endif
		)
	{
		Mesh->SetDestructibleMesh(NewDestructionMesh);
	}
	OnUpdateDestructionState();
}

void ASCDoor::UpdateNavArea()
{
	if (PointLinks.Num() > 0)
	{
		// Allow AI to navigate through the broken door
		if (bIsDestroyed)
		{
			PointLinks[0].SetAreaClass(UNavArea_Default::StaticClass());
			NavigationModifier->SetAreaClass(UNavArea_Default::StaticClass());
		}
		// Don't allow counselors to try and navigate through a locked door
		else if (IsLockedOrBarricaded())
		{
			PointLinks[0].SetAreaClass(USCNavArea_LockedDoorMultiAgent::StaticClass());
			NavigationModifier->SetAreaClass(USCNavArea_LockedDoorMultiAgent::StaticClass());
		}
		// Make sure AI can navigate through the door
		else if (!PointLinks[0].GetAreaClass()->IsChildOf(USCNavArea_UnlockedDoor::StaticClass()))
		{
			PointLinks[0].SetAreaClass(USCNavArea_UnlockedDoor::StaticClass());
			NavigationModifier->SetAreaClass(USCNavArea_UnlockedDoor::StaticClass());
		}
	}
}

// ~Utilities
//////////////////////////////////////////////////////////////////////////////////
