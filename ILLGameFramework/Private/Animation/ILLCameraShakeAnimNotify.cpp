// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLCameraShakeAnimNotify.h"

#include "MessageLog.h"
#include "UObjectToken.h"

UILLCameraShakeAnimNotify::UILLCameraShakeAnimNotify(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, Scale(1.f)
{
#if WITH_EDITORONLY_DATA
	// A nice purple, why not
	NotifyColor = FColor(200, 128, 255, 255);
#endif
}

void UILLCameraShakeAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
#if WITH_EDITOR
	if (CameraShake == nullptr)
	{
		// Log error
		FMessageLog("PIE").Error()
			->AddToken(FTextToken::Create(FText::FromString(TEXT("Trying to play a null camera shake in animation "))))
			->AddToken(FUObjectToken::Create(Animation));
		return;
	}
#endif

	if (ACharacter* Owner = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		bool bPlayed = false;
		APlayerController* PC = Cast<APlayerController>(Owner->GetController());
		if ((!PC || !PC->IsLocalController()) && bPlayForAllClients && GEngine)
			PC = GEngine->GetFirstLocalPlayerController(Owner->GetWorld());

		if (PC && PC->IsLocalController())
		{
			if (PC->IsLocalController())
			{
				PC->ClientPlayCameraShake(CameraShake, Scale);
				bPlayed = true;
			}
		}

		// We didn't play the shake so let's see if the local player is close enough
		if (bPlayed == false && ShakeRange > 0.f)
		{
			if (GEngine)
			{
				if (APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(Owner->GetWorld()))
				{
					const FVector LocalPlayerPosition = LocalPC->PlayerCameraManager->GetCameraLocation();
					const FVector PlayingPlayerPosition = MeshComp->GetComponentLocation();

					const float DistSqr = FVector::DistSquared(LocalPlayerPosition, PlayingPlayerPosition);
					if (DistSqr <= FMath::Square(ShakeRange))
					{
						const float Distance = FMath::Sqrt(DistSqr);
						const float ScaleAlpha = FMath::Clamp((Distance - ShakeFalloffStart) / FMath::Max(ShakeRange - ShakeFalloffStart, KINDA_SMALL_NUMBER), 0.f, 1.f);
						const float PlayScale = FMath::Lerp(NearbyScale, 0.f, ScaleAlpha);

						LocalPC->ClientPlayCameraShake(CameraShake, PlayScale);
					}
				}
			}
		}
	}
}

FString UILLCameraShakeAnimNotify::GetNotifyName_Implementation() const
{
	if (CameraShake)
	{
		return CameraShake->GetName();
	}
	else
	{
		return FString(TEXT("ILL Camera Shake"));
	}
}
