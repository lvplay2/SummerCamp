// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SCAnimNotifyState_SloMo.generated.h"

class UWorldSettings;

/**
 * @class UILLPropSpawnerNotifyState
 */
UCLASS(meta=(DisplayName="SC Slow Motion"))
class SUMMERCAMP_API USCAnimNotifyState_SloMo
: public UAnimNotifyState
{
	GENERATED_UCLASS_BODY()

public:
	~USCAnimNotifyState_SloMo();

	// Begin UAnimNotifyState interface
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual FString GetNotifyName_Implementation() const override;
	// End UAnimNotifyState interface

	// Sets slomo to this value when the notify state starts and turns it back to 1 when this notify state ends
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	float SloMoSpeed;

private:
	// List of all world settings affected (needed for PIE/splitscreen support)
	UPROPERTY()
	TArray<AWorldSettings*> ModifiedWorldSettings;

	/** Resets time dilation to 1.f for all worlds */
	void ResetSloMoSpeed();
};
