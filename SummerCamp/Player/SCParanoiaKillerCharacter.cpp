// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCParanoiaKillerCharacter.h"

#include "SCPlayerController_Paranoia.h"
#include "ILLGameBlueprintLibrary.h"
#include "SCGameState.h"
#include "SCInGameHUD.h"
#include "SCPlayerState.h"
#include "SCContextKillActor.h"
#include "SCDynamicContextKill.h"

ASCParanoiaKillerCharacter::ASCParanoiaKillerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASCParanoiaKillerCharacter::SetupPlayerInputComponent(class UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);

	InInputComponent->BindAction(TEXT("KLR_UseAbility"), IE_Pressed, this, &ThisClass::OnChangeIntoCounselor);
}

void ASCParanoiaKillerCharacter::OnReceivedPlayerState()
{
	ASCCharacter::OnReceivedPlayerState();

	ASCPlayerState* PS = GetPlayerState();
	if (Role == ROLE_Authority && PS)
	{
		// Spawn grab kills
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Instigator;

		const TArray<TSubclassOf<ASCContextKillActor>>& PickedGrabKills = PS->GetPickedGrabKills();
		for (int32 KillIndex = 0; KillIndex < GRAB_KILLS_MAX; ++KillIndex)
		{
			UClass* KillClass = (PickedGrabKills.IsValidIndex(KillIndex) && PickedGrabKills[KillIndex]) ? *PickedGrabKills[KillIndex] : *GrabKillClasses[KillIndex];
			if (!KillClass)
				continue;

			GrabKills[KillIndex] = GetWorld()->SpawnActor<ASCDynamicContextKill>(KillClass, SpawnParams);

			// Could be null at this point, just don't.
			if (GrabKills[KillIndex])
			{
				GrabKills[KillIndex]->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				// Move the volume to the ground.
				GrabKills[KillIndex]->SetActorRelativeLocation(FVector(0.f, 0.f, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
			}
		}
	}
}

ASCCounselorCharacter* ASCParanoiaKillerCharacter::GetLocalCounselor() const
{
	if (ASCPlayerController_Paranoia* PC = Cast<ASCPlayerController_Paranoia>(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld())))
	{
		if (PC == GetController())
			return PC->GetCounselorPawn();
	}

	return Super::GetLocalCounselor();
}

void ASCParanoiaKillerCharacter::OnChangeIntoCounselor()
{
	if (bUnderWater || IsAttemptingGrab() || IsGrabKilling() || IsInSpecialMove() || GrabbedCounselor != nullptr)
		return;

	if (ASCPlayerController_Paranoia* PC = Cast<ASCPlayerController_Paranoia>(Controller))
	{
		if (PC->CanPlayerTransform())
			PC->SERVER_TransformIntoCounselor();
	}
}