// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCItemKiosk.generated.h"

class ASCItem;

enum class EILLInteractMethod : uint8;

class USCInteractComponent;
class USCSpecialMoveComponent;

/**
 * @class ASCItemKiosk
 */
UCLASS()
class SUMMERCAMP_API ASCItemKiosk
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN AActor interface
	virtual void PostInitializeComponents() override;
	// END AActor interface

	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	UFUNCTION()
	int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	UFUNCTION()
	void OnUseAborted(ASCCharacter* Interactor);

	UFUNCTION()
	void OnUsedComplete(ASCCharacter* Interactor);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCInteractComponent* InteractionPoint;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* UseSpecialMove;

	// Class of the item to spawn when using this kiosk
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PickupClass")
	TSubclassOf<ASCItem> ItemClass;
};
