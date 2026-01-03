// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCNoiseMaker.h"

#include "ILLInventoryComponent.h"
#include "SCCounselorCharacter.h"
#include "SCGameMode.h"
#include "SCNoiseMaker_Projectile.h"
#include "SCPlayerState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCNoiseMaker"

ASCNoiseMaker::ASCNoiseMaker(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, ThrowDelayTime(0.5f)
{
	FriendlyName = LOCTEXT("FriendlyName", "Noise Maker");
}

void ASCNoiseMaker::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Cache the min/max pitch.
	if (VelocityPitchCurve)
		VelocityPitchCurve->FloatCurve.GetTimeRange(PitchConstraints.X, PitchConstraints.Y);
}

bool ASCNoiseMaker::Use(bool bFromInput)
{
	if (ProjectileClass)
	{
		FTimerDelegate Throw;
		Throw.BindLambda([this]()
		{
			if (IsValid(this))
			{
				ThrowProjectile();
			}
		});
		GetWorldTimerManager().SetTimer(TimerHandle_ThrowTimer, Throw, ThrowDelayTime, false);
	}

	return false;
}

void ASCNoiseMaker::ThrowProjectile()
{
	// Release the projectile! BE FREE!!!
	if (ProjectileClass)
	{
		if (UILLInventoryComponent* Inventory = GetOuterInventory())
		{
			if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(Inventory->GetCharacterOwner()))
			{
				if (UWorld* World = GetWorld())
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.Owner = this;
					SpawnParams.Instigator = Character;

					ASCNoiseMaker_Projectile* SpawnedProjectile = World->SpawnActor<ASCNoiseMaker_Projectile>(ProjectileClass, GetActorLocation(), GetActorRotation(), SpawnParams);
					if (SpawnedProjectile)
					{
						SpawnedProjectile->SetActorRotation(FRotator::ZeroRotator);

						// Clamp the target direction pitch for our velocity curve.
						FRotator CameraRotation = Character->GetControlRotation().GetNormalized();
						CameraRotation.Pitch = FMath::Clamp(CameraRotation.Pitch, PitchConstraints.X, PitchConstraints.Y);

						// Average our max pitch and current pitch andgle to apply a bit of upward force
						FRotator MaxRotation = CameraRotation;
						MaxRotation.Pitch = PitchConstraints.Y;
						const FVector TossDirection = (CameraRotation.Vector().GetSafeNormal() + MaxRotation.Vector().GetSafeNormal()).GetSafeNormal();

						// Get our initial velocity based on pitch.
						const float Velocity = VelocityPitchCurve->GetFloatValue(CameraRotation.Pitch);

						SpawnedProjectile->InitVelocity(Velocity * TossDirection);

						if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
						{
							GameMode->HandleScoreEvent(Character->PlayerState, UseScoreEvent);
						}

						Character->OnForceDestroyItem(this);
					}
				}

				// Track use
				if (ASCPlayerState* PS = Character->GetPlayerState())
				{
					PS->TrackItemUse(GetClass(), (uint8)ESCItemStatFlags::Used);
				}
			}
		}
	}

}

#undef LOCTEXT_NAMESPACE
