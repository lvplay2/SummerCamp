// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_CharacterAttack.generated.h"

/**
 * @class UBTTask_CharacterAttack
 */
UCLASS()
class SUMMERCAMP_API UBTTask_CharacterAttack
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	// End UBTTaskNode interface

protected:
	// Minimum amount of time to aim the gun
	UPROPERTY(EditAnywhere, Category="Condition")
	float MinGunAimTime;

	// Maximum amount of time to aim the gun
	UPROPERTY(EditAnywhere, Category="Condition")
	float MaxGunAimTime;

	// Time until we finish ticking this task and fire
	float TimeUntilFire;

	// Min value of the random offset range for Easy difficulty
	UPROPERTY(EditAnywhere, Category="Condition")
	float MinRangeEasy;

	// Max value of the random offset range for Easy difficulty
	UPROPERTY(EditAnywhere, Category = "Condition")
	float MaxRangeEasy;

	// Min value of the random offset range for Normal difficulty
	UPROPERTY(EditAnywhere, Category = "Condition")
	float MinRangeNormal;

	// Max value of the random offset range for Normal difficulty
	UPROPERTY(EditAnywhere, Category = "Condition")
	float MaxRangeNormal;

	/** Updates the desired control rotation and rotation multiplier based on game difficulty */
	void UpdateAccuracyForDifficulty(FRotator& Rotation, float& RotationSpeedMultiplier);
};
