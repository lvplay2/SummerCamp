// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCDestructibleActor.h"

#include "SCCharacter.h"
#include "SCGameState.h"
#include "SCWeapon.h"

/*
				WARNING: THIS CLASS USES NET DORMANCY
	This means that if you need to update any replicated variables,
	logic, RPCs or other networking whatnot, you NEED to update the
	logic handling for dormancy.
*/

ASCDestructibleActor::ASCDestructibleActor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, Health(100.f)
, DestructionForce(1000.f)
, DestructionRadius(3000.f)
, MeshLifespanAfterDestruction(10.f)
{
	bReplicates = true;
	NetUpdateFrequency = 10.f;

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	DestructibleMesh = CreateDefaultSubobject<UDestructibleComponent>(TEXT("DestructibleMesh"));
	DestructibleMesh->SetupAttachment(RootComponent);
	DestructibleMesh->OnComponentBeginOverlap.AddDynamic(this, &ASCDestructibleActor::OnOverlapBegin);
}

void ASCDestructibleActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCDestructibleActor, Health);
}

void ASCDestructibleActor::BeginPlay()
{
	Super::BeginPlay();

	SetNetDormancy(DORM_DormantAll); // Start out asleep
}

void ASCDestructibleActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_MeshLifespan);
	}
}

float ASCDestructibleActor::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	check(Role == ROLE_Authority);

	// Can't damage something that's already destroyed.
	if (Health <= 0.f)
		return 0.f;

	// Check if we should ignore damage from this instigator
	for (TSoftClassPtr<ASCCharacter> IgnoredCharacterClass : IgnoredDamageDealers)
	{
		if (IgnoredCharacterClass.Get() && DamageCauser->IsA(IgnoredCharacterClass.Get()))
		{
			return 0.f;
		}
	}

	ForceNetUpdate(); // Make sure the next change to health gets replicated
	Health -= Damage;
	OnRep_Health(Health + Damage);

	return Damage;
}

void ASCDestructibleActor::OnRep_Health(float PreviousHealth)
{
	if (Health <= 0.f)
	{
		ApplyDestruction(GetActorLocation());
	}
}

void ASCDestructibleActor::ApplyDestruction(FVector ImpactPoint)
{
	if (!DestructibleMesh)
		return;

	if (HasActorBegunPlay())
	{
		OnActorDestroyed();

		DestructibleMesh->SetSimulatePhysics(true);
		DestructibleMesh->ApplyRadiusDamage(100.f, ImpactPoint, DestructionRadius, DestructionForce, true);
		DestructibleMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		DestructibleMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
		DestructibleMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		DestructibleMesh->OnComponentBeginOverlap.Clear();
		DestructibleMesh->CastShadow = false;

		FTimerDelegate Delegate;
		Delegate.BindLambda([this]()
		{
			if (IsValid(this))
			{
				DestructibleMesh->DestroyComponent();
				DestructibleMesh = nullptr;
			}
		});
		GetWorldTimerManager().SetTimer(TimerHandle_MeshLifespan, Delegate, MeshLifespanAfterDestruction, false);
	}
	else
	{
		DestructibleMesh->DestroyComponent();
		DestructibleMesh = nullptr;
	}

	// Let the game state know this actor is dead
	UWorld* World = GetWorld();
	if (ASCGameState* GameState = Cast<ASCGameState>(World ? World->GetGameState() : nullptr))
		GameState->ActorBroken(this);
}

void ASCDestructibleActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Role != ROLE_Authority)
		return;

	if (ASCWeapon* Weapon = Cast<ASCWeapon>(OtherActor))
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(Weapon->GetCharacterOwner()))
		{
			if (Character->IsAttacking() == false)
				return;

			TakeDamage(Weapon->WeaponStats.Damage, FDamageEvent(), Character->GetController(), OtherActor);
		}
	}
}
