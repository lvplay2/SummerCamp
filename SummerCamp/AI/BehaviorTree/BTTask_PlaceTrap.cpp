// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_PlaceTrap.h"

#include "SCCharacterAIController.h"
#include "SCCounselorCharacter.h"
#include "SCTrap.h"

UBTTask_PlaceTrap::UBTTask_PlaceTrap(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Place Trap");
	bCreateNodeInstance = true;

	TimerDelegate_PlaceTrap = FTimerDelegate::CreateUObject(this, &ThisClass::OnPlaceTrapTimerDone);
}

EBTNodeResult::Type UBTTask_PlaceTrap::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	MyOwnerComp = &OwnerComp;
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	MyCharacter = (BotBlackboard && BotController) ? Cast<ASCCharacter>(BotController->GetPawn()) : nullptr;
	if (!MyCharacter)
		return EBTNodeResult::Failed;

	if (ASCTrap* CurrentTrap = Cast<ASCTrap>(MyCharacter->GetEquippedItem()))
	{
		if (ASCCounselorCharacter* MyCounselor = Cast<ASCCounselorCharacter>(MyCharacter))
			MyCounselor->OnLargeItemPressed();
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_PlaceTrap, TimerDelegate_PlaceTrap, 3.f, false);

		return EBTNodeResult::InProgress;
	}

	return EBTNodeResult::Failed;
}

void UBTTask_PlaceTrap::OnPlaceTrapTimerDone()
{
	if (ASCCounselorCharacter* MyCounselor = Cast<ASCCounselorCharacter>(MyCharacter))
		MyCounselor->OnLargeItemReleased();

	FinishLatentTask(*MyOwnerComp, EBTNodeResult::Succeeded);

	MyCharacter = nullptr;
	MyOwnerComp = nullptr;
}
