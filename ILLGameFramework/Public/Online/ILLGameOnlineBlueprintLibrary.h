// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

// Needed for FBlueprintSessionResult
#include "OnlineBlueprintCallProxyBase.h"
#include "FindSessionsCallbackProxy.h"

// Needed for IOnlineIdentity::EPrivilegeResults
#include "OnlineIdentityInterface.h"

#include "ILLGameOnlineBlueprintLibrary.generated.h"

class AILLPartyBeaconMemberState;
class AILLPlayerController;
struct FBlueprintSessionResult;

// Is anti-cheat blocking online matches?
extern ILLGAMEFRAMEWORK_API bool GAntiCheatBlockingOnlineMatches;

/**
 * @enum ESessionSortColumn
 */
UENUM(BlueprintType)
enum class ESessionSortColumn : uint8
{
	Name,
	Map,
	Mode,
	Players,
	Ping
};

/**
 * @class UILLGameOnlineBlueprintLibrary
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLGameOnlineBlueprintLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	//////////////////////////////////////////////////
	// Online availability
public:
	/** @return true if PlayerController has completed the platform login process. */
	UFUNCTION(BlueprintPure, Category="Privileges")
	static bool HasPlatformLogin(AILLPlayerController* PlayerController);

	/** @return true if PlayOnline privilege is available to PlayerController. */
	UFUNCTION(BlueprintPure, Category="Privileges")
	static bool HasPlayOnlinePrivilege(AILLPlayerController* PlayerController);

	/** @return true if PlayerController has PrivilegeFlags in their PlayOnline privilege flags. */
	static bool HasPlayOnlineFailureFlags(AILLPlayerController* PlayerController, IOnlineIdentity::EPrivilegeResults PrivilegeFlags);
	
	/** @return true if offline matches in general are blocked by some mutually exclusive state. */
	UFUNCTION(BlueprintPure, Category="Privileges")
	static bool ShouldEnableOfflineMatches(AILLPlayerController* PlayerController);

	/** @return true if online matches in general are blocked by some mutually exclusive state. This is used by ShouldEnableLANMatch, ShouldEnablePrivateMatch and ShouldEnableQuickPlay. */
	UFUNCTION(BlueprintPure, Category="Privileges")
	static bool ShouldEnableOnlineMatches(AILLPlayerController* PlayerController);

	/** @return true if LAN Match functionality should be enabled. */
	UFUNCTION(BlueprintPure, Category="Privileges")
	static bool ShouldEnableLANMatch(AILLPlayerController* PlayerController);

	/** @return true if Private Match functionality should be enabled. */
	UFUNCTION(BlueprintPure, Category="Privileges")
	static bool ShouldEnablePrivateMatch(AILLPlayerController* PlayerController);

	/** @return true if QuickPlay functionality should be enabled. */
	UFUNCTION(BlueprintPure, Category="Privileges")
	static bool ShouldEnableQuickPlay(AILLPlayerController* PlayerController);

	/** @return Display name of the last input event. Useful for title screens. Hiding it after login starts recommended. */
	UFUNCTION(BlueprintPure, Category="Profile")
	static FString GetLastInputNickname(AILLPlayerController* PlayerController);

	/**
	 * Signs a user out of the platform and backend, before reloading the default map to show the title screen.
	 *
	 * @param PlayerController Player to sign out.
	 * @param FailureText Optional failure reason. When empty no error dialog should display.
	 */
	UFUNCTION(BlueprintCallable, Category="Profile")
	static void SignOutAndDisconnect(AILLPlayerController* PlayerController, FText FailureText);

	//////////////////////////////////////////////////
	// Overlays
public:
	/**
	 * Displays the account upgrade UI if available.
	 *
	 * @param PlayerController Player to display the UI for.
	 * @return true if the account update UI was opened.
	 */
	UFUNCTION(BlueprintCallable, Category="Overlays")
	static bool ShowAccountUpgradeUI(AILLPlayerController* PlayerController);

	/**
	 * Displays the account upgrade UI if PlayerController lacks online privileges due to no PSPlus (IOnlineIdentity::EPrivilegeResults::AccountTypeFailure).
	 *
	 * @param PlayerController Player to display the UI for.
	 * @return true if the account update UI was opened.
	 */
	UFUNCTION(BlueprintCallable, Category="Overlays")
	static bool ConditionalShowAccountUpgradeUI(AILLPlayerController* PlayerController);

	/** Opens the external invite UI for PlayerController for our NAME_GameSession. */
	UFUNCTION(BlueprintCallable, Category="Overlays", meta=(DeprecatedFunction, DeprecationMessage="Function is deprecated, use ShowExternalInviteUI instead."))
	static bool ShowExternalGameInviteUI(AILLPlayerController* PlayerController);

	/**
	 * Opens the external invite UI for PlayerController for our "Now Playing" session.
	 * When in a Party, then that is the "Now Playing" session. Otherwise it is the Game session.
	 */
	UFUNCTION(BlueprintCallable, Category="Overlays")
	static bool ShowExternalInviteUI(AILLPlayerController* PlayerController);

	/** Opens WebURL in an OSS external UI. */
	UFUNCTION(BlueprintCallable, Category="Overlays")
	static bool ShowExternalWebUI(const FString& WebURL);

	/** Opens the profile external UI for Requestee, requested for Requestor. */
	UFUNCTION(BlueprintCallable, Category="Overlays")
	static bool ShowExternalProfileUI(AILLPlayerState* Requestor, AILLPlayerState* Requestee);
	UFUNCTION(BlueprintCallable, Category="Overlays")
	static bool ShowPartyMemberExternalProfileUI(AILLPlayerState* Requestor, AILLPartyBeaconMemberState* Requestee);

	UFUNCTION(BlueprintCallable, Category="Overlays", meta=(DeprecatedFunction, DeprecationMessage="Use ShowExternalWebUI instead"))
	static bool ShowExternalUIWebURL(const FString& WebURL) { return ShowExternalWebUI(WebURL); }
	UFUNCTION(BlueprintCallable, Category="Overlays", meta=(DeprecatedFunction, DeprecationMessage="Use ShowExternalWebUI instead"))
	static bool ShowURL(const FString& WebURL) { return ShowExternalWebUI(WebURL); }

	//////////////////////////////////////////////////
	// Current game session
public:
	/** @return true if we can invite people to our current game session. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static bool CanInviteToCurrentGameSession(UObject* WorldContextObject);

	/** @return Current game session name. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static FString GetCurrentGameSessionName(UObject* WorldContextObject);

	/** @return Current game session player count, or -1 if not available. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static int32 GetCurrentGameSessionPlayerCount(UObject* WorldContextObject);

	/** @return Current game session player count, or -1 if not available. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static int32 GetCurrentGameSessionPlayerMax(UObject* WorldContextObject);

	/** @return true if the current NAME_GameSession is anti-cheat protected. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static bool IsCurrentGameSessionAntiCheatProtected(UObject* WorldContextObject);

	/** @return true if the current NAME_GameSession is hosted by a dedicated server. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static bool IsCurrentGameSessionDedicated(UObject* WorldContextObject);

	/** @return true if the current NAME_GameSession is full. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static bool IsCurrentGameSessionFull(UObject* WorldContextObject);

	/** @return true if the current NAME_GameSession we are in is a LAN session. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static bool IsCurrentGameSessionLAN(UObject* WorldContextObject);

	/** @return true if the current NAME_GameSession we are in is a private (not advertised) session. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static bool IsCurrentGameSessionPrivate(UObject* WorldContextObject);

	/** @return true if there is no current NAME_GameSession because we are a standalone. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static bool IsCurrentGameSessionStandalone(UObject* WorldContextObject);
	
	/** @return true if a local player is the game session server. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static bool IsLocalPlayerGameSessionServer(UObject* WorldContextObject);

	//////////////////////////////////////////////////
	// Game session parsing
public:
	/** @return Game mode that Result is loaded into. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static FString GetGameModeSetting(const FBlueprintSessionResult& Result);

	/** @return Map name that Result is loaded into. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static FString GetMapNameSetting(const FBlueprintSessionResult& Result);

	/** @return Maximum of players in Result. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static int32 GetMaxPlayers(const FBlueprintSessionResult& Result);

	/** @return Number of players in Result. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static int32 GetNumPlayers(const FBlueprintSessionResult& Result);

	/** @return true if Result is secure. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static bool IsSessionAntiCheatProtected(const FBlueprintSessionResult& Result);

	/** @return true if Result is a private (not advertised) session. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static bool IsSessionPrivate(const FBlueprintSessionResult& Result);

	/** @return true if Result is a dedicated session. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static bool IsSessionDedicated(const FBlueprintSessionResult& Result);

	/** @return true if Result is a passworded session. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static bool IsSessionPassworded(const FBlueprintSessionResult& Result);

	/** @return Copy of SessionList with all of the full sessions culled. */
	UFUNCTION(BlueprintCallable, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static TArray<FBlueprintSessionResult> GetNonFullSessions(const TArray<FBlueprintSessionResult>& SessionList);

	/** @return Copy of SessionList with all of the empty sessions culled. */
	UFUNCTION(BlueprintCallable, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static TArray<FBlueprintSessionResult> GetNonEmptySessions(const TArray<FBlueprintSessionResult>& SessionList);

	/** @return Sorted version of SessionList. */
	UFUNCTION(BlueprintCallable, Category="Online|Session", meta=(DeprecatedFunction, DeprecationMessage="Platform consistency issues"))
	static TArray<FBlueprintSessionResult> SortSessionList(const TArray<FBlueprintSessionResult>& SessionList, const ESessionSortColumn Column, const bool bAscending);
	
	//////////////////////////////////////////////////
	// Game session hosting
public:
	/** Kicks a player from the session if you are the host. */
	UFUNCTION(BlueprintCallable, Category="Online|Session|Host")
	static bool CanKickPlayer(AILLPlayerController* Requestor, AILLPlayerState* PlayerState);

	/** Kicks a player from the session if you are the host. */
	UFUNCTION(BlueprintCallable, Category="Online|Session|Host")
	static void KickPlayer(AILLPlayerController* Requestor, AILLPlayerState* PlayerState);
};
