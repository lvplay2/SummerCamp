// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "BTTask_MoveToExtended.h"

#include "AIController.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Tasks/AITask_MoveTo.h"
#include "VisualLogger.h"

UBTTask_MoveToExtended::UBTTask_MoveToExtended(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Move To Extended");
	bUseGameplayTasks = GET_AI_CONFIG_VAR(bEnableBTAITasks);
	bNotifyTick = !bUseGameplayTasks;
	bNotifyTaskFinished = true;

	AcceptableRadius = GET_AI_CONFIG_VAR(AcceptanceRadius);
	MaxNoVelocityTime = .5f;
	bStopOnOverlap = GET_AI_CONFIG_VAR(bFinishMoveOnGoalOverlap);
	bAllowStrafe = GET_AI_CONFIG_VAR(bAllowStrafing);
	bAllowPartialPath = GET_AI_CONFIG_VAR(bAcceptPartialPaths);
	bProjectGoalLocation = true;
	bUseRandomPointIfProjectionFails = true;
	RandomPointRadius = 1500.f;
	bUsePathfinding = true;

	ObservedBlackboardValueTolerance = AcceptableRadius * 0.95f;

	// accept only actors and vectors
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveToExtended, BlackboardKey), AActor::StaticClass());
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveToExtended, BlackboardKey));
}

EBTNodeResult::Type UBTTask_MoveToExtended::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type NodeResult = EBTNodeResult::InProgress;

	FILLBTMoveToTaskMemory* MyMemory = reinterpret_cast<FILLBTMoveToTaskMemory*>(NodeMemory);
	MyMemory->PreviousGoalLocation = FAISystem::InvalidLocation;
	MyMemory->MoveRequestID = FAIRequestID::InvalidRequest;

	AAIController* MyController = OwnerComp.GetAIOwner();
	MyMemory->bWaitingForPath = bUseGameplayTasks ? false : MyController->ShouldPostponePathUpdates();
	if (!MyMemory->bWaitingForPath)
	{
		NodeResult = PerformMoveTask(OwnerComp, NodeMemory);
	}
	else
	{
		UE_VLOG(MyController, LogBehaviorTree, Log, TEXT("Pathfinding requests are freezed, waiting..."));
	}

	if (NodeResult == EBTNodeResult::InProgress && bObserveBlackboardValue)
	{
		UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
		if (ensure(BlackboardComp))
		{
			if (MyMemory->BBObserverDelegateHandle.IsValid())
			{
				UE_VLOG(MyController, LogBehaviorTree, Warning, TEXT("UBTTask_MoveToExtended::ExecuteTask \'%s\' Old BBObserverDelegateHandle is still valid! Removing old Observer."), *GetNodeName());
				BlackboardComp->UnregisterObserver(BlackboardKey.GetSelectedKeyID(), MyMemory->BBObserverDelegateHandle);
			}
			MyMemory->BBObserverDelegateHandle = BlackboardComp->RegisterObserver(BlackboardKey.GetSelectedKeyID(), this, FOnBlackboardChangeNotification::CreateUObject(this, &UBTTask_MoveToExtended::OnBlackboardValueChange));
		}
	}	
	
	return NodeResult;
}

EBTNodeResult::Type UBTTask_MoveToExtended::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FILLBTMoveToTaskMemory* MyMemory = reinterpret_cast<FILLBTMoveToTaskMemory*>(NodeMemory);
	if (!MyMemory->bWaitingForPath)
	{
		if (MyMemory->MoveRequestID.IsValid())
		{
			AAIController* MyController = OwnerComp.GetAIOwner();
			if (MyController && MyController->GetPathFollowingComponent())
			{
				MyController->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, MyMemory->MoveRequestID);
			}
		}
		else
		{
			MyMemory->bObserverCanFinishTask = false;
			UAITask_MoveTo* MoveTask = MyMemory->Task.Get();
			if (MoveTask)
			{
				MoveTask->ExternalCancel();
			}
			else
			{
				UE_VLOG(&OwnerComp, LogBehaviorTree, Error, TEXT("Can't abort path following! bWaitingForPath:false, MoveRequestID:invalid, MoveTask:none!"));
			}
		}
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_MoveToExtended::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	FILLBTMoveToTaskMemory* MyMemory = reinterpret_cast<FILLBTMoveToTaskMemory*>(NodeMemory);
	MyMemory->Task.Reset();

	if (bObserveBlackboardValue)
	{
		UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
		if (ensure(BlackboardComp) && MyMemory->BBObserverDelegateHandle.IsValid())
		{
			BlackboardComp->UnregisterObserver(BlackboardKey.GetSelectedKeyID(), MyMemory->BBObserverDelegateHandle);
		}

		MyMemory->BBObserverDelegateHandle.Reset();
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_MoveToExtended::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (OwnerComp.IsPaused())
		return;

	AAIController* MyController = OwnerComp.GetAIOwner();
	FILLBTMoveToTaskMemory* MyMemory = reinterpret_cast<FILLBTMoveToTaskMemory*>(NodeMemory);
	if (!MyController)
		return;

	if (MaxNoVelocityTime > 0.f && MyController->GetPawn() && MyController->GetPawn()->GetVelocity().IsNearlyZero())
	{
		const float OldNoVelocityTime = TotalNoVelocityTime;
		TotalNoVelocityTime += DeltaSeconds;
		if (OldNoVelocityTime < MaxNoVelocityTime && TotalNoVelocityTime >= MaxNoVelocityTime)
		{
			UE_VLOG(MyController, LogBehaviorTree, Log, TEXT("Hit zero velocity limit of %3.2fs!"), MaxNoVelocityTime);
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
	}
	else
	{
		TotalNoVelocityTime = 0.f;
	}

	if (MyMemory->bWaitingForPath && !MyController->ShouldPostponePathUpdates())
	{
		UE_VLOG(MyController, LogBehaviorTree, Log, TEXT("Pathfinding requests are unlocked!"));
		MyMemory->bWaitingForPath = false;

		const EBTNodeResult::Type NodeResult = PerformMoveTask(OwnerComp, NodeMemory);
		if (NodeResult != EBTNodeResult::InProgress)
		{
			FinishLatentTask(OwnerComp, NodeResult);
		}
	}
}

void UBTTask_MoveToExtended::OnMessage(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, FName Message, int32 SenderID, bool bSuccess)
{
	// AIMessage_RepathFailed means task has failed
	bSuccess &= (Message != UBrainComponent::AIMessage_RepathFailed);

	// Check if we have reached our final destination
	FILLBTMoveToTaskMemory* MyMemory = reinterpret_cast<FILLBTMoveToTaskMemory*>(NodeMemory);
	if (bSuccess && Message == UBrainComponent::AIMessage_MoveFinished && MyMemory && FNavigationSystem::IsValidLocation(MyMemory->PreviousGoalLocation))
	{
		++BreadCrumbCount;
		if (BreadCrumbLimit <= 0 || BreadCrumbCount < BreadCrumbLimit)
		{
			// If our goal differs (we have not moved far enough or it has moved) then perform another move request instead of completing in the Super call
			const FVector Goal = CalcGoalLocation(OwnerComp);
			if (MyMemory->PreviousGoalLocation != Goal)
			{
				const EBTNodeResult::Type NodeResult = PerformMoveTask(OwnerComp, NodeMemory);
				if (NodeResult != EBTNodeResult::InProgress)
				{
					FinishLatentTask(OwnerComp, NodeResult);
				}
				return;
			}
		}
	}

	Super::OnMessage(OwnerComp, NodeMemory, Message, SenderID, bSuccess);
}

void UBTTask_MoveToExtended::OnGameplayTaskDeactivated(UGameplayTask& Task)
{
	// AI move task finished
	UAITask_MoveTo* MoveTask = Cast<UAITask_MoveTo>(&Task);
	if (MoveTask && MoveTask->GetAIController() && MoveTask->GetState() != EGameplayTaskState::Paused)
	{
		UBehaviorTreeComponent* BehaviorComp = GetBTComponentForTask(Task);
		if (BehaviorComp)
		{
			uint8* RawMemory = BehaviorComp->GetNodeMemory(this, BehaviorComp->FindInstanceContainingNode(this));
			FILLBTMoveToTaskMemory* MyMemory = reinterpret_cast<FILLBTMoveToTaskMemory*>(RawMemory);

			if (MyMemory->bObserverCanFinishTask && (MoveTask == MyMemory->Task))
			{
				const bool bSuccess = MoveTask->WasMoveSuccessful();
				FinishLatentTask(*BehaviorComp, bSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
			}
		}
	}
}

FString UBTTask_MoveToExtended::GetStaticDescription() const
{
	FString KeyDesc("invalid");
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass() ||
		BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		KeyDesc = BlackboardKey.SelectedKeyName.ToString();
	}

	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *KeyDesc);
}

void UBTTask_MoveToExtended::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (BlackboardComp)
	{
		const FString KeyValue = BlackboardComp->DescribeKeyValue(BlackboardKey.GetSelectedKeyID(), EBlackboardDescription::OnlyValue);

		FILLBTMoveToTaskMemory* MyMemory = reinterpret_cast<FILLBTMoveToTaskMemory*>(NodeMemory);
		const bool bIsUsingTask = MyMemory->Task.IsValid();
		
		const FString ModeDesc =
			MyMemory->bWaitingForPath ? TEXT("(WAITING)") :
			bIsUsingTask ? TEXT("(task)") :
			TEXT("");

		Values.Add(FString::Printf(TEXT("move target: %s%s"), *KeyValue, *ModeDesc));
	}
}

uint16 UBTTask_MoveToExtended::GetInstanceMemorySize() const
{
	return sizeof(FILLBTMoveToTaskMemory);
}

#if WITH_EDITOR

FName UBTTask_MoveToExtended::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.MoveTo.Icon");
}

#endif	// WITH_EDITOR

FVector UBTTask_MoveToExtended::CalcGoalLocation(UBehaviorTreeComponent& OwnerComp) const
{
	const UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
	AAIController* MyController = OwnerComp.GetAIOwner();
	FVector GoalLocation = FNavigationSystem::InvalidLocation;

	if (MyController && MyBlackboard)
	{
		// Grab the value from the Blackboard
		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
		{
			UObject* KeyValue = MyBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID());
			AActor* TargetActor = Cast<AActor>(KeyValue);
			if (TargetActor)
			{
				GoalLocation = TargetActor->GetActorLocation();
			}
			else
			{
				UE_VLOG(MyController, LogBehaviorTree, Warning, TEXT("UBTTask_MoveToExtended::ExecuteTask tried to go to actor while BB %s entry was empty"), *BlackboardKey.SelectedKeyName.ToString());
			}
		}
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			GoalLocation = MyBlackboard->GetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID());
		}

		// Limit this move request to BreadCrumbDistance
		if (MyController->GetPawn() && BreadCrumbDistance > 0.f && FNavigationSystem::IsValidLocation(GoalLocation))
		{
			const FVector MyLocation = MyController->GetPawn()->GetActorLocation();
			const FVector GoalDelta = GoalLocation - MyLocation;
			if (GoalDelta.SizeSquared() >= FMath::Square(BreadCrumbDistance))
			{
				const FVector GoalDirection = GoalDelta.GetSafeNormal();
				GoalLocation = MyLocation + GoalDirection*BreadCrumbDistance;
			}
		}

		// See if we can project the location to the navmesh, if not, try to find a point around that area
		if (bUseRandomPointIfProjectionFails)
		{
			UWorld* World = GetWorld();
			UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World);
			const FNavAgentProperties& AgentProps = MyController->GetNavAgentPropertiesRef();
			FNavLocation NavLocation;
			if (NavSys && !NavSys->ProjectPointToNavigation(GoalLocation, NavLocation, INVALID_NAVEXTENT, &AgentProps))
			{
				// try to find a location around this area
				if (NavSys->GetRandomReachablePointInRadius(GoalLocation, RandomPointRadius, NavLocation))
					GoalLocation = NavLocation.Location;
			}
		}
	}

	return GoalLocation;
}

EBlackboardNotificationResult UBTTask_MoveToExtended::OnBlackboardValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID)
{
	UBehaviorTreeComponent* BehaviorComp = Cast<UBehaviorTreeComponent>(Blackboard.GetBrainComponent());
	if (BehaviorComp == nullptr)
	{
		return EBlackboardNotificationResult::RemoveObserver;
	}

	uint8* RawMemory = BehaviorComp->GetNodeMemory(this, BehaviorComp->FindInstanceContainingNode(this));
	FILLBTMoveToTaskMemory* MyMemory = reinterpret_cast<FILLBTMoveToTaskMemory*>(RawMemory);

	const EBTTaskStatus::Type TaskStatus = BehaviorComp->GetTaskStatus(this);
	if (TaskStatus != EBTTaskStatus::Active)
	{
		UE_VLOG(BehaviorComp, LogBehaviorTree, Error, TEXT("BT MoveTo \'%s\' task observing BB entry while no longer being active!"), *GetNodeName());

		// resetting BBObserverDelegateHandle without unregistering observer since 
		// returning EBlackboardNotificationResult::RemoveObserver here will take care of that for us
		// ILLFONIC CHANGE BEGIN: dhumphries: pointer guard MyMemory
		if (MyMemory != nullptr)
			MyMemory->BBObserverDelegateHandle.Reset();
		// ILLFONIC CHANGE END

		return EBlackboardNotificationResult::RemoveObserver;
	}
	
	// this means the move has already started. MyMemory->bWaitingForPath == true would mean we're waiting for right moment to start it anyway,
	// so we don't need to do anything due to BB value change 
	if (MyMemory != nullptr && MyMemory->bWaitingForPath == false && BehaviorComp->GetAIOwner() != nullptr)
	{
		check(BehaviorComp->GetAIOwner()->GetPathFollowingComponent());

		// Check if new goal is almost identical to previous one
		FVector TargetLocation = CalcGoalLocation(*BehaviorComp);
		const bool bUpdateMove = (FVector::DistSquared(TargetLocation, MyMemory->PreviousGoalLocation) > FMath::Square(ObservedBlackboardValueTolerance));

		if (bUpdateMove)
		{
			// don't abort move if using AI tasks - it will mess things up
			if (MyMemory->MoveRequestID.IsValid())
			{
				UE_VLOG(BehaviorComp, LogBehaviorTree, Log, TEXT("Blackboard value for goal has changed, aborting current move request"));
				StopWaitingForMessages(*BehaviorComp);
				BehaviorComp->GetAIOwner()->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::NewRequest, MyMemory->MoveRequestID, EPathFollowingVelocityMode::Keep);
			}

			if (!bUseGameplayTasks && BehaviorComp->GetAIOwner()->ShouldPostponePathUpdates())
			{
				// NodeTick will take care of requesting move
				MyMemory->bWaitingForPath = true;
			}
			else
			{
				const EBTNodeResult::Type NodeResult = PerformMoveTask(*BehaviorComp, RawMemory);
				if (NodeResult != EBTNodeResult::InProgress)
				{
					FinishLatentTask(*BehaviorComp, NodeResult);
				}
			}
		}
	}

	return EBlackboardNotificationResult::ContinueObserving;
}

UAITask_MoveTo* UBTTask_MoveToExtended::PrepareMoveTask(UBehaviorTreeComponent& OwnerComp, UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest)
{
	UAITask_MoveTo* MoveTask = ExistingTask ? ExistingTask : NewBTAITask<UAITask_MoveTo>(OwnerComp);
	if (MoveTask)
	{
		MoveTask->SetUp(MoveTask->GetAIController(), MoveRequest);
	}

	return MoveTask;
}

EBTNodeResult::Type UBTTask_MoveToExtended::PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
	FILLBTMoveToTaskMemory* MyMemory = reinterpret_cast<FILLBTMoveToTaskMemory*>(NodeMemory);
	AAIController* MyController = OwnerComp.GetAIOwner();

	EBTNodeResult::Type NodeResult = EBTNodeResult::Failed;
	bAlreadyAtGoal = false;
	if (MyController && MyController->GetPawn() && MyBlackboard)
	{
		FAIMoveRequest MoveReq;
		MoveReq.SetNavigationFilter(*FilterClass ? FilterClass : MyController->GetDefaultNavigationFilterClass());
		MoveReq.SetAllowPartialPath(bAllowPartialPath);
		MoveReq.SetAcceptanceRadius(AcceptableRadius);
		MoveReq.SetCanStrafe(bAllowStrafe);
		MoveReq.SetReachTestIncludesAgentRadius(bStopOnOverlap);
		MoveReq.SetProjectGoalLocation(bProjectGoalLocation);
		MoveReq.SetUsePathfinding(bUsePathfinding);

		const FVector GoalLocation = CalcGoalLocation(OwnerComp);
		if (!FNavigationSystem::IsValidLocation(GoalLocation))
		{
			return EBTNodeResult::Failed;
		}

		MoveReq.SetGoalLocation(GoalLocation);
		MyMemory->PreviousGoalLocation = GoalLocation;

		if (MoveReq.IsValid())
		{
			if (GET_AI_CONFIG_VAR(bEnableBTAITasks))
			{
				UAITask_MoveTo* MoveTask = MyMemory->Task.Get();
				const bool bReuseExistingTask = (MoveTask != nullptr);

				MoveTask = PrepareMoveTask(OwnerComp, MoveTask, MoveReq);
				if (MoveTask)
				{
					MyMemory->bObserverCanFinishTask = false;

					if (bReuseExistingTask)
					{
						if (MoveTask->IsActive())
						{
							UE_VLOG(MyController, LogBehaviorTree, Verbose, TEXT("\'%s\' reusing AITask %s"), *GetNodeName(), *MoveTask->GetName());
							MoveTask->ConditionalPerformMove();
						}
						else
						{
							UE_VLOG(MyController, LogBehaviorTree, Verbose, TEXT("\'%s\' reusing AITask %s, but task is not active - handing over move performing to task mechanics"), *GetNodeName(), *MoveTask->GetName());
						}
					}
					else
					{
						MyMemory->Task = MoveTask;
						UE_VLOG(MyController, LogBehaviorTree, Verbose, TEXT("\'%s\' task implementing move with task %s"), *GetNodeName(), *MoveTask->GetName());
						MoveTask->ReadyForActivation();
					}

					MyMemory->bObserverCanFinishTask = true;
					NodeResult = (MoveTask->GetState() != EGameplayTaskState::Finished) ? EBTNodeResult::InProgress :
						MoveTask->WasMoveSuccessful() ? EBTNodeResult::Succeeded :
						EBTNodeResult::Failed;
				}
			}
			else
			{
				FPathFollowingRequestResult RequestResult = MyController->MoveTo(MoveReq);
				if (RequestResult.Code == EPathFollowingRequestResult::RequestSuccessful)
				{
					MyMemory->MoveRequestID = RequestResult.MoveId;
					WaitForMessage(OwnerComp, UBrainComponent::AIMessage_MoveFinished, RequestResult.MoveId);
					WaitForMessage(OwnerComp, UBrainComponent::AIMessage_RepathFailed);

					NodeResult = EBTNodeResult::InProgress;
				}
				else if (RequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
				{
					bAlreadyAtGoal = true;
					NodeResult = EBTNodeResult::Succeeded;
				}
			}
		}
	}

	return NodeResult;
}
