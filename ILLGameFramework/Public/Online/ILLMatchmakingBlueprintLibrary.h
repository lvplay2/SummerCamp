// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ILLMatchmakingBlueprintLibrary.generated.h"

/**
 * @class UILLMatchmakingBlueprintLibrary
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLMatchmakingBlueprintLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Starts matchmaking.
	 *
	 * @param bForLAN Perform LAN matchmaking?
	 * @return true if matchmaking was started.
	 */
	UFUNCTION(BlueprintCallable, Category="Matchmaking", meta=(WorldContext="WorldContextObject"))
	virtual bool BeginMatchmaking(UObject* WorldContextObject, const bool bForLAN);

	/** @return Running time for matchmaking in String form. */
	UFUNCTION(BlueprintPure, Category="Matchmaking", meta=(WorldContext="WorldContextObject"))
	static FString GetMatchmakingRunningTimeString(UObject* WorldContextObject, const bool bAlwaysShowMinute = true);

	/** @return Localized matchmaking status text. */
	UFUNCTION(BlueprintPure, Category="Matchmaking", meta=(WorldContext="WorldContextObject"))
	static FText GetMatchmakingStatusText(UObject* WorldContextObject);

	/** @return Remaining matchmaking ban time in seconds. */
	UFUNCTION(BlueprintPure, Category="Matchmaking", meta=(WorldContext="WorldContextObject"))
	static int32 GetMatchmakingBanRemainingSeconds(UObject* WorldContextObject);

	/** @return true if the local player is banned from matchmaking. */
	UFUNCTION(BlueprintPure, Category="Matchmaking", meta=(WorldContext="WorldContextObject"))
	static bool HasMatchmakingBan(UObject* WorldContextObject);

	/** @return true if the local player is in the low priority queue for matchmaking. */
	UFUNCTION(BlueprintPure, Category="Matchmaking", meta=(WorldContext="WorldContextObject"))
	static bool HasLowPriorityMatchmaking(UObject* WorldContextObject);
	
	/** @return true if the local player is matchmaking and leading a party or not in a party. */
	UFUNCTION(BlueprintPure, Category="Matchmaking", meta=(WorldContext="WorldContextObject"))
	static bool IsLeadingMatchmaking(UObject* WorldContextObject);

	/** @return true if the local player is matchmaking. */
	UFUNCTION(BlueprintPure, Category="Matchmaking", meta=(WorldContext="WorldContextObject"))
	static bool IsMatchmaking(UObject* WorldContextObject);
	UFUNCTION(BlueprintPure, Category="Matchmaking", meta=(WorldContext="WorldContextObject", DeprecatedFunction, DeprecationMessage="Please use IsMatchmaking instead"))
	static bool IsMatchmakingBlockingSessions(UObject* WorldContextObject) { return IsMatchmaking(WorldContextObject); }

	/** @return true if regionalized matchmaking is supported. */
	UFUNCTION(BlueprintPure, Category="Matchmaking", meta=(WorldContext="WorldContextObject"))
	static bool IsRegionalizedMatchmakingSupported(UObject* WorldContextObject);
};
