// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCBridgeNavUnit.generated.h"

class ASCRepairPart;

class USCScoringEvent;
class USCRepairComponent;

enum class EILLInteractMethod : uint8;

/**
 * @class ASCBridgeNavUnit
 */
UCLASS()
class SUMMERCAMP_API ASCBridgeNavUnit
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	// END AActor interface

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCRepairComponent* RepairComponent;

	UFUNCTION(BlueprintPure)
	bool GetIsRepaired() const;

	/** @return true if the specified part is needed */
	bool IsRepairPartNeeded(TSubclassOf<ASCRepairPart> Part, USCRepairComponent*& AssociatedRepairComponent) const;

protected:
	/** Interaction method, called from LinkedDoor only */
	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);
};
