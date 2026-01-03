// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlayerStart.h"

#include "UObjectToken.h"
#include "MapErrors.h"
#include "MessageLog.h"

AILLPlayerStart::AILLPlayerStart(const FObjectInitializer& ObjectInitializer) 
: Super(ObjectInitializer) 
{
}

bool AILLPlayerStart::IsEncroached(APawn* PawnToFit/* = nullptr*/) const
{
	return GetWorld() && GetWorld()->EncroachingBlockingGeometry(PawnToFit ? (AActor*)PawnToFit : (AActor*)this, GetActorLocation(), GetActorRotation());
}

#if WITH_EDITOR
void AILLPlayerStart::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	// Validate when aren't finished moving too, so it updates as we drag
	if (!bFinished)
	{
		Validate();
	}
}

void AILLPlayerStart::CheckForErrors()
{
	// Also validate, so that the editor sprites update
	Validate();

	Super::CheckForErrors();

	FMessageLog MapCheck("MapCheck");

	// Check for blocking geometry
	if (IsEncroached() || (GetBadSprite() && GetBadSprite()->bVisible))
	{
		static FName NAME_SpawnEncroached(TEXT("SpawnEncroached"));
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} is blocked by encroaching geometry!")), Arguments)))
			->AddToken(FMapErrorToken::Create(NAME_SpawnEncroached));
	}
	else if (GroundDistanceLimit > 0.f)
	{
		// Check vertical placement
		FHitResult Hit(1.f);
		const FVector CapsuleBottom = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		const FVector TraceStart = CapsuleBottom;
		const FVector TraceEnd = CapsuleBottom - FVector(0.f, 0.f, GroundDistanceLimit);
		static FName NAME_SpawnFindFloor = FName(TEXT("SpawnFindFloor"));
		GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, FCollisionQueryParams(NAME_SpawnFindFloor, false));
		if (!Hit.bBlockingHit)
		{
			static FName NAME_SpawnFloating(TEXT("SpawnFloating"));
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
			MapCheck.Warning()
				->AddToken(FUObjectToken::Create(this))
				->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} is floating too far above the ground")), Arguments)))
				->AddToken(FMapErrorToken::Create(NAME_SpawnFloating));
		}
	}
}
#endif

void AILLPlayerStart::Validate()
{
	Super::Validate();

	if (IsEncroached())
	{
		// Show the BadSprite
		if (GetGoodSprite())
		{
			GetGoodSprite()->SetVisibility(false);
		}
		if (GetBadSprite())
		{
			GetBadSprite()->SetVisibility(true);
		}
	}
}
