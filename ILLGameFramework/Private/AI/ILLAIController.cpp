// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLAIController.h"

#include "ILLCharacter.h"
#include "ILLPlayerState.h"
#include "SoundNodeLocalPlayer.h"
#include "BrainComponent.h"
#include "BehaviorTree/BehaviorTree.h"

DEFINE_LOG_CATEGORY(LogILLAI);

AILLAIController::AILLAIController(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void AILLAIController::BeginDestroy()
{
	Super::BeginDestroy();

	// Update USoundNodeLocalPlayer
	if (!GExitPurge)
	{
		const uint32 UniqueID = GetUniqueID();
		FAudioThread::RunCommandOnAudioThread([UniqueID]()
		{
			USoundNodeLocalPlayer::GetLocallyControlledActorCache().Remove(UniqueID);
		});
	}
}

void AILLAIController::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	// Update USoundNodeLocalPlayer
	bool bLocallyControlled = IsLocalController();
	if (AILLCharacter* Char = Cast<AILLCharacter>(GetPawn()))
	{
		bLocallyControlled = bLocallyControlled || Char->IsLocalPlayerViewTarget();
	}
	const uint32 UniqueID = GetUniqueID();
	FAudioThread::RunCommandOnAudioThread([UniqueID, bLocallyControlled]()
	{
		USoundNodeLocalPlayer::GetLocallyControlledActorCache().Add(UniqueID, bLocallyControlled);
	});

	// Delayed switching behavior trees
	if (PendingBehaviorTree != nullptr &&
		BrainComponent != nullptr)
	{
		BrainComponent->StopLogic(FString::Printf(TEXT("Switching BehaviorTree to %s"), *GetNameSafe(PendingBehaviorTree)));

		// Run the new one
		RunBehaviorTree(PendingBehaviorTree);

		PendingBehaviorTree = nullptr;
	}
}

void AILLAIController::ChangeBehaviorTree(UBehaviorTree* InBehaviorTree)
{
	check(InBehaviorTree != nullptr);

	PendingBehaviorTree = InBehaviorTree;
}

void AILLAIController::InitPlayerState()
{
	Super::InitPlayerState();

	// Acknowledge the player state immediately so that HasFullyTraveled can return true
	if (AILLPlayerState* MyPS = Cast<AILLPlayerState>(PlayerState))
	{
		MyPS->bHasAckedPlayerState = true;
		MyPS->ForceNetUpdate();
	}
}

bool AILLAIController::RunBehaviorTree(UBehaviorTree* BTAsset)
{
	const UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent);

	// If we are calling RunBehaviorTree internally to the behavior tree tick, this can cause
	// Destructive actions, potentially trashing our internal memory
	// Catch this here and call our delayed ChangeBehaviorTree function instead
	if (BTComp &&
		BTComp->IsTicking())
	{
		ChangeBehaviorTree(BTAsset);
		return false; // False should prevent further execution
	}
	else
	{
		return Super::RunBehaviorTree(BTAsset);
	}
}
