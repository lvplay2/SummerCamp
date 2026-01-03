// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLPlayerController.h"
#include "SCPlayerController_Menu.generated.h"

/**
 * @class ASCPlayerController_Menu
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCPlayerController_Menu
: public AILLPlayerController
{
	GENERATED_UCLASS_BODY()

	// Begin APlayerController interface
	virtual void ReceivedPlayer() override;
	// End APlayerController interface

	// Begin AILLPlayerController interface
	virtual void SetBackendAccountId(const FILLBackendPlayerIdentifier& InBackendAccountId) override;
	virtual void AcknowledgePlayerState(APlayerState* PS) override;
	// End AILLPlayerController interface

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

	/** Delegate to bind to and call when needing to change to the Jason in the main menu. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChangeToJasonMainMenuCamera);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Menu")
	FChangeToJasonMainMenuCamera OnChangeToJasonMainMenuCamera;

	/** Delegate to bind to and call when needing to change to the lobby background. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChangeToLobbyBackground);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Menu")
	FChangeToLobbyBackground OnChangeToLobbyBackground;

	// When selecting single player challenges this will keep track of which one was last selected so we can load it rather than the default.
	UPROPERTY(BlueprintReadWrite, Transient)
	int32 SelectedChallengeIndex;
};
