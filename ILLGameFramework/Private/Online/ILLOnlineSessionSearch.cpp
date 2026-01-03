// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLOnlineSessionSearch.h"

#include "OnlineSubsystem.h"

#include "ILLGameOnlineBlueprintLibrary.h"
#include "ILLOnlineSessionClient.h"

FILLOnlineSessionSearch::FILLOnlineSessionSearch()
: bSkipQuerySettingsEvaluation(false)
{
	// Remove unwanted default SearchParams from the Super
	QuerySettings.SearchParams.Empty();
}

void FILLOnlineSessionSearch::SortSearchResults()
{
	if (!bSkipQuerySettingsEvaluation)
	{
		for (int32 ResultIndex = 0; ResultIndex < SearchResults.Num(); )
		{
			bool bShouldKeep = true;
			const FOnlineSessionSearchResult& Result = SearchResults[ResultIndex];
			for (FSearchParams::TConstIterator SearchIt(QuerySettings.SearchParams); SearchIt; ++SearchIt)
			{
				const FName Key = SearchIt.Key();
				if (Key == SEARCH_PRESENCE)
					continue;
#ifdef SETTING_PLAYERCOUNT
				// HACKish: pjackson: Skip ensuring the player count matches after results are pinged, since UILLOnlineSessionClient::FindBestSessionResult should only accept servers with enough room for our party size
				if (Key == SETTING_PLAYERCOUNT)
					continue;
#endif

				const FOnlineSessionSearchParam& SearchParam = SearchIt.Value();
				const FOnlineSessionSetting* MatchingSetting = Result.Session.SessionSettings.Settings.Find(Key);
				if (MatchingSetting)
				{
					switch (SearchParam.ComparisonOp)
					{
					case EOnlineComparisonOp::NotEquals:
						bShouldKeep = (MatchingSetting->Data != SearchParam.Data);
						break;
					case EOnlineComparisonOp::GreaterThan:
					case EOnlineComparisonOp::GreaterThanEquals:
					case EOnlineComparisonOp::LessThan:
					case EOnlineComparisonOp::LessThanEquals:
						// TODO: pjackson: Unused for now, so we can get away with it...
						check(0);
						break;
					case EOnlineComparisonOp::Near:
					case EOnlineComparisonOp::Equals:
					default:
						bShouldKeep = (MatchingSetting->Data == SearchParam.Data);
						break;
					}
				}

				if (!bShouldKeep)
				{
					UE_LOG(LogOnlineGame, Verbose, TEXT("Filtered session %s because %s !(%s %s)"), *Result.Session.OwningUserName, *Key.ToString(), EOnlineComparisonOp::ToString(SearchParam.ComparisonOp), *SearchParam.Data.ToString());
					SearchResults.RemoveAt(ResultIndex);
					break;
				}
			}

			if (bShouldKeep)
			{
				++ResultIndex;
			}
		}
	}

	// Sort all results by ping, lowest first, as a general baseline and to optimize matchmaking
	struct FCompareSessionByPing
	{
		FCompareSessionByPing() {}

		FORCEINLINE bool operator()(const FOnlineSessionSearchResult& A, const FOnlineSessionSearchResult& B) const
		{
			return A.PingInMs < B.PingInMs;
		}
	};
	SearchResults.Sort(FCompareSessionByPing());
}
