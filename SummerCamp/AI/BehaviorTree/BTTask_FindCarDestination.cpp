// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindCarDestination.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "Components/SplineComponent.h"

#include "SCCharacterAIController.h"
#include "SCDriveableVehicle.h"
#include "SCEscapeVolume.h"
#include "SCGameState.h"

extern TAutoConsoleVariable<int32> CVarDebugVehicles;

UBTTask_FindCarDestination::UBTTask_FindCarDestination(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Car Destination");

	CarDestinationKey.AddVectorFilter(this, *NodeName);
	CarEscapeRouteKey.AddObjectFilter(this, *NodeName, USplineComponent::StaticClass());
	EscapeIndexKey.AddIntFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindCarDestination::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Get the current road path the car should be following and update our destination to the next point if appropriate
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	ASCDriveableVehicle* BotCar = BotController ? Cast<ASCDriveableVehicle>(BotController->GetPawn()) : nullptr;

	if (BotBlackboard && BotCar)
	{
		const FBlackboard::FKey CarEscapeRouteKeyId = BotBlackboard->GetKeyID(CarEscapeRouteKey.SelectedKeyName);
		if (CarEscapeRouteKeyId != FBlackboard::InvalidKey)
		{
			if (USplineComponent* EscapeRoute = Cast<USplineComponent>(BotBlackboard->GetValue<UBlackboardKeyType_Object>(CarEscapeRouteKeyId)))
			{
				const FBlackboard::FKey EscapeIndexKeyId = BotBlackboard->GetKeyID(EscapeIndexKey.SelectedKeyName);
				const FBlackboard::FKey CarDestinationId = BotBlackboard->GetKeyID(CarDestinationKey.SelectedKeyName);
				if (EscapeIndexKeyId != FBlackboard::InvalidKey && CarDestinationId != FBlackboard::InvalidKey)
				{
					int32 EscapeIndex = BotBlackboard->GetValue<UBlackboardKeyType_Int>(EscapeIndexKeyId);
					if (EscapeIndex > INDEX_NONE && EscapeIndex < EscapeRoute->GetNumberOfSplinePoints())
					{
						const FVector PreviousGoalLocation = EscapeRoute->GetLocationAtSplinePoint(EscapeIndex, ESplineCoordinateSpace::World);
						const FVector CarLocation = BotCar->GetActorLocation();

						// Let's make sure the car made it close enough to the goal location before updating it to the next point on the road
						if ((PreviousGoalLocation - CarLocation).SizeSquared2D() <= FMath::Square(500.f))
						{
							EscapeIndex++;
							BotBlackboard->SetValue<UBlackboardKeyType_Int>(EscapeIndexKeyId, EscapeIndex);

							const FVector NewVehicleDestination = EscapeRoute->GetLocationAtSplinePoint(EscapeIndex, ESplineCoordinateSpace::World);
							BotBlackboard->SetValue<UBlackboardKeyType_Vector>(CarDestinationId, NewVehicleDestination);

							if (CVarDebugVehicles.GetValueOnGameThread() > 0)
							{
								DrawDebugSphere(GetWorld(), NewVehicleDestination, 15.f, 9, FColor::Green, false, 5.f, 0, 1.25f);
							}

							return EBTNodeResult::Succeeded;
						}
						else // If we didn't, just use the same destination
						{
							BotBlackboard->SetValue<UBlackboardKeyType_Int>(EscapeIndexKeyId, EscapeIndex);
							BotBlackboard->SetValue<UBlackboardKeyType_Vector>(CarDestinationId, PreviousGoalLocation);
							if (CVarDebugVehicles.GetValueOnGameThread() > 0)
							{
								DrawDebugSphere(GetWorld(), PreviousGoalLocation, 15.f, 9, FColor::Red, false, 5.f, 0, 1.25f);
							}
							return EBTNodeResult::Succeeded;
						}
					}
				}
			}
		}
	}

	// Oh boy
	return EBTNodeResult::Failed;
}
