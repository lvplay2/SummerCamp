// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_PlayAnimation.h"
#include "BTTask_SCPlayAnimation.generated.h"

class ASCCharacter;

/**
 * @class UBTTask_SCPlayAnimation
 */
UCLASS()
class SUMMERCAMP_API UBTTask_SCPlayAnimation 
: public UBTTaskNode
{
	GENERATED_UCLASS_BODY()
	
	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif
	// End UBTTaskNode interface

	/** Start the animation playback */
	EBTNodeResult::Type PlayAnimation(UAnimMontage* MontageToPlay);

	/** Called when the animation has finished playing */
	void OnAnimationTimerDone();

	// Animation asset to play for female characters
	UPROPERTY(Category = Node, EditAnywhere)
	UAnimMontage* FemaleAnimationToPlay;

	// Animation asset to play for male characters
	UPROPERTY(Category = Node, EditAnywhere)
	UAnimMontage* MaleAnimationToPlay;

	// Should the animation loop
	UPROPERTY(Category = Node, EditAnywhere)
	bool bLoopAnimation;

	// Keep track of the animation we are playing
	UPROPERTY()
	UAnimMontage* CurrentPlayingAnimation;

	// Keep track of who is executing this task
	UPROPERTY()
	UBehaviorTreeComponent* MyOwnerComp;

	// Keep track of the character we are playing the animation on
	UPROPERTY()
	ASCCharacter* MyCharacter;

	// Timer info for when the animation is complete
	FTimerDelegate TimerDelegate;
	FTimerHandle TimerHandle;
};
