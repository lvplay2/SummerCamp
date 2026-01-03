// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindComsDevice.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCBRadio.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCFuseBox.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCInteractComponent.h"
#include "SCPhoneJunctionBox.h"
#include "SCPolicePhone.h"
#include "SCPowerGridVolume.h"
#include "SCPowerPlant.h"
#include "SCSpecialMoveComponent.h"

UBTTask_FindComsDevice::UBTTask_FindComsDevice(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Closest Communication Device");

	DistanceLimit = 2000.f;

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindComsDevice::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	UWorld* World = GetWorld();
	const ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
	const ASCGameState* GameState = World->GetGameState<ASCGameState>();
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World);
	if (!GameMode || !GameState)
		return EBTNodeResult::Failed;

	// Police phone
	TArray<USCInteractComponent*> PossibleDevices;
	for (ASCPhoneJunctionBox* PhoneBox : GameMode->GetAllPhoneJunctionBoxes())
	{
		if (IsValid(PhoneBox) && !PhoneBox->IsPoliceCalled() && PhoneBox->HasFuse())
		{
			if (!PhoneBox->IsPhoneBoxRepaired())
			{
				// We need to repair it
				PossibleDevices.AddUnique(PhoneBox->GetInteractComponent());
			}
			else
			{
				if (ASCPolicePhone* Phone = PhoneBox->GetConnectedPhone())
				{
					PossibleDevices.AddUnique(Phone->GetInteractComponent());
				}
			}
		}
	}

	// CB Radio
	for (ASCCBRadio* CBRadio : GameMode->GetAllCBRadios())
	{
		if (IsValid(CBRadio) && !CBRadio->IsHunterCalled())
		{
			if (!CBRadio->HasPower())
			{
				// Don't have power, so we need to fix the fuse box for this area
				if (USCPowerPlant* PowerPlant = GameState->GetPowerPlant())
				{
					for (ASCPowerGridVolume* PowerGrid : PowerPlant->GetPowerGrids())
					{
						if (PowerGrid->ContainsPoweredObject(CBRadio))
						{
							if (ASCFuseBox* FuseBox = PowerGrid->GetFuseBox())
							{
								PossibleDevices.AddUnique(FuseBox->InteractComponent);
								break;
							}

							break;
						}
					}
				}
			}
			else
			{
				PossibleDevices.AddUnique(CBRadio->GetInteractComponent());
			}
		}
	}

	const FNavAgentProperties& AgentProps = BotController->GetNavAgentPropertiesRef();
	const FVector BotLocation = BotPawn->GetActorLocation();
	const float MaxSearchDistanceSq = FMath::Square(DistanceLimit);
	FVector ClosestComsDevice(FNavigationSystem::InvalidLocation);
	float ClosestComsDeviceDistSq = FLT_MAX;
	USCInteractComponent* DesiredComsDevice = nullptr;

	// Find the closest coms device/coms repair
	for (USCInteractComponent* InteractComponent : PossibleDevices)
	{
		// Skip items other AI are looking at
		if (!InteractComponent->AcceptsPendingCharacterAI(BotController))
			continue;

		// Get all child component, so we can find the special move to use as a nav point
		FVector InteractPoint(FNavigationSystem::InvalidLocation);
		TArray<USceneComponent*> ChildComponents;
		InteractComponent->GetChildrenComponents(false, ChildComponents);
		for (USceneComponent* Child : ChildComponents)
		{
			if (Child->IsA<USCSpecialMoveComponent>())
			{
				InteractPoint = Child->GetComponentLocation();
				break;
			}
		}

		if (FNavigationSystem::IsValidLocation(InteractPoint))
		{
			const float DistanceToComsDeviceSq = (BotLocation - InteractPoint).SizeSquared();
			if (DistanceToComsDeviceSq < ClosestComsDeviceDistSq && DistanceToComsDeviceSq <= MaxSearchDistanceSq)
			{
				FNavLocation ProjectedLocation;
				if (NavSys->ProjectPointToNavigation(InteractPoint, ProjectedLocation, INVALID_NAVEXTENT, &AgentProps))
				{
					ClosestComsDevice = ProjectedLocation.Location;
					ClosestComsDeviceDistSq = DistanceToComsDeviceSq;
					DesiredComsDevice = InteractComponent;
				}
			}
		}
	}

	// Set the blackboard key with the location of the coms device, and assign our desired interactable
	if (FNavigationSystem::IsValidLocation(ClosestComsDevice))
	{
		const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		if (BlackboardKeyId != FBlackboard::InvalidKey)
		{
			BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, ClosestComsDevice);
			BotController->SetDesiredInteractable(DesiredComsDevice);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
