// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLSpawnDecalAnimNotify.h"
#include "ILLCharacter.h"

TAutoConsoleVariable<int32> CVarDebugDecal(
	TEXT("Ill.DebugDecalAnimNotify"),
	0,
	TEXT("Displays debug information for spawning decals through ILL Spawn Decal Anim Notify.\n")
	TEXT(" 0: None\n")
	TEXT(" 1: Display debug info"));

UILLSpawnDecalAnimNotify::UILLSpawnDecalAnimNotify(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

FString UILLSpawnDecalAnimNotify::GetNotifyName_Implementation() const
{
	if (!NotifyDisplayName.IsNone())
		return NotifyDisplayName.ToString();

	return FString(TEXT("ILL Spawn Decal"));
}

void UILLSpawnDecalAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (const AActor* const OwningActor = MeshComp->GetOwner())
	{
		const UWorld* const World = OwningActor->GetWorld();

		for (auto& Decal : Decals)
		{
			const FRotator RelRotation = OwningActor->GetRootComponent() == MeshComp ? FRotator::ZeroRotator : MeshComp->RelativeRotation;
			const FVector RayStart = MeshComp->GetSocketLocation(Decal.CharacterSocket) + Decal.SocketOffset;
			const FVector RayEnd = RayStart + (OwningActor->GetActorRotation().RotateVector(RelRotation.UnrotateVector(Decal.RayDirection)) * Decal.RayLength);
			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(OwningActor);
			QueryParams.bTraceComplex = true;
			bool bHit = false;

			if (World->LineTraceSingleByChannel(HitResult, RayStart, RayEnd, ECollisionChannel::ECC_Visibility, QueryParams))
			{
				bool SpawnDecal = true;
				if (HitResult.GetActor())
					SpawnDecal = !HitResult.GetActor()->IsA(AILLCharacter::StaticClass());

				if (SpawnDecal)
				{
					bHit = true;

					FRotator RandomDecalRotation = HitResult.ImpactNormal.Rotation();
					if (Decal.bApplyRandomRotation)
					{
						RandomDecalRotation.Roll = FMath::FRandRange(-180.0f, 180.0f);
					}

					UGameplayStatics::SpawnDecalAttached(Decal.DecalMaterial, FVector(Decal.DecalDepth, Decal.DecalSize, Decal.DecalSize), HitResult.Component.Get(), NAME_None, HitResult.ImpactPoint, RandomDecalRotation, EAttachLocation::KeepWorldPosition);
				}
			}

			if (CVarDebugDecal.GetValueOnGameThread() > 0)
			{
				DrawDebugLine(World, RayStart, RayEnd, bHit ? FColor::Green : FColor::Red, false, 10.f);
			}
		}
	}
}
