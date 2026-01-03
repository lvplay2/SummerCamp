// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCInteractComponent.h"

#include "SCVehicleSeatComponent.generated.h"

class ASCCharacter;
class USCContextKillComponent;
class ASCCounselorCharacter;
class ASCDriveableVehicle;
class ASCRepairPart;
class USCSpacerCapsuleComponent;
class USCSpecialMoveComponent;

/**
 * @class USCVehicleSeatComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCVehicleSeatComponent
: public USCInteractComponent
{
	GENERATED_BODY()
	
public:
	USCVehicleSeatComponent(const FObjectInitializer& ObjectInitializer);

	// BEGIN UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END UActorComponent interface

	// BEGIN UILLInteractableComponent interface
	virtual void OnInteractBroadcast(AActor* Interactor, EILLInteractMethod) override;
	virtual int32 CanInteractWithBroadcast(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation) override;
	// END UILLInteractableComponent interface

	/** Turn on or off our spacers for this seat */
	void EnableExitSpacers(ASCCharacter* Interactor, const bool bEnabled) const;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Seat")
	FName AttachSocket;

	FORCEINLINE bool IsDriverSeat() const { return bIsDriverSeat; }
	FORCEINLINE bool IsFrontSeat() const { return bIsFrontSeat; }

	/** The Animation the Vehicle should play when this seat is being entered */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* EnterVehicleMontage;

	/** The Animation the Vehicle should play when this seat is being exited */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ExitVehicleMontage;

	/** The Animation the Counselor should play when being grabbed out of their seat */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* CounselorGrabbedMontage;

	/** Is a Vehicle Part required to interact with this seat? */
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Vehicle Seat")
	TSubclassOf<ASCRepairPart> RequiredPartClass;

	UPROPERTY(Transient)
	ASCCounselorCharacter* CounselorInSeat;

	/** Timer for exiting the vehicle */
	FTimerHandle TimerHandle_ExitSeat;

	/** Use the vehicle's camera when the character enters it instead of first person camera? */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	bool bUseVehicleCamera;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Seat")
	bool bIsDriverSeat;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Seat")
	bool bIsFrontSeat;

	UPROPERTY(Transient)
	TArray<USCSpecialMoveComponent*> SpecialMoveComponents;

	UPROPERTY(Transient)
	TArray<USCContextKillComponent*> KillComponents;

	// List of all children spacer components
	UPROPERTY(Transient)
	TArray<USCSpacerCapsuleComponent*> SpacerComponents;

public:
	bool IsSeatBlocked() const;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Seat")
	ASCDriveableVehicle* ParentVehicle;
};
