// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "DestructibleComponent.h"
#include "SCDestructibleActor.generated.h"

class ASCCharacter;

/**
 * @class ASCDestructibleActor
 */
UCLASS()
class SUMMERCAMP_API ASCDestructibleActor
: public AActor
{
	GENERATED_BODY()

public:
	ASCDestructibleActor(const FObjectInitializer& ObjectInitializer);

	// Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	// End AActor Interface
	
	// Characters to ignore damage from
	UPROPERTY(EditDefaultsOnly)
	TArray<TSoftClassPtr<ASCCharacter>> IgnoredDamageDealers;

	// The destructible mesh object
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UDestructibleComponent* DestructibleMesh;

	/** Blueprint Event when the actor is destroyed, WILL NOT BE CALLED BEFORE BEGINPLAY (such as coming into relevancy range when destroyed) */
	UFUNCTION(BlueprintImplementableEvent, Category = "SCDestructibleActor")
	void OnActorDestroyed();

protected:
	// The amount of health this destructible actor has.
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_Health)
	float Health;

	// The amount of force applied to the destructibleMesh when the actor is destroyed
	UPROPERTY(EditDefaultsOnly)
	float DestructionForce;

	// The damage radius to apply DestructionForce in from the impact point (ActorLocation)
	UPROPERTY(EditDefaultsOnly)
	float DestructionRadius;

	/** Actor is destroyed if Health replcoates when less than 1 */
	UFUNCTION()
	void OnRep_Health(float PreviousHealth);

	/** Force the destructible mesh to destroy */
	UFUNCTION()
	void ApplyDestruction(FVector ImpactPoint);

	/** Detect overlap with ASCWeapon and apply damage if the hit is valid. */
	UFUNCTION(BlueprintCallable, Category = "Overlap")
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Time in seconds to cleanup the destruction mesh after being fully destroyed (handled locally)
	UPROPERTY(EditDefaultsOnly, Category = "SCDestructibleActor")
	float MeshLifespanAfterDestruction;

	FTimerHandle TimerHandle_MeshLifespan;
};
