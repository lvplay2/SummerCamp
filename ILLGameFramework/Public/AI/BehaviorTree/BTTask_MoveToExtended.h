// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "AITypes.h"
#include "BTTask_MoveToExtended.generated.h"

class UAITask_MoveTo;
class UNavigationQueryFilter;

/**
 * @struct FILLBTMoveToTaskMemory
 */
struct FILLBTMoveToTaskMemory
{
	/** Move request ID */
	FAIRequestID MoveRequestID;

	FDelegateHandle BBObserverDelegateHandle;
	FVector PreviousGoalLocation;

	TWeakObjectPtr<UAITask_MoveTo> Task;

	uint8 bWaitingForPath : 1;
	uint8 bObserverCanFinishTask : 1;
};

/**
 * @class UBTTask_MoveToExtended
 */
UCLASS(Config=Game)
class ILLGAMEFRAMEWORK_API UBTTask_MoveToExtended
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;
	virtual void OnMessage(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, FName Message, int32 RequestID, bool bSuccess) override;
	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
	virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR
	// End UBTTaskNode interface

protected:
	/** @return Distance-limited goal location for movement based on our Pawn's location. */
	virtual FVector CalcGoalLocation(UBehaviorTreeComponent& OwnerComp) const;

	/** Notification for when our target location vector value updates. */
	virtual EBlackboardNotificationResult OnBlackboardValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID);

	/** Creates an FAIMoveRequest to send to the AAIController. */
	virtual EBTNodeResult::Type PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);
	
	/** Prepares move task for activation. */
	virtual UAITask_MoveTo* PrepareMoveTask(UBehaviorTreeComponent& OwnerComp, UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest);

protected:
	// Acceptable goal radius
	UPROPERTY(config, Category="Node", EditAnywhere, meta=(ClampMin="0.0", UIMin="0.0"))
	float AcceptableRadius;

	// Maximum time that we can be without velocity before shorting out
	UPROPERTY(config, Category="Node", EditAnywhere, meta=(ClampMin="0.0", UIMin="0.0"))
	float MaxNoVelocityTime;

	// Total time we have gone without velocity while ticking thus far
	float TotalNoVelocityTime;

	// Navigation filter class, "None" will result in default filter being used
	UPROPERTY(Category="Node", EditAnywhere)
	TSubclassOf<UNavigationQueryFilter> FilterClass;

	// Sensitivity of BlackboardKey monitoring mechanism for vector value changes. Value is recommended to be less then AcceptableRadius!
	UPROPERTY(Category="Blackboard", EditAnywhere, AdvancedDisplay, meta=(ClampMin="1", UIMin="1", EditCondition="bObserveBlackboardValue"))
	float ObservedBlackboardValueTolerance;

	// Observe vector BlackboardKey changes?
	UPROPERTY()
	uint32 bObserveBlackboardValue : 1;

	// Distance limit for an individual sub-move request, useful for navigation invoker setups
	UPROPERTY(Category="BreadCrumbing", EditAnywhere)
	float BreadCrumbDistance;

	// Maximum number of movement sub-steps to take to move to our destination, <=0 for none
	UPROPERTY(Category="BreadCrumbing", EditAnywhere)
	int32 BreadCrumbLimit;

	// Number of sub-step movements performed
	int32 BreadCrumbCount;

	// Allow strafing movement?
	UPROPERTY(Category="Node", EditAnywhere)
	uint32 bAllowStrafe : 1;

	// Use incomplete path when goal can't be reached?
	UPROPERTY(Category="Node", EditAnywhere, AdvancedDisplay)
	uint32 bAllowPartialPath : 1;

	// Project goal location on navigation data (navmesh) before using?
	UPROPERTY(Category="Node", EditAnywhere, AdvancedDisplay)
	uint32 bProjectGoalLocation : 1;

	// Try to use a random point in the random point radius if projection of the goal location to navmesh fails? 
	UPROPERTY(Category="Node", EditAnywhere, AdvancedDisplay)
	uint32 bUseRandomPointIfProjectionFails : 1;

	// How big of a radius should we try to find a random point around our goal location? 
	UPROPERTY(Category = "Node", EditAnywhere, AdvancedDisplay)
	float RandomPointRadius;

	// Use the agent's radius in the AcceptableRadius?
	UPROPERTY(Category="Node", EditAnywhere)
	uint32 bStopOnOverlap : 1;

	// Should the move use path finding? Not exposed on purpose, please use BTTask_MoveDirectlyToward
	uint32 bUsePathfinding : 1;

	// Use GameplayTasks?
	uint32 bUseGameplayTasks : 1;

	// Were we EPathFollowingRequestResult::AlreadyAtGoal?
	uint32 bAlreadyAtGoal : 1;
};
