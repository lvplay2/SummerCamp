// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCStatsAndScoringData.h"

#include "JsonUtilities.h"

#include "SCGameMode.h"
#include "SCItem.h"
#include "SCPerkData.h"
#include "SCPlayerController.h"
#include "SCPlayerState.h"

USCStatBadge::USCStatBadge(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

USCScoringEvent::USCScoringEvent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

USCParanoiaScore::USCParanoiaScore(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

FString USCScoringEvent::GetDisplayMessage(const float Modifier/* = 1.f*/, const TArray<USCPerkData*>& PerkModifiers/* = TArray<USCPerkData*>()*/) const
{
	FString Message = EventDescription.ToString();

	if (Message.Contains(TEXT("<exp>")))
	{
		int32 EXP = Experience * Modifier;
		for (const USCPerkData* Perk : PerkModifiers)
		{
			EXP += EXP * Perk->XPModifier;
		}
		Message = Message.Replace(TEXT("<exp>"), *FString::FromInt(EXP));
	}

	return Message;
}

int32 USCScoringEvent::GetModifiedExperience(const float Modifier, const TArray<USCPerkData*>& PerkModifiers) const
{
	int32 EXP = Experience * Modifier;
	for (const USCPerkData* Perk : PerkModifiers)
	{
		EXP += EXP * Perk->XPModifier;
	}

	return EXP;
}

FString USCParanoiaScore::GetDisplayMessage(const float Modifier/* = 1.f*/, const TArray<USCPerkData*>& PerkModifiers/* = TArray<USCPerkData*>()*/) const
{
	FString Message = ScoreDescription.ToString();

	if (Message.Contains(TEXT("<pts>")))
	{
		int32 PTS = Points * Modifier;
		for (const USCPerkData* Perk : PerkModifiers)
		{
			// Uh, Lets not use XPModifiers for points.
			// PTS += PTS * Perk->XPModifier;
		}
		Message = Message.Replace(TEXT("<pts>"), *FString::FromInt(PTS));
	}

	return Message;
}

int32 USCParanoiaScore::GetModifiedScore(const float Modifier, const TArray<USCPerkData*>& PerkModifiers) const
{
	int32 PTS = Points * Modifier;
	for (const USCPerkData* Perk : PerkModifiers)
	{
		// Uh, Lets not use XPModifiers for points.
		// PTS += PTS * Perk->XPModifier;
	}

	return PTS;
}

void FSCPlayerStats::ParseStats(const FString& JsonString, const ASCPlayerState* OwningPlayer)
{
	// Parse Json
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		// Get Stats Object
		const TSharedPtr<FJsonObject>* StatsData;
		if (JsonObject->TryGetObjectField(TEXT("StatsProfile"), StatsData))
		{
			// Jason Stats
			const TSharedPtr<FJsonObject>* JasonData;
			if ((*StatsData)->TryGetObjectField(TEXT("Jason"), JasonData))
			{
				(*JasonData)->TryGetNumberField(TEXT("MatchesPlayed"), JasonMatchesPlayed);
				(*JasonData)->TryGetNumberField(TEXT("CounselorsKilled"), CounselorsKilled);
				(*JasonData)->TryGetNumberField(TEXT("Deaths"), JasonDeathsByCounselors);

				// Kills performed
				const TArray<TSharedPtr<FJsonValue>>* KillsPerformed = nullptr;
				if ((*JasonData)->TryGetArrayField(TEXT("KillsPerformed"), KillsPerformed))
				{
					ContextKillsPerformed.Empty();
					if (!FJsonObjectConverter::JsonArrayToUStruct(*KillsPerformed, &ContextKillsPerformed, 0, 0))
					{
						UE_LOG(LogSC, Warning, TEXT("ParseStats: Failed to load preformed kills from the backend."));
					}
				}

				// Jason classes played
				const TArray<TSharedPtr<FJsonValue>>* JasonModels = nullptr;
				if ((*JasonData)->TryGetArrayField(TEXT("Models"), JasonModels))
				{
					TArray<TSoftClassPtr<ASCKillerCharacter>> AchievementKillers;
					if (ASCGameMode* GM = OwningPlayer->GetWorld()->GetAuthGameMode<ASCGameMode>())
						AchievementKillers = GM->GetAchievementTrackedKillerClasses();

					JasonsPlayed.Empty();
					for (TSharedPtr<FJsonValue> JasonModel : *JasonModels)
					{
						const TSharedPtr<FJsonObject>* ModelData;
						if (JasonModel->TryGetObject(ModelData))
						{
							FString KillerCharacter;
							(*ModelData)->TryGetStringField(TEXT("Model"), KillerCharacter);
							int32 SlashPosition;

							if (KillerCharacter.FindChar('/', SlashPosition))
							{
								const FString KillerPath = KillerCharacter.Left(SlashPosition);
								const FString KillerClassname = KillerCharacter.Mid(SlashPosition + 1);
								const bool bHasClassPath = KillerCharacter.Contains(TEXT("Game/Characters/Killers/Jason/"));

								const FString ClassString = bHasClassPath ? KillerCharacter : FString::Printf(TEXT("/Game/Characters/Killers/Jason/%s/%s"), *KillerPath, *KillerClassname);
								TSoftClassPtr<ASCKillerCharacter> KillerClass = TSoftClassPtr<ASCKillerCharacter>(FSoftObjectPath(ClassString));
								if (AchievementKillers.Contains(KillerClass))
								{
									JasonsPlayed.Add(KillerClass);
								}
							}
						}
					}
				}
			}

			// Counselor Stats
			const TSharedPtr<FJsonObject>* CounselorData;
			if ((*StatsData)->TryGetObjectField(TEXT("Counselor"), CounselorData))
			{
				(*CounselorData)->TryGetNumberField(TEXT("MatchesPlayed"), CounselorMatchesPlayed);

				// Counselor classes played
				const TArray<TSharedPtr<FJsonValue>>* CounselorModels = nullptr;
				if ((*CounselorData)->TryGetArrayField(TEXT("Models"), CounselorModels))
				{
					TArray<TSoftClassPtr<ASCCounselorCharacter>> AchievementCounselors;
					if (ASCGameMode* GM = OwningPlayer->GetWorld()->GetAuthGameMode<ASCGameMode>())
						AchievementCounselors = GM->GetAchievementTrackedCounselorClasses();

					CounselorsPlayed.Empty();
					for (TSharedPtr<FJsonValue> CounselorModel : *CounselorModels)
					{
						const TSharedPtr<FJsonObject>* ModelData;
						if (CounselorModel->TryGetObject(ModelData))
						{
							FString CounselorCharacter;
							(*ModelData)->TryGetStringField(TEXT("Model"), CounselorCharacter);
							int32 SlashPosition;

							if (CounselorCharacter.FindChar('/', SlashPosition))
							{
								const FString CounselorPath = CounselorCharacter.Left(SlashPosition);
								const FString CounselorClassname = CounselorCharacter.Mid(SlashPosition + 1);
								const bool bHasClassPath = CounselorCharacter.Contains(TEXT("Game/Characters/Counselors/"));

								const FString ClassString = bHasClassPath ? CounselorCharacter : FString::Printf(TEXT("/Game/Characters/Counselors/%s/%s"), *CounselorPath, *CounselorClassname);
								TSoftClassPtr<ASCCounselorCharacter> CounselorClass = TSoftClassPtr<ASCCounselorCharacter>(FSoftObjectPath(ClassString));
								if (AchievementCounselors.Contains(CounselorClass))
								{
									CounselorsPlayed.Add(CounselorClass);
								}
							}
						}
					}
				}
			}

			// Feats
			const TSharedPtr<FJsonObject>* Feats;
			if ((*StatsData)->TryGetObjectField(TEXT("Feats"), Feats))
			{
				// Jason Feats
				(*Feats)->TryGetNumberField(TEXT("CarSlams"), CarSlamCount);
				(*Feats)->TryGetNumberField(TEXT("DoorsBrokenDown"), DoorsBrokenDown);
				(*Feats)->TryGetNumberField(TEXT("ThrowingKnifeHits"), ThrowingKnifeHits);
				(*Feats)->TryGetNumberField(TEXT("WallsBrokenDown"), WallsBrokenDown);

				// Counselor Feats
				(*Feats)->TryGetNumberField(TEXT("BoatRepairs"), BoatPartsRepaired);
				(*Feats)->TryGetNumberField(TEXT("CarRepairs"), CarPartsRepaired);
				(*Feats)->TryGetNumberField(TEXT("ClosedWindowsJumps"), ClosedWindowsJumpedThrough);
				(*Feats)->TryGetNumberField(TEXT("DeathsByJason"), DeathsByJason);
				(*Feats)->TryGetNumberField(TEXT("DeathsByCounselors"), DeathsByOtherCounselors);
				(*Feats)->TryGetNumberField(TEXT("BoatDriverEscapes"), EscapedInBoatAsDriverCount);
				(*Feats)->TryGetNumberField(TEXT("BoatPassengerEscapes"), EscapedInBoatAsPassengerCount);
				(*Feats)->TryGetNumberField(TEXT("CarDriverEscapes"), EscapedInCarAsDriverCount);
				(*Feats)->TryGetNumberField(TEXT("CarPassengerEscapes"), EscapedInCarAsPassengerCount);
				(*Feats)->TryGetNumberField(TEXT("PoliceEscapes"), EscapedWithPoliceCount);
				(*Feats)->TryGetNumberField(TEXT("GridRepairs"), GridPartsRepaired);
				(*Feats)->TryGetNumberField(TEXT("JasonKills"), KilledJasonCount);
				(*Feats)->TryGetNumberField(TEXT("OtherCounselorsKilled"), KilledAnotherCounselorCount);
				(*Feats)->TryGetNumberField(TEXT("KnockedJasonMaskOff"), KnockedJasonMaskOffCount);
				(*Feats)->TryGetNumberField(TEXT("PoliceCalled"), PoliceCallCount);
				(*Feats)->TryGetNumberField(TEXT("SweaterStuns"), SweaterStunCount);
				(*Feats)->TryGetNumberField(TEXT("TommyCalled"), TommyJarvisCallCount);
				(*Feats)->TryGetNumberField(TEXT("TommyMatchesPlayed"), TommyJarvisMatchesPlayed);
			}

			// Item stats
			const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
			if ((*StatsData)->TryGetArrayField(TEXT("Items"), Items))
			{
				ItemStats.Empty();
				for (TSharedPtr<FJsonValue> Item : *Items)
				{
					const TSharedPtr<FJsonObject>* ItemObject;
					if (Item->TryGetObject(ItemObject))
					{
						FSCItemStats ItemInfo;
						FString ItemClass;
						(*ItemObject)->TryGetStringField(TEXT("ItemClass"), ItemClass);
						const bool bHasClassPath = ItemClass.Contains(TEXT("Game/Blueprints/Items/"));

						FString ClassString = bHasClassPath ? ItemClass : FString::Printf(TEXT("/Game/Blueprints/Items/%s"), *ItemClass);
						ClassString.ReplaceInline(TEXT("//"), TEXT("/")); // FIXME: pjackson: HACK! Somehow double-slashes are getting into the backend
						if (UClass* LoadedItem = LoadObject<UClass>(nullptr, *ClassString))
						{
							ItemInfo.ItemClass = LoadedItem;
							(*ItemObject)->TryGetNumberField(TEXT("TimesPickedUp"), ItemInfo.TimesPickedUp);
							(*ItemObject)->TryGetNumberField(TEXT("TimesUsed"), ItemInfo.TimesUsed);
							(*ItemObject)->TryGetNumberField(TEXT("CounselorsHit"), ItemInfo.CounselorsHit);
							(*ItemObject)->TryGetNumberField(TEXT("JasonHits"), ItemInfo.JasonHits);
							(*ItemObject)->TryGetNumberField(TEXT("Kills"), ItemInfo.Kills);
							(*ItemObject)->TryGetNumberField(TEXT("Stuns"), ItemInfo.Stuns);

							ItemStats.Add(ItemInfo);
						}
						else
						{
							UE_LOG(LogSC, Warning, TEXT("ParseStats: Failed to load item %s from the backend."), *ClassString);
						}
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogSC, Warning, TEXT("ParseStats: Failed to parse JSON from backend. JSON Data: %s"), *JsonString);
	}
}

void FSCPlayerStats::WriteJasonFeats(TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>>& JsonWriter) const
{
	JsonWriter->WriteObjectStart(TEXT("Feats"));
	if (CarSlamCount > 0)
		JsonWriter->WriteValue(TEXT("CarSlams"), CarSlamCount);
	if (DoorsBrokenDown > 0)
		JsonWriter->WriteValue(TEXT("DoorsBrokenDown"), DoorsBrokenDown);
	if (ThrowingKnifeHits > 0)
		JsonWriter->WriteValue(TEXT("ThrowingKnifeHits"), ThrowingKnifeHits);
	if (WallsBrokenDown > 0)
		JsonWriter->WriteValue(TEXT("WallsBrokenDown"), WallsBrokenDown);
	if (Suicides > 0)
		JsonWriter->WriteValue(TEXT("Suicides"), Suicides);
	JsonWriter->WriteObjectEnd();
}

void FSCPlayerStats::WriteCounselorFeats(TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>>& JsonWriter) const
{
	JsonWriter->WriteObjectStart(TEXT("Feats"));
	if (BoatPartsRepaired > 0)
		JsonWriter->WriteValue(TEXT("BoatRepairs"), BoatPartsRepaired);
	if (CarPartsRepaired > 0)
		JsonWriter->WriteValue(TEXT("CarRepairs"), CarPartsRepaired);
	if (ClosedWindowsJumpedThrough > 0)
		JsonWriter->WriteValue(TEXT("ClosedWindowsJumps"), ClosedWindowsJumpedThrough);
	if (DeathsByJason > 0)
		JsonWriter->WriteValue(TEXT("DeathsByJason"), DeathsByJason);
	if (DeathsByOtherCounselors > 0)
		JsonWriter->WriteValue(TEXT("DeathsByCounselors"), DeathsByOtherCounselors);
	if (EscapedInBoatAsDriverCount > 0)
		JsonWriter->WriteValue(TEXT("BoatDriverEscapes"), EscapedInBoatAsDriverCount);
	if (EscapedInBoatAsPassengerCount > 0)
		JsonWriter->WriteValue(TEXT("BoatPassengerEscapes"), EscapedInBoatAsPassengerCount);
	if (EscapedInCarAsDriverCount > 0)
		JsonWriter->WriteValue(TEXT("CarDriverEscapes"), EscapedInCarAsDriverCount);
	if (EscapedInCarAsPassengerCount > 0)
		JsonWriter->WriteValue(TEXT("CarPassengerEscapes"), EscapedInCarAsPassengerCount);
	if (EscapedWithPoliceCount > 0)
		JsonWriter->WriteValue(TEXT("PoliceEscapes"), EscapedWithPoliceCount);
	if (GridPartsRepaired > 0)
		JsonWriter->WriteValue(TEXT("GridRepairs"), GridPartsRepaired);
	if (KilledJasonCount > 0)
		JsonWriter->WriteValue(TEXT("JasonKills"), KilledJasonCount);
	if (KilledAnotherCounselorCount > 0)
		JsonWriter->WriteValue(TEXT("OtherCounselorsKilled"), KilledAnotherCounselorCount);
	if (KnockedJasonMaskOffCount > 0)
		JsonWriter->WriteValue(TEXT("KnockedJasonMaskOff"), KnockedJasonMaskOffCount);
	if (PoliceCallCount > 0)
		JsonWriter->WriteValue(TEXT("PoliceCalled"), PoliceCallCount);
	if (SweaterStunCount > 0)
		JsonWriter->WriteValue(TEXT("SweaterStuns"), SweaterStunCount);
	if (TommyJarvisCallCount > 0)
		JsonWriter->WriteValue(TEXT("TommyCalled"), TommyJarvisCallCount);
	if (TommyJarvisMatchesPlayed > 0)
		JsonWriter->WriteValue(TEXT("TommyMatchesPlayed"), TommyJarvisMatchesPlayed);
	if (Suicides > 0)
		JsonWriter->WriteValue(TEXT("Suicides"), Suicides);
	JsonWriter->WriteObjectEnd();
}

void FSCPlayerStats::PerformedKill(const FName KillName)
{
	const int32 KillIndex = GetContextKillIndex(KillName);
	if (KillIndex != -1)
		ContextKillsPerformed[KillIndex].KillCount++;
	else
		ContextKillsPerformed.Add(FSCKillStats(KillName, 1));
}

bool FSCPlayerStats::HasPerformedKill(const FName KillName)
{
	for (const FSCKillStats KillStat : ContextKillsPerformed)
	{
		if (KillStat.KillName == KillName)
			return true;
	}

	return false;
}

int32 FSCPlayerStats::GetContextKillIndex(const FName KillName)
{
	for (int i = 0; i < ContextKillsPerformed.Num(); ++i)
	{
		if (ContextKillsPerformed[i].KillName == KillName)
			return i;
	}

	return -1;
}

int32 FSCPlayerStats::GetItemStatsIndex(TSubclassOf<ASCItem> ItemClass) const
{
	for (int i = 0; i < ItemStats.Num(); ++i)
	{
		if (ItemStats[i].ItemClass == ItemClass)
			return i;
	}

	return -1;
}

FSCItemStats FSCPlayerStats::GetItemStats(TSubclassOf<ASCItem> ItemClass) const
{
	const int32 ItemIndex = GetItemStatsIndex(ItemClass);
	if (ItemIndex != -1)
		return ItemStats[ItemIndex];

	return FSCItemStats(ItemClass);
}

FSCItemStats::FSCItemStats(TSubclassOf<ASCItem> Item, uint8 StatFlags /*= 0*/)
: ItemClass(Item)
{
	TimesPickedUp = TimesUsed = CounselorsHit = JasonHits = Kills = Stuns = 0;

	if (StatFlags & (uint8)ESCItemStatFlags::PickedUp)
		TimesPickedUp = 1;
	if (StatFlags & (uint8)ESCItemStatFlags::Used)
		TimesUsed = 1;
	if (StatFlags & (uint8)ESCItemStatFlags::HitCounselor)
		CounselorsHit = 1;
	if (StatFlags & (uint8)ESCItemStatFlags::HitJason)
		JasonHits = 1;
	if (StatFlags & (uint8)ESCItemStatFlags::Kill)
		Kills = 1;
	if (StatFlags & (uint8)ESCItemStatFlags::Stun)
		Stuns = 1;
}

FSCItemStats& FSCItemStats::operator +=(const FSCItemStats& OtherItem)
{
	if (OtherItem.ItemClass == ItemClass)
	{
		TimesPickedUp += OtherItem.TimesPickedUp;
		TimesUsed += OtherItem.TimesUsed;
		CounselorsHit += OtherItem.CounselorsHit;
		JasonHits += OtherItem.JasonHits;
		Kills += OtherItem.Kills;
		Stuns += OtherItem.Stuns;
	}

	return *this;
}