// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLPlayerController_Lobby.h"
#include "SCGameInstance.h"
#include "SCPlayerController_Lobby.generated.h"

/**
 * @class ASCPlayerController_Lobby
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCPlayerController_Lobby
: public AILLPlayerController_Lobby
{
	GENERATED_UCLASS_BODY()

	// Begin APlayerController interface
	virtual void ReceivedPlayer() override;
	// End APlayerController interface

	// Begin AILLPlayerController interface
	virtual void AcknowledgePlayerState(APlayerState* PS) override;
	virtual void SetBackendAccountId(const FILLBackendPlayerIdentifier& InBackendAccountId) override;
	// End AILLPlayerController interface

	/** Notifies clients of their P2P host backend account ID in QuickPlay matches. Otherwise this is blank to ensure Private Match host drops do not get an infraction. */
	UFUNCTION(Client, Reliable)
	virtual void ClientReceiveP2PHostAccountId(const FILLBackendPlayerIdentifier& HostAccountId);

	/** Checks if the current menu is our lobby, and if not forces it to be up front. */
	void EnsureViewingLobby();
	
	/** Sends character preferences to server */
	void SendPreferencesToServer();
	bool bSentPreferencesToServer;

	/** Delegate to bind to and call when needing to change the characters camera in the character select menus. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChangeCharacterCamera, float, SequencerTime);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Menu")
	FChangeCharacterCamera OnChangeCharacterCamera;

	/** Delegate to bind to and call when needing to change to the counselor background scene in the character select menus. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChangeToCounselorScene, bool, ClothingSelection);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Menu")
	FChangeToCounselorScene OnChangeToCounselorScene;

	/** Delegate to bind to and call when needing to change to the Jason background scene in the Jason select menus. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChangeToJasonScene);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Menu")
	FChangeToJasonScene OnChangeToJasonScene;

	/** Delegate to bind to and call when needing to change to the lobby background. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChangeToLobbyBackground, bool, ShowLobbyBackground);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Menu")
	FChangeToLobbyBackground OnChangeToLobbyBackground;

	/** Delegate to bind to and call when needing to change to the pamela background. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChangeToPamelaBackground);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Menu")
	FChangeToPamelaBackground OnChangeToPamelaBackground;
};
