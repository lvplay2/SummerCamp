// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCInteractComponent.h"

#include "SCRepairComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCOnRepairDelegate);

/**
 * @class USCRepairComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCRepairComponent
: public USCInteractComponent
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN UILLInteractableComponent interface
	virtual void OnInteractBroadcast(AActor* Interactor, EILLInteractMethod InteractMethod) override;
	virtual int32 CanInteractWithBroadcast(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation) override;
	// END UILLInteractableComponent interface

	/** Returns the number of required parts to repair this component */
	FORCEINLINE int32 GetNumRequiredParts() const { return RequiredPartClasses.Num(); }
	FORCEINLINE TArray<TSubclassOf<class ASCRepairPart>>& GetRequiredPartClasses() { return RequiredPartClasses; }
	FORCEINLINE const TArray<TSubclassOf<class ASCRepairPart>>& GetRequiredPartClasses() const { return RequiredPartClasses; }

	UFUNCTION()
	void ForceRepair();

	UPROPERTY(BlueprintAssignable, Category = "Repair")
	FSCOnRepairDelegate OnRepair;

	UFUNCTION(BlueprintPure, Category = "Repair")
	bool IsRepaired() const { return bIsRepaired; }

protected:
	/** Required parts to flag this component as "Repaired" */
	UPROPERTY(EditDefaultsOnly, Category = "Repair")
	TArray<TSubclassOf<class ASCRepairPart>> RequiredPartClasses;

private:
	UPROPERTY(Replicated, ReplicatedUsing=OnRep_IsRepaired)
	uint32 bIsRepaired : 1;

	UFUNCTION()
	void OnRep_IsRepaired();
};
