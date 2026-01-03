// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameState_Hunt.h"

#include "SCBackendRequestManager.h"
#include "SCCBRadio.h"
#include "SCDriveableVehicle.h"
#include "SCGameInstance.h"
#include "SCGameMode_Hunt.h"
#include "SCInGameHUD.h"
#include "SCKillerCharacter.h"
#include "SCPhoneJunctionBox.h"
#include "SCPlayerController.h"
#include "SCPlayerState_Hunt.h"
#include "SCWorldSettings.h"

ASCGameState_Hunt::ASCGameState_Hunt(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASCGameState_Hunt::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	int32 NumKills = 0;
	int32 NumEscaped = 0;
	int32 NumSurvived = 0;
	for (APlayerState* Player : PlayerArray)
	{
		if (ASCPlayerState_Hunt* PS = Cast<ASCPlayerState_Hunt>(Player))
		{
			if (!PS->IsActiveCharacterKiller())
			{
				if (PS->GetIsDead())
					NumKills++;
				else if (PS->GetEscaped())
					NumEscaped++;
				else if (!PS->GetIsDead())
					NumSurvived++;
			}
		}
	}

	// Report match stats
	if (Role == ROLE_Authority)
	{
		USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
		USCBackendRequestManager* BRM = GI->GetBackendRequestManager<USCBackendRequestManager>();
		if (BRM->CanSendAnyRequests())
		{
			// Build our Json body
			FString Payload;
			TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload);
			JsonWriter->WriteObjectStart();
			JsonWriter->WriteValue(TEXT("MapName"), GetWorld() ? *GetWorld()->GetMapName() : TEXT("Invalid"));
			FString PlatformName = UGameplayStatics::GetPlatformName();
			if (PlatformName != TEXT("PS4") && PlatformName != TEXT("XboxOne"))
				PlatformName = TEXT("Steam");
			JsonWriter->WriteValue(TEXT("Platform"), PlatformName);
			JsonWriter->WriteValue(TEXT("StartedAt"), MatchStartTime);
			JsonWriter->WriteValue(TEXT("EndedAt"), MatchEndTime);

			// Player match stats
			JsonWriter->WriteArrayStart(TEXT("Players"));
			for (const APlayerState* Player : PlayerArray)
			{
				if (const ASCPlayerState_Hunt* PS = Cast<ASCPlayerState_Hunt>(Player))
				{
					ASCPlayerController* PC = Cast<ASCPlayerController>(PS->GetOwner());
					if (PC && PS->DidSpawnIntoMatch())
					{
						JsonWriter->WriteObjectStart(); // Player
						JsonWriter->WriteValue(TEXT("AccountID"), PC->GetBackendAccountId().GetIdentifier());

						JsonWriter->WriteArrayStart(TEXT("Badges"));
						for (TSubclassOf<USCStatBadge> Badge : PS->GetBadgesEarned())
						{
							if (const USCStatBadge* const StatBadge = Badge ? Badge->GetDefaultObject<USCStatBadge>() : nullptr)
								JsonWriter->WriteValue(StatBadge->BackendId);
						}
						JsonWriter->WriteArrayEnd(); // Badges

						const FSCPlayerStats PlayerMatchStats = PS->GetMatchStats();

						if (PS->IsActiveCharacterKiller())
						{
							JsonWriter->WriteObjectStart(TEXT("Jason"));

							FString KillerClassPlayed = TEXT("Unknown");
							if (PlayerMatchStats.JasonsPlayed.Num() > 0)
							{
								KillerClassPlayed = PlayerMatchStats.JasonsPlayed[0]->GetPathName();
								const int32 ClassPos = KillerClassPlayed.Find(TEXT("Jason/"));
								if (ClassPos != -1)
									KillerClassPlayed = KillerClassPlayed.RightChop(ClassPos + 6);
							}

							JsonWriter->WriteValue(TEXT("Model"), KillerClassPlayed);
							JsonWriter->WriteValue(TEXT("CounselorsKilled"), PlayerMatchStats.CounselorsKilled);
							JsonWriter->WriteValue(TEXT("EscapesAllowed"), NumEscaped);
							JsonWriter->WriteValue(TEXT("SurvivesAllowed"), NumSurvived);
							JsonWriter->WriteValue(TEXT("CounselorsFaced"), SpawnedCounselorCount);
							JsonWriter->WriteValue(TEXT("Deaths"), (int32)PS->GetIsDead());

							// Feats
							PlayerMatchStats.WriteJasonFeats(JsonWriter);

							// Items
							JsonWriter->WriteArrayStart(TEXT("Items"));
							for (const FSCItemStats ItemStats : PlayerMatchStats.ItemStats)
							{
								if (!IsValid(ItemStats.ItemClass.Get()))
									continue;

								JsonWriter->WriteObjectStart();
								FString ItemClass = ItemStats.ItemClass->GetPathName();
								const int32 ClassPos = ItemClass.Find(TEXT("Items/"));
								if (ClassPos != -1)
									ItemClass = ItemClass.RightChop(ClassPos + 6);
								JsonWriter->WriteValue(TEXT("ItemClass"), ItemClass);
								JsonWriter->WriteValue(TEXT("TimesPickedUp"), ItemStats.TimesPickedUp);
								JsonWriter->WriteValue(TEXT("TimesUsed"), ItemStats.TimesUsed);
								JsonWriter->WriteValue(TEXT("CounselorsHit"), ItemStats.CounselorsHit);
								JsonWriter->WriteValue(TEXT("JasonHits"), ItemStats.JasonHits);
								JsonWriter->WriteValue(TEXT("Kills"), ItemStats.Kills);
								JsonWriter->WriteValue(TEXT("Stuns"), ItemStats.Stuns);
								JsonWriter->WriteObjectEnd();
							}
							JsonWriter->WriteArrayEnd(); // Items

							// Kills
							JsonWriter->WriteArrayStart(TEXT("KillsPerformed"));
							for (FSCKillStats Kill : PlayerMatchStats.ContextKillsPerformed)
							{
								JsonWriter->WriteObjectStart();
								JsonWriter->WriteValue(TEXT("KillName"), Kill.KillName.ToString());
								JsonWriter->WriteValue(TEXT("KillCount"), Kill.KillCount);
								JsonWriter->WriteObjectEnd();
							}
							JsonWriter->WriteArrayEnd(); // Kills

							JsonWriter->WriteObjectEnd(); // Jason
						}
						else
						{
							JsonWriter->WriteObjectStart(TEXT("Counselor"));

							FString CounselorClassPlayed = TEXT("Unknown");
							if (PlayerMatchStats.CounselorsPlayed.Num() > 0)
							{
								CounselorClassPlayed = PlayerMatchStats.CounselorsPlayed[0]->GetPathName();
								const int32 ClassPos = CounselorClassPlayed.Find(TEXT("Counselors/"));
								if (ClassPos != -1)
									CounselorClassPlayed = CounselorClassPlayed.RightChop(ClassPos + 11);
							}

							JsonWriter->WriteValue(TEXT("Model"), CounselorClassPlayed);
							JsonWriter->WriteValue(TEXT("KilledAt"), PS->GetDeathTime());
							JsonWriter->WriteValue(TEXT("EscapedAt"), PS->GetEscapeTime());

							// Feats
							PlayerMatchStats.WriteCounselorFeats(JsonWriter);

							// Items
							JsonWriter->WriteArrayStart(TEXT("Items"));
							for (const FSCItemStats ItemStats : PlayerMatchStats.ItemStats)
							{
								JsonWriter->WriteObjectStart();
								FString ItemClass = ItemStats.ItemClass->GetPathName();
								const int32 ClassPos = ItemClass.Find(TEXT("Items/"));
								if (ClassPos != -1)
									ItemClass = ItemClass.RightChop(ClassPos + 6);
								JsonWriter->WriteValue(TEXT("ItemClass"), ItemClass);
								JsonWriter->WriteValue(TEXT("TimesPickedUp"), ItemStats.TimesPickedUp);
								JsonWriter->WriteValue(TEXT("TimesUsed"), ItemStats.TimesUsed);
								JsonWriter->WriteValue(TEXT("CounselorsHit"), ItemStats.CounselorsHit);
								JsonWriter->WriteValue(TEXT("JasonHits"), ItemStats.JasonHits);
								JsonWriter->WriteValue(TEXT("Kills"), ItemStats.Kills);
								JsonWriter->WriteValue(TEXT("Stuns"), ItemStats.Stuns);
								JsonWriter->WriteObjectEnd();
							}
							JsonWriter->WriteArrayEnd(); // Items

							JsonWriter->WriteObjectEnd(); // Counselor
						}

						JsonWriter->WriteObjectEnd(); // Player
					}
				}
			}
			JsonWriter->WriteArrayEnd(); // Players

			JsonWriter->WriteObjectEnd(); // Stats
			JsonWriter->Close();

			BRM->ReportMatchStats(Payload, FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ASCGameState_Hunt::OnMatchStatsPosted));
		}
	}
}

void ASCGameState_Hunt::HandleLeavingMap()
{
	Super::HandleLeavingMap();

	// Clear the SpawnedCharacterClass for everyone, so they are able to respawn on the next map
	for (APlayerState* Player : PlayerArray)
	{
		if (ASCPlayerState_Hunt* PS = Cast<ASCPlayerState_Hunt>(Player))
		{
			PS->SpawnedCharacterClass = nullptr;
		}
	}
}

void ASCGameState_Hunt::OnMatchStatsPosted(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("OnMatchStatsPosted: ResponseCode %i Reason: %s"), ResponseCode, *ResponseContents);
	}
}
