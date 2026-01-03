// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindRadio.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCabin.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCIndoorComponent.h"
#include "SCInteractComponent.h"
#include "SCRadio.h"


UBTTask_FindRadio::UBTTask_FindRadio(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Closest Radio");

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindRadio::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	ASCCounselorCharacter* BotPawn = (BotBlackboard && BotController) ? Cast<ASCCounselorCharacter>(BotController->GetPawn()) : nullptr;
	if (!BotPawn)
		return EBTNodeResult::Failed;

	// Clear interaction hack done in all of these...
	BotController->SetDesiredInteractable(nullptr);

	// Check requirements
	if (BotPawn->IsInWheelchair())
		return EBTNodeResult::Failed;
	if (!BotPawn->IsIndoors())
		return EBTNodeResult::Failed;

	FVector RadioLocation(FNavigationSystem::InvalidLocation);
	float ClosestRadioDistSq = FLT_MAX;
	USCInteractComponent* DesiredRadioInteractable = nullptr;

	// Find all radios in the cabin we are in
	for (const USCIndoorComponent* IndoorComponent : BotPawn->OverlappingIndoorComponents)
	{
		if (ASCCabin* Cabin = Cast<ASCCabin>(IndoorComponent->GetOwner()))
		{
			for (AActor* Child : Cabin->GetChildren())
			{
				ASCRadio* Radio = Cast<ASCRadio>(Child);
				if (!Radio || Radio->IsPlaying() || Radio->IsBroken())
					continue;

				USCInteractComponent* RadioInteractComp = Radio->GetInteractComponent();
				if (RadioInteractComp && RadioInteractComp->AcceptsPendingCharacterAI(BotController))
				{
					FVector InteractLocation;
					if (RadioInteractComp->BuildAIInteractionLocation(BotController, InteractLocation))
					{
						const float DistanceToRadioSq = (InteractLocation - BotPawn->GetActorLocation()).SizeSquared();
						if (DistanceToRadioSq < ClosestRadioDistSq)
						{
							ClosestRadioDistSq = DistanceToRadioSq;
							RadioLocation = InteractLocation;
							DesiredRadioInteractable = RadioInteractComp;
						}
					}
				}
			}
		}
	}

	// Set the blackboard key with the location of the radio, and assign our desired interactable
	if (FNavigationSystem::IsValidLocation(RadioLocation))
	{
		const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		if (BlackboardKeyId != FBlackboard::InvalidKey)
		{
			BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, RadioLocation);
			BotController->SetDesiredInteractable(DesiredRadioInteractable, true);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}