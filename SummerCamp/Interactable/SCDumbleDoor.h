// Copyright (C) 2015-2017 IllFonic, LLC. and Gun Media

#pragma once

#include "SCDoor.h"

#include "SCDumbleDoor.generated.h"

/**
 * @class ASCDumbleDoor
 */
UCLASS()
class SUMMERCAMP_API ASCDumbleDoor
: public ASCDoor
{
	GENERATED_BODY()

protected:
	// Allow the character to modify us so they can handle our RPCs
	friend ASCCharacter;

public:
	ASCDumbleDoor(const FObjectInitializer& ObjectInitializer);

	// Begin AActor interface
	virtual void BeginPlay() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	// End AActor interface

	//////////////////////////////////////////////////////////////////////////////////
	// Components
public:
	// Extra Door Meshes
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UDestructibleComponent* Mesh2;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UBoxComponent* DoorCollision2;

	// Offsets for Mesh and Mesh2 when opening
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	FVector OpenDelta;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	FVector OpenDelta2;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* DoorsOpenSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* DoorsClosedSound;
public:
	/// ////////////////////////////////
	/// all the extra destructed meshes 

	// Destructible mesh To swap in when health falls below LightDestructionThreshold
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* LightDestructionMesh2;

	// Destructible mesh To swap in when health falls below MediumDestructionThreshold
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* MediumDestructionMesh2;

	// Destructible mesh To swap in when health falls below HeavyDestructionThreshold
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* HeavyDestructionMesh2;

public:
	// Begin ASCDoor Interface
	virtual void OpenCloseDoorDriverBegin(FAnimNotifyData NotifyData) override;
	virtual void OpenCloseDoorDriverTick(FAnimNotifyData NotifyData) override;
	virtual void OpenCloseDoorDriverEnd(FAnimNotifyData NotifyData) override;
	virtual void DoorKillDriverTick(FAnimNotifyData NotifyData) override;
	virtual void DoorKillDriverEnd(FAnimNotifyData NotifyData) override;
	virtual void ToggleLock(ASCCharacter* Interactor) override;
	virtual void OpenCloseDoorCompleted(ASCCharacter* Interactor) override;
	virtual void Local_DestroyDoor(AActor* DamageCauser, float Impulse) override;
	virtual void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod) override;
	virtual void OnRep_Health(int32 OldHealth) override;
protected:
	virtual void OnRep_IsOpen() override;
	virtual void OnDestroyDoorMesh() override;
	// End ASCDoor Interface

public:
	virtual void UpdateDestructionStateExtra(UDestructibleMesh* NewDestructionMesh);

protected:
	bool bIsDoorTransformDriverActive;
	FVector ClosedLocation;
	FVector ClosedLocation2;
	float StartLerp;
	float TargetLerp;
};
