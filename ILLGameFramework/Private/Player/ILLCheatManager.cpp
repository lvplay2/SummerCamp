// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLCheatManager.h"

#include "ILLGameInstance.h"
#include "ILLGameMode.h"
#include "ILLPlayerController.h"

UILLCheatManager::UILLCheatManager(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLCheatManager::ForceServerInstanceRecycle()
{
	AILLPlayerController* Outer = GetOuterAILLPlayerController();
	UWorld* World = Outer->GetWorld();
	UILLGameInstance* GameInstance = CastChecked<UILLGameInstance>(Outer->GetGameInstance());
	AILLGameMode* GameMode = World->GetAuthGameMode<AILLGameMode>();

	// Force instance recycling
	GameInstance->CheckInstanceRecycle(true);

	// Immediately update the game session so we close to matchmaking
	GameMode->UpdateGameSession();

	// Call to RestartGame to start the replacement/recycle process, with a short delay
	FTimerHandle Dummy;
	World->GetTimerManager().SetTimer(Dummy, GameMode, &AILLGameMode::RestartGame, 1.f);
}

void UILLCheatManager::TestCrash(float Delay/* = 0.f*/)
{
	AILLPlayerController* Outer = GetOuterAILLPlayerController();
	UWorld* World = Outer->GetWorld();
	if (Delay > 0.f)
	{
		FTimerHandle Dummy;
		World->GetTimerManager().SetTimer(Dummy, this, &ThisClass::CrashDummy, Delay);
	}
	else
	{
		CrashDummy();
	}
}

void UILLCheatManager::CrashDummy()
{
	UE_LOG(LogIGF, Warning, TEXT("Forcing a crash."));
	int* Pointer = nullptr;
	*Pointer = 1;
}
