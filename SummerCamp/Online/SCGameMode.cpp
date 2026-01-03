// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameMode.h"

#include "AssetRegistryModule.h"
#include "BehaviorTree/BehaviorTree.h"
#include "MessageLog.h"
#include "UObjectToken.h"

#include "ILLBackendBlueprintLibrary.h"
#include "ILLGameBlueprintLibrary.h"

#if WITH_EAC
# include "IEasyAntiCheatServer.h"
#endif

#include "SCActorSpawnerComponent.h"
#include "SCBackendRequestManager.h"
#include "SCBridgeCore.h"
#include "SCBridgeNavUnit.h"
#include "SCCabin.h"
#include "SCCabinet.h"
#include "SCCabinGroupVolume.h"
#include "SCCabinSpawner.h"
#include "SCCBRadio.h"
#include "SCCharacter.h"
#include "SCContextKillDamageType.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCCounselorPlayerStart.h"
#include "SCDoubleCabinet.h"
#include "SCDriveableVehicle.h"
#include "SCEscapePod.h"
#include "SCEscapeVolume.h"
#include "SCGameBlueprintLibrary.h"
#include "SCGameInstance.h"
#include "SCGameSession.h"
#include "SCGameState.h"
#include "SCHidingSpot.h"
#include "SCHunterPlayerStart.h"
#include "SCInGameHUD.h"
#include "SCItem.h"
#include "SCKillerCharacter.h"
#include "SCKillerPlayerStart.h"
#include "SCOnlineMatchmakingClient.h"
#include "SCOnlineSessionClient.h"
#include "SCPamelaTape.h"
#include "SCPamelaTapeDataAsset.h"
#include "SCPhoneJunctionBox.h"
#include "SCPlayerController.h"
#include "SCPlayerStart.h"
#include "SCPlayerState.h"
#include "SCPoliceSpawner.h"
#include "SCRepairComponent.h"
#include "SCRepairPart.h"
#include "SCRepairPartSpawner.h"
#include "SCShoreItemSpawner.h"
#include "SCSinglePlayerDamageType.h"
#include "SCSpecialItem.h"
#include "SCSpectatorPawn.h"
#include "SCSpectatorStart.h"
#include "SCThrowable.h"
#include "SCTommyTape.h"
#include "SCTommyTapeDataAsset.h"
#include "SCVehicleSpawner.h"
#include "SCWeapon.h"
#include "SCWorldSettings.h"

#if WITH_EDITOR
# include "UnrealEd.h"
#endif

TAutoConsoleVariable<int32> CVarDebugSpectating(TEXT("sc.DebugSpectating"), 0,
	TEXT("Displays debug information for spectating.\n")
	TEXT(" 0: None\n")
	TEXT(" 1: Display debug info"));

namespace MatchState
{
	const FName WaitingPreMatch(TEXT("WaitingPreMatch"));
	const FName PreMatchIntro(TEXT("PreMatchIntro"));
	const FName WaitingPostMatchOutro(TEXT("WaitingPostMatchOutro"));
	const FName PostMatchOutro(TEXT("PostMatchOutro"));
}

ASCGameMode::ASCGameMode(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	HUDClass = ASCInGameHUD::StaticClass();
	PlayerControllerClass =	ASCPlayerController::StaticClass();
	PlayerStateClass = ASCPlayerState::StaticClass();
	SpectatorClass = ASCSpectatorPawn::StaticClass();
	DefaultPawnClass = nullptr;

	bDelayedStart = true;

	bUseSeamlessTravel = SC_SEAMLESS_TRAVEL;

	// Grab default counselor classes
	const TSoftClassPtr<ASCCounselorCharacter> AthletePawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Athlete/Athlete_Counselor.Athlete_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> PrepPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Prep/Prep_Counselor.Prep_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> BookwormPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Bookworm/Bookworm_Counselor.Bookworm_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> FlirtPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Flirt/Flirt_Counselor.Flirt_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> NerdPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Nerd/Nerd_Counselor.Nerd_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> ToughPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Tough/Tough_Counselor.Tough_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> HeroPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Hero/Hero_Counselor.Hero_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> RockerPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Rocker/Rocker_Counselor.Rocker_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> JockPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Jock/Jock_Counselor.Jock_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> HeadPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Head/Head_Counselor.Head_Counselor_C'")));

	// DLC Characters
	const TSoftClassPtr<ASCCounselorCharacter> StonerPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Stoner/Stoner_Counselor.Stoner_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> BikerPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Biker/Biker_Counselor.Biker_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> ShellyPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Shelly/Shelly_Counselor.Shelly_Counselor_C'")));
	const TSoftClassPtr<ASCCounselorCharacter> CattyPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Catty/Catty_Counselor.Catty_Counselor_C'")));

	CounselorCharacterClasses.Add(AthletePawnObject);
	CounselorCharacterClasses.Add(PrepPawnObject);
	CounselorCharacterClasses.Add(BookwormPawnObject);
	CounselorCharacterClasses.Add(FlirtPawnObject);
	CounselorCharacterClasses.Add(NerdPawnObject);
	CounselorCharacterClasses.Add(ToughPawnObject);
	CounselorCharacterClasses.Add(HeroPawnObject);
	CounselorCharacterClasses.Add(RockerPawnObject);
	CounselorCharacterClasses.Add(JockPawnObject);
	CounselorCharacterClasses.Add(HeadPawnObject);
	CounselorCharacterClasses.Add(StonerPawnObject);

	// NOTE: lpederson: These needs to stay as the last classes so the Japanese SKU can exclude them. I know. I'm sorry.
	CounselorCharacterClasses.Add(BikerPawnObject);
	CounselorCharacterClasses.Add(ShellyPawnObject);
	CounselorCharacterClasses.Add(CattyPawnObject);

	// Flirt class
	FlirtCharacterClass = FlirtPawnObject;

	AchievementTrackedCounselorClasses.Add(AthletePawnObject);
	AchievementTrackedCounselorClasses.Add(PrepPawnObject);
	AchievementTrackedCounselorClasses.Add(BookwormPawnObject);
	AchievementTrackedCounselorClasses.Add(FlirtPawnObject);
	AchievementTrackedCounselorClasses.Add(NerdPawnObject);
	AchievementTrackedCounselorClasses.Add(ToughPawnObject);
	AchievementTrackedCounselorClasses.Add(HeroPawnObject);
	AchievementTrackedCounselorClasses.Add(RockerPawnObject);
	AchievementTrackedCounselorClasses.Add(JockPawnObject);
	AchievementTrackedCounselorClasses.Add(HeadPawnObject);

	// Grab hunter character
	const TSoftClassPtr<ASCCounselorCharacter> HunterPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/Hunter/Hunter_Counselor.Hunter_Counselor_C'")));
	HunterCharacterClass = HunterPawnObject;

	// Grab android character
	const TSoftClassPtr<ASCCounselorCharacter> KMPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Counselors/KM14/KM_Counselor.KM_Counselor_C'")));
	AndroidCharacterClass = KMPawnObject;

	// Set hunter weapon class
	const TSoftClassPtr<ASCWeapon> ShotgunObject(FSoftObjectPath(TEXT("/Game/Blueprints/Items/Large/Weapons/Counselor/Ranged/Shotgun.Shotgun_C")));
	HunterWeaponClass = ShotgunObject;

	// Set baseball bat weapon class
	const TSoftClassPtr<ASCWeapon> BaseballBatObject(FSoftObjectPath(TEXT("Blueprint'/Game/Blueprints/Items/Large/Weapons/Counselor/Melee/BaseballBat.BaseballBat_C'")));
	BaseballBatClass = BaseballBatObject;

	// Set map class
	const TSoftClassPtr<ASCSpecialItem> MapObject(FSoftObjectPath(TEXT("Blueprint'/Game/Blueprints/Items/Special/Map.Map_C'")));
	MapClass = MapObject;

	// Set walkie talkie class
	const TSoftClassPtr<ASCSpecialItem> WalkieTalkieObject(FSoftObjectPath(TEXT("Blueprint'/Game/Blueprints/Items/Special/WalkieTalkie.WalkieTalkie_C'")));
	WalkieTalkieClass = WalkieTalkieObject;

	// Set Med Spray class
	const TSoftClassPtr<ASCItem> MedSprayObject(FSoftObjectPath(TEXT("Blueprint'/Game/Blueprints/Items/Small/BP_FirstAid.BP_FirstAid_C'")));
	MedSprayClass = MedSprayObject;

	// Set PocketKnife class
	const TSoftClassPtr<ASCItem> PocketKnifeObject(FSoftObjectPath(TEXT("Blueprint'/Game/Blueprints/Items/Small/BP_PocketKnife.BP_PocketKnife_C'")));
	PocketKnifeClass = PocketKnifeObject;

	// Set car keys class
	const TSoftClassPtr<ASCRepairPart> CarKeysObject(FSoftObjectPath(TEXT("Blueprint'/Game/Blueprints/Items/Small/CarParts/CarKeys.CarKeys_C'")));
	CarKeysClass = CarKeysObject;

	// Grab default killer classes
	const TSoftClassPtr<ASCKillerCharacter> SackHeadPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/J2/Jason_SackHead.Jason_SackHead_C'")));
	const TSoftClassPtr<ASCKillerCharacter> BigNeckPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/J3/Jason_BigNeck.Jason_BigNeck_C'")));
	const TSoftClassPtr<ASCKillerCharacter> J4PawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/J4/Jason_J4.Jason_J4_C'")));
	const TSoftClassPtr<ASCKillerCharacter> J5PawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/J5/Jason_J5.Jason_J5_C'")));
	const TSoftClassPtr<ASCKillerCharacter> UtilityBeltPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/J6/Jason_Utility.Jason_Utility_C'")));
	const TSoftClassPtr<ASCKillerCharacter> ZombiePawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/J7/Jason_Zombie.Jason_Zombie_C'")));
	const TSoftClassPtr<ASCKillerCharacter> ManhattanPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/J8/Jason_Manhattan.Jason_Manhattan_C'")));
	const TSoftClassPtr<ASCKillerCharacter> SwollenHeadPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/J9/Jason_SwollenHead.Jason_SwollenHead_C'")));
	const TSoftClassPtr<ASCKillerCharacter> SpacesonPawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/JX/Jason_Uber.Jason_Uber_C'")));
	const TSoftClassPtr<ASCKillerCharacter> SaviniOnePawnObject(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/JTS1/Jason_TS1.Jason_TS1_C'")));

	// Set killer classes
	KillerCharacterClasses.Add(SackHeadPawnObject);
	KillerCharacterClasses.Add(BigNeckPawnObject);
	KillerCharacterClasses.Add(J4PawnObject);
	KillerCharacterClasses.Add(J5PawnObject);
	KillerCharacterClasses.Add(UtilityBeltPawnObject);
	KillerCharacterClasses.Add(ZombiePawnObject);
	KillerCharacterClasses.Add(ManhattanPawnObject);
	KillerCharacterClasses.Add(SwollenHeadPawnObject);
	KillerCharacterClasses.Add(SpacesonPawnObject);
	KillerCharacterClasses.Add(SaviniOnePawnObject);

	// WIP Characters
	// Make sure these get moved up above once we're ready to ship them!
#if !UE_BUILD_SHIPPING

#endif

	AchievementTrackedKillerClasses.Add(SackHeadPawnObject);
	AchievementTrackedKillerClasses.Add(BigNeckPawnObject);
	AchievementTrackedKillerClasses.Add(UtilityBeltPawnObject);
	AchievementTrackedKillerClasses.Add(ZombiePawnObject);
	AchievementTrackedKillerClasses.Add(ManhattanPawnObject);
	AchievementTrackedKillerClasses.Add(SwollenHeadPawnObject);

	// Now add all characters to CharacterClasses
	CharacterClasses.Append(CounselorCharacterClasses);
	CharacterClasses.Append(KillerCharacterClasses);
	CharacterClasses.Add(HunterCharacterClass);
	CharacterClasses.Add(AndroidCharacterClass);

	// Set master kill list
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Need to do this if running in the editor with -game to make sure that the assets in the following path are available
	AssetRegistry.ScanPathsSynchronous({TEXT("/Game/Blueprints/ContextKills/KillInfo")});

	// Get all the assets in the folder
	TArray<FAssetData> KillInfoAssetList;
	AssetRegistry.GetAssetsByPath(FName("/Game/Blueprints/ContextKills/KillInfo"), KillInfoAssetList);

	// Loop through all the assets, get their default object, and add them to the master kill list
	for (const FAssetData& Kill : KillInfoAssetList)
	{
		FString KillInfoClass = FString::Printf(TEXT("%s_C"), *Kill.ObjectPath.ToString());
		const ConstructorHelpers::FClassFinder<USCContextKillDamageType> KillData(*KillInfoClass);
		if (KillData.Class)
			MasterKillList.Add(KillData.Class);
	}	

	// Set Score Events
	static ConstructorHelpers::FClassFinder<USCScoringEvent> CounselorTimeBonusObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/CounselorTimeBonus.CounselorTimeBonus_C'"));
	CounselorAliveTimeBonusScoreEvent = CounselorTimeBonusObject.Class;
	static ConstructorHelpers::FClassFinder<USCScoringEvent> JasonKilledAllObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/KillAllCounselors.KillAllCounselors_C'"));
	JasonKilledAllCounselorsScoreEvent = JasonKilledAllObject.Class;
	static ConstructorHelpers::FClassFinder<USCScoringEvent> CounselorEscapedObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/Escape.Escape_C'"));
	CounselorEscapedScoreEvent = CounselorEscapedObject.Class;
	static ConstructorHelpers::FClassFinder<USCScoringEvent> TeamEscapedObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/TeamEscaped.TeamEscaped_C'"));
	TeamEscapedScoreEvent = TeamEscapedObject.Class;
	static ConstructorHelpers::FClassFinder<USCScoringEvent> FriendlyFireObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/FriendlyFire.FriendlyFire_C'"));
	FriendlyFireScoreEvent = FriendlyFireObject.Class;
	static ConstructorHelpers::FClassFinder<USCScoringEvent> KillJasonObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/KillJason.KillJason_C'"));
	KillJasonScoreEvent = KillJasonObject.Class;
	static ConstructorHelpers::FClassFinder<USCScoringEvent> KillCounselorObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/KillCounselor.KillCounselor_C'"));
	KillCounselorScoreEvent = KillCounselorObject.Class;
	static ConstructorHelpers::FClassFinder<USCScoringEvent> JasonWeaponKillrObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/Butcher.Butcher_C'"));
	JasonWeaponKillScoreEvent = JasonWeaponKillrObject.Class;
	static ConstructorHelpers::FClassFinder<USCScoringEvent> UnscathedObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/Unscathed.Unscathed_C'"));
	UnscathedScoreEvent = UnscathedObject.Class;
	static ConstructorHelpers::FClassFinder<USCScoringEvent> CompletedMatchObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/CompletedMatch.CompletedMatch_C'"));
	CompletedMatchScoreEvent = CompletedMatchObject.Class;

	// Set Badges
	static ConstructorHelpers::FClassFinder<USCStatBadge> PlayAsTommyJarvisBadgeObject(TEXT("Blueprint'/Game/Blueprints/Badges/SecondWind.SecondWind_C'"));
	TommyJarvisBadge = PlayAsTommyJarvisBadgeObject.Class;
	static ConstructorHelpers::FClassFinder<USCStatBadge> EscapeWithPoliceBadgeObject(TEXT("Blueprint'/Game/Blueprints/Badges/Reinforcements.Reinforcements_C'"));
	EscapeWithPoliceBadge = EscapeWithPoliceBadgeObject.Class;
	static ConstructorHelpers::FClassFinder<USCStatBadge> SurviveBadgeObject(TEXT("Blueprint'/Game/Blueprints/Badges/Ninja.Ninja_C'"));
	SurviveBadge = SurviveBadgeObject.Class;
	static ConstructorHelpers::FClassFinder<USCStatBadge> SurviveWithFearBadgeObject(TEXT("Blueprint'/Game/Blueprints/Badges/ChadFace.ChadFace_C'"));
	SurviveWithFearBadge = SurviveWithFearBadgeObject.Class;

	// AI
	static ConstructorHelpers::FObjectFinder<UBehaviorTree> CounselorBehaviorTreeObject(TEXT("BehaviorTree'/Game/Blueprints/AI/Counselor/OfflineBotsCounselorBehaviorTree.OfflineBotsCounselorBehaviorTree'"));
	CounselorBehaviorTree = CounselorBehaviorTreeObject.Object;

	MinimumPlayerCount = 1;
	RoundTime = -1; // Default to no timelimit
	PostMatchOutroWaitTime = 10;
	PostMatchWaitTime = 20;
	PostMatchWaitPendingReplacementTime = 10; // Compensate for the 5+s it can take to find a replacement
}

void ASCGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_HunterTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCGameMode::SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC)
{
#if WITH_EAC
	// Manually un-register the OldPC from EasyAntiCheat and register the NewPC
	// @see USCGameInstance::Init
	if (Role == ROLE_Authority && IEasyAntiCheatServer::IsAvailable())
	{
		IEasyAntiCheatServer::Get().OnClientDisconnect(OldPC);
		IEasyAntiCheatServer::Get().OnClientConnect(NewPC);
	}
#endif

	Super::SwapPlayerControllers(OldPC, NewPC);
}

void ASCGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

#if WITH_EAC
	// Manually register with EasyAntiCheat, because the plugin does not and can not handle seamless travel correctly
	// @see USCGameInstance::Init
	if (Role == ROLE_Authority && IEasyAntiCheatServer::IsAvailable())
	{
		IEasyAntiCheatServer::Get().OnClientConnect(NewPlayer);
	}
#endif
}

void ASCGameMode::Logout(AController* Exiting)
{
#if WITH_EAC
	// Manually un-register the OldPC from EasyAntiCheat
	// @see USCGameInstance::Init
	if (Role == ROLE_Authority && IEasyAntiCheatServer::IsAvailable())
	{
		if (APlayerController* PC = Cast<APlayerController>(Exiting))
		{
			IEasyAntiCheatServer::Get().OnClientDisconnect(PC);
		}
	}
#endif

	Super::Logout(Exiting);
}

bool ASCGameMode::HasMatchStarted() const
{
	if (!Super::HasMatchStarted())
		return false;

	return (GetMatchState() != MatchState::WaitingPreMatch && GetMatchState() != MatchState::PreMatchIntro);
}

bool ASCGameMode::HasMatchEnded() const
{
	if (GetMatchState() == MatchState::PostMatchOutro)
		return true;

	return Super::HasMatchEnded();
}

void ASCGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (ErrorMessage.IsEmpty())
	{
		FString DifficultyString;
		if (FParse::Value(*Options, TEXT("Difficulty="), DifficultyString))
		{
			if (DifficultyString == TEXT("Normal") || DifficultyString == TEXT("1"))
				ModeDifficulty = ESCGameModeDifficulty::Normal;
			else if (DifficultyString == TEXT("Easy") || DifficultyString == TEXT("0"))
				ModeDifficulty = ESCGameModeDifficulty::Easy;
			else if (DifficultyString == TEXT("Hard") || DifficultyString == TEXT("2"))
				ModeDifficulty = ESCGameModeDifficulty::Hard;
			else
				UE_LOG(LogSC, Warning, TEXT("Unrecognized game mode difficulty: %s"), *DifficultyString);
		}
	}
}

UClass* ASCGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (ASCPlayerState* PS = Cast<ASCPlayerState>(InController->PlayerState))
	{
		if (PS->CanSpawnActiveCharacter())
		{
			return PS->GetActiveCharacterClass();
		}
		else if (PS->GetSpectating())
		{
			return SpectatorClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ASCGameMode::StartPlay()
{
	Super::StartPlay();

	Cast<ASCWorldSettings>(GetWorldSettings())->LoadRandomLevel();
}

void ASCGameMode::EndMatch()
{
	if (IsMatchInProgress())
	{
		// Notify remote clients that their HUD needs to do a thing
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
			{
				if (ASCPlayerState* PS = Cast<ASCPlayerState>(PC->PlayerState))
				{
					PC->CLIENT_PlayHUDEndMatchOutro(PS->GetIsDead(), PS->GetActiveCharacterClass()->IsChildOf<ASCKillerCharacter>(), !PS->DidSpawnIntoMatch());
				}
			}
		}

		// Start the wait before the post-match outro
		// After it is done playing this function will be hit again for below
		//
		// NOTE: Order of operations is important here; setting the matchstate also changes what hud widget is currently being used. 
		// (i.e. OnRepMatchState will swap the CounselorHUD to the SpectatorHUD, which doesn't have certain UI cleanup functionality).
		SetMatchState(MatchState::WaitingPostMatchOutro);
	}
	else if (GetMatchState() == MatchState::WaitingPostMatchOutro)
	{
		// Start the post-match outro
		SetMatchState(MatchState::PostMatchOutro);
	}
	else if (GetMatchState() == MatchState::PostMatchOutro)
	{
		SetMatchState(MatchState::WaitingPostMatch);
	}
}

bool ASCGameMode::AllowPausing(APlayerController* PC/* = nullptr*/)
{
	// We only allow pausing once the match has started
	// AILLGameMode will catch the match state change and pause it when the match starts
	return IsMatchInProgress() && Super::AllowPausing(PC);
}

void ASCGameMode::LoadRandomItems()
{
	const ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());
	UWorld* World = GetWorld();

#if WITH_EDITOR
	// NOTE: Not using PIE since it gets cleared after StartPlay finishes (resulting in no messages being visible)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(FPackageName::GetShortName(GWorld->GetOutermost())));
		Arguments.Add(TEXT("TimeStamp"), FText::AsDateTime(FDateTime::Now()));
		FText PageName(FText::Format(NSLOCTEXT("SC.Spawning", "SpawningInfo.PageName", "{Name} - {TimeStamp}"), Arguments));
		FMessageLog("Spawning").NewPage(PageName);
	}

	if (WorldSettings->RandomLevels.Num() > 0 && WorldSettings->RandomLevels.IsValidIndex(WorldSettings->RandomLevelIndex))
		FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("Using sublevel %s"), *WorldSettings->RandomLevels[WorldSettings->RandomLevelIndex].ToString())));
	else
		FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("No random sublevel"))));

	const double SpawnStartTime = FPlatformTime::Seconds();
#endif

	// Grab Shore Spawners
	for (TActorIterator<ASCShoreItemSpawner> It(World, ASCShoreItemSpawner::StaticClass()); It; ++It)
	{
		ShoreLocations.Add(*It);
	}

	// Keep track of all escape volumes
	EscapeVolumes.Empty();
	for (TActorIterator<ASCEscapeVolume> It(World, ASCEscapeVolume::StaticClass()); It; ++It)
	{
		EscapeVolumes.Add(*It);
	}

	// Escape Pods
	for (TActorIterator<ASCEscapePod> It(World, ASCEscapePod::StaticClass()); It; ++It)
	{
		EscapePods.Add(*It);
	}

	// Bridge Cores
	for (TActorIterator<ASCBridgeCore> It(World, ASCBridgeCore::StaticClass()); It; ++It)
	{
		BridgeCores.Add(*It);
	}

	// Bridge Nav Units
	for (TActorIterator<ASCBridgeNavUnit> It(World, ASCBridgeNavUnit::StaticClass()); It; ++It)
	{
		BridgeNavUnits.Add(*It);
	}

#if WITH_EDITOR
	FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("%d shore spawners"), ShoreLocations.Num())));
#endif

	// have to spawn the cabins before we grab all of the hiding spots and cabinets
	SpawnCabins();

	SpawnVehicles();

	// Keep track of all hiding spots in the level
	AllHidingSpots.Empty();
	for (TActorIterator<ASCHidingSpot> It(World, ASCHidingSpot::StaticClass()); It; ++It)
	{
		if (IsValid(*It) == false)
			continue;

		AllHidingSpots.Add(*It);
	}

	// Compile an array of cabinets in the level for random access
	Cabinets.Empty();
	int32 DoubleCabinets = 0;
	for (TActorIterator<ASCCabinet> It(World, ASCCabinet::StaticClass()); It; ++It)
	{
		if (IsValid(*It) == false)
			continue;

		Cabinets.Add(*It);

		if ((*It)->IsA(ASCDoubleCabinet::StaticClass()))
			++DoubleCabinets;
	}

#if WITH_EDITOR
	FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("%d cabinet spawners"), Cabinets.Num() + DoubleCabinets)));
#endif

	SpawnRepairItems();

	ActorSpawners.Empty();
	for (TObjectIterator<USCActorSpawnerComponent> It; It; ++It)
	{
		USCActorSpawnerComponent* Spawner = *It;
		if (IsValid(Spawner) == false)
			continue;

		if (World == Spawner->GetWorld() && Spawner->GetSpawnedActor() == nullptr && !Spawner->GetOwner()->IsA<ASCCabinet>())
			ActorSpawners.Add(*It);
	}

#if WITH_EDITOR
	FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("%d actor spawners"), ActorSpawners.Num())));
#endif

	// Turn on to track down double item spawns
#if 0
	{
		const int32 SpawnerCount = ActorSpawners.Num();
		for (int32 iSpawner = 0; iSpawner < SpawnerCount - 1; ++iSpawner)
		{
			const FVector iSpawnerLocation = ActorSpawners[iSpawner]->GetComponentLocation();

			for (int32 jSpawner = iSpawner + 1; jSpawner < SpawnerCount; ++jSpawner)
			{
				const FVector jSpawnerLocation = ActorSpawners[jSpawner]->GetComponentLocation();

				if (FVector::DistSquared(iSpawnerLocation, jSpawnerLocation) <= FMath::Square(10.f))
				{
					FString ErrorMsg = FString::Printf(TEXT("Actor spawner %s of %s is VERY close (%.1f cm) to actor spawner %s of %s"),
						*ActorSpawners[iSpawner]->GetName(), *ActorSpawners[iSpawner]->GetOwner()->GetName(), FVector::Dist(iSpawnerLocation, jSpawnerLocation),
						*ActorSpawners[jSpawner]->GetName(), *ActorSpawners[jSpawner]->GetOwner()->GetName());
# if WITH_EDITOR
					FMessageLog("Spawning").Error(FText::FromString(ErrorMsg));
# else
					UE_LOG(LogTemp, Error, ErrorMsg);
# endif
				}
			}
		}
	}
#endif

	// Check spawning override and let actor spawners use the random spawning logic
	if (WorldSettings->bOverrideSpawning)
	{
#if WITH_EDITOR
		FMessageLog("Spawning").Info(FText::FromString(TEXT("Spawns overridden, using all spawn points")));
#endif
		// Have to handle cabinets separately so that the items actually get added to them
		for (ASCCabinet* Cabinet : Cabinets)
		{
			Cabinet->SpawnItem();
			Cabinet->SpawnItem(); // call it twice just in case its a double cabinet
		}

		// Handle the rest of the actor spawners
		for (const auto& Spawner : ActorSpawners)
		{
			Spawner->SpawnActor();
		}
	}
	else
	{
		SpawnLargeItems();

		SpawnSmallItems();

		AttemptSpawnPamelaTapes();
		AttemptSpawnTommyTapes();

#if WITH_EDITOR
		FMessageLog("Spawning").Info(FText::FromString(TEXT("-- Spawning Finished --")));
		FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("Items spawned in %.3lf milliseconds"), (FPlatformTime::Seconds() - SpawnStartTime) * 1000.0)));

		for (auto& KeyValue : SpawnerMap)
		{
			for (int32 iSpawner(0); iSpawner < KeyValue.Value.Num(); ++iSpawner)
			{
				if (KeyValue.Value[iSpawner]->GetSpawnedActor())
				{
					KeyValue.Value.RemoveAt(iSpawner--, 1, false);
				}
			}
		}

		for (const auto& KeyValue : SpawnerMap)
		{
			if (KeyValue.Value.Num() > 0)
				FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("%d spawns left for %s"), KeyValue.Value.Num(), *KeyValue.Key->GetName())));
		}

		for (auto &KeyValue : CabinetMap)
		{
			for (int32 iCabinet(0); iCabinet < KeyValue.Value.Num(); ++iCabinet)
			{
				if (KeyValue.Value[iCabinet]->IsCabinetFull())
				{
					KeyValue.Value.RemoveAt(iCabinet--, 1, false);
				}
			}
		}

		for (const auto& KeyValue : CabinetMap)
		{
			if (KeyValue.Value.Num() > 0)
				FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("%d cabinets left for %s"), KeyValue.Value.Num(), *KeyValue.Key->GetName())));
		}
#endif
		// Just in case stuff gets deleted out from under us
		// Although this shouldn't be accessed past this point
		SpawnerMap.Empty();
		CabinetMap.Empty();
	}

	// Keep track of all CB radios in the level
	SpawnedCBRadios.Empty();
	for (TActorIterator<ASCCBRadio> It(World, ASCCBRadio::StaticClass()); It; ++It)
	{
		if (IsValid(*It) == false)
			continue;

		SpawnedCBRadios.Add(*It);
	}

	// Now that we're done spawning, force the GC to run to place the hitch at startup
	GEngine->PerformGarbageCollectionAndCleanupActors();

#if WITH_EDITOR
	FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("Items spawned and GC run in %.3lf milliseconds"), (FPlatformTime::Seconds() - SpawnStartTime) * 1000.0)));
#endif
}

void ASCGameMode::InitStartSpot_Implementation(AActor* StartSpot, AController* NewPlayer)
{
	Super::InitStartSpot_Implementation(StartSpot, NewPlayer);

	// Store off the StartSpot used to spawn at last
	// This is used for the spawn intro sequence then set to NULL
	NewPlayer->StartSpot = StartSpot;
}

bool ASCGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

AActor* ASCGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Choose a player start
	AActor* FoundPlayerStart = nullptr;
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;

	ASCPlayerState* PlayerState = Cast<ASCPlayerState>(Player->PlayerState);
	const bool bWillBeKiller = PlayerState->IsActiveCharacterKiller();

	TArray<ASCPlayerStart*> UnOccupiedStartPoints;
	TArray<ASCPlayerStart*> OccupiedStartPoints;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if ((*It)->IsA<APlayerStartPIE>() && Player->IsPlayerController())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			FoundPlayerStart = *It;
			break;
		}

		if (ASCPlayerStart* PlayerStart = Cast<ASCPlayerStart>(*It))
		{
			if (!PawnToFit || PawnToFit->IsA(ASCSpectatorPawn::StaticClass()))
			{
				// Accept the first SCSpectatorStart found when there is no PawnToFit, until these are placed in all of the maps
				if (PlayerStart->IsA(ASCSpectatorStart::StaticClass()))
				{
					FoundPlayerStart = PlayerStart;
					break;
				}

				// Do not use hunter starts for spectators
				if (PlayerStart->IsA(ASCHunterPlayerStart::StaticClass()))
					continue;

				// Do not use kuller starts for spectators either
				if (PlayerStart->IsA(ASCKillerPlayerStart::StaticClass()))
					continue;

				// No Pawn to fit, always accept
				UnOccupiedStartPoints.Add(PlayerStart);
			}
			else
			{
				// Skip spawns that have some blocker in the way
				if (PlayerStart->IsEncroached(PawnToFit))
					continue;

				if (bWillBeKiller)
				{
					if (!Cast<ASCKillerPlayerStart>(PlayerStart))
						continue;
				}
				else if (Cast<ASCCounselorPlayerStart>(PlayerStart))
				{
					// Do not use hunter starts for counselors
					if (Cast<ASCHunterPlayerStart>(PlayerStart))
						continue;
				}
				else
				{
					continue;
				}

				FVector ActorLocation = PlayerStart->GetActorLocation();
				const FRotator ActorRotation = PlayerStart->GetActorRotation();
				if (!GetWorld()->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation))
				{
					UnOccupiedStartPoints.Add(PlayerStart);
				}
				else if (GetWorld()->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
				{
					OccupiedStartPoints.Add(PlayerStart);
				}
			}
		}
	}
	if (FoundPlayerStart == nullptr)
	{
		if (UnOccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
		}
		else if (OccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
		}
	}
	return FoundPlayerStart;
}

bool ASCGameMode::CanSpectate_Implementation(APlayerController* Viewer, APlayerState* ViewTarget)
{
	bool bCanSpectatePlayer = false;
	if (Super::CanSpectate_Implementation(Viewer, ViewTarget))
	{
		if (ASCPlayerState* TargetPS = Cast<ASCPlayerState>(ViewTarget))
		{
			if (!TargetPS->IsActiveCharacterKiller() && !TargetPS->GetSpectating())
			{
				bCanSpectatePlayer = true;

				// Check to make sure the ViewTarget isn't escaping on a spline
				if (ASCGameState* GS = Cast<ASCGameState>(GameState))
				{
					for (ASCCharacter* Player : GS->PlayerCharacterList)
					{
						ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Player);
						if (Counselor && Counselor->PlayerState == ViewTarget)
						{
							bCanSpectatePlayer = !Counselor->IsEscaping();
						}
					}
				}
			}

			if (CVarDebugSpectating.GetValueOnGameThread() > 0)
			{
				if (bCanSpectatePlayer)
				{
					GEngine->AddOnScreenDebugMessage((uint64)-1, 5.f, FColor::Green, FString::Printf(TEXT("Can spectate: %s"), *TargetPS->PlayerName));

				}
				else
				{
					GEngine->AddOnScreenDebugMessage((uint64)-1, 5.f, FColor::Red, FString::Printf(TEXT("Can't spectate: %s Killer: %s Spectating: %s"), *TargetPS->PlayerName, TargetPS->IsActiveCharacterKiller() ? TEXT("true") : TEXT("false"), TargetPS->GetSpectating() ? TEXT("true") : TEXT("false")));
				}
			}
		}
	}

	return bCanSpectatePlayer;
}

void ASCGameMode::OnMatchStateSet()
{
	if (MatchState == MatchState::WaitingPreMatch)
	{
		HandleWaitingPreMatch();
	}
	else if (MatchState == MatchState::PreMatchIntro)
	{
		HandlePreMatchIntro();
	}
	else if (MatchState == MatchState::WaitingPostMatchOutro)
	{
		HandleWaitingPostMatchOutro();
	}
	else if (MatchState == MatchState::PostMatchOutro)
	{
		HandlePostMatchOutro();
	}

	Super::OnMatchStateSet();
}

void ASCGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	// Set a time to wait after everyone loads in
	ASCGameState* GS = GetGameState<ASCGameState>();
	GS->RemainingTime = GetHoldWaitingForPlayersTime();

	// Start next tick if possible
	if (GS->RemainingTime <= 0)
	{
		GS->RemainingTime = 1;
		ForceGameSecondTimerNextTick();
	}
}

void ASCGameMode::HandleMatchHasStarted()
{
	ASCGameState* GS = GetGameState<ASCGameState>();

	// Make sure all counselor classes are loaded before calling super so that we have our classes when attempting to spawn.
	for (int32 PlayerIndex = 0; PlayerIndex < GS->PlayerArray.Num(); ++PlayerIndex)
	{
		ASCPlayerState* PS = Cast<ASCPlayerState>(GS->PlayerArray[PlayerIndex]);
		if (!PS->IsPickedKiller())
		{
			TSoftClassPtr<ASCCounselorCharacter> RequestedCounselor = PS->GetPickedCounselorClass();
			if (!RequestedCounselor.IsNull())
				RequestedCounselor.LoadSynchronous();
		}
	}

	Super::HandleMatchHasStarted();

	// Set our match time limit
	GS->RemainingTime = RoundTime;

	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
		{
			// Check if players should start their spawn intro
			// As far as I can tell all clients should play it when they ack their Pawn/PlayerState, so this is really typically for listen hosts
			PC->TryStartSpawnIntroSequence();

			// Bump their idle timer to give a match start grace period
			PC->BumpIdleTime();
		}
	}
}

void ASCGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	// Wait a little bit before traveling back to the lobby
	ASCGameState* GS = GetGameState<ASCGameState>();
	GS->RemainingTime = GetHoldWaitingPostMatchTime();
}

void ASCGameMode::HandleLeavingMap()
{
	Super::HandleLeavingMap();

	// Clear the ActiveCharacterClass on everyone so that it doesn't carry across levels
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		if (*It == nullptr)
			continue;

		if (ASCPlayerState* PS = Cast<ASCPlayerState>((*It)->PlayerState))
		{
			PS->SetActiveCharacterClass(nullptr);
		}
	}
}

TSubclassOf<AGameSession> ASCGameMode::GetGameSessionClass() const
{
	return ASCGameSession::StaticClass();
}

void ASCGameMode::GameSecondTimer()
{
	Super::GameSecondTimer();

	ASCGameState* GS = Cast<ASCGameState>(GameState);

	// If the game world is paused we don't want to update timers.
	if (GetWorld()->IsPaused())
		return;

	if (MatchState == MatchState::WaitingToStart)
	{
#if WITH_EDITOR
		if (GEditor && GEditor->bIsSimulatingInEditor)
		{
			SetMatchState(MatchState::PreMatchIntro);
		}
		else
#endif
		{
			if (NumPlayers >= MinimumPlayerCount && HasEveryPlayerLoadedIn())
			{
#if WITH_EDITOR
				if (GetWorld()->IsPlayInEditor() && CVarEmulateMatchStartInPIE.GetValueOnGameThread() == 0)
				{
					// Skip directly to match start by default in PIE
					SetMatchState(MatchState::PreMatchIntro);

					// Force a game session update for IsOpenToMatchmaking
					QueueGameSessionUpdate();
				}
				else
#endif
				{
					// Hold for a brief period of time after everybody has loaded
					// Wait for all profiles to be received before entering the PreMatchIntro like below
					if (GS->RemainingTime > 0 && (PreMatchWaitTime > 0 || CanCompleteWaitingPreMatch()))
					{
						--GS->RemainingTime;
						if (GS->RemainingTime == 0)
						{
							if (PreMatchWaitTime > 0)
							{
								SetMatchState(MatchState::WaitingPreMatch);
							}
							else
							{
								// Skip straight to the pre-match intro
								SetMatchState(MatchState::PreMatchIntro);
							}

							// Force a game session update for IsOpenToMatchmaking
							QueueGameSessionUpdate();
						}
					}
				}
			}
			else
			{
				// Ensure this resets if someone starts joining immediately after everyone finishes, or if we drop below MinimumPlayerCount
				GS->RemainingTime = GetHoldWaitingForPlayersTime();
			}
		}
	}
	else if (MatchState == MatchState::PreMatchIntro)
	{
	}
	else if (MatchState == MatchState::PostMatchOutro)
	{
	}
	else
	{
		// Tick down RemainingTime
		if (GS->RemainingTime > 0)
		{
			--GS->RemainingTime;
		}

		if (MatchState == MatchState::WaitingPreMatch)
		{
			// Wait period before the match intro begins
			TickWaitingPreMatch();
			if (GS->RemainingTime == 0 && CanCompleteWaitingPreMatch())
			{
				SetMatchState(MatchState::PreMatchIntro);
			}
		}
		else if (MatchState == MatchState::InProgress)
		{
			++GS->ElapsedInProgressTime;

			if (GS->RemainingTime == 0)
			{
				EndMatch();
			}
			else
			{
				// Kick idlers
				UWorld* World = GetWorld();
				for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
				{
					if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
					{
						PC->IdleCheck();
					}
				}

				// update hunter timer
				if (GetWorldTimerManager().IsTimerActive(TimerHandle_HunterTimer))
				{
					GS->HunterTimeRemaining = GetWorldTimerManager().GetTimerRemaining(TimerHandle_HunterTimer);
				}

				if (Role == ROLE_Authority)
				{
					// Update replicated timers
					if (GS->PoliceTimeRemaining > 0)
					{
						--GS->PoliceTimeRemaining;
						GS->OnRep_PoliceTimeRemaining();

						if (GS->PoliceTimeRemaining <= 0)
							SpawnPolice();
					}
				}
			}
		}
		else if (MatchState == MatchState::WaitingPostMatchOutro)
		{
			if (GS->RemainingTime == 0)
			{
				// This will start the post-match outro
				EndMatch();
			}
		}
		else if (MatchState == MatchState::WaitingPostMatch)
		{
			if (GS->RemainingTime == 0)
			{
				RestartGame();
			}
		}
	}
}

bool ASCGameMode::IsOpenToMatchmaking() const
{
	if (Super::IsOpenToMatchmaking() && MatchState == MatchState::WaitingToStart)
	{
		if (USCOnlineSessionClient* OSC = Cast<USCOnlineSessionClient>(GetGameInstance()->GetOnlineSession()))
		{
			// Wait for our minimum number of players to be loaded in
			if (OSC->GetNumAcceptedLobbyMembers() < MinimumPlayerCount || !HasEveryPlayerLoadedIn())
				return true;
		}
	}

	return false;
}

bool ASCGameMode::HasReceivedAllPlayerProfiles() const
{
	UWorld* World = GetWorld();

#if WITH_EDITOR
	// Always skip this in PIE
	if (World->IsPlayInEditor())
		return true;
#endif

	if (!IsRunningDedicatedServer())
	{
		// Skip checks when local player is in offline mode
		AILLPlayerController* FirstLocalPC = UILLGameBlueprintLibrary::GetFirstLocalPlayerController(World);
		if (FirstLocalPC && UILLBackendBlueprintLibrary::IsInOfflineMode(FirstLocalPC))
			return true;
	}

	// Check all player states
	if (ASCGameState* GS = CastChecked<ASCGameState>(GameState))
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(PS))
			{
				if (!PlayerState->HasReceivedProfile())
					return false;
			}
		}
	}

	return true;
}

float ASCGameMode::GetCurrentDifficultyDownwardAlpha(const float Deviation/* = .5f*/, const float Base/* = 1.f*/) const
{
	switch (ModeDifficulty)
	{
	case ESCGameModeDifficulty::Easy: return Base + Deviation;
	case ESCGameModeDifficulty::Hard: return Base - Deviation;
	case ESCGameModeDifficulty::Normal: return Base;
	default:
		check(0);
		break;
	}

	return Base;
}

float ASCGameMode::GetCurrentDifficultyUpwardAlpha(const float Deviation/* = .5f*/, const float Base/* = 1.f*/) const
{
	switch (ModeDifficulty)
	{
	case ESCGameModeDifficulty::Easy: return Base - Deviation;
	case ESCGameModeDifficulty::Hard: return Base + Deviation;
	case ESCGameModeDifficulty::Normal: return Base;
	default:
		check(0);
		break;
	}

	return Base;
}

void ASCGameMode::OnPostMatchOutroComplete()
{
	// Call to EndMatch, which should kick us into MatchState::WaitingPostMatch
	EndMatch();
}

void ASCGameMode::HandleWaitingPreMatch()
{
	ASCGameState* GS = GetGameState<ASCGameState>();

	// Set our WaitingPreMatch delay
	GS->RemainingTime = PreMatchWaitTime;

	// Force a game session update
	QueueGameSessionUpdate();

	// Match setup polling
	TickWaitingPreMatch();
}

void ASCGameMode::HandlePreMatchIntro()
{
	ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());

	// We need to begin play now so that sequence actors can work. @see ASCGameState::OnRep_MatchState
	WorldSettings->NotifyBeginPlay();

	// Now start the level intro
	WorldSettings->StartLevelIntro(bSkipLevelIntro);

	// Force a game session update
	QueueGameSessionUpdate();
}

void ASCGameMode::HandleWaitingPostMatchOutro()
{
	// Wait a little bit before starting the outro
	ASCGameState* GS = GetGameState<ASCGameState>();
	GS->RemainingTime = PostMatchOutroWaitTime;
}

void ASCGameMode::HandlePostMatchOutro()
{
	// Start the level outro
	ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());
	WorldSettings->StartLevelOutro();
}

int32 ASCGameMode::GetHoldWaitingForPlayersTime() const
{
	// When a private match or we are full, use the shorter time
	ASCGameSession* GS = CastChecked<ASCGameSession>(GameSession);
	if (!GS->IsQuickPlayMatch() || GS->AtCapacity(false))
		return HoldWaitingForPlayersTime;

#if WITH_EDITOR
	// Make PIE a little less suck
	if (GetWorld() && GetWorld()->IsPlayInEditor())
		return HoldWaitingForPlayersTime;
#endif

	// Wait longer for more people
	return HoldWaitingForPlayersTimeNonFullQuickPlay;
}

int32 ASCGameMode::GetHoldWaitingPostMatchTime() const
{
	USCGameInstance* GameInstance = CastChecked<USCGameInstance>(GetGameInstance());
	if (GameInstance->IsPendingRecycling())
	{
		USCOnlineMatchmakingClient* OMC = CastChecked<USCOnlineMatchmakingClient>(GameInstance->GetOnlineMatchmaking());
		if (OMC->SupportsServerReplacement())
			return PostMatchWaitPendingReplacementTime;
	}

	return PostMatchWaitTime;
}

void ASCGameMode::FinishMatch()
{
#if !UE_BUILD_SHIPPING
	EndMatch();
#endif
}

bool ASCGameMode::GetShoreLocation(const FVector& PlayerLocation, ASCItem* Item, FTransform& OutTransform)
{
	if (ShoreLocations.Num() == 0)
	{
#if WITH_EDITOR
		FMessageLog("PIE").Error(FText::FromString(TEXT("Not enough shore item spawner locations to drop all items!")));
#endif
		UE_LOG(LogSC, Warning, TEXT("No shore item spawners were placed on the map, %s will be in a bad location!"), *GetNameSafe(Item));

		return false;
	}

	// Find our best location
	float ShortestDistanceSq = FLT_MAX;
	int32 BestLocationIndex = INDEX_NONE;
	const int32 ShoreLocationCount = ShoreLocations.Num();
	for (int32 iShoreLocation(0); iShoreLocation < ShoreLocationCount; ++iShoreLocation)
	{
		// Don't use shore locations already in use
		if (ShoreLocations[iShoreLocation]->bIsInUse)
			continue;

		const float DistanceSq = FVector::DistSquaredXY(PlayerLocation, ShoreLocations[iShoreLocation]->GetActorLocation());
		if (DistanceSq < ShortestDistanceSq)
		{
			BestLocationIndex = iShoreLocation;
			ShortestDistanceSq = DistanceSq;
		}
	}

	if (BestLocationIndex == INDEX_NONE)
	{
#if WITH_EDITOR
		FMessageLog("PIE").Error(FText::FromString(TEXT("Not enough shore item spawner locations to drop all items!")));
#endif
		UE_LOG(LogSC, Warning, TEXT("Not enough shore item spawner locations to drop all items, %s will be in a bad location!"), *GetNameSafe(Item));

		return false;
	}

	// We found a best location! Use it and mark it as used
	ASCShoreItemSpawner* BestShoreSpawner = ShoreLocations[BestLocationIndex];
	OutTransform = BestShoreSpawner->GetActorTransform();
	Item->CurrentShoreSpawner = BestShoreSpawner;
	BestShoreSpawner->bIsInUse = true;

	// Snap to ground if we can
	if (UWorld* World = GetWorld())
	{
		static const FName NAME_ShoreTrace(TEXT("ShoreTrace"));
		FCollisionQueryParams Params(NAME_ShoreTrace);

		FVector StartLocation = OutTransform.GetTranslation() + FVector::UpVector * 250.f;
		const FVector EndLocation = OutTransform.GetTranslation() + FVector::UpVector * -250.f;

		int32 Iterations = 0;
		FHitResult Hit;
		while (World->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECollisionChannel::ECC_WorldDynamic, Params))
		{
			// Don't loop for forever
			if (Iterations++ >= 16)
				break;

			if (Hit.Actor.IsValid())
			{
				// Don't collide with characters or items
				AActor* HitActor = Hit.Actor.Get();
				if (HitActor->IsA(ACharacter::StaticClass()) || HitActor->IsA(ASCItem::StaticClass()))
				{
					Params.AddIgnoredActor(HitActor);
					StartLocation = Hit.ImpactPoint;
					continue;
				}
			}

			// Location found!
			const float ZOffset = Item->GetMesh()->Bounds.BoxExtent.Z * 0.425f; // CLEANUP: Magic number is same scalar we use for dropping items (see DROP_EXTENT_SCALAR)
			OutTransform.SetTranslation(Hit.Location + FVector::UpVector * ZOffset);
			break;
		}
	}

	return true;
}

void ASCGameMode::ContextKillScored(AController* Killer, AController* KilledPlayer, const UDamageType* const DamageType)
{
	ASCPlayerState* KillerPlayerState = Killer ? Cast<ASCPlayerState>(Killer->PlayerState) : nullptr;
	ASCPlayerState* VictimPlayerState = KilledPlayer ? Cast<ASCPlayerState>(KilledPlayer->PlayerState) : nullptr;

	KillerPlayerState->ScoreKill(VictimPlayerState, /*PlayerKillFullScore*/0, DamageType);

	if (KillerPlayerState->IsActiveCharacterKiller())
	{
		HandleScoreEvent(KillerPlayerState, KillCounselorScoreEvent);

		if (VictimPlayerState)
		{
			VictimPlayerState->ScoreDeath(KillerPlayerState, /*DeathScore*/0, DamageType);

			// Score alive time
			ASCGameState* GS = Cast<ASCGameState>(GameState);
			const float TimeAlive = GS->ElapsedInProgressTime / 60.f;
			HandleScoreEvent(VictimPlayerState, CounselorAliveTimeBonusScoreEvent, TimeAlive);
		}
	}
	else if (VictimPlayerState->IsActiveCharacterKiller())
	{
		HandleScoreEvent(KillerPlayerState, KillJasonScoreEvent);
	}
}

void ASCGameMode::CharacterKilled(AController* Killer, AController* KilledPlayer, ASCCharacter* KilledCharacter, const UDamageType* const DamageType)
{
	ASCPlayerState* KillerPlayerState = Killer ? Cast<ASCPlayerState>(Killer->PlayerState) : nullptr;
	ASCPlayerState* VictimPlayerState = KilledPlayer ? Cast<ASCPlayerState>(KilledPlayer->PlayerState) : nullptr;

	if (VictimPlayerState)
	{
		// Notify everybody of their "role" in this death
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			if (ASCPlayerState* TestPS = Cast<ASCPlayerState>((*It)->PlayerState))
			{
				if (TestPS == KillerPlayerState && TestPS == VictimPlayerState)
				{
					TestPS->InformAboutSuicide(DamageType);
				}
				else if (TestPS == KillerPlayerState)
				{
					TestPS->InformAboutKill(DamageType, VictimPlayerState);
				}
				else if (TestPS == VictimPlayerState)
				{
					TestPS->InformAboutKilled(KillerPlayerState, DamageType);
				}
				else
				{
					TestPS->InformAboutDeath(KillerPlayerState, DamageType, VictimPlayerState);
				}
			}
		}
	}

	// Scoring
	bool bWasContextKill = DamageType ? DamageType->IsA<USCContextKillDamageType>() : false;
	ASCCharacter* Character = nullptr;
	if (Killer && (Character = Cast<ASCCharacter>(Killer->GetCharacter())) != nullptr)
	{
		// Looks like we died while in a context kill without receiving the CK info... DC?
		if (Character->IsInContextKill())
		{
			bWasContextKill = true;
		}
	}

	if (KillerPlayerState && KillerPlayerState != VictimPlayerState)
	{
		// If this is a single player kill score it separately.
		if (const USCSinglePlayerDamageType* const SPDamage = Cast<USCSinglePlayerDamageType>(DamageType))
		{
			HandleScoreEvent(KillerPlayerState, SPDamage->ScoreEvent);
			KillerPlayerState->ScoreKill(VictimPlayerState, /*PlayerKillFullScore*/0, DamageType);
		}
		else
		{
			if (!bWasContextKill)
				KillerPlayerState->ScoreKill(VictimPlayerState, /*PlayerKillFullScore*/0, DamageType);

			if (KillerPlayerState->IsActiveCharacterKiller())
			{
				if (!bWasContextKill)
				{
					HandleScoreEvent(KillerPlayerState, JasonWeaponKillScoreEvent);
					HandleScoreEvent(KillerPlayerState, KillCounselorScoreEvent);
				}

			}
			else if (VictimPlayerState)
			{
				if (!VictimPlayerState->IsActiveCharacterKiller())
				{
					// Killed another counselor
					HandleScoreEvent(KillerPlayerState, FriendlyFireScoreEvent);
				}
			}
		}
	}
	if (VictimPlayerState && !bWasContextKill)
	{
		VictimPlayerState->ScoreDeath(KillerPlayerState, /*DeathScore*/0, DamageType);

		// Score alive time
		ASCGameState* GS = Cast<ASCGameState>(GameState);
		const float TimeAlive = GS->ElapsedInProgressTime / 60.f;
		HandleScoreEvent(VictimPlayerState, CounselorAliveTimeBonusScoreEvent, TimeAlive);
	}

	if (ASCCounselorCharacter* KilledCounselor = Cast<ASCCounselorCharacter>(KilledCharacter))
	{
		ASCGameState* GS = Cast<ASCGameState>(GameState);
		if (KilledCounselor->bIsHunter)
		{
			check(!GS->bHunterDied);
			GS->bHunterDied = true;
			++GS->DeadCounselorCount; // 02/03/2021 - re-enable Hunter Selection
		}
		else
		{
			++GS->DeadCounselorCount;

			if (CanSpawnHunter())
			{
				HunterToSpawn = ChooseHunter();
				if (HunterToSpawn == KilledPlayer)
				{
					if (UWorld* World = GetWorld())
						World->GetTimerManager().SetTimer(TimerHandle_HunterTimer, this, &ThisClass::AttemptSpawnChoosenHunter, GS->HunterDelayTime, false);
				}
				else
				{
					SpawnHunter();
				}
			}
		}
	}

	if (KilledCharacter)
	{
		KilledCharacter->SetActorTickEnabled(false); // FIXME: pjackson: Why? And why only on the server?

													 // Tell players to perform death cleanup
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(KilledPlayer))
			PC->CLIENT_OnCharacterDeath(KilledCharacter);

		// Enter spectator
		Spectate(KilledCharacter->GetController());
	}
}

bool ASCGameMode::CharacterEscaped(AController* CounselorController, ASCCounselorCharacter* CounselorCharacter)
{
	UWorld* World = GetWorld();
	ASCGameState* GS = Cast<ASCGameState>(GameState);
	ASCPlayerState* EscapedPlayerState = (World && GameState && CounselorController && CounselorCharacter) ? Cast<ASCPlayerState>(CounselorController->PlayerState) : nullptr;
	if (!EscapedPlayerState || EscapedPlayerState->GetIsDead() || EscapedPlayerState->GetEscaped())
		return false;

	ASCPlayerController* EscapedPlayerController = Cast<ASCPlayerController>(CounselorController);

	// Inform all players about the escape
	for (APlayerState* Player : GS->PlayerArray)
	{
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(Player))
		{
			PS->InformAboutEscape(EscapedPlayerState);
		}
	}

	// Score escape, unscathed, and time alive
	HandleScoreEvent(EscapedPlayerState, CounselorEscapedScoreEvent);

	if (!EscapedPlayerState->GetHasTakenDamage())
	{
		HandleScoreEvent(EscapedPlayerState, UnscathedScoreEvent);
	}

	const float TimeAlive = GS->ElapsedInProgressTime / 60.f;
	HandleScoreEvent(EscapedPlayerState, CounselorAliveTimeBonusScoreEvent, TimeAlive);

	// Score escape
	EscapedPlayerState->ScoreEscape(/*EscapeScore*/0, CounselorCharacter->GetVehicle());

	// Tell players to perform some ... death cleanup?
	if (EscapedPlayerController)
		EscapedPlayerController->CLIENT_OnCharacterDeath(CounselorCharacter);

	// Spawn AI for the counselor so they continue to follow their escape spline after the local controller unpossesses it
	if (ASCAIController* AIController =  World->SpawnActor<ASCAIController>())
	{
		if (ASCCharacterAIController* OldCharacterAI = Cast<ASCCharacterAIController>(CounselorController))
		{
			// Copy the old PlayerState across, otherwise the unreferenced CounselorController will get destroyed, taking the PlayerState with it
			AIController->PlayerState = OldCharacterAI->PlayerState;
			OldCharacterAI->PlayerState = nullptr;
		}

		AIController->Possess(CounselorCharacter);
	}

	// Put them into spectator
	Spectate(CounselorController);

	if (CounselorCharacter->bIsHunter)
	{
		GS->bHunterEscaped = true;
		++GS->EscapedCounselorCount; // 02/03/2021 - re-enable Hunter Selection
	}
	else
	{
		++GS->EscapedCounselorCount;
	}

	if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(CounselorController))
	{
		if (!CounselorCharacter->bIsHunter)
		{
			if (CanSpawnHunter())
			{
				HunterToSpawn = ChooseHunter();
				if (HunterToSpawn == PlayerController)
				{
					GetWorldTimerManager().SetTimer(TimerHandle_HunterTimer, this, &ThisClass::SpawnHunter, GS->HunterDelayTime, false);
				}
				else
				{
					SpawnHunter();
				}
			}
		}
	}

	return true;
}

void ASCGameMode::Spectate(AController* Player)
{
	if (ASCPlayerController* Controller = Cast<ASCPlayerController>(Player))
	{
		if (ASCPlayerState* SCPlayerState = Cast<ASCPlayerState>(Controller->PlayerState))
		{
			if (SCPlayerState->GetSpectating())
				return;

			FTransform SpecStart;
			// If the player escaped, get the location where the spectator camera should start
			if (ASCEscapeVolume* EscapeVolume = Cast<ASCEscapeVolume>(Controller->GetViewTarget()))
			{
				SpecStart = EscapeVolume->SpectatorStartPoint->GetComponentTransform();
			}

			SCPlayerState->SetSpectating(true);
			Controller->ChangeState(NAME_Spectating);
			Controller->ClientGotoState(NAME_Spectating);

			// Set the spectator camera start location
			if (SpecStart.GetLocation() != FVector::ZeroVector)
			{
				Controller->ClientSetSpectatorCamera(SpecStart.GetLocation(), FRotator(SpecStart.GetRotation()));
			}
		}
	}
}

void ASCGameMode::SpawnCabins()
{
	// We'll need these quite a bit
	ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());

	// Grab cabin groups and spawn number of cabins in min/max range for that group
	// Make sure we don't go over the level's cabin limit
	int32 TotalCabinsSpawned = 0;
	const int32 TotalCabinsToSpawn = WorldSettings->TotalCabinCount;
	if (TotalCabinsToSpawn > 0)
	{
		for (TActorIterator<ASCCabinGroupVolume> It(GetWorld(), ASCCabinGroupVolume::StaticClass()); It; ++It)
		{
			ASCCabinGroupVolume* Group = *It;
			// Make sure it's valid (cause reasons)
			if (IsValid(Group) == false)
				continue;

			TArray<AActor*> ActorList;
			Group->Volume->GetOverlappingActors(ActorList, ASCCabinSpawner::StaticClass());

			// Continue to next cabin group if there are no spawners in this one (could be on the wrong level, etc.)
			if (ActorList.Num() <= 0)
				continue;

			const int32 NumCabinsToSpawn = FMath::RandRange(Group->MinCabinCount, Group->MaxCabinCount);
			int32 NumCabinsSpawned = 0;

			// Spawn the number of cabins we rolled
			for (int32 iCabin(0); iCabin < NumCabinsToSpawn; ++iCabin)
			{
				const int32 SpawnerIndex = FMath::RandHelper(ActorList.Num());

				if (Cast<ASCCabinSpawner>(ActorList[SpawnerIndex])->SpawnCabin() == false)
					continue;

				ActorList.RemoveAt(SpawnerIndex, 1, false);

				// Increment our total number of cabins spawned for this group and check against the number of cabins allowed for the level
				++NumCabinsSpawned;
				if (TotalCabinsToSpawn > 0 && TotalCabinsSpawned + NumCabinsSpawned >= TotalCabinsToSpawn)
				{
					break;
				}

				// This is a paranoid check for assholes that set the MaxCabinCount to larger than the number of actual cabin spawners (THANKS LUKE!)
				if (ActorList.Num() == 0)
				{
#if WITH_EDITOR
					FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("Not enough cabin spawners in %s to facilitate %d cabins"), *Group->GetName(), NumCabinsToSpawn)));
#endif
					break;
				}
			}

			// Add the number of cabins spawned to total cabins spawned the and check against the level's cabin limit
			// If we haven't reached it, we'll move onto the next cabin group
			TotalCabinsSpawned += NumCabinsSpawned;
			if (TotalCabinsToSpawn > 0 && TotalCabinsSpawned >= TotalCabinsToSpawn)
			{
				break;
			}
		}

#if WITH_EDITOR
		FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("Spawned cabins %d/%d"), TotalCabinsSpawned, TotalCabinsToSpawn)));
#endif
	}

	// Keep track of all cabins in the level
	AllCabins.Empty();
	for (TActorIterator<ASCCabin> It(GetWorld(), ASCCabin::StaticClass()); It; ++It)
	{
		if (IsValid(*It))
			AllCabins.Add(*It);
	}
}

void ASCGameMode::SpawnVehicles()
{
	SpawnedVehicles.Empty();

	// World settings will be used throughout this function
	const ASCWorldSettings* WorldSettings = Cast <ASCWorldSettings>(GetWorldSettings());

	// Grab all the vehicle spawners in the map for quick reference later
	TArray<ASCVehicleSpawner*> VehicleSpawners;
	for (TActorIterator<ASCVehicleSpawner> It(GetWorld(), ASCVehicleSpawner::StaticClass()); It; ++It)
	{
		if (IsValid(*It))
		{
			VehicleSpawners.Add(*It);
		}
	}

#if WITH_EDITOR
	// Do some editor logging
	if (VehicleSpawners.Num() <= 0)
	{
		FMessageLog(TEXT("Spawning")).Error(FText::FromString(TEXT("No Vehicle Spawners placed in level")));
	}
	else
	{
		FMessageLog(TEXT("Spawning")).Info(FText::FromString(FString::Printf(TEXT("%d vehicle spawners"), VehicleSpawners.Num())));
	}
#endif

	// Map the vehicle spawners according to the vehicles the world settings wants to spawn
	TMap<TSubclassOf<ASCDriveableVehicle>, TArray<ASCVehicleSpawner*>> VehicleSpawnerMap;
	TArray<TSubclassOf<ASCDriveableVehicle>> VehiclesToSpawn;
	for (int32 VehicleDataIndex(0); VehicleDataIndex < WorldSettings->VehiclesToSpawn.Num(); ++VehicleDataIndex)
	{
		TArray<TSubclassOf<ASCDriveableVehicle>> ValidVehicleClasses;
		for (TSubclassOf<ASCDriveableVehicle> VehicleClass : WorldSettings->VehiclesToSpawn[VehicleDataIndex].VehicleClasses)
		{
			if (IsAllowedVehicleClass(VehicleClass))
			{
				ValidVehicleClasses.AddUnique(VehicleClass);
			}
		}
		if (ValidVehicleClasses.Num() == 0)
			continue;

		int32 VehicleIndex = FMath::RandHelper(ValidVehicleClasses.Num());
		TSubclassOf<ASCDriveableVehicle> VehicleClass = ValidVehicleClasses[VehicleIndex];

		VehiclesToSpawn.Add(VehicleClass);

		if (!VehicleSpawnerMap.Contains(VehicleClass))
		{
			VehicleSpawnerMap.Add(VehicleClass);

			// Loop through the vehicle spawners on the map and see if they support this vehicle class
			for (int32 VehicleSpawnerIndex(0); VehicleSpawnerIndex < VehicleSpawners.Num(); ++VehicleSpawnerIndex)
			{
				ASCVehicleSpawner* VehicleSpawner = VehicleSpawners[VehicleSpawnerIndex];
				if (VehicleSpawner->ActorSpawner->SupportsType(VehicleClass))
					VehicleSpawnerMap[VehicleClass].Add(VehicleSpawner);
			}
		}
	}

	// Lambda for checking distance between spawned vehicles
	const auto SpawnDistanceCheck = [this, &WorldSettings](AActor* SpawnedVehicle, ASCVehicleSpawner* Spawner)
	{
		if (!SpawnedVehicle)
			return true;

		if ((SpawnedVehicle->GetActorLocation() - Spawner->GetActorLocation()).SizeSquared() >= FMath::Square(WorldSettings->MinVehicleSpawnDistance))
		{
			return true;
		}

		return false;
	};

	// Iterate through the vehicle map
	for (int32 VehicleClassIndex(0); VehicleClassIndex < VehiclesToSpawn.Num(); ++VehicleClassIndex)
	{
		// Grab a reference to the array of spawners for this vehicle class
		TArray<ASCVehicleSpawner*>& Spawners = VehicleSpawnerMap[VehiclesToSpawn[VehicleClassIndex]];

		ASCDriveableVehicle* SpawnedVehicle = nullptr;
		while (!SpawnedVehicle && Spawners.Num() > 0)
		{
			// Grab a random spawner
			const int32 SpawnerIndex = FMath::RandHelper(Spawners.Num());

			// Check if the selected spawner is far enough away from any other spawned vehicles
			if (SpawnedVehicles.Num() > 0)
			{
				bool DistanceOK = true;
				for (int32 VehicleIndex(0); VehicleIndex < SpawnedVehicles.Num(); ++VehicleIndex)
				{
					DistanceOK &= SpawnDistanceCheck(SpawnedVehicles[VehicleIndex], Spawners[SpawnerIndex]);
				}

				if (DistanceOK)
				{
					// Distance is good, spawn the fucker
					if (Spawners[SpawnerIndex]->SpawnVehicle(VehiclesToSpawn[VehicleClassIndex]))
					{
						SpawnedVehicle = Cast<ASCDriveableVehicle>(Spawners[SpawnerIndex]->GetSpawnedVehicle());
					}
				}
#if WITH_EDITOR
				else
				{
					FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("Vehicle spawn point %s is too close (%.2fcm) to other spawned vehicles (%d) and will not be used."), *GetNameSafe(Spawners[SpawnerIndex]), WorldSettings->MinVehicleSpawnDistance, SpawnedVehicles.Num())));
				}
#endif
			}
			else
			{
				// This is the first vehicle to spawn, go ahead and just spawn it.
				if (Spawners[SpawnerIndex]->SpawnVehicle(VehiclesToSpawn[VehicleClassIndex]))
				{
					SpawnedVehicle = Cast<ASCDriveableVehicle>(Spawners[SpawnerIndex]->GetSpawnedVehicle());
				}
			}

			// Add the vehicle to our list
			if (SpawnedVehicle)
			{
				SpawnedVehicles.Add(SpawnedVehicle);
			}
#if WITH_EDITOR
			else
			{
				FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("Could not spawn a vehicle with spawner %s!"), *GetNameSafe(Spawners[SpawnerIndex]))));
			}
#endif

			// Remove the spawner. It has either spawned a vehicle or isn't viable.
			Spawners.RemoveAt(SpawnerIndex, 1, false);
		}
	}

	// Add placed vehicles too
	for (TActorIterator<ASCDriveableVehicle> It(GetWorld(), ASCDriveableVehicle::StaticClass()); It; ++It)
	{
		if (IsValid(*It))
			SpawnedVehicles.AddUnique(*It);
	}

#if WITH_EDITOR
	if (SpawnedVehicles.Num() < WorldSettings->VehiclesToSpawn.Num())
	{
		FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("Couldn't spawn all vehicles %d/%d!"), SpawnedVehicles.Num(), WorldSettings->VehiclesToSpawn.Num())));
	}
	else
	{
		FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("%d vehicles spawned"), SpawnedVehicles.Num())));
	}
#endif
}

void ASCGameMode::AttemptSpawnPamelaTapes()
{
#if WITH_EDITOR
	if (GetWorld()->IsPlayInEditor()) // Not convinced I need this
		return;
#endif

	USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
	USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
	RequestManager->GetActivePamelaTapes(FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::RequestPamelaTapesCallback));
}

void ASCGameMode::RequestPamelaTapesCallback(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("RequestPamelaTapesCallback: ResponseCode %i Reason: %s"), ResponseCode, *ResponseContents);
		return;
	}

	ActivePamelaTapes.Empty();

	// Parse the data
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContents);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* ObjArray = nullptr;
		if (JsonObject->TryGetArrayField(TEXT("ActivePamelaTapes"), ObjArray))
		{
			for (TSharedPtr<FJsonValue> Obj : *ObjArray)
			{
				const TSharedPtr<FJsonObject>* Entry;
				Obj->TryGetObject(Entry);

				FString ClassName;
				(*Entry)->TryGetStringField(TEXT("ClassName"), ClassName);

				FString ClassString = FString::Printf(TEXT("/Game/Blueprints/PamelaTapes/%s.%s_C"), *ClassName, *ClassName);
				ActivePamelaTapes.Add(LoadObject<UClass>(NULL, *ClassString, NULL, LOAD_None, NULL));
			}
		}
	}

	if (ActivePamelaTapes.Num() > 0)
		SpawnPamelaTapes();
}

void ASCGameMode::SpawnPamelaTapes()
{
	// For now we'll only be spawning 1 tape per match
	const int32 TapeIndex = FMath::RandHelper(ActivePamelaTapes.Num());
	const USCPamelaTapeDataAsset* const TapeData = Cast<USCPamelaTapeDataAsset>(ActivePamelaTapes[TapeIndex]->GetDefaultObject());
	if (TapeData)
	{
		// Spawn a pamela tape pickup in an unused Cabinet
		if (Cabinets.Num() > 0)
		{
			ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());
			if (!WorldSettings || !WorldSettings->PamelaTapeClass)
				return;

			TArray<ASCCabinet*> PamelaTapeCabinets;
			for (ASCCabinet* Cabinet : Cabinets)
			{
				if (!Cabinet)
					continue;

				if (Cabinet->ItemSpawner->SupportsType(WorldSettings->PamelaTapeClass))
				{
					if (!Cabinet->IsCabinetFull())
					{
						PamelaTapeCabinets.Add(Cabinet);
					}
				}
			}

			if (PamelaTapeCabinets.Num() > 0)
			{
				const int32 CabinetIndex = FMath::RandHelper(PamelaTapeCabinets.Num());

				PamelaTapeCabinets[CabinetIndex]->SpawnItem(WorldSettings->PamelaTapeClass);
				ASCPamelaTape* SpawnedTape = nullptr;
				if (ASCDoubleCabinet* DC = Cast<ASCDoubleCabinet>(PamelaTapeCabinets[CabinetIndex]))
				{
					SpawnedTape = Cast<ASCPamelaTape>(DC->ContainedItem2);
				}

				if (!SpawnedTape)
				{
					SpawnedTape = Cast<ASCPamelaTape>(PamelaTapeCabinets[CabinetIndex]->ContainedItem);
				}

				if (SpawnedTape)
				{
					SpawnedTape->TapeData = ActivePamelaTapes[TapeIndex];
					SpawnedTape->SetFriendlyName(TapeData->TapeName);
				}
			}
		}
	}
}

void ASCGameMode::AttemptSpawnTommyTapes()
{
#if WITH_EDITOR
	if (GetWorld()->IsPlayInEditor()) // Not convinced I need this
		return;
#endif

	USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
	USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
	RequestManager->GetActiveTommyTapes(FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::RequestTommyTapesCallback));
}

void ASCGameMode::RequestTommyTapesCallback(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("RequestTommyTapesCallback: ResponseCode %i Reason: %s"), ResponseCode, *ResponseContents);
		return;
	}

	ActiveTommyTapes.Empty();

	// Parse the data
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContents);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* ObjArray = nullptr;
		if (JsonObject->TryGetArrayField(TEXT("ActiveTommyTapes"), ObjArray))
		{
			for (TSharedPtr<FJsonValue> Obj : *ObjArray)
			{
				const TSharedPtr<FJsonObject>* Entry;
				Obj->TryGetObject(Entry);

				FString ClassName;
				(*Entry)->TryGetStringField(TEXT("ClassName"), ClassName);

				FString ClassString = FString::Printf(TEXT("/Game/Blueprints/TommyTapes/%s.%s_C"), *ClassName, *ClassName);
				ActiveTommyTapes.Add(LoadObject<UClass>(NULL, *ClassString, NULL, LOAD_None, NULL));
			}
		}
	}

	if (ActiveTommyTapes.Num() > 0)
		SpawnTommyTapes();
}

void ASCGameMode::SpawnTommyTapes()
{
	// For now we'll only be spawning 1 tape per match
	const int32 TapeIndex = FMath::RandHelper(ActiveTommyTapes.Num());
	const USCTommyTapeDataAsset* const TapeData = Cast<USCTommyTapeDataAsset>(ActiveTommyTapes[TapeIndex]->GetDefaultObject());
	if (TapeData)
	{
		// Spawn a Tommy tape pickup in an unused Cabinet
		if (Cabinets.Num() > 0)
		{
			ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());
			if (!WorldSettings || !WorldSettings->TommyTapeClass)
				return;

			TArray<ASCCabinet*> TommyTapeCabinets;
			for (ASCCabinet* Cabinet : Cabinets)
			{
				if (!Cabinet)
					continue;

				if (Cabinet->ItemSpawner->SupportsType(WorldSettings->TommyTapeClass))
				{
					if (!Cabinet->IsCabinetFull())
					{
						TommyTapeCabinets.Add(Cabinet);
					}
				}
			}

			if (TommyTapeCabinets.Num() > 0)
			{
				const int32 CabinetIndex = FMath::RandHelper(TommyTapeCabinets.Num());

				TommyTapeCabinets[CabinetIndex]->SpawnItem(WorldSettings->TommyTapeClass);
				ASCTommyTape* SpawnedTape = nullptr;
				if (ASCDoubleCabinet* DC = Cast<ASCDoubleCabinet>(TommyTapeCabinets[CabinetIndex]))
				{
					SpawnedTape = Cast<ASCTommyTape>(DC->ContainedItem2);
				}

				if (!SpawnedTape)
				{
					SpawnedTape = Cast<ASCTommyTape>(TommyTapeCabinets[CabinetIndex]->ContainedItem);
				}

				if (SpawnedTape)
				{
					SpawnedTape->TapeData = ActiveTommyTapes[TapeIndex];
					SpawnedTape->SetFriendlyName(TapeData->TapeName);
				}
			}
		}
	}
}

void ASCGameMode::SpawnLargeItems()
{
	const ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());

	// Create subsets of actor spawners so we don't needlessly access the entire array
	SpawnerMap.Empty();
	for (const FLargeItemSpawnData& SpawnData : WorldSettings->LargeItemSpawnData)
	{
		SpawnerMap.Add(SpawnData.ActorClass);
		for (auto Spawner : ActorSpawners)
		{
			if (Spawner->SupportsType(SpawnData.ActorClass))
			{
				SpawnerMap[SpawnData.ActorClass].Add(Spawner);
			}
		}
	}

	const auto SpawnItems = [this](TSubclassOf<AActor> ActorClass, int32 MinCount, int32 MaxCount)
	{
		const int32 SpawnerCount = SpawnerMap[ActorClass].Num();
		if (SpawnerCount > 0)
		{
			const int32 NumItemsToSpawn = FMath::RandRange(MinCount, MaxCount);
#if WITH_EDITOR
			FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("spawning %d [%d->%d] %s items"), NumItemsToSpawn, MinCount, MaxCount, *ActorClass->GetName())));
#endif
			for (int32 iSpawner(0); iSpawner < NumItemsToSpawn; ++iSpawner)
			{
				if (SpawnerMap[ActorClass].Num() <= 0)
				{
					// We ran out of spawners
#if WITH_EDITOR
					FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("Out of spawners, spawned %d of %d actor %s!"), iSpawner, NumItemsToSpawn, *ActorClass->GetName())));
#endif
					break;
				}

				// Access the spawner map to grab the subset of actor spawners that support the passed in item class
				const int32 SpawnerIndex = FMath::RandHelper(SpawnerMap[ActorClass].Num());
				if (!SpawnerMap[ActorClass][SpawnerIndex]->SpawnActor(ActorClass))
				{
					--iSpawner;
				}

				SpawnerMap[ActorClass].RemoveAt(SpawnerIndex, 1, false);
			}
		}
		else
		{
#if WITH_EDITOR
			FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("Not enough actor spawners for %s!"), *ActorClass->GetName())));
#endif
		}
	};

	// Loop through the designer set classes and spawn some items!
	for (const auto& SpawnData : WorldSettings->LargeItemSpawnData)
	{
		if (SpawnData.ActorClass != nullptr)
			SpawnItems(SpawnData.ActorClass, SpawnData.MinCount, SpawnData.MaxCount);
	}
}

void ASCGameMode::SpawnSmallItems()
{
	const ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());

	// Create subsets of cabinets so we don't fuck with other items trying to spawn in a cabinet that may not be viable for a different item
	CabinetMap.Empty();
	for (const FSmallItemSpawnData& SpawnData : WorldSettings->SmallItemSpawnData)
	{
		CabinetMap.Add(SpawnData.ItemClass);
		for (ASCCabinet* Cabinet : Cabinets)
		{
			if (Cabinet->ItemSpawner->SupportsType(TSubclassOf<AActor>(SpawnData.ItemClass))) // CLEANUP: pjackson: Why cast? Why?
			{
				CabinetMap[SpawnData.ItemClass].Add(Cabinet);
			}
		}
	}

	const auto SpawnItems = [this](const FSmallItemSpawnData& SpawnData)
	{
		const int32 CabinetCount = CabinetMap[SpawnData.ItemClass].Num();
		if (CabinetCount > 0)
		{
			const int32 NumItemsToSpawn = FMath::RandRange(SpawnData.MinCount, SpawnData.MaxCount);
#if WITH_EDITOR
			FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("spawning %d [%d->%d] %s items"), NumItemsToSpawn, SpawnData.MinCount, SpawnData.MaxCount, *SpawnData.ItemClass->GetName())));
#endif
			TArray<FVector> SpawnedItemLocations;
			for (int32 iSpawner(0); iSpawner < NumItemsToSpawn; ++iSpawner)
			{
				if (CabinetMap[SpawnData.ItemClass].Num() <= 0)
				{
					// Somehow we ran out of cabinets?
#if WITH_EDITOR
					FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("Out of small item spawners, spawned %d of %d items %s!"), SpawnedItemLocations.Num(), NumItemsToSpawn, *SpawnData.ItemClass->GetName())));
#endif
					break;
				}

				const int32 SpawnerIndex = FMath::RandHelper(CabinetMap[SpawnData.ItemClass].Num());
				if (CabinetMap[SpawnData.ItemClass][SpawnerIndex]->IsCabinetFull())
				{
					CabinetMap[SpawnData.ItemClass].RemoveAt(SpawnerIndex, 1, false);
					--iSpawner;
					continue;
				}

				// Let's skip checking distance if the number of items we need to spawn is equal to the number of spawners left
				const bool bCheckDistance = CabinetMap[SpawnData.ItemClass].Num() > NumItemsToSpawn - SpawnedItemLocations.Num();
#if WITH_EDITOR
				if (!bCheckDistance)
					FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("Almost out of cabinets for spawning %s, ignoring distance checking %d"), *SpawnData.ItemClass->GetName(), SpawnData.MinSpawnDistance)));
#endif

				bool bDistanceOK = true;
				if (bCheckDistance && SpawnData.MinSpawnDistance > 0.f) // If the MinSpawnDistance is 0.f, skip checking against it
				{
					for (const FVector PreviousItemLocation : SpawnedItemLocations)
					{
						if ((CabinetMap[SpawnData.ItemClass][SpawnerIndex]->GetActorLocation() - PreviousItemLocation).SizeSquared() < FMath::Square(SpawnData.MinSpawnDistance))
						{
							bDistanceOK = false;
							CabinetMap[SpawnData.ItemClass].RemoveAt(SpawnerIndex, 1, false);
							--iSpawner;
							break;
						}
					}
				}

				if (bDistanceOK)
				{
					if (CabinetMap[SpawnData.ItemClass][SpawnerIndex]->SpawnItem(SpawnData.ItemClass))
					{
						SpawnedItemLocations.Add(CabinetMap[SpawnData.ItemClass][SpawnerIndex]->GetActorLocation());
					}
					else
					{
						--iSpawner;
					}

					if (CabinetMap[SpawnData.ItemClass][SpawnerIndex]->IsCabinetFull())
						CabinetMap[SpawnData.ItemClass].RemoveAt(SpawnerIndex, 1, false);
				}
			}
		}
		else
		{
#if WITH_EDITOR
			FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("Not enough actor spawners for %s!"), *SpawnData.ItemClass->GetName())));
#endif
		}
	};

	for (const auto& SpawnData : WorldSettings->SmallItemSpawnData)
	{
		if (SpawnData.ItemClass != nullptr)
			SpawnItems(SpawnData);
	}
}

template <typename ActorType>
void ASCGameMode::SpawnRepairParts(const TArray<ActorType*>& ActorsNeedingRepair, const float MinDistanceBetweenParts, const float MaxDistanceFromPartOwner, TArray<FVector> SpawnedPartLocations/* = TArray<FVector>()*/, const int32 NumPartsToSpawn/* = 1*/)
{
	const auto SpawnDistanceCheck = [MinDistanceBetweenParts](const TArray<FVector>& PartLocations, const AActor* PartSpawner)
	{
		if (MinDistanceBetweenParts > 0.f)
		{
			for (const FVector PartLocation : PartLocations)
			{
				if ((PartLocation - PartSpawner->GetActorLocation()).SizeSquared() < FMath::Square(MinDistanceBetweenParts))
				{
					return false;
				}
			}
		}
		return true;
	};

#if WITH_EDITOR
	FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("%d actors need repair"), ActorsNeedingRepair.Num())));
#endif

	for (int32 ActorIndex(0); ActorIndex < ActorsNeedingRepair.Num(); ++ActorIndex)
	{
		TArray<UActorComponent*> RepairComponents = ActorsNeedingRepair[ActorIndex]->GetComponentsByClass(USCRepairComponent::StaticClass());
		TArray<TSubclassOf<ASCRepairPart>> PartClasses;
		for (const UActorComponent* Comp : RepairComponents)
		{
			for (const auto& PartClass : Cast<USCRepairComponent>(Comp)->GetRequiredPartClasses())
			{
				PartClasses.Add(PartClass);
			}
		}

		if (PartClasses.Num() == 0)
		{
#if WITH_EDITOR
			FMessageLog("Spawning").Error(FText::FromString(FString::Printf(TEXT("No part classes to spawn for %s!"), *ActorsNeedingRepair[ActorIndex]->GetName())));
#endif
			continue;
		}

		const FVector ActorPosition = ActorsNeedingRepair[ActorIndex]->GetActorLocation();

		// Map of repair spawners that satisfy the MaxDistanceFromPartOwner requirement and supports the PartClass type
		TMap<TSubclassOf<ASCRepairPart>, TArray<ASCRepairPartSpawner*>> RepairSpawnerMap;
		TMap<TSubclassOf<ASCRepairPart>, TArray<ASCCabinet*>> RepairCabinetMap;
		for (const TSubclassOf<ASCRepairPart> PartClass : PartClasses)
		{
			if (const ASCRepairPart* const DefaultPart = Cast<ASCRepairPart>(PartClass->GetDefaultObject()))
			{
				if (DefaultPart->bIsLarge)
				{
					RepairSpawnerMap.Add(PartClass);
					for (int32 iPartSpawner(0); iPartSpawner < PartSpawners.Num(); ++iPartSpawner)
					{
						if (PartSpawners[iPartSpawner]->ActorSpawner->SupportsType(PartClass))
						{
							if (!PartSpawners[iPartSpawner]->ActorSpawner->GetSpawnedActor())
							{
								if (MaxDistanceFromPartOwner == 0.f || (PartSpawners[iPartSpawner]->GetActorLocation() - ActorPosition).SizeSquared() < FMath::Square(MaxDistanceFromPartOwner))
								{
									RepairSpawnerMap[PartClass].Add(PartSpawners[iPartSpawner]);
								}
							}
						}
					}
				}
				else
				{
					RepairCabinetMap.Add(PartClass);
					for (int32 iCabinet(0); iCabinet < Cabinets.Num(); ++iCabinet)
					{
						if (Cabinets[iCabinet]->ItemSpawner->SupportsType(PartClass))
						{
							if (!Cabinets[iCabinet]->IsCabinetFull())
							{
								if (MaxDistanceFromPartOwner == 0.f || (Cabinets[iCabinet]->GetActorLocation() - ActorPosition).SizeSquared() < FMath::Square(MaxDistanceFromPartOwner))
								{
									RepairCabinetMap[PartClass].Add(Cabinets[iCabinet]);
								}
							}
						}
					}
				}
			}
		}

		for (const TSubclassOf<ASCRepairPart> PartClass : PartClasses)
		{
			if (const ASCRepairPart* const DefaultPart = Cast<ASCRepairPart>(PartClass->GetDefaultObject()))
			{
				// It's a big one! Find a normal actor spawner in the map that supports this guy.
				if (DefaultPart->bIsLarge)
				{
					for (int32 iSpawner(0); iSpawner < RepairSpawnerMap[PartClass].Num(); ++iSpawner)
					{
						// Check if we have valid spawners left in our map
						if (RepairSpawnerMap[PartClass].Num() <= 0)
						{
#if WITH_EDITOR
							FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("No spawners outside min part distance: %.2f - Ignoring check and spawning within range of actor needing repair: %s"), MinDistanceBetweenParts, *ActorsNeedingRepair[ActorIndex]->GetName())));
#endif
							// Try to spawn anywhere within range of the actor this part repairs
							bool bPartSpawned = false;
							for (int32 iPartSpawner(0); iPartSpawner < PartSpawners.Num(); ++iPartSpawner)
							{
								bool bDistanceOK = true;
								if (MaxDistanceFromPartOwner > 0.f)
								{
									if ((PartSpawners[iPartSpawner]->GetActorLocation() - ActorPosition).SizeSquared() >= FMath::Square(MaxDistanceFromPartOwner))
									{
										bDistanceOK = false;
									}
								}

								if (bDistanceOK)
								{
									if (PartSpawners[iPartSpawner]->SpawnPart(PartClass))
									{
										SpawnedPartLocations.Add(PartSpawners[iPartSpawner]->GetActorLocation());
										bPartSpawned = true;
										break;
									}
								}
							}

#if WITH_EDITOR
							if (!bPartSpawned)
							{
								FMessageLog("Spawning").Error(FText::FromString(FString::Printf(TEXT("Unable to spawn part: %s. Check spawners within %.2f of %s"), *PartClass->GetName(), MaxDistanceFromPartOwner, *ActorsNeedingRepair[ActorIndex]->GetName())));
							}
#endif
							continue;
						}

						// So we have some spawners, pick one at random
						const int32 SpawnerIndex = FMath::RandHelper(RepairSpawnerMap[PartClass].Num());

						// Check its location against any other parts we've already spawned
						if (!SpawnDistanceCheck(SpawnedPartLocations, RepairSpawnerMap[PartClass][SpawnerIndex]))
						{
							// It's too close, gtfo.
							RepairSpawnerMap[PartClass].RemoveAt(SpawnerIndex, 1, false);
							--iSpawner;
							continue;
						}

						// Spawner is in a good spot, let's spawn some shit
						if (RepairSpawnerMap[PartClass][SpawnerIndex]->SpawnPart(PartClass))
						{
							SpawnedPartLocations.Add(RepairSpawnerMap[PartClass][SpawnerIndex]->GetActorLocation());
							break;
						}
						else // This spawner may have spawned a part previously and this section of our map didn't get updated. It happens.
						{
							--iSpawner;
						}

						// Used up, get rid of it.
						RepairSpawnerMap[PartClass].RemoveAt(SpawnerIndex, 1, false);
					}
				}
				else // Small items spawn in cabinets
				{
					// Same shit as above but with cabinets. hnnnggggg
					for (int32 iCabinet(0); iCabinet < RepairCabinetMap[PartClass].Num(); ++iCabinet)
					{
						if (RepairCabinetMap[PartClass].Num() < 0)
						{
#if WITH_EDITOR
							FMessageLog("Spawning").Warning(FText::FromString(FString::Printf(TEXT("No cabinets outside min part distance: %.2f - Ignoring check and spawning within range of actor needing repair: %s"), MinDistanceBetweenParts, *ActorsNeedingRepair[ActorIndex]->GetName())));
#endif
							// Try to spawn anywhere within range of the actor this part repairs
							bool bPartSpawned = false;
							for (int32 iCabinetSpawner(0); iCabinetSpawner < Cabinets.Num(); ++iCabinetSpawner)
							{
								bool bDistanceOK = true;
								if (MaxDistanceFromPartOwner > 0.f)
								{
									if ((Cabinets[iCabinetSpawner]->GetActorLocation() - ActorPosition).SizeSquared() >= FMath::Square(MaxDistanceFromPartOwner))
									{
										bDistanceOK = false;
									}
								}

								if (bDistanceOK)
								{
									if (Cabinets[iCabinetSpawner]->SpawnItem(PartClass))
									{
										SpawnedPartLocations.Add(Cabinets[iCabinetSpawner]->GetActorLocation());
										bPartSpawned = true;
										break;
									}
								}
							}

#if WITH_EDITOR
							if (!bPartSpawned)
							{
								FMessageLog("Spawning").Error(FText::FromString(FString::Printf(TEXT("Unable to spawn part: %s. Check cabinets within %.2f of %s"), *PartClass->GetName(), MaxDistanceFromPartOwner, *ActorsNeedingRepair[ActorIndex]->GetName())));
							}
#endif
							continue;
						}

						// So we have some cabinets, pick one at random
						const int32 CabinetIndex = FMath::RandHelper(RepairCabinetMap[PartClass].Num());

						if (!SpawnDistanceCheck(SpawnedPartLocations, RepairCabinetMap[PartClass][CabinetIndex]))
						{
							// It's too close, gtfo.
							RepairCabinetMap[PartClass].RemoveAt(CabinetIndex, 1, false);
							--iCabinet;
							continue;
						}

						// Cabinet is in a good spot, let's spawn some shit
						if (RepairCabinetMap[PartClass][CabinetIndex]->SpawnItem(PartClass))
						{
							SpawnedPartLocations.Add(RepairCabinetMap[PartClass][CabinetIndex]->GetActorLocation());
							break;
						}
						else // This cabinet may have been filled in a previous loop
						{
							--iCabinet;
						}

						if (RepairCabinetMap[PartClass][CabinetIndex]->IsCabinetFull())
							RepairCabinetMap[PartClass].RemoveAt(CabinetIndex, 1, false);
					}
				}
			}
		}

#if WITH_EDITOR
		FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("%d parts spawned for %s"), SpawnedPartLocations.Num(), *ActorsNeedingRepair[ActorIndex]->GetName())));
#endif
	}
}

void ASCGameMode::HandleScoreEvent(APlayerState* ScoringPlayer, TSubclassOf<USCScoringEvent> ScoreEventClass, const float ScoreModifier /*= 1.f*/)
{
	if (!ScoreEventClass)
		return;

	const USCScoringEvent* const ScoreEventData = ScoreEventClass->GetDefaultObject<USCScoringEvent>();

	if (ScoreEventData->bAllCounselors)
	{
		for (APlayerState* Player : GameState->PlayerArray)
		{
			if (ASCPlayerState* PS = Cast<ASCPlayerState>(Player))
			{
				if (!PS->IsActiveCharacterKiller() && PS->DidSpawnIntoMatch())
				{
					PS->HandleScoreEvent(ScoreEventClass, ScoreModifier);
				}
			}
		}
	}
	else if (ScoringPlayer)
	{
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(ScoringPlayer))
		{
			if (PS->DidSpawnIntoMatch())
			{
				PS->HandleScoreEvent(ScoreEventClass, ScoreModifier);
			}
		}
	}
}

int32 ASCGameMode::GetNumberOfCounselorClasses() const
{
	int32 CounselorClassCount = CounselorCharacterClasses.Num();
	// HACK: lpederson: Removing catty from the list of possible counselors on Japanese SKU
	if (USCGameBlueprintLibrary::IsJapaneseSKU(this))
		CounselorClassCount -= 3;

	return CounselorClassCount;
}

const TSoftClassPtr<ASCCounselorCharacter> ASCGameMode::GetRandomCounselorClass(const ASCPlayerState* PickingPlayerState) const
{
	const int32 CounselorClassCount = GetNumberOfCounselorClasses();
	ASCCounselorCharacter* DefaultCharacter = nullptr;
	TSoftClassPtr<ASCCounselorCharacter> SelectedClass = CounselorCharacterClasses[0];
	do
	{
		SelectedClass = CounselorCharacterClasses[FMath::RandHelper(CounselorClassCount)];
		SelectedClass.LoadSynchronous(); // TODO: Make Async
		DefaultCharacter = Cast<ASCCounselorCharacter>(SelectedClass.Get()->GetDefaultObject());

#if WITH_EDITOR
		if (GetWorld()->IsPlayInEditor())
			break;
#endif

	} while (DefaultCharacter->UnlockLevel > PickingPlayerState->GetPlayerLevel() || !DefaultCharacter->UnlockEntitlement.IsEmpty());

	return SelectedClass;
}

TSoftClassPtr<ASCCounselorCharacter> ASCGameMode::GetHeroCharacterClass() const
{
	if (UWorld* World = GetWorld())
	{
		if (ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(World->GetWorldSettings()))
		{
			if (!WorldSettings->HeroCharacterClass.IsNull())
				return WorldSettings->HeroCharacterClass;
		}
	}

	return HunterCharacterClass;
}

const TSoftClassPtr<ASCKillerCharacter> ASCGameMode::GetRandomKillerClass(const ASCPlayerState* PickingPlayerState) const
{
	ASCKillerCharacter* DefaultCharacter = nullptr;
	TSoftClassPtr<ASCKillerCharacter> SelectedClass = KillerCharacterClasses[0];
	do 
	{
		SelectedClass = KillerCharacterClasses[FMath::RandHelper(KillerCharacterClasses.Num())];
		SelectedClass.LoadSynchronous(); // TODO: Make Async
		DefaultCharacter = Cast<ASCKillerCharacter>(SelectedClass.Get()->GetDefaultObject());

#if WITH_EDITOR
		if (GetWorld()->IsPlayInEditor())
			break;
#endif

	} while (DefaultCharacter->UnlockLevel > PickingPlayerState->GetPlayerLevel() || !DefaultCharacter->UnlockEntitlement.IsEmpty());

	return SelectedClass;
}

void ASCGameMode::StartHunterTimer()
{
	if (ASCGameState* GS = GetGameState<ASCGameState>())
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HunterTimer, this, &ThisClass::AttemptSpawnHunter, GS->HunterResponseTime, false);
	}
}

bool ASCGameMode::CanSpawnHunter()
{
	// Are we already attempting to spawn a hunter?
	if (IsValid(HunterToSpawn))
		return false;

	ASCGameState* GS = GetGameState<ASCGameState>();
	if (!GS)
		return false;

	if (GS->bIsHunterSpawned)
		return false;

	if (GS->HunterTimeRemaining != 0)
		return false;

	const int32 NumCounselors = (NumPlayers + NumBots) - 1;// subtract one for the killer player

	// we never want a hunter to spawn if there is only one counselor
	if (NumCounselors == 1)
		return false;

	int32 TotalDeadAndEscaped = GS->EscapedCounselorCount + GS->DeadCounselorCount;
	//if there are less than 2 people who died and thats not everyone, continue to wait until more people died before spawning the hunter
	if (TotalDeadAndEscaped < 2 && TotalDeadAndEscaped != NumCounselors)
		return false;

	// Make sure there is actually a player that can be spawned as hunter.
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
		{
			if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(PC->PlayerState))
			{
				if (!PlayerState->bLoggedOut && (PlayerState->GetIsDead() || PlayerState->GetEscaped()))
					return true;
			}
		}
		else if (ASCCounselorAIController* CounselorBot = Cast<ASCCounselorAIController>(*It))
		{
			if (ASCPlayerState* BotPS = Cast<ASCPlayerState>(CounselorBot->PlayerState))
			{
				if (!BotPS->bLoggedOut && (BotPS->GetIsDead() || BotPS->GetEscaped()))
					return true;
			}
		}
	}

	// Otherwise return false
	return false;
}

AController* ASCGameMode::ChooseHunter()
{
	TArray<ASCPlayerController*> DeadPlayers;
	TArray<ASCPlayerController*> EscapedPlayers;
	TArray<ASCCounselorAIController*> EligibleBots;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
		{
			if (ASCPlayerState* PlayerState = Cast<ASCPlayerState>(PC->PlayerState))
			{
				if (PlayerState->bLoggedOut)
					continue;

				if (PlayerState->GetIsDead())
					DeadPlayers.Add(PC);
				else if (PlayerState->GetEscaped())
					EscapedPlayers.Add(PC);
			}
		}
		else if (ASCCounselorAIController* CounselorBot = Cast<ASCCounselorAIController>(*It))
		{
			if (ASCPlayerState* BotPS = Cast<ASCPlayerState>(CounselorBot->PlayerState))
			{
				if (BotPS->bLoggedOut)
					continue;

				if (BotPS->GetIsDead() || BotPS->GetEscaped())
					EligibleBots.Add(CounselorBot);
			}
		}
	}

	// Pick a random spectator, preferring players that died
	if (DeadPlayers.Num() > 0)
		return DeadPlayers[FMath::RandHelper(DeadPlayers.Num())];
	else if (EscapedPlayers.Num() > 0)
		return EscapedPlayers[FMath::RandHelper(EscapedPlayers.Num())];
	else if (EligibleBots.Num() > 0)
		return EligibleBots[FMath::RandHelper(EligibleBots.Num())];

	return nullptr;
}

void ASCGameMode::AttemptSpawnHunter()
{
	ASCGameState* GS = GetGameState<ASCGameState>();
	if (!GS)
		return;

	GS->HunterTimeRemaining = 0;

	if (!CanSpawnHunter())
		return;

	HunterToSpawn = ChooseHunter();
	if (HunterToSpawn)
		SpawnHunter();
	else // In case it fucks up, queue it up again
		StartHunterTimer();
}

void ASCGameMode::AttemptSpawnChoosenHunter()
{
	// Same as above, but no CanSpawnHunter() check

	if (!IsValid(HunterToSpawn))
		HunterToSpawn = ChooseHunter();

	if (HunterToSpawn)
		SpawnHunter();
	else
		StartHunterTimer();
}

void ASCGameMode::SpawnHunter()
{
	if (!IsValid(HunterToSpawn))
	{
		StartHunterTimer();
		return;
	}

	ASCGameState* GS = GetGameState<ASCGameState>();
	if (!GS)
		return;

	// We want to request a soft ptr once and use it consistently.
	TSoftClassPtr<ASCCounselorCharacter> HeroCharacter = GetHeroCharacterClass();

	// Make sure our hero class is loaded
	if (!HeroCharacter.Get())
	{
		// It wasn't, so we'll get right back to spawning this guy
		FStreamableDelegate Delegate;
		Delegate.BindUObject(this, &ThisClass::SpawnHunter);
		USCGameInstance* GameInstance = Cast<USCGameInstance>(GetGameInstance());
		GameInstance->StreamableManager.RequestAsyncLoad(HeroCharacter.ToSoftObjectPath(), Delegate);
		return;
	}

	ASCKillerCharacter* Killer = GS->CurrentKiller;

	HunterSpawnLocation = [&]() -> FVector
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(HunterToSpawn))
			return PC->GetFocalLocation();
		else if (APawn* BotPawn = HunterToSpawn->GetPawn())
			return BotPawn->GetActorLocation();

		return FNavigationSystem::InvalidLocation;
	}();
	HunterSpawnRotation = HunterToSpawn->GetControlRotation();

	ASCHunterPlayerStart* FoundPlayerStart = nullptr;
	TArray<ASCHunterPlayerStart*> UnOccupiedStartPoints;
	TArray<ASCHunterPlayerStart*> OccupiedStartPoints;
	APawn* PawnToFit = HeroCharacter->GetDefaultObject<APawn>();
	for (TActorIterator<ASCHunterPlayerStart> It(GetWorld()); It; ++It)
	{
		ASCHunterPlayerStart* PlayerStart = *It;

		FVector ActorLocation = PlayerStart->GetActorLocation();
		const FRotator ActorRotation = PlayerStart->GetActorRotation();

		if (Killer)
		{
			if (FVector::DistSquaredXY(ActorLocation, Killer->GetActorLocation()) < FMath::Square(MinHunterSpawnDistFromJason))
				continue;
		}

		if (!PlayerStart->IsEncroached(PawnToFit))
		{
			UnOccupiedStartPoints.Add(PlayerStart);
		}
		else if (GetWorld()->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
		{
			OccupiedStartPoints.Add(PlayerStart);
		}
	}
	if (UnOccupiedStartPoints.Num() > 0)
	{
		FoundPlayerStart = UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
	}
	else if (OccupiedStartPoints.Num() > 0)
	{
		FoundPlayerStart = OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
	}

	if (FoundPlayerStart)
	{
		HunterSpawnLocation = FoundPlayerStart->GetActorLocation();
		HunterSpawnRotation = FoundPlayerStart->GetActorRotation();

		InitStartSpot(FoundPlayerStart, HunterToSpawn);
	}

	ASCPlayerState* HunterPlayerState = Cast<ASCPlayerState>(HunterToSpawn->PlayerState);
	if (HunterPlayerState)
	{
		HunterPlayerState->bPlayedSpawnIntro = false;
		HunterPlayerState->AsyncSetActiveCharacterClass(HeroCharacter);
		HunterPlayerState->SetSpectating(false);
	}

	// unposess the current pawn
	HunterToSpawn->UnPossess();

	if (FoundPlayerStart)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(HunterToSpawn))
		{
			PC->BumpIdleTime();
			PC->TryStartSpawnIntroSequence(FoundPlayerStart);
			GetWorldTimerManager().SetTimer(TimerHandle_HunterSpawnIntroCompleted, this, &ThisClass::HunterIntroComplete, FMath::Max(0.1f, FoundPlayerStart->GetSpawnSequenceLength(PC) - 0.1f));
		}
		else
		{
			// Spawn bots right away
			HunterIntroComplete();

			// Update the bot's name
			if (HunterPlayerState)
				HunterPlayerState->SetPlayerName(HeroCharacter->GetDefaultObject<ASCCharacter>()->GetCharacterName().ToString());
		}
	}
}

void ASCGameMode::HunterIntroComplete()
{
	if (!IsValid(HunterToSpawn))
	{
		StartHunterTimer();
		return;
	}

	// We want to request a soft ptr once and use it consistently.
	TSoftClassPtr<ASCCounselorCharacter> HeroCharacter = GetHeroCharacterClass();

	// Make sure our hero class is loaded
	if (!HeroCharacter.Get())
	{
		// It wasn't, so we'll get right back to spawning this guy
		FStreamableDelegate Delegate;
		Delegate.BindUObject(this, &ThisClass::HunterIntroComplete);
		USCGameInstance* GameInstance = Cast<USCGameInstance>(GetGameInstance());
		GameInstance->StreamableManager.RequestAsyncLoad(HeroCharacter.ToSoftObjectPath(), Delegate);
		return;
	}

	ASCPlayerState* HunterPlayerState = Cast<ASCPlayerState>(HunterToSpawn->PlayerState);
	if (HunterPlayerState)
	{
		// The player disconnected while the intro was playing, pick a new hunter.
		if (HunterPlayerState->bLoggedOut)
			return;

		// Store scoring data before we reset this PlayerState
		const TArray<FSCCategorizedScoreEvent> CategoriezedScores = HunterPlayerState->GetEventCategoryScores();
		const TArray<TSubclassOf<USCScoringEvent>> PlayerScoringEvents = HunterPlayerState->GetScoreEvents();
		const bool bWasDamaged = HunterPlayerState->GetHasTakenDamage();

		// Reset the PlayerState and set the active character class to the hunter
		HunterPlayerState->Reset();
		HunterPlayerState->bPlayedSpawnIntro = true;
		HunterPlayerState->AsyncSetActiveCharacterClass(HeroCharacter);

		// Restore scoring data
		HunterPlayerState->SetEventCategoryScores(CategoriezedScores);
		HunterPlayerState->SetPlayerScoreEvents(PlayerScoringEvents);
		if (bWasDamaged)
			HunterPlayerState->TookDamage();

		// Restore perks
		HunterPlayerState->SetActivePerks(HunterPlayerState->GetPickedPerks());
	}

	FActorSpawnParameters HunterSpawnParams;
	HunterSpawnParams.Owner = HunterToSpawn;
	HunterSpawnParams.Instigator = Instigator;

	// spawn hunter class
	ASCCounselorCharacter* Hunter = GetWorld()->SpawnActor<ASCCounselorCharacter>(HeroCharacter.Get(), HunterSpawnLocation, HunterSpawnRotation, HunterSpawnParams);

	// give them a weapon.
	TSoftClassPtr<ASCItem> DefaultWeaponClass = Hunter->DefaultLargeItem ? Hunter->DefaultLargeItem : HunterWeaponClass;
	Hunter->GiveItem(DefaultWeaponClass);

	// give them a walkie talkie.
	Hunter->GiveItem(WalkieTalkieClass);

	// give them a map.
	Hunter->GiveItem(MapClass);

	// give them a med spray.
	Hunter->GiveItem(MedSprayClass);

	// give them a pocket knife.
	Hunter->GiveItem(PocketKnifeClass);

	// possess hunter.
	HunterToSpawn->Possess(Hunter);
	if (HunterPlayerState)
	{
		HunterPlayerState->SpawnedAsHunter();
		HunterPlayerState->EarnedBadge(TommyJarvisBadge);

		if (ASCPlayerState* HuntPlayerState = Cast<ASCPlayerState>(HunterPlayerState))
			HuntPlayerState->SpawnedCharacterClass = Hunter->GetClass();
	}

	// Put the player back in a Playing state
	HunterToSpawn->ChangeState(NAME_Playing);

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(HunterToSpawn))
	{
		PC->ClientGotoState(NAME_Playing);
		PC->BumpIdleTime();
	}
	else if (ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(HunterToSpawn))
	{
		BotController->RunBehaviorTree(CounselorBehaviorTree);
	}

	ASCGameState* GS = GetGameState<ASCGameState>();
	if (!GS)
		return;

	GS->bIsHunterSpawned = true;

	GetWorldTimerManager().ClearTimer(TimerHandle_HunterTimer);

	HunterToSpawn = nullptr;
}

void ASCGameMode::StartPoliceTimer(APlayerState* CallingPlayer)
{
	if (ASCGameState* GS = Cast<ASCGameState>(GameState))
	{
		float PoliceResponseTime = GS->PoliceResponseTime;
		float PoliceResponseModifier = 0.0f;
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(CallingPlayer))
		{
			for (const USCPerkData* Perk : PS->GetActivePerks())
			{
				check(Perk);

				if (!Perk)
					continue;

				PoliceResponseModifier += Perk->PoliceResponseTimeModifier;
			}
		}

		PoliceResponseTime += PoliceResponseTime * PoliceResponseModifier;

		GS->PoliceTimeRemaining = (int) PoliceResponseTime;
	}
}

void ASCGameMode::SpawnPolice()
{
	TArray<ASCPoliceSpawner*> PoliceSpawns;
	for (TActorIterator<ASCPoliceSpawner> It(GetWorld(), ASCPoliceSpawner::StaticClass()); It; ++It)
	{
		PoliceSpawns.AddUnique(*It);
	}

	const int32 NumSpawners = PoliceSpawns.Num();
	if (NumSpawners == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No police spawners found in level!"));
		return;
	}

	int32 index = FMath::RandRange(0, PoliceSpawns.Num() - 1);
	while (PoliceSpawns.Num() > 0 && !PoliceSpawns[index]->ActivateSpawner())
	{
		PoliceSpawns.RemoveAt(index);
		index = FMath::RandRange(0, PoliceSpawns.Num() - 1);
	}

	if (PoliceSpawns.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("All police spawners reported unable to spawn..."));
	}

	if (ASCGameState* GS = Cast<ASCGameState>(GameState))
	{
		if (GS->PoliceTimeRemaining > 0)
			GS->PoliceTimeRemaining = -1.f;
	}
}

void ASCGameMode::SpawnRepairItems()
{
	const ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());

	// Compile an array of part spawners in the level for random access
	PartSpawners.Empty();
	for (TActorIterator<ASCRepairPartSpawner> It(GetWorld(), ASCRepairPartSpawner::StaticClass()); It; ++It)
	{
		if (IsValid(*It) == false)
			continue;

		PartSpawners.Add(*It);
	}

#if WITH_EDITOR
	FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("%d part spawners"), PartSpawners.Num())));
#endif

	// Spawn repair parts
	if (PartSpawners.Num() > 0 && Cabinets.Num() > 0)
	{
		// Spawn vehicle parts
		SpawnRepairParts<ASCDriveableVehicle>(SpawnedVehicles, WorldSettings->MinVehiclePartSpawnDistance, WorldSettings->MaxCarPartDistance);

		// Compile an array of phone junction boxes in the level
		SpawnedPhoneBoxes.Empty();
		TArray<FVector> PhoneBoxLocations;
		for (TActorIterator<ASCPhoneJunctionBox> It(GetWorld(), ASCPhoneJunctionBox::StaticClass()); It; ++It)
		{
			if (IsValid(*It) == false)
				continue;

			SpawnedPhoneBoxes.Add(*It);
			PhoneBoxLocations.Add((*It)->GetActorLocation());
		}

#if WITH_EDITOR
		FMessageLog("Spawning").Info(FText::FromString(FString::Printf(TEXT("%d phone boxes"), SpawnedPhoneBoxes.Num())));
#endif

		// Spawn phone box fuses
		SpawnRepairParts<ASCPhoneJunctionBox>(SpawnedPhoneBoxes, WorldSettings->MinFuseSpawnDistance, WorldSettings->MaxFuseDistance, PhoneBoxLocations, WorldSettings->TotalFuseCount);
	}
}