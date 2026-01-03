// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindRepairPartGoal.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCGameMode.h"
#include "SCInventoryComponent.h"
#include "SCRepairComponent.h"
#include "SCRepairPart.h"

UBTTask_FindRepairPartGoal::UBTTask_FindRepairPartGoal(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Repair Part Goal");

	DistanceLimit = 3000.f;

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindRepairPartGoal::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	ASCCounselorCharacter* BotPawn = (BotBlackboard && BotController) ? Cast<ASCCounselorCharacter>(BotController->GetPawn()) : nullptr;
	if (!BotPawn)
		return EBTNodeResult::Failed;

	// Clear interaction hack done in all of these...
	BotController->SetDesiredInteractable(nullptr);

	UWorld* World = GetWorld();
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World);
	const ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();

	// Get a list of all of our current repair items
	TArray<ASCRepairPart*> CurrentRepairItems;
	if (ASCRepairPart* CurrentRepairPart = Cast<ASCRepairPart>(BotPawn->GetEquippedItem()))
		CurrentRepairItems.AddUnique(CurrentRepairPart);

	for (AILLInventoryItem* Item : BotPawn->GetSmallInventory()->GetItemList())
	{
		if (Item->IsA<ASCRepairPart>())
			CurrentRepairItems.AddUnique(Cast<ASCRepairPart>(Item));
	}

	if (CurrentRepairItems.Num() > 0)
	{
		// Keep track of all possible repairs
		TMap<USCInteractComponent*, FVector> PossibleRepairs;
		for (ASCRepairPart* Part : CurrentRepairItems)
		{
			PossibleRepairs.Append(Part->GetRepairGoalCache());
		}

		const FNavAgentProperties& AgentProps = BotController->GetNavAgentPropertiesRef();
		const FVector BotLocation = BotPawn->GetActorLocation();
		const float MaxSearchDistanceSq = FMath::Square(DistanceLimit);
		FVector ClosestRepairLocation(FNavigationSystem::InvalidLocation);
		float ClosestRepairDistSq = FLT_MAX;
		USCInteractComponent* DesiredRepair = nullptr;

		// Find the closest repair
		for (TPair<USCInteractComponent*,FVector> RepairComponent : PossibleRepairs)
		{
			USCInteractComponent* InteractComponent = RepairComponent.Key;
			const FVector& InteractPoint = RepairComponent.Value;

			// Verify this interactable is not pending another AI
			if (!FNavigationSystem::IsValidLocation(InteractPoint) || !InteractComponent || !InteractComponent->AcceptsPendingCharacterAI(BotController))
				continue;

			// Evaluate distance
			const float DistanceToRepairSq = (BotLocation - InteractPoint).SizeSquared();
			if (DistanceToRepairSq < ClosestRepairDistSq && DistanceToRepairSq <= MaxSearchDistanceSq)
			{
				FNavLocation ProjectedLocation;
				if (NavSys->ProjectPointToNavigation(InteractPoint, ProjectedLocation, INVALID_NAVEXTENT, &AgentProps))
				{
					ClosestRepairLocation = ProjectedLocation.Location;
					ClosestRepairDistSq = DistanceToRepairSq;
					DesiredRepair = InteractComponent;
				}
			}
		}

		// Set the blackboard key with the location of the repair, and assign our desired interactable
		if (FNavigationSystem::IsValidLocation(ClosestRepairLocation))
		{
			const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
			if (BlackboardKeyId != FBlackboard::InvalidKey)
			{
				BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, ClosestRepairLocation);
				BotController->SetDesiredInteractable(DesiredRepair);
				return EBTNodeResult::Succeeded;
			}
		}
	}

	return EBTNodeResult::Failed;
}
