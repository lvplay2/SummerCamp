// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ILLPartyBlueprintLibrary.generated.h"

class AILLPartyBeaconMemberState;

/**
 * @class UILLPartyBlueprintLibrary
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPartyBlueprintLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** @return true if we can create a party. */
	UFUNCTION(BlueprintPure, Category="Parties", meta=(WorldContext="WorldContextObject"))
	static bool CanCreateParty(UObject* WorldContextObject);

	/** @return true if we can invite to our party. */
	UFUNCTION(BlueprintPure, Category="Parties", meta=(WorldContext="WorldContextObject"))
	static bool CanInviteToParty(UObject* WorldContextObject);

	/** @return Current party session player count, or -1 if not available. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static int32 GetNumPartyMembers(UObject* WorldContextObject);

	/** @return Current party session player count, or -1 if not available. */
	UFUNCTION(BlueprintPure, Category="Online|Session", meta=(WorldContext="WorldContextObject"))
	static int32 GetMaxPartyMembers(UObject* WorldContextObject);

	/** @return Localized party leader status description. Blank for none. */
	UFUNCTION(BlueprintPure, Category="Parties", meta=(WorldContext="WorldContextObject"))
	static FText GetPartyLeaderStatusDescription(UObject* WorldContextObject);

	/** @return true if our local player is in a party. You can be creating, joining, leading or a member of a party for this to return true. */
	UFUNCTION(BlueprintPure, Category="Parties", meta=(WorldContext="WorldContextObject"))
	static bool IsInParty(UObject* WorldContextObject);

	/** @return true if our local player is in a party and is the leader. */
	UFUNCTION(BlueprintPure, Category="Parties", meta=(WorldContext="WorldContextObject"))
	static bool IsInPartyAsLeader(UObject* WorldContextObject);

	/** @return true if our local player is in a party and is not the leader. */
	UFUNCTION(BlueprintPure, Category="Parties", meta=(WorldContext="WorldContextObject"))
	static bool IsInPartyAsMember(UObject* WorldContextObject);

	/** @return true if our local player is joining a party. */
	UFUNCTION(BlueprintPure, Category="Parties", meta=(WorldContext="WorldContextObject"))
	static bool IsJoiningParty(UObject* WorldContextObject);

	/** @return true if LAN session menu operations are blocked by parties. */
	UFUNCTION(BlueprintPure, Category="Parties", meta=(WorldContext="WorldContextObject"))
	static bool IsPartyBlockingLANSessions(UObject* WorldContextObject);

	/** @return true if online session related menu operations are blocked by parties. */
	UFUNCTION(BlueprintPure, Category="Parties", meta=(WorldContext="WorldContextObject"))
	static bool IsPartyBlockingOnlineSessions(UObject* WorldContextObject);

	/** @return true if MemberState is the party leader. */
	UFUNCTION(BlueprintPure, Category="Parties")
	static bool IsPartyLeader(AILLPartyBeaconMemberState* MemberState);

	/** Opens the external invite UI for your current party session. */
	UFUNCTION(BlueprintCallable, Category="Parties", meta=(DeprecatedFunction, DeprecationMessage="Function is deprecated, use ShowExternalInviteUI instead.", WorldContext="WorldContextObject"))
	static void InviteFriendToParty(UObject* WorldContextObject);

	/** Kicks MemberState from the party. Only works if a local player is the party leader/host. */
	UFUNCTION(BlueprintCallable, Category="Parties", meta=(WorldContext="WorldContextObject"))
	static void KickPartyMember(AILLPartyBeaconMemberState* MemberState);
};
