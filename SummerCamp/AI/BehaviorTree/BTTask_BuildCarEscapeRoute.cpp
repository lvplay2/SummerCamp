// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_BuildCarEscapeRoute.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

#include "Components/SplineComponent.h"

#include "SCCharacterAIController.h"
#include "SCDriveableVehicle.h"
#include "SCEscapeVolume.h"
#include "SCGameState.h"
#include "SCRoadPoint.h"

UBTTask_BuildCarEscapeRoute::UBTTask_BuildCarEscapeRoute(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Build Car Escape Route");

	CarEscapeRouteKey.AddObjectFilter(this, *NodeName, USplineComponent::StaticClass());
	EscapeIndexKey.AddIntFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_BuildCarEscapeRoute::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	ASCDriveableVehicle* BotCar = BotController ? Cast<ASCDriveableVehicle>(BotController->GetPawn()) : nullptr;
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();

	if (BotCar && BotBlackboard)
	{
		UWorld* World = GetWorld();
		ASCGameState* GS = World ? World->GetGameState<ASCGameState>() : nullptr;
		if (GS)
		{
			// Loop through road points to find which ones are labeled as goal locations
			TArray<ASCRoadPoint*> GoalPoints;
			for (ASCRoadPoint* RoadPoint : GS->RoadPoints)
			{
				if (IsValid(RoadPoint) && RoadPoint->bGoalLocation)
					GoalPoints.Add(RoadPoint);
			}

			// Pick a goal at random for funsies
			int32 GoalIndex = GoalPoints.Num() > 0 ? FMath::RandHelper(GoalPoints.Num()) : INDEX_NONE;
			if (GoalIndex != INDEX_NONE)
			{
				// We're about to A* like a boss, need to setup some shit, first.
				const FVector CarLocation = BotCar->GetActorLocation();
				ASCRoadPoint* Start = nullptr;
				const ASCRoadPoint* Goal = GoalPoints[GoalIndex];

				// SCORES ARE FOR WHORES
				TMap<ASCRoadPoint*, float> NeighborScores;
				TMap<ASCRoadPoint*, float> GoalScores;

				// Figure out where we want to start from
				float ClosestDistSq = FLT_MAX;
				for (ASCRoadPoint* RoadPoint : GS->RoadPoints)
				{
					if (!IsValid(RoadPoint))
						continue;

					NeighborScores.Add(RoadPoint, FLT_MAX);
					GoalScores.Add(RoadPoint, FLT_MAX);

					float DistanceSq = (CarLocation - RoadPoint->GetActorLocation()).SizeSquared2D();
					if (DistanceSq < ClosestDistSq)
					{
						ClosestDistSq = DistanceSq;
						Start = RoadPoint;
					}
				}

				// Keeps track of our evaluated and un-evaluated points
				TArray<ASCRoadPoint*> OpenSet;
				TArray<ASCRoadPoint*> ClosedSet;
				OpenSet.Add(Start); // Our initial point to evaluate

				// The point that most effeciently this point
				TMap<ASCRoadPoint*, ASCRoadPoint*> CameFrom;

				// Our final ideal path (goal is first entry, loop backward to build actual path of points)
				TArray<ASCRoadPoint*> TotalPath;
				
				// Start always has a neighbor score of 0
				NeighborScores[Start] = 0.f;
				GoalScores[Start] = (Goal->GetActorLocation() - Start->GetActorLocation()).Size();

				while (OpenSet.Num() > 0)
				{
					ASCRoadPoint* Current = OpenSet[0];
					for (int32 i(0); i < OpenSet.Num(); ++i)
					{
						if (GoalScores[OpenSet[i]] < GoalScores[Current])
							Current = OpenSet[i];
					}

					// We made it. Build the new spline and set it up.
					if (Current == Goal)
					{
						TotalPath.Add(Current);
						while (CameFrom.Contains(Current))
						{
							Current = CameFrom[Current];
							TotalPath.Add(Current);
						}

						USplineComponent* Spline = NewObject<USplineComponent>(GetOuter());
						Spline->ClearSplinePoints();
						for (int32 i(TotalPath.Num() - 1); i >= 0; --i) // Add them in reverse (first point is the goal)
						{
							Spline->AddSplinePoint(TotalPath[i]->GetActorLocation(), ESplineCoordinateSpace::World);
						}
						Spline->UpdateSpline();

						// Set the escape route object (it's a spline) and initialize the index to 0
						const auto CarEscapeRouteKeyId = BotBlackboard->GetKeyID(CarEscapeRouteKey.SelectedKeyName);
						const auto EscapeIndexKeyId = BotBlackboard->GetKeyID(EscapeIndexKey.SelectedKeyName);
						if (CarEscapeRouteKeyId != FBlackboard::InvalidKey && EscapeIndexKeyId != FBlackboard::InvalidKey)
						{
							BotBlackboard->SetValue<UBlackboardKeyType_Object>(CarEscapeRouteKeyId, Spline);
							BotBlackboard->SetValue<UBlackboardKeyType_Int>(EscapeIndexKeyId, 0);
							return EBTNodeResult::Succeeded;
						}

						break;
					}

					OpenSet.Remove(Current);
					ClosedSet.Add(Current);

					for (ASCRoadPoint* Neighbor : Current->Neighbors)
					{
						// This neighbor has already been evaluated
						if (ClosedSet.Contains(Neighbor))
							continue;

						// We want to evaluate every point
						if (!OpenSet.Contains(Neighbor))
							OpenSet.Add(Neighbor);

						// Tally it up!
						float score = NeighborScores[Current] + (Neighbor->GetActorLocation() - Current->GetActorLocation()).Size();
						if (score >= NeighborScores[Neighbor])
							continue;

						// This location is the new bestest. Update the bestest scores.
						if (!CameFrom.Contains(Neighbor))
							CameFrom.Add(Neighbor, Current);
						else
							CameFrom[Neighbor] = Current;

						NeighborScores[Neighbor] = score;
						GoalScores[Neighbor] = NeighborScores[Neighbor] + (Neighbor->GetActorLocation() - Goal->GetActorLocation()).Size();
					}
				}
			}
		}
	}

	// Something went very wrong
	return EBTNodeResult::Failed;
}