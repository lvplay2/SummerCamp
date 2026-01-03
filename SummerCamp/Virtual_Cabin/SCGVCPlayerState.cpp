// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "OnlineAchievementsInterface.h"
#include "OnlineEventsInterface.h"
#include "OnlineKeyValuePair.h"
#include "OnlineLeaderboardInterface.h"
#include "OnlinePresenceInterface.h"
#include "OnlineSubsystem.h"
#include "SCGVCPlayerState.h"

#define VCprint(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::White,text)


void ASCGVCPlayerState::UpdateAchievement(const FName AchievementName, const float AchievementProgress)
{
	UE_LOG(LogOnline, Verbose, TEXT("VC Player: Updating achievement %s with progress=%f"), *AchievementName.ToString(), AchievementProgress);
	VCprint("Update Achievement in state");
	if (AchievementProgress < 100.0f)
		return;

	ClientUpdateAchievement(AchievementName, AchievementProgress);
}

void ASCGVCPlayerState::ClientUpdateAchievement_Implementation(const FName AchievementName, const float AchievementProgress)
{
	UE_LOG(LogOnline, Verbose, TEXT("Client: Updating achievement %s with progress=%f"), *AchievementName.ToString(), AchievementProgress);
	VCprint("CLIENT Update Achievement in state");
	if (AchievementProgress < 100.0f)
		return;

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineAchievementsPtr Achievements = OnlineSub ? OnlineSub->GetAchievementsInterface() : nullptr;

	if (!Achievements.IsValid() || !UniqueId.IsValid())
	{
		UE_LOG(LogOnline, Verbose, TEXT("Client: Failed to update achievement %s, AchiementPtr or UniqueId is invalid"), *AchievementName.ToString());
		return;
	}

	// Write achievement progress
	FOnlineAchievementsWritePtr WriteObject = MakeShareable(new FOnlineAchievementsWrite);
	WriteObject->SetFloatStat(AchievementName, AchievementProgress);

	FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();
	Achievements->WriteAchievements(*UniqueId, WriteObjectRef);
}