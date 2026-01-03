// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindClosest.h"

#include "SCCharacter.h"
#include "SCCounselorAIController.h"

UBTTask_FindClosest::UBTTask_FindClosest(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MaxJasonStimulusAge = 60.f;
}

bool UBTTask_FindClosest::IsJasonStimulusValid(UBehaviorTreeComponent& OwnerComp) const
{
	ASCCounselorAIController* CounselorController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	if (!CounselorController)
		return false;

	// Check stimulus validity
	if (!FNavigationSystem::IsValidLocation(CounselorController->LastKillerStimulusLocation) || CounselorController->LastKillerStimulusTime == 0.f)
		return false;

	// Only accept stimulus newer than MaxJasonStimulusAge seconds old
	if (MaxJasonStimulusAge > 0.f && GetWorld()->GetTimeSeconds()-CounselorController->LastKillerStimulusTime >= MaxJasonStimulusAge)
		return false;

	return true;
}

FVector UBTTask_FindClosest::CalculateEpicenter(UBehaviorTreeComponent& OwnerComp) const
{
	FVector Result(FNavigationSystem::InvalidLocation);

	ASCCounselorAIController* CounselorController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	if (ASCCharacter* CounselorPawn = CounselorController ? Cast<ASCCharacter>(CounselorController->GetPawn()) : nullptr)
	{
		Result = CounselorPawn->GetActorLocation();

		// Project the result away AwayFromJasonDistance units
		if (AwayFromJasonDistance > 0.f && IsJasonStimulusValid(OwnerComp))
		{
			const FVector Delta = Result - CounselorController->LastKillerStimulusLocation;
			const FVector Direction = Delta.GetSafeNormal();
			Result += Direction * AwayFromJasonDistance;
		}
	}

	return Result;
}
