// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/SceneComponent.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCContextComponent.generated.h"

class ASCCharacter;

DECLARE_DYNAMIC_DELEGATE_OneParam(FSCSpecialMoveCreationDelegate, class ASCCharacter*, Character);

UENUM(BlueprintType)
enum class EContextPositionReqs : uint8
{
	None,
	LocationOnly,
	RotationOnly,
	LocationAndRotation,
};


UCLASS(Abstract, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SUMMERCAMP_API USCContextComponent
: public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN UActorComponent interface
	virtual void InitializeComponent() override;
	// END UActorComponent interface

	/** Callback when character reaches their special move destination */
	UFUNCTION()
	virtual	void BeginAction();

	/** Called on context kill finished */
	UFUNCTION()
	virtual	void EndAction();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	ASCCharacter* GetCurrentKiller() const { return Killer; };

protected:

	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	EContextPositionReqs InteractorPositionRequirement;

	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	TSubclassOf<USCSpecialMove_SoloMontage> InteractorSpecialClass;

	UPROPERTY()
	ASCCharacter* Killer;

	UPROPERTY()
	USCSpecialMove_SoloMontage* KillerSpecialMove;

	UPROPERTY()
	FDestinationReachedDelegate DestinationReachedDelegate;
};
