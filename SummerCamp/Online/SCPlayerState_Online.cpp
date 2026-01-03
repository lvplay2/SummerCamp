// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerState_Online.h"

#include "SCAchievementInfo.h"
#include "SCCounselorCharacter.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCKillerCharacter.h"

ASCPlayerState_Online::ASCPlayerState_Online(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ASCPlayerState_Online::OnMatchEnded()
{
	Super::OnMatchEnded();

	TSubclassOf<ASCCharacter> CharacterClass = GetActiveCharacterClass();
	if (!CharacterClass) // You late joined. NO STATS FOR YOU!
		return;

	const ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>();
	if (IsActiveCharacterKiller())
	{
		MatchStats.JasonMatchesPlayed++;
		MatchStats.JasonsPlayed.AddUnique(TSoftClassPtr<ASCKillerCharacter>(CharacterClass));
	}
	else
	{
		MatchStats.CounselorMatchesPlayed++;
		MatchStats.CounselorsPlayed.AddUnique(TSoftClassPtr<ASCCounselorCharacter>(CharacterClass));

		// Currently the hunter, so also add the counselor we played as before
		if (GM && GM->GetHeroCharacterClass().Get() && GM->GetHeroCharacterClass().Get() == CharacterClass)
			MatchStats.CounselorsPlayed.AddUnique(TSoftClassPtr<ASCCounselorCharacter>(PickedCounselorClass));
	}

	if (Role == ROLE_Authority)
	{
		UpdateLeaderboardStats(LEADERBOARD_STAT_PLAYER_LEVEL, PlayerLevel);

		if (IsActiveCharacterKiller())
		{
			UpdateAchievement(ACH_FRIDAY_THE_13TH);

			const int32 TotalJasonMatches = MatchStats.JasonMatchesPlayed + PersistentStats.JasonMatchesPlayed;
			UpdateLeaderboardStats(LEADERBOARD_STAT_JASON_MATCHES, TotalJasonMatches);

			const float SequelProgress = ((float)TotalJasonMatches / (float)SCAchievements::SequelMatchCount) * 100.0f;
			UpdateAchievement(ACH_THE_SEQUEL, SequelProgress);

			const float FinalChapterProgress = ((float)TotalJasonMatches / (float)SCAchievements::FinalChapterMatchCount) * 100.0f;
			UpdateAchievement(ACH_FINAL_CHAPTER, FinalChapterProgress);

			// Check if we have played as every Jason
			int32 TotalJasonsPlayed = 0;
			const TArray<TSoftClassPtr<ASCKillerCharacter>>& KillerClasses = GM->GetAchievementTrackedKillerClasses();
			for (const TSoftClassPtr<ASCKillerCharacter> KillerClass : KillerClasses)
			{
				if (KillerClass.IsNull())
					continue;

				if (!MatchStats.JasonsPlayed.Contains(KillerClass) && !PersistentStats.JasonsPlayed.Contains(KillerClass))
					continue;

				TotalJasonsPlayed++;
			}

			const float SeenEveryMovieProgress = ((float)TotalJasonsPlayed / (float)KillerClasses.Num()) * 100.0f;
			UpdateAchievement(ACH_SEEN_EVERY_MOVIE, SeenEveryMovieProgress);
			UpdateLeaderboardStats(LEADERBOARD_STAT_SEEN_EVERY_MOVIE, TotalJasonsPlayed);
		}
		else
		{
			UpdateAchievement(ACH_SUMMERCAMP);

			const int32 TotalCounselorMatches = MatchStats.CounselorMatchesPlayed + PersistentStats.CounselorMatchesPlayed;
			UpdateLeaderboardStats(LEADERBOARD_STAT_COUNSELOR_MATCHES, TotalCounselorMatches);

			const float CampCounselorProgress = ((float)TotalCounselorMatches / (float)SCAchievements::CampCounselorMatchCount) * 100.0f;
			UpdateAchievement(ACH_CAMP_COUNSELOR, CampCounselorProgress);

			const float HeadCounselorProgress = ((float)TotalCounselorMatches / (float)SCAchievements::HeadCounselorMatchCount) * 100.0f;
			UpdateAchievement(ACH_HEAD_COUNSELOR, HeadCounselorProgress);

			// Check if we have played as every counselor
			int32 TotalCounselorsPlayed = 0;
			const TArray<TSoftClassPtr<ASCCounselorCharacter>>& CounselorClasses = GM->GetAchievementTrackedCounselorClasses();
			for (const TSoftClassPtr<ASCCounselorCharacter> CounselorClass : CounselorClasses)
			{
				if (CounselorClass.IsNull())
					continue;

				if (!MatchStats.CounselorsPlayed.Contains(CounselorClass) && !PersistentStats.CounselorsPlayed.Contains(CounselorClass))
					continue;

				TotalCounselorsPlayed++;
			}

			const float SuperFanProgress = ((float)TotalCounselorsPlayed / (float)CounselorClasses.Num()) * 100.0f;
			UpdateAchievement(ACH_SUPER_FAN, SuperFanProgress);
			UpdateLeaderboardStats(LEADERBOARD_STAT_SUPER_FAN, TotalCounselorsPlayed);

			// If playing as the flirt, and we were the only survivor
			ASCGameState* GS = GetWorld()->GetGameState<ASCGameState>();
			if (GM && GS && ActiveCharacterClass && (GM->GetFlirtCharacterClass().Get() == ActiveCharacterClass) && GS->IsOnlySurvivingCounselor(this))
			{
				UpdateLeaderboardStats(LEADERBOARD_STAT_SOLO_FLIRT_SURVIVES);
				UpdateAchievement(ACH_UNLIKELY_GIRL);
			}
		}
	}
}
