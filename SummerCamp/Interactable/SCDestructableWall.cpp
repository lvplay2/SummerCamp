// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCDestructableWall.h"

#include "Animation/AnimMontage.h"

#include "SCAnimDriverComponent.h"
#include "SCContextKillComponent.h"
#include "SCCounselorAnimInstance.h"
#include "SCCounselorCharacter.h"
#include "SCGameState_Hunt.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"
#include "SCPlayerState.h"
#include "SCSpacerCapsuleComponent.h"
#include "SCSpecialMoveComponent.h"
#include "SCWeapon.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCDestructableWall"

ASCDestructableWall::ASCDestructableWall(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, LightDestructionThreshold(0.8f)
, MediumDestructionThreshold(0.6f)
, HeavyDestructionThreshold(0.4f)
, HitExplosionForce(1000.f)
, BurstExplosionForce(5000.f)
, BaseHealth(150.f)
, bIsDestroyed(false)
, WallLifespanAfterDestruction(10.f)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	//NetCullDistanceSquared = FMath::Square(500.f); // Comment in to test relevance locally

	ContextMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContextMesh"));
	ContextMesh->SetupAttachment(RootComponent);
	ContextMesh->SetMobility(EComponentMobility::Stationary);
	ContextMesh->SetCollisionResponseToChannel(ECC_AnimCameraBlocker, ECollisionResponse::ECR_Ignore);
	ContextMesh->bGenerateOverlapEvents = false;

	OuterContextKillComponent = CreateDefaultSubobject<USCContextKillComponent>(TEXT("OuterContextKillComponent"));
	OuterContextKillComponent->SetupAttachment(RootComponent);

	BurstDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("BurstDriver"));
	BurstDriver->SetNotifyName(TEXT("WallSmashDriver"));

	BreakDownDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("BreakDownDriver"));
	BreakDownDriver->SetNotifyName(TEXT("BreakDownSync"));

	BreakDownLoopDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("BreakDownLoopDriver"));
	BreakDownLoopDriver->SetNotifyName(TEXT("BreakDownLoop"));

	DestructibleSection = CreateDefaultSubobject<UDestructibleComponent>(TEXT("DestructibleSection"));
	DestructibleSection->SetupAttachment(ContextMesh);
	DestructibleSection->bGenerateOverlapEvents = false;

	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	BlockingCollision->SetupAttachment(RootComponent);
	BlockingCollision->bGenerateOverlapEvents = false;

	BurstFromInsideSpecialMove = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("Inside Burst Special"));
	BurstFromInsideSpecialMove->bTakeCameraControl = false;
	BurstFromInsideSpecialMove->SetupAttachment(RootComponent);

	BurstFromOutsideSpecialMove = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("Outside Burst Special"));
	BurstFromOutsideSpecialMove->bTakeCameraControl = false;
	BurstFromOutsideSpecialMove->SetupAttachment(RootComponent);
	
	BreakDownFromInside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("BreakDownFromInside_SpecialMoveHandler"));
	BreakDownFromInside_SpecialMoveHandler->SetupAttachment(RootComponent);
	BreakDownFromInside_SpecialMoveHandler->bTakeCameraControl = false;

	BreakDownFromOutside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("BreakDownFromOutside_SpecialMoveHandler"));
	BreakDownFromOutside_SpecialMoveHandler->SetupAttachment(RootComponent);
	BreakDownFromOutside_SpecialMoveHandler->bTakeCameraControl = false;

	InsideSpacer = CreateDefaultSubobject<USCSpacerCapsuleComponent>(TEXT("IndoorSpacer"));
	InsideSpacer->SetupAttachment(RootComponent);

	OutsideSpacer = CreateDefaultSubobject<USCSpacerCapsuleComponent>(TEXT("OutsideSpacer"));
	OutsideSpacer->SetupAttachment(RootComponent);

	FacingDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Facing"));
	FacingDirection->SetupAttachment(ContextMesh);

	HealthData.Health = 1.f;
}

void ASCDestructableWall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCDestructableWall, HealthData);
}

void ASCDestructableWall::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent)
	{
		InteractComponent->OnInteract.RemoveDynamic(this, &ASCDestructableWall::ActivateKill);
		InteractComponent->OnInteract.AddDynamic(this, &ASCDestructableWall::OnInteract);
		InteractComponent->OnHoldStateChanged.AddDynamic(this, &ASCDestructableWall::OnHoldStateChanged);

		InteractComponent->OnCanInteractWith.BindDynamic(this, &ASCDestructableWall::CanInteractWith);
		InteractComponent->OnShouldHighlight.BindDynamic(this, &ASCDestructableWall::ShouldHighlight);
	}

	if (BreakDownDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		FAnimNotifyEventDelegate EndDelegate;
		BeginDelegate.BindDynamic(this, &ASCDestructableWall::BreakDownSync);
		EndDelegate.BindDynamic(this, &ASCDestructableWall::BreakDownDamage);
		BreakDownDriver->InitializeBeginEnd(BeginDelegate, EndDelegate);
		BreakDownDriver->SetAutoClear(false);
	}

	if (BreakDownLoopDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCDestructableWall::BreakDownLoop);
		BreakDownLoopDriver->InitializeBegin(BeginDelegate);
		BreakDownLoopDriver->SetAutoClear(false);
	}

	// Rage
	BurstFromInsideSpecialMove->SpecialStarted.AddDynamic(this, &ASCDestructableWall::BurstStarted);
	BurstFromInsideSpecialMove->SpecialComplete.AddDynamic(this, &ASCDestructableWall::BurstComplete);
	BurstFromInsideSpecialMove->SpecialAborted.AddDynamic(this, &ASCDestructableWall::BurstComplete);
	BurstFromOutsideSpecialMove->SpecialStarted.AddDynamic(this, &ASCDestructableWall::BurstStarted);
	BurstFromOutsideSpecialMove->SpecialComplete.AddDynamic(this, &ASCDestructableWall::BurstComplete);
	BurstFromOutsideSpecialMove->SpecialAborted.AddDynamic(this, &ASCDestructableWall::BurstComplete);

	// Break Down
	BreakDownFromInside_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCDestructableWall::OnBreakDownStart);
	BreakDownFromInside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDestructableWall::OnBreakDownStop);
	BreakDownFromInside_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDestructableWall::OnBreakDownStop);

	BreakDownFromOutside_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCDestructableWall::OnBreakDownStart);
	BreakDownFromOutside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCDestructableWall::OnBreakDownStop);
	BreakDownFromOutside_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCDestructableWall::OnBreakDownStop);

	if (BurstDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCDestructableWall::WallBurst);
		BurstDriver->InitializeBegin(BeginDelegate);
	}

	if (Role == ROLE_Authority)
	{
		ForceNetUpdate();
		HealthData.Health = 1.f;
	}
}

void ASCDestructableWall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_WallLifespan);
	}
}

int32 ASCDestructableWall::CanInteractWith(class AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (bIsDestroyed || IsBeingBrokenDown())
		return 0;
	
	int32 Flags = 0;
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
	{
		if (Killer->GetGrabbedCounselor() == nullptr)
		{
			Flags |= InteractComponent->InteractMethods;

			if (!Killer->IsRageActive())
				Flags |= (int32) EILLInteractMethod::Hold;
		}
	}

	return Flags;
}

bool ASCDestructableWall::ShouldHighlight(class AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	return CanInteractWith(Character, ViewLocation, ViewRotation) > 0;
}

// Destroy Wall -------------------------------------------------------------

void ASCDestructableWall::DestroyWall(const AActor* DamageCauser, float Impulse)
{
	if (bIsDestroyed)
		return;

	bIsDestroyed = true;

	if (DamageCauser && DamageCauser->Role == ROLE_Authority)
	{
		// Track the number of walls destroyed
		const ASCCharacter* Character = Cast<ASCCharacter>(DamageCauser);
		if (ASCPlayerState* PS = Character ? Character->GetPlayerState() : nullptr)
		{
			PS->BrokeWallDown();
		}
	}

	BlockingCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (HasActorBegunPlay())
	{
		if (DestructibleSection)
		{
			const FVector Facing = [this, DamageCauser]()
			{
				if (DamageCauser)
					return (DamageCauser->GetActorLocation() - InteractComponent->GetComponentLocation());

				return FacingDirection->GetComponentRotation().Vector();
			}().GetSafeNormal2D();

			DestructibleSection->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
			DestructibleSection->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
			DestructibleSection->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
			DestructibleSection->CastShadow = false;
			DestructibleSection->SetSimulatePhysics(true);
			const FVector HitLocation = InteractComponent->GetComponentLocation() + (Facing * 10.f);
			DestructibleSection->ApplyRadiusDamage(100.f, HitLocation, 300.f, Impulse, true);
		}

		UGameplayStatics::SpawnEmitterAttached(DestroyParticle, InteractComponent);
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), WallDestroySoundCue, GetActorLocation());

		FTimerDelegate Delegate;
		Delegate.BindLambda([this]()
		{
			if (IsValid(this) && DestructibleSection)
			{
				DestructibleSection->DestroyComponent();
				DestructibleSection = nullptr;
			}
		});
		GetWorldTimerManager().SetTimer(TimerHandle_WallLifespan, Delegate, WallLifespanAfterDestruction, false);
	}
	else if (DestructibleSection)
	{
		DestructibleSection->DestroyComponent();
		DestructibleSection = nullptr;
	}
}

void ASCDestructableWall::OnBreakDownStart(ASCCharacter* Interactor)
{
	if (KillerBreakingDown == nullptr)
		KillerBreakingDown = Cast<ASCKillerCharacter>(Interactor);

	if (KillerBreakingDown == nullptr)
		return;

	SetOwner(KillerBreakingDown);

	USCSpecialMove_SoloMontage* BreakDoorSpecialMove = Cast<USCSpecialMove_SoloMontage>(KillerBreakingDown->GetActiveSpecialMove());
	BreakDoorSpecialMove->ResetTimer(-1.f);

	BreakDownDriver->SetNotifyOwner(KillerBreakingDown);
	BreakDownLoopDriver->SetNotifyOwner(KillerBreakingDown);

	bAlwaysRelevant = true;
}

void ASCDestructableWall::OnBreakDownStop(ASCCharacter* Interactor)
{
	if (KillerBreakingDown)
	{
		if (KillerBreakingDown->IsLocallyControlled())
		{
			KillerBreakingDown->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
			KillerBreakingDown->ClearBreakDownActor();
		}

		KillerBreakingDown = nullptr;
	}

	if (Interactor && Interactor->IsLocallyControlled())
	{
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
		if (ASCKillerCharacter* KillerInteractor = Cast<ASCKillerCharacter>(Interactor))
			KillerInteractor->ClearBreakDownActor();
	}

	bBreakDownSucceeding = false;
	BreakDownDriver->SetNotifyOwner(nullptr);
	BreakDownLoopDriver->SetNotifyOwner(nullptr);
	SetOwner(nullptr);

	bAlwaysRelevant = false;
}

void ASCDestructableWall::BreakDownSync(FAnimNotifyData NotifyData)
{
	if (KillerBreakingDown == nullptr)
		return;

	// Is the next swing going to destroy the door?
	ASCWeapon* Weapon = KillerBreakingDown->GetCurrentWeapon();
	const float WeaponDamage = Weapon->WeaponStats.Damage * KillerBreakingDown->DamageWorldModifier;
	if (WeaponDamage >= GetHealth())
	{
		static const FName NAME_BreakSuccess(TEXT("Success"));
		bBreakDownSucceeding = true;

		// Go to the success state
		NotifyData.AnimInstance->Montage_JumpToSection(NAME_BreakSuccess);

		if (USCSpecialMove_SoloMontage* BreakDownSpecialMove = Cast<USCSpecialMove_SoloMontage>(KillerBreakingDown->GetActiveSpecialMove()))
		{
			const UAnimMontage* Montage = BreakDownSpecialMove->GetContextMontage();
			BreakDownSpecialMove->ResetTimer(Montage->GetSectionLength(Montage->GetSectionIndex(NAME_BreakSuccess)));
		}
	}
}

void ASCDestructableWall::BreakDownDamage(FAnimNotifyData NotifyData)
{
	if (KillerBreakingDown == nullptr || KillerBreakingDown->Role != ROLE_Authority)
		return;

	// Is the next swing going to destroy the door?
	ASCWeapon* Weapon = KillerBreakingDown->GetCurrentWeapon();
	const float WeaponDamage = Weapon->WeaponStats.Damage * KillerBreakingDown->DamageWorldModifier;
	TakeDamage(WeaponDamage, FDamageEvent(), KillerBreakingDown->GetCharacterController(), KillerBreakingDown);
}

void ASCDestructableWall::BreakDownLoop(FAnimNotifyData NotifyData)
{
	TryAbortBreakDown();
}

bool ASCDestructableWall::TryAbortBreakDown(bool bForce /*= false*/)
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
			static const FName NAME_BreakAbort(TEXT("Abort"));

			// Set our next section
			const FName CurrentSectionName = KillerAnimInstance->Montage_GetCurrentSection(Montage);
			KillerAnimInstance->Montage_SetNextSection(CurrentSectionName, NAME_BreakAbort, Montage);

			// Figure out when we'll be done playing
			const float CurrentPosition = KillerAnimInstance->Montage_GetPosition(Montage);
			const float TimeLeftForSection = Montage->GetSectionTimeLeftFromPos(CurrentPosition);
			const float NewSectionTime = Montage->GetSectionLength(Montage->GetSectionIndex(NAME_BreakAbort));
			const float TotalTimeLeft = TimeLeftForSection + NewSectionTime;

			// Set that new timeout!
			SpecialMove->ResetTimer(TotalTimeLeft);

			return true;
		}
	}

	return false;
}

bool ASCDestructableWall::IsInteractorInside(const AActor* Interactor) const
{
	if (!Interactor)
		return false;

	return ((GetActorLocation() - Interactor->GetActorLocation()).GetSafeNormal2D() | FacingDirection->GetForwardVector()) > 0.f;
}

float ASCDestructableWall::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!DamageCauser->IsA(ASCKillerCharacter::StaticClass()))
	{
		return 0.f;
	}

	ForceNetUpdate();
	FSCWallHealth OldHealthData = HealthData;
	HealthData.Health = FMath::Max(HealthData.Health - (Damage / BaseHealth), 0.f);
	HealthData.bLastHitFromInside = IsInteractorInside(DamageCauser);

	if (HealthData.Health <= 0.f)
	{
		HealthData.Health = 0.f;
		DestroyWall(DamageCauser, HitExplosionForce);
	}

	// Handle updating meshes for listen servers
	OnRep_Health(OldHealthData);

	return Damage;
}

void ASCDestructableWall::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		// Make sure killer is in rage
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor))
		{
			if (ContextKillComponent->IsEnabled() || OuterContextKillComponent->IsEnabled())
			{
				const FVector KillerDirection = (Killer->GetActorLocation() - GetActorLocation()).GetSafeNormal();

				// See if there's a counselor on the other side of the wall
				UWorld* World = GetWorld();
				if (ASCGameState* State = World ? Cast<ASCGameState>(World->GetGameState()) : nullptr)
				{
					for (ASCCharacter* OtherCharacter : State->PlayerCharacterList)
					{
						if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherCharacter))
						{
							const FVector CounselorDirection = (Counselor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
							if (InteractComponent->IsInInteractRange(Counselor, Counselor->GetActorLocation(), Counselor->GetActorRotation(), Counselor->GetInteractableManagerComponent()->bUseCameraRotation))
							{
								if (FMath::Sign(KillerDirection | GetActorForwardVector()) != FMath::Sign(CounselorDirection | GetActorForwardVector()))
								{
									// Initiate kill with found counselor
									const float distToContextKill = FVector::DistSquared(Killer->GetActorLocation(), ContextKillComponent->GetComponentLocation());
									const float distToOuterContextKill = FVector::DistSquared(Killer->GetActorLocation(), OuterContextKillComponent->GetComponentLocation());
									if (distToContextKill < distToOuterContextKill)
									{
										if (ContextKillComponent->IsEnabled())
										{
											ContextKillComponent->SERVER_ActivateContextKill(Killer, Counselor);
											return;
										}
									}
									else
									{
										if (OuterContextKillComponent->IsEnabled())
										{
											OuterContextKillComponent->SERVER_ActivateContextKill(Killer, Counselor);
											return;
										}
									}
								}
							}
						}
					}
				}
			}

			if (Killer->IsRageActive())
			{
				if (IsInteractorInside(Killer))
				{
					BurstFromInsideSpecialMove->ActivateSpecial(Killer);
					OutsideSpacer->ActivateSpacer(Killer);
					InsideSpacer->ActivateSpacer(Killer);
				}
				else
				{
					BurstFromOutsideSpecialMove->ActivateSpecial(Killer);
					OutsideSpacer->ActivateSpacer(Killer);
					InsideSpacer->ActivateSpacer(Killer);
				}
			}
			else
			{
				Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
			}
		}
		else
		{
			Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
		}
		break;
	default:
		Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
		break;
	}
}

void ASCDestructableWall::OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState NewState)
{
	if (NewState == EILLHoldInteractionState::Canceled)
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
			Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
	}
}

void ASCDestructableWall::BurstStarted(ASCCharacter* Interactor)
{
	BurstDriver->SetNotifyOwner(Interactor);
}

void ASCDestructableWall::BurstComplete(ASCCharacter* Interactor)
{
	BurstDriver->SetNotifyOwner(nullptr);

	if (Interactor->IsLocallyControlled())
	{
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
	}

	InsideSpacer->DeactivateSpacer();
	OutsideSpacer->DeactivateSpacer();
}


void ASCDestructableWall::WallBurst(FAnimNotifyData NotifyData)
{
	DestroyWall(NotifyData.AnimInstance->GetOwningActor(), BurstExplosionForce);
}

void ASCDestructableWall::OnRep_Health(FSCWallHealth OldHealthData)
{
	// Handle partial destruction
	if (GetHealth() > 0.f)
	{
		if (DestructibleSection)
		{
			const float PreviousDestructionRatio = OldHealthData.Health;
			const float DestructionRatio = HealthData.Health;

			if (DestructionRatio <= HeavyDestructionThreshold && PreviousDestructionRatio > HeavyDestructionThreshold)
			{
				DestructibleSection->SetDestructibleMesh(HealthData.bLastHitFromInside ? HeavyDestructionInsideMesh : HeavyDestructionOutsideMesh);
			}
			else if (DestructionRatio <= MediumDestructionThreshold && PreviousDestructionRatio > MediumDestructionThreshold)
			{
				DestructibleSection->SetDestructibleMesh(HealthData.bLastHitFromInside ? MediumDestructionInsideMesh : MediumDestructionOutsideMesh);
			}
			else if (DestructionRatio <= LightDestructionThreshold && PreviousDestructionRatio > LightDestructionThreshold)
			{
				DestructibleSection->SetDestructibleMesh(HealthData.bLastHitFromInside ? LightDestructionInsideMesh : LightDestructionOutsideMesh);
			}
		}

		if (HasActorBegunPlay())
		{
			UGameplayStatics::SpawnEmitterAttached(HitParticle, InteractComponent);
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), WallHitSoundCue, GetActorLocation());
		}
	}
	// Handle full destruction that we didn't get an RPC for
	else if (bIsDestroyed == false)
	{
		if (DestructibleSection)
			DestructibleSection->SetDestructibleMesh(HealthData.bLastHitFromInside ? HeavyDestructionInsideMesh : HeavyDestructionOutsideMesh);
		DestroyWall(nullptr, HitExplosionForce);
	}
}

#undef LOCTEXT_NAMESPACE
