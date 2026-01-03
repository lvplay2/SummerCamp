// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameBlueprintLibrary.h"

#include "ILLBackendBlueprintLibrary.h"
#include "ILLGameInstance.h"
#include "ILLGameMode.h"
#include "ILLGameOnlineBlueprintLibrary.h"
#include "ILLGameSession.h"
#include "ILLGameViewportClient.h"
#include "ILLLocalPlayer.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPlayerInput.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

bool UILLGameBlueprintLibrary::ShouldShowTitleScreen(AILLPlayerController* PlayerController)
{
	// Check for a platform login
	if (!UILLGameOnlineBlueprintLibrary::HasPlatformLogin(PlayerController))
		return true;

	// Unless we are in user-requested offline mode...
	if (!UILLBackendBlueprintLibrary::HasUserRequestedOfflineMode())
	{
		// Check for backend authentication too
		if (!UILLBackendBlueprintLibrary::HasBackendLogin(PlayerController))
			return true;
	}

	return false;
}

bool UILLGameBlueprintLibrary::IsLoadingSaveGame(AILLPlayerController* PlayerController) // CLEANUP: pjackson: Rename this
{
	if (PlayerController)
	{
		if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer()))
		{
			return LocalPlayer->IsLoadingSettingsSaveGame();
		}
	}

	return false;
}

bool UILLGameBlueprintLibrary::IsSaveGameThreadActive(AILLPlayerController* PlayerController) // CLEANUP: pjackson: Rename this
{
	if (PlayerController)
	{
		if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer()))
		{
			return LocalPlayer->IsSettingsSaveGameThreadActive();
		}
	}

	return false;
}

void UILLGameBlueprintLibrary::MarkObjectPendingKill(UObject* Object)
{
	if (Object)
	{
		Object->MarkPendingKill();
	}
}

FString UILLGameBlueprintLibrary::TimeSecondsToTimeString(const int32 TimeSeconds, const bool bAlwaysShowMinute/* = false*/, const bool bAlwaysShowHour/* = false*/)
{
	const int Hours = TimeSeconds / 3600;
	const int Minutes = TimeSeconds / 60;
	const int Seconds = TimeSeconds % 60;

	if (Hours > 0 || bAlwaysShowHour)
	{
		return FString::Printf(TEXT("%02i:%02i:%02i"), Hours, Minutes % 60, Seconds);
	}
	else if (Minutes > 0 || bAlwaysShowHour || bAlwaysShowMinute)
	{
		return FString::Printf(TEXT("%02i:%02i"), Minutes, Seconds);
	}

	return FString::Printf(TEXT("%02i"), Seconds);
}

bool UILLGameBlueprintLibrary::IsUsingGamepad(APlayerController* PlayerController)
{
	if (PlayerController)
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PlayerController->PlayerInput))
		{
			return Input->IsUsingGamepad();
		}
	}

	return false;
}

AILLPlayerController* UILLGameBlueprintLibrary::GetFirstLocalPlayerController(UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController())
		{
			return Cast<AILLPlayerController>(LocalPlayer->PlayerController);
		}
	}

	return nullptr;
}

bool UILLGameBlueprintLibrary::IsFriendsWith(APlayerController* LocalPlayerController, APlayerState* OtherPlayerState)
{
	UWorld* World = GEngine->GetWorldFromContextObject(LocalPlayerController, EGetWorldErrorMode::LogAndReturnNull);
	if (!World || !OtherPlayerState)
		return false;

	ULocalPlayer* LocalPlayer = CastChecked<ULocalPlayer>(LocalPlayerController->Player);
	if (!LocalPlayer)
		return false;

	if (IOnlineSubsystem* OSS = Online::GetSubsystem(World))
	{
		IOnlineFriendsPtr FriendsInterface = OSS->GetFriendsInterface();
		if (FriendsInterface.IsValid())
		{
			// Only PS4 uses the friends list names, so we're just grabbing the whole list there
			return FriendsInterface->IsFriend(LocalPlayer->GetControllerId(), *OtherPlayerState->UniqueId.GetUniqueNetId().Get(), EFriendsLists::ToString(EFriendsLists::Default));
		}
	}

	return false;
}

TArray<AActor*> UILLGameBlueprintLibrary::SortActorsByDistanceTo(const TArray<AActor*>& ActorList, AActor* OtherActor)
{
	TArray<AActor*> Results = ActorList;
	Results.Sort(
		[&OtherActor](const AActor& A, const AActor& B) -> bool
		{
			return OtherActor->GetSquaredDistanceTo(&A) < OtherActor->GetSquaredDistanceTo(&B);
		}
	);
	return Results;
}

int32 UILLGameBlueprintLibrary::GetEstimatedLevelLoadPercentage()
{
#if 0
	// Determine the total number of levels and the number of those that are loaded
	int32 TotalLevels = 0;
	int32 LoadedLevels = 0;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.World())
		{
			TotalLevels += Context.World()->StreamingLevels.Num();
			for (int32 LevelIndex = 0; LevelIndex < Context.World()->StreamingLevels.Num(); ++LevelIndex)
			{
				ULevelStreaming* StreamingLevel = Context.World()->StreamingLevels[LevelIndex];
				if (StreamingLevel->GetLoadedLevel())
				{
					++LoadedLevels;
				}
			}
			/*for (auto LevelIt(Context.World()->GetLevelIterator()); LevelIt; ++LevelIt)
			{
				const ULevel* Level = *LevelIt;
				if (Level->OwningWorld) // OwningWorld is assigned in ULevel::PostLoad
				{
					++LoadedLevels;
				}
			}*/
		}
	}

	// Only display when we are loading a lot of sublevels
	if (TotalLevels >= 2 && LoadedLevels >= 0)
	{
		return FMath::RoundToInt( FMath::Clamp(float(LoadedLevels) / float(TotalLevels), 0.05f, 1.f) * 100.f );
	}
#endif
	// Display nothing
	return -1;
}

void UILLGameBlueprintLibrary::ShowLoadingWidget(UObject* WorldContextObject, TSubclassOf<UILLUserWidget> LoadingScreenWidget)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance()))
		{
			GI->ShowLoadingWidget(LoadingScreenWidget);
		}
	}
}

void UILLGameBlueprintLibrary::HideLoadingScreen(UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance()))
		{
			GI->HideLoadingScreen();
		}
	}
}

void UILLGameBlueprintLibrary::ShowLoadingAndTravel(UObject* WorldContextObject, const FString& TravelPath, bool bAbsolute/* = true*/, FString Options/* = FString(TEXT(""))*/, bool bForceClientTravel/* = false*/)
{
	UE_LOG(LogLevel, Verbose, TEXT("ShowLoadingAndTravel: %s"), *TravelPath);

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	FString Cmd = TravelPath;
	if (Options.Len() > 0)
	{
		if (!Options.StartsWith(TEXT("?")))
			Cmd += FString(TEXT("?"));
		Cmd += Options;
	}

	// Build a FURL from the Cmd
	FWorldContext& WorldContext = GEngine->GetWorldContextFromWorldChecked(World);
	const ETravelType TravelType = (bAbsolute ? TRAVEL_Absolute : TRAVEL_Relative);
	FURL TravelURL(&WorldContext.LastURL, *Cmd, TravelType);
	if (TravelURL.IsLocalInternal())
	{
		// Make sure the file exists if we are opening a local file
		if (!GEngine->MakeSureMapNameIsValid(TravelURL.Map))
		{
			UE_LOG(LogLevel, Warning, TEXT("WARNING: The map '%s' does not exist."), *TravelURL.Map);
		}
	}

	// Show the loading screen
	if (!IsRunningDedicatedServer())
	{
		if (UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance()))
		{
			GI->ShowLoadingScreen(TravelURL);
		}
	}

	// Now travel
	if (bForceClientTravel || World->GetNetMode() == NM_Standalone || World->GetNetMode() == NM_Client)
	{
		GEngine->SetClientTravel(World, *TravelURL.ToString(), TravelType);
	}
	else
	{
		// Tell all connected players to show their loading screens first
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AILLPlayerController* PC = Cast<AILLPlayerController>(*Iterator);
			if (PC && !PC->IsLocalPlayerController())
				PC->ClientShowLoadingScreen(TravelURL);
		}

		World->ServerTravel(TravelURL.ToString(), bAbsolute);
	}
}

void UILLGameBlueprintLibrary::ShowLoadingAndReturnToMainMenu(AILLPlayerController* PlayerController, FText ReturnReason)
{
	UE_LOG(LogLevel, Verbose, TEXT("ShowLoadingAndReturnToMainMenu: %s"), *ReturnReason.ToString());

	if (UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr)
	{
		check(PlayerController->IsLocalController());

		if (!IsRunningDedicatedServer())
		{
			if (UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance()))
			{
				GI->ShowLoadingScreen(FURL()); // FIXME: pjackson: ShowReturningToMainMenuLoadingScreen?
			}
		}

		// Call to ReturnToMainMenuHost when the server and ReturnReason is empty (no error)
		// This displays a better error message to clients
		if (AILLGameMode* GameMode = ReturnReason.IsEmpty() ? World->GetAuthGameMode<AILLGameMode>() : nullptr)
		{
			if (AILLGameSession* GameSession = Cast<AILLGameSession>(GameMode->GameSession))
			{
				GameSession->ReturnToMainMenuHost();
				return;
			}
		}

		// Return to the main menu
		PlayerController->ClientReturnToMainMenu(ReturnReason);
	}
}
