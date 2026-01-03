// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_SetCounselorFlashlightMode.h"

#include "SCCounselorCharacter.h"

UBTTask_SetCounselorFlashlightMode::UBTTask_SetCounselorFlashlightMode(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Set Counselor Flashlight Mode");
}

EBTNodeResult::Type UBTTask_SetCounselorFlashlightMode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	if (ASCCounselorCharacter* BotPawn = BotController ? Cast<ASCCounselorCharacter>(BotController->GetPawn()) : nullptr)
	{
		BotController->FlashlightMode = FlashlightMode;
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

FString UBTTask_SetCounselorFlashlightMode::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), FlashlightModeToString(FlashlightMode));
}
