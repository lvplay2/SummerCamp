// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLAnimNotify_ForceFeedback.h"

#include "MessageLog.h"
#include "UObjectToken.h"

UILLAnimNotify_ForceFeedback::UILLAnimNotify_ForceFeedback(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(238, 49, 36, 255);
#endif
}

void UILLAnimNotify_ForceFeedback::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
#if WITH_EDITOR
	if (ForceFeedback == nullptr)
	{
		// Log error
		FMessageLog("PIE").Error()
			->AddToken(FTextToken::Create(FText::FromString(TEXT("Trying to play a null force feedback in animation "))))
			->AddToken(FUObjectToken::Create(Animation));
		return;
	}
#endif

	if (ACharacter* Owner = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		bool bPlayed = false;
		if (APlayerController* PC = Cast<APlayerController>(Owner->GetController()))
		{
			if (PC->IsLocalController())
			{
				PlayForceFeedback(PC);
				bPlayed = true;
			}
		}

		// We didn't play the shake so let's see if the local player is close enough
		if (bPlayed == false && ForceFeedbackRemotePlayerRange > 0.f && GEngine)
		{
			if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(Owner->GetWorld()))
			{
				const FVector LocalPlayerPosition = PC->PlayerCameraManager->GetCameraLocation();
				const FVector PlayingPlayerPosition = MeshComp->GetComponentLocation();

				const float DistSqr = FVector::DistSquared(LocalPlayerPosition, PlayingPlayerPosition);
				if (DistSqr <= FMath::Square(ForceFeedbackRemotePlayerRange))
				{
					PlayForceFeedback(PC);
				}
			}
		}
	}
}

FString UILLAnimNotify_ForceFeedback::GetNotifyName_Implementation() const
{
	if (ForceFeedback)
	{
		return FString::Printf(TEXT("FF: %s"), *ForceFeedback->GetName());
	}
	else
	{
		return FString(TEXT("ILL Force Feedback"));
	}
}

void UILLAnimNotify_ForceFeedback::PlayForceFeedback(APlayerController* PlayerController) const
{
	switch (PlayStyle)
	{
	default: // Fallthrough
	case EILLForceFeedbackStyle::OneOff:
		PlayerController->ClientPlayForceFeedback(ForceFeedback, false, false, FeedbackTag);
		break;
	case EILLForceFeedbackStyle::Looping:
		PlayerController->ClientPlayForceFeedback(ForceFeedback, true, false, FeedbackTag);
		break;
	case EILLForceFeedbackStyle::Stop:
		PlayerController->ClientStopForceFeedback(ForceFeedback, FeedbackTag);
		break;
	}
}
