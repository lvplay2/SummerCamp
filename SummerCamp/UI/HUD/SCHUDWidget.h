// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCHUDWidget.generated.h"

class ASCCounselorCharacter;
class USCInteractComponent;
class USCInteractMinigameWidget;
class USCMiniMapBase;
class USCWiggleWidget;

enum class EKillerAbilities : uint8;
enum class ESCSkullTypes : uint8;

/**
 * @class USCHUDWidget
 * @brief Base HUD widget class.
 */
UCLASS()
class SUMMERCAMP_API USCHUDWidget
: public USCUserWidget
{
	GENERATED_BODY()
	
public:
	/** */
	UFUNCTION(BlueprintCallable, Category = "SCHUD")
	USCMiniMapBase* GetMinimap();

	/** Preps match start animation(s) when "waiting for players" screen is still up. This is to work-around a single frame pop, where widgets show at their full opacity. */
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void OnPrepMatchStart();

	/** Triggers match start animation(s). */
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void OnMatchStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void OnShowScoreboard(bool bShow);

	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void OnShowEmoteSelect(bool bShow);

	/** */
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void OnMatchEnded();

	/** */
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void OnTwoMinuteWarning();

	/** Notify the HUD that the player is out of bounds */
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void OnOutOfBoundsWarning();

	/** Notify the HUD that the player has re-entered the bounds */
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void OnInBounds();

	/** */
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void OnShowPoliceHasBeenCalled(bool bHasPoliceArrived);

	/** */
	UFUNCTION(BlueprintImplementableEvent, Category = "Character")
	void OnInventoryChanged();

	/** */
	UFUNCTION(BlueprintImplementableEvent, Category = "Killer")
	void OnAttemptMorph();

	/** Starts a fade-in effect from being faded out. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Effects")
	void OnStartFadeIn();

	/** Starts a fade-out effect, leaving it hidden until OnStartFadeIn is called.  */
	UFUNCTION(BlueprintImplementableEvent, Category = "Effects")
	void OnStartFadeOut();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnCombatStanceChanged(bool Enabled);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Spectator")
	void OnShowSpectatorIntro(bool bDied, bool bLateJoin);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Spectator")
	void OnSpectatorIntroFinished();

	UFUNCTION(BlueprintImplementableEvent, Category = "Spectator")
	void OnMatchIsWaitingPostMatchOutro(bool bDied, bool bIsKiller, bool bLateJoin);

	UFUNCTION(BlueprintImplementableEvent, Category = "Spectator")
	void OnToggleSpecatorHUD(bool bShow);

	UFUNCTION(BlueprintImplementableEvent, Category = "Spectator")
	void OnToggleSpecatorShowAll(bool bShow);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Spectator")
	void OnSpectatorUpdateStatus(const FString& StatusText, const FString& NextMode);

	UFUNCTION(BlueprintImplementableEvent, Category = "Score Event")
	void OnShowScoreEvent(const FString& Message, const uint8 Category);

	UFUNCTION(BlueprintImplementableEvent, Category = "Objective Event")
	void OnShowObjectiveEvent(const FString& Message);

	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor Map")
	void OnShowCounselorMap();

	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor Map")
	void CloseCounselorMap();

	/** Set if the health bar should be displayed */
	UFUNCTION(BlueprintImplementableEvent, Category = "Debug")
	void OnShowHealthBar(bool bShow);

	UPROPERTY(BlueprintReadOnly, Category = "Fear")
	FLinearColor FearOpacity;

	UFUNCTION(BlueprintImplementableEvent, Category = "Breath")
	void OnShowBreath();

	UFUNCTION(BlueprintImplementableEvent, Category = "Breath")
	void OnHideBreath();

	UFUNCTION(BlueprintImplementableEvent, Category = "Rage")
	void OnRageActive();

	UFUNCTION(BlueprintImplementableEvent, Category = "Rage")
	void OnShowJasonRage();

	UFUNCTION(BlueprintImplementableEvent, Category = "Rage")
	void OnJasonAbilityUnlocked(EKillerAbilities UnlockedAbility);

	UFUNCTION(BlueprintImplementableEvent, Category = "Killer")
	void OnShowKnifeReticle();

	UFUNCTION(BlueprintImplementableEvent, Category = "Killer")
	void OnHideKnifeReticle();

	UFUNCTION(BlueprintImplementableEvent, Category = "Minigame")
	void OnStartInteractMinigame();

	UFUNCTION(BlueprintImplementableEvent, Category = "Minigame")
	void OnStopInteractMinigame();

	UPROPERTY(BlueprintReadWrite, Category="OverlayWidgets", Transient)
	USCInteractMinigameWidget* InteractMinigameInstance;

	UFUNCTION(BlueprintImplementableEvent, Category = "Minigame")
	void OnStartWiggleWidget();

	UFUNCTION(BlueprintImplementableEvent, Category = "Minigame")
	void OnStopWiggleWidget();

	UPROPERTY(BlueprintReadWrite, Category="OverlayWidgets", Transient)
	USCWiggleWidget* WiggleWidgetInstance;

	UFUNCTION(BlueprintImplementableEvent, Category = "Abilities")
	void SetSweaterAbilityAvailable(bool Available);

	UFUNCTION(BlueprintImplementableEvent, Category = "Aiming")
	void OnStartAiming();

	UFUNCTION(BlueprintImplementableEvent, Category = "Aiming")
	void OnEndAiming();

	UFUNCTION(BlueprintImplementableEvent, Category = "Hud")
	void SetNonPersistentWidgetVisibility(bool bShouldHide);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Paranoia")
	ESlateVisibility GetParanoiaAbilityVisiblity() const;

	/** Adds a new failed skull line widget to the HUD. Defined in blueprint (KillerHUDWidget graph) */
	UFUNCTION(BlueprintImplementableEvent, Category = "SinglePlayer Killer")
	void OnFailedSkull(const ESCSkullTypes FailedType);

	/** Adds a new success skull line widget to the HUD. Defined in blueprint (KillerHUDWidget graph) */
	UFUNCTION(BlueprintImplementableEvent, Category = "SinglePlayer Killer")
	void OnSuccessSkull(const ESCSkullTypes SuccessType);

protected:
	// 
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SCHUD")
	USCMiniMapBase* Minimap;

public:
	UPROPERTY(EditDefaultsOnly, Category = "SCHUD")
	TSubclassOf<USCUserWidget> ObjectiveMenuClass;

	/** Gets the owning counselor, even if they're currently driving a vehicle */
	UFUNCTION(BlueprintPure, Category = "SCHUD")
	ASCCounselorCharacter* GetOwningCounselor() const;
};
