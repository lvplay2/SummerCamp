// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "BTTask_FindWanderPoint.h"

#include "ILLAIController.h"

UBTTask_FindWanderPoint::UBTTask_FindWanderPoint(const FObjectInitializer& ObjectInitializer) 
: Super(ObjectInitializer)
{
	NodeName = "Find Wander Point";

	// Accept only actors and vectors
	NearKey.AddObjectFilter(this, *NodeName, AActor::StaticClass());
	NearKey.AddVectorFilter(this, *NodeName);

	AcceptableRadiusMin = 1000.f;
	AcceptableRadiusMax = 1500.f;

	// Accept only vectors
	WanderPointKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindWanderPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AILLAIController* OwnerController = Cast<AILLAIController>(OwnerComp.GetAIOwner()))
	{
		if (UBlackboardComponent* OwnerBlackboard = OwnerComp.GetBlackboardComponent())
		{
			const FSphere WanderCenterPoint = BuildWanderCenterPoint(OwnerController, OwnerBlackboard);
			if (WanderCenterPoint.Center == FVector::ZeroVector || !FNavigationSystem::IsValidLocation(WanderCenterPoint.Center) || WanderCenterPoint.W <= 0.f)
			{
				return EBTNodeResult::Failed;
			}

			const FNavAgentProperties& AgentProps = OwnerController->GetNavAgentPropertiesRef();
			UWorld* World = OwnerController->GetWorld();
			UNavigationSystem* NavSystem = UNavigationSystem::GetCurrent(World);
			ANavigationData* NavData = NavSystem ? NavSystem->GetNavDataForProps(AgentProps) : nullptr;

			FNavLocation ReachableWanderPoint;
			FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, World, FilterClass);
			if (!NavSystem->GetRandomReachablePointInRadius(WanderCenterPoint.Center, WanderCenterPoint.W, ReachableWanderPoint, nullptr, QueryFilter))
			{
				return EBTNodeResult::Failed;
			}

			if (!NavSystem->ProjectPointToNavigation(ReachableWanderPoint.Location, ReachableWanderPoint, AgentProps.GetExtent(), NavData, QueryFilter))
			{
				return EBTNodeResult::Failed;
			}

			const FBlackboard::FKey WanderKeyID = OwnerBlackboard->GetKeyID(WanderPointKey.SelectedKeyName);
			if (WanderKeyID != FBlackboard::InvalidKey)
			{
				OwnerBlackboard->SetValue<UBlackboardKeyType_Vector>(WanderKeyID, ReachableWanderPoint.Location);
				return EBTNodeResult::Succeeded;
			}
		}
	}

	return EBTNodeResult::Failed;
}

FSphere UBTTask_FindWanderPoint::BuildWanderCenterPoint(AILLAIController* OwnerController, UBlackboardComponent* OwnerBlackboard) const
{
	// Attempt to find a center point from the NearKey
	return BuildWanderCenterFromKey(OwnerController, OwnerBlackboard, NearKey, FMath::RandRange(AcceptableRadiusMin, AcceptableRadiusMax));
}

FSphere UBTTask_FindWanderPoint::BuildWanderCenterFromKey(AILLAIController* OwnerController, UBlackboardComponent* OwnerBlackboard, const FBlackboardKeySelector& Key, const float Radius) const
{
	// Wander near the NearKey
	const FBlackboard::FKey KeyID = OwnerBlackboard->GetKeyID(Key.SelectedKeyName);
	if (KeyID != FBlackboard::InvalidKey)
	{
		const auto KeyType = OwnerBlackboard->GetKeyType(KeyID);
		if (KeyType == UBlackboardKeyType_Object::StaticClass())
		{
			UObject* KeyValue = OwnerBlackboard->GetValue<UBlackboardKeyType_Object>(KeyID);
			if (AActor* NearActor = Cast<AActor>(KeyValue))
			{
				return FSphere(NearActor->GetActorLocation(), Radius);
			}
		}
		else if (KeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			return FSphere(OwnerBlackboard->GetValue<UBlackboardKeyType_Vector>(KeyID), Radius);
		}
	}

	// Fallback to the owning pawn
	if (APawn* OwningPawn = OwnerController->GetPawn())
	{
		return FSphere(OwningPawn->GetActorLocation(), Radius);
	}

	return FSphere(FNavigationSystem::InvalidLocation, 0.f);
}
