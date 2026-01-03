// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_CounselorFindFleeLocation.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCGameState.h"
#include "SCKillerCharacter.h"

UBTTask_CounselorFindFleeLocation::UBTTask_CounselorFindFleeLocation(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Counselor Find FleeLocation");

	AcceptableRadius = 50.f;
	JasonJukeHorizontalRange = 22.5f;
	JasonJukeFallbackHorizontalOffset = 22.5f;

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_CounselorFindFleeLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCounselorAIController* MyController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	if (MyController)
		MyController->SetDesiredInteractable(nullptr);
	ASCCounselorCharacter* MyCounselor = MyController ? Cast<ASCCounselorCharacter>(MyController->GetPawn()) : nullptr;
	UWorld* World = MyCounselor ? MyCounselor->GetWorld() : nullptr;
	UNavigationSystem* NavSystem = World ? UNavigationSystem::GetCurrent(World) : nullptr;
	const FNavAgentProperties& AgentProps = MyController->GetNavAgentPropertiesRef();
	ANavigationData* NavData = NavSystem ? NavSystem->GetNavDataForProps(AgentProps) : nullptr;
	ASCGameState* GameState = World ? World->GetGameState<ASCGameState>() : nullptr;

	if (NavSystem && GameState && GameState->CurrentKiller && MyCounselor && MyBlackboard)
	{
		const FBlackboard::FKey BlackboardKeyId = MyBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		if (BlackboardKeyId != FBlackboard::InvalidKey)
		{
			const FVector CounselorLocation = MyCounselor->GetActorLocation();
			const FVector LastKnownKillerLocation = MyController->LastKillerStimulusLocation;
			if (FNavigationSystem::IsValidLocation(LastKnownKillerLocation))
			{
				const FVector CounselorToJason = (CounselorLocation - LastKnownKillerLocation).GetSafeNormal();
				const FRotator RoughDirection = FRotationMatrix::MakeFromX(CounselorToJason).Rotator();

				for (int32 Attempts = 0; Attempts < 2; ++Attempts)
				{
					// Sway randomly on Yaw to break up the linear run-away
					FRotator TestDirection = RoughDirection;
					float YawAdjust = FMath::RandRange(-JasonJukeHorizontalRange, JasonJukeHorizontalRange);
					if (Attempts > 0)
					{
						// Sway HARDER
						YawAdjust += JasonJukeFallbackHorizontalOffset * FMath::Sign(YawAdjust);
					}
					TestDirection.Yaw += YawAdjust;
					const FVector TestDestination = CounselorLocation + TestDirection.Vector() * FleeDistance;

					// Find a random reachable point around the TestDestination
					FNavLocation ResultLocation;
					FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, World, FilterClass);
					if (NavSystem->GetRandomReachablePointInRadius(TestDestination, AcceptableRadius, ResultLocation, nullptr, QueryFilter))
					{
						// Make sure it is valid and projects before accepting it
						FNavLocation ProjectedLocation;
						if (FAISystem::IsValidLocation(ResultLocation) && NavSystem->ProjectPointToNavigation(ResultLocation, ProjectedLocation, INVALID_NAVEXTENT, &AgentProps))
						{
							MyBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, ResultLocation);
							return EBTNodeResult::Succeeded;
						}
					}
				}
			}
		}
	}

	return EBTNodeResult::Failed;
}
