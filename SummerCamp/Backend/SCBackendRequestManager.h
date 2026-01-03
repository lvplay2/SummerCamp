// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLBackendRequestManager.h"
#include "SCBackendRequestManager.generated.h"

class USCBackendPlayer;

namespace InfractionTypes
{
	extern const FString LeftDuringIntroSequence;
	extern const FString HostAbandonedMatch;
	extern const FString JasonLeftWhileAlive;
	extern const FString JasonLeftWhileBeingKilled;
	extern const FString CounselorBetrayedCounselor;
	extern const FString CounselorLeftWhileAlive;
	extern const FString CounselorLeftWhileBeingKilled;
}

/**
 * @struct FSCNewsEntry
 */
USTRUCT(BlueprintType)
struct FSCNewsEntry
{
	GENERATED_USTRUCT_BODY()

	// News item identifier
	UPROPERTY(BlueprintReadOnly)
	FString NewsItemId;

	// Time this was posted to the server
	UPROPERTY(BlueprintReadOnly)
	FString PostedTime;

	// News headline
	UPROPERTY(BlueprintReadOnly)
	FString Headline;

	// News contents
	UPROPERTY(BlueprintReadOnly)
	FString Contents;
};

// Delegate fired for news requests
DECLARE_DELEGATE_TwoParams(FSCOnBackendNewsDelegate, TArray<FSCNewsEntry>& /*NewsEntries*/, int32 /*ArticleCount*/);

/**
 * @class USCBackendNewsHTTPRequestTask
 */
UCLASS()
class USCBackendNewsHTTPRequestTask
: public UILLBackendHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void AsyncPrepareResponse() override;
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

	// Begin UILLBackendHTTPRequestTask interface
	virtual FString AsyncGetURLParameters() const override { return URLParameters; }
	// End UILLBackendHTTPRequestTask interface

public:
	// Request parameters
	FString URLParameters;

	// Article count parsed from AsyncPrepareResponse
	int32 ArticleCount;

	// News entries parsed from AsyncPrepareResponse
	TArray<FSCNewsEntry> NewsEntries;

	// Callback delegate for completion
	FSCOnBackendNewsDelegate CompletionDelegate;
};

/**
 * @class USCBackendRequestManager
 */
UCLASS()
class SUMMERCAMP_API USCBackendRequestManager
: public UILLBackendRequestManager
{
	GENERATED_UCLASS_BODY()

	// Begin UILLBackendRequestManager interface
	virtual void Init() override;
	// End UILLBackendRequestManager interface

	/** Retrieve a player's account profile */
	void GetPlayerAccountProfile(const FILLBackendPlayerIdentifier& AccountId, FOnILLBackendSimpleRequestDelegate Callback);

	/** Add Experience to profile */
	void AddExperienceToProfile(const FILLBackendPlayerIdentifier& AccountId, const int32 Experience, FOnILLBackendSimpleRequestDelegate Callback);

	/** Unlock achievement */
	void UnlockAchievement(const FILLBackendPlayerIdentifier& AccountId, const FString& AchievementID, FOnILLBackendSimpleRequestDelegate Callback);

	/** Reports AccountID for InfractionID. */
	void ReportInfraction(const FILLBackendPlayerIdentifier& AccountId, const FString& InfractionID);

	/** Single player Challenge Profile */
	void GetPlayerChallengeProfile(const FILLBackendPlayerIdentifier& AccountId, FOnILLBackendSimpleRequestDelegate Callback);

	/** Single player Challenge Complete */
	void ReportChallengeComplete(const FILLBackendPlayerIdentifier& AccountId, const FString& ChallengeData, FOnILLBackendSimpleRequestDelegate Callback);

	/** Purchases Items */
	bool PurchaseItem(USCBackendPlayer* Player, const FString& SetNameString, const FString& ItemID, FOnILLBackendSimpleRequestDelegate Callback);

	/** Purchases Perk Roll */
	bool PurchasePerkRoll(USCBackendPlayer* Player, FOnILLBackendSimpleRequestDelegate Callback);

	/** Sells back the Perk. */
	bool SellBackPerk(USCBackendPlayer* Player, const FString& InstanceID, FOnILLBackendSimpleRequestDelegate Callback);

	/** Report match stats */
	void ReportMatchStats(const FString& MatchStats, FOnILLBackendSimpleRequestDelegate Callback);

	/** Retrieve a player's persistent stats */
	void GetPlayerStats(const FILLBackendPlayerIdentifier& AccountId, FOnILLBackendSimpleRequestDelegate Callback);

	/** Retrieve a player's achievements (badges, etc...) */
	void GetPlayerAchievements(const FILLBackendPlayerIdentifier& AccountId, FOnILLBackendSimpleRequestDelegate Callback);

	/** Gets the current Active Pamela Tape. */
	void GetActivePamelaTapes(FOnILLBackendSimpleRequestDelegate Callback);

	/** Gets the current Active Tommy Tape. */
	void GetActiveTommyTapes(FOnILLBackendSimpleRequestDelegate Callback);

	/** Adds the found Pamela Tape to the players inventory. */
	void AddPamelaTapeToInventory(const FILLBackendPlayerIdentifier& AccountId, const FString& PamelaTapeID, FOnILLBackendSimpleRequestDelegate Callback);

	/** Adds the found Tommy Tape to the players inventory. */
	void AddTommyTapeToInventory(const FILLBackendPlayerIdentifier& AccountId, const FString& TommyTapeID, FOnILLBackendSimpleRequestDelegate Callback);

	/** Some lazy asshole put me in here without bothering to type a sentence. Probably that jerk Dan. */
	void GetModifiers(FOnILLBackendSimpleRequestDelegate Callback);

	/** Requests to get the news from the backend. */
	void GetNews(FSCOnBackendNewsDelegate Callback);

	// Registered endpoint handles
	FILLBackendEndpointHandle AuthService_GetAccount;
	FILLBackendEndpointHandle ArbiterService_JasonSelect;
	FILLBackendEndpointHandle ProfileAuthority_AddExperience;
	FILLBackendEndpointHandle ProfileAuthority_AchievementsUnlock;
	FILLBackendEndpointHandle ProfileAuthority_AddInfraction;
	FILLBackendEndpointHandle ProfileAuthority_GetChallengeProfile;
	FILLBackendEndpointHandle ProfileAuthority_ReportChallengeComplete;
	FILLBackendEndpointHandle ProfileAuthority_PushPurchase;
	FILLBackendEndpointHandle ProfileAuthority_PushPurchasePerkRoll;
	FILLBackendEndpointHandle ProfileAuthority_CashInPerk;
	FILLBackendEndpointHandle ProfileAuthority_ReportMatchStats;
	FILLBackendEndpointHandle ProfileAuthority_GetStats;
	FILLBackendEndpointHandle ProfileAuthority_GetAchievements;
	FILLBackendEndpointHandle ProfileAuthority_GetActivePamelaTapes;
	FILLBackendEndpointHandle ProfileAuthority_AddPamelaTape;
	FILLBackendEndpointHandle ProfileAuthority_GetActiveTommyTapes;
	FILLBackendEndpointHandle ProfileAuthority_AddTommyTape;
	FILLBackendEndpointHandle ProfileAuthority_GetModifiers;
	FILLBackendEndpointHandle NewsService_GetNews;
};
