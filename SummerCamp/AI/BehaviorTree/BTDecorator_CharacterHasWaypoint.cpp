// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTDecorator_CharacterHasWaypoint.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

#include "SCCharacter.h"
#include "SCCharacterAIController.h"
#include "SCWaypoint.h"

UBTDecorator_CharacterHasWaypoint::UBTDecorator_CharacterHasWaypoint(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Character has waypoint");

	// Accept only SCWaypoints
	WaypointKey.AddObjectFilter(this, *NodeName, ASCWaypoint::StaticClass());
}

bool UBTDecorator_CharacterHasWaypoint::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	bool bCharacterHasWaypointSet = false;
	bool bBlackboardHasWaypointSet = false;

	// Check if the character has a starting waypoint set
	ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	if (ASCCharacter* BotPawn = BotController ? Cast<ASCCharacter>(BotController->GetPawn()) : nullptr)
		bCharacterHasWaypointSet = BotPawn->StartingWaypoint != nullptr;

	// Check if we have one set in our blackboard
	if (UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent())
	{
		// Make sure our selected key is valid
		const FBlackboard::FKey WaypointKeyId = BotBlackboard->GetKeyID(WaypointKey.SelectedKeyName);
		if (WaypointKeyId != FBlackboard::InvalidKey)
		{
			const TSubclassOf<UBlackboardKeyType> WaypointKeyType = BotBlackboard->GetKeyType(WaypointKeyId);
			if (WaypointKeyType == UBlackboardKeyType_Object::StaticClass())
			{
				bBlackboardHasWaypointSet = Cast<ASCWaypoint>(BotBlackboard->GetValue<UBlackboardKeyType_Object>(WaypointKeyId)) != nullptr;
			}
		}
	}

	return bCharacterHasWaypointSet || bBlackboardHasWaypointSet;
}
