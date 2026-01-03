// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCSpecialItem.h"
#include "SCPamelaSweater.generated.h"

class AILLCharacter;
class ASCCounselorCharacter;
class USCCounselorActiveAbility;
class ASkeletalMeshActor;

/**
 * @class ASCPamelaSweater
 */
UCLASS()
class SUMMERCAMP_API ASCPamelaSweater
: public ASCSpecialItem
{
	GENERATED_UCLASS_BODY()

public:
	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	// End AActor interface

	// Begin ASCItem interface
	virtual int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation) override;
	// End ASCItem interface

	/** Put the sweater on */
	void PutOn();

	/** Gets the active ability for the sweater */
	USCCounselorActiveAbility* GetAbility() const { return SweaterAbility; }

protected:
	/** The ability class for the sweater */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<USCCounselorActiveAbility> SweaterAbilityClass;

	/** The ability class given to the counselor wearing the sweater */
	UPROPERTY()
	USCCounselorActiveAbility* SweaterAbility;
};
