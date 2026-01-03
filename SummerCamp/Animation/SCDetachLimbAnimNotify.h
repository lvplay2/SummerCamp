// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "SCCharacterBodyPartComponent.h"
#include "SCDetachLimbAnimNotify.generated.h"

/**
 * @class USCDetachLimbAnimNotify
 */
UCLASS()
class SUMMERCAMP_API USCDetachLimbAnimNotify 
: public UAnimNotify
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	// End UAnimNotify interface

	/** The limb to detach from the counselor */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Limb")
	ELimb LimbToDetach;

	/** Impulse to be applied to the limb once it is detached from the counselor */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Limb")
	FVector Impulse;
	
	/** Optional: Blood mask that overrides the mask set in the limb for this character */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Limb")
	UTexture* BloodMaskOverride;

	/** Should this limb be shown when detached? 
	 * True - The limb will show and the impulse will be applied
	 * False - The limb won't be shown, but all blood will still be applied to the parent
	 */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Limb")
	bool bShowLimbAfterDetach;
};
