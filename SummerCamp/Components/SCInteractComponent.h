// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLInteractableComponent.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "SCInteractComponent.generated.h"

class ASCCharacter;
class ASCCharacterAIController;

class UImage;
class USCInteractIconWidget;
class USCInteractWidget;
class USCSplineCamera;

/**
 * @enum EPipState
 */
UENUM()
enum EInteractMinigamePipState
{
	None,
	Active,
	Hit,
	Miss
};

/**
 * @struct FInteractMinigamePip
 */
USTRUCT(BlueprintType)
struct FInteractMinigamePip
{
	GENERATED_BODY()

	FInteractMinigamePip()
		: State(EInteractMinigamePipState::None)
		, bIsFat(true)
	{
	}

	// Width of the pip in degrees
	UPROPERTY(EditDefaultsOnly, Category = "Default", meta = (ClampMin = "0.0", ClampMax = "360.0", UIMin = "0.0", UIMax = "360.0"))
	float Width;

	// Start position of the pip in degrees
	UPROPERTY(EditDefaultsOnly, Category = "Default", meta = (ClampMin = "0.0", ClampMax = "360.0", UIMin = "0.0", UIMax = "360.0"))
	float Position;

	// State of the pip
	UPROPERTY(BlueprintReadOnly, Category = "Default")
	TEnumAsByte<EInteractMinigamePipState> State;

	UPROPERTY(EditDefaultsOnly, Category = "Default", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	int InteractKey;

	// Reference to the UImage that represents this pip
	UPROPERTY()
	UImage* WidgetRef;

	UPROPERTY()
	bool bIsFat;
};

/**
 * @class USCInteractComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCInteractComponent
: public UILLInteractableComponent
{
	GENERATED_BODY()

public:
	USCInteractComponent(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent interface
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	// End UActorComopnent interface

	// Begin USceneComponent interface
#if WITH_EDITOR
	/**
	 * Enables/Disables properties based on InteractType
	 * @param PropertyChangedEvent
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void BeginPlay() override;
	// End USceneComponent interface

	// Begin UILLInteractableComponent interface
	virtual void OnHoldStateChangedBroadcast(AActor* Interactor, EILLHoldInteractionState NewState) override;
	virtual bool IsInteractionAllowed(AILLCharacter* Character) override;
	virtual void OnInRangeBroadcast(AActor* Interactor) override;
	virtual void StartHighlightBroadcast(AActor* Interactor) override;
	virtual void StopHighlightBroadcast(AActor* Interactor) override;
	virtual void BroadcastBecameBest(AActor* Interactor) override;
	virtual void BroadcastLostBest(AActor* Interactor) override;
	// End UILLInteractableComponent interface

	/**
	 * Event to handle failing to get to a specific position when hold interacting
	 * @param Interactor Character that failed the special move
	 */
	UFUNCTION()
	void OnHoldSpecialMoveFailed(ASCCharacter* Interactor);

	/** Get the hold time for the specified character */
	virtual float GetHoldTimeLimit(const ASCCharacter* Interactor) const;

	// Enable/Disable minigame properties
	UPROPERTY()
	uint32 bShowMinigameProperties : 1;

	// Pip data for interact minigame
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Minigame", meta=(EditCondition="bShowMinigameProperties"))
	TArray<FInteractMinigamePip> Pips;

	UPROPERTY(EditDefaultsOnly, Category = "Minigame", meta=(EditCondition="bShowMinigameProperties"))
	bool bRandomizePips;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame", meta = (EditCondition = "bShowMinigameProperties"))
	USoundCue* MinigameLoopCue;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame", meta = (EditCondition = "bShowMinigameProperties"))
	USoundCue* MinigameSuccessCue;

	// Sound cue to play on the counselor when a minigame pop is failed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame", meta = (EditCondition = "bShowMinigameProperties"))
	USoundCue* MinigameFailCue;

	// Sound cue to play on the killer when a minigame pip is failed (The first failure always notifies)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame", meta = (EditCondition = "bShowMinigameProperties"))
	USoundCue* NotifyKillerMinigameFailCue;

	// Number of failures allowed before the killer gets a sound notification
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame", meta = (EditCondition = "bShowMinigameProperties"))
	float AllowableFailsBeforeKillerNotify;

	// Animation to play when interacting with this object
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* CounselorInteractAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* KillerInteractAnim;

	/** Activate a special interaction camera */
	UFUNCTION()
	void PlayInteractionCamera(ASCCharacter* Interactor);

	/** Stop any special interaction cameras that are playing */
	UFUNCTION()
	void StopInteractionCamera(ASCCharacter* Interactor);

	/**
	 * Tell our widget to draw at a specific location on the screen instead of world space
	 * @param bDrawAtLocation true to draw at a screen space location, false to draw at interactable world location
	 * @param ScreenLocationRatio the screen coordinates to draw our widget at in 0 - 1 space
	 */
	UFUNCTION()
	void SetDrawAtLocation(const bool bDrawAtLocation, const FVector2D ScreenLocationRatio = FVector2D::UnitVector);

	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	TSubclassOf<USCInteractIconWidget> InteractionIconWidgetClass;

	/** If true, using attack can trigger interactions */
	FORCEINLINE bool CanAttackToInteract() const { return bCanAttackToInteract; }

	/** If true, using grab can trigger interactions */
	FORCEINLINE bool CanGrabToInteract() const { return bInteractOnGrab; }

	/**
	 * If bAutoReturnCamera is true, the camera animation for this interaction will be stopped.
	 * @return true if the camera was returned, false otherwise.
	 */
	bool TryAutoReturnCamera(ASCCharacter* Interactor);

protected:
	/** @return true if Interactor is allowed to affect the InteractionWidgetInstance. */
	bool CanAffectInteractWidget(AActor* Interactor) const;

	/** Helper function to spawn InteractWidgetInstance. */
	void AutoCreateInteractWidget(AActor* Interactor);

	// Interaction widget to display for this interactable
	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	TSubclassOf<USCInteractWidget> InteractionWidgetClass;

	// Instance of our interact widget
	UPROPERTY()
	USCInteractWidget* InteractWidgetInstance;

	// The Special interaction camera that is currently playing
	UPROPERTY()
	USCSplineCamera* PlayingCamera;

	// Cached list of SplineCameras parented to this interactable
	UPROPERTY()
	TArray<USCSplineCamera*> AttachedSplineCameras;

	// Time to take when blending from player controller to spline cam
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float BlendInTime;

	// Time to take after special complete to return the camera to player controller
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float BlendBackTime;

	// If true, using attack can trigger interactions
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	bool bCanAttackToInteract;

	// Can we interact with this object by performing a grab?
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	bool bInteractOnGrab;

	// Do we want to update the player control rotation to match our spline cam rotation before returning?
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	bool bForceNewCameraRotation;

	UPROPERTY()
	bool bHoldInteracting;

	// Should the camera be returned when the interaction is failed or canceled?
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	bool bAutoReturnCamera;

	// Special move started by this interactable
	UPROPERTY(Transient)
	USCSpecialMove_SoloMontage* ActivatedSpecialMove;

private:
	float WidgetDestructionTimer;
	
	//////////////////////////////////////////////////
	// AI support
public:
	/** @return true if this component will accept CharacterAI as pending. */
	virtual bool AcceptsPendingCharacterAI(const ASCCharacterAIController* CharacterAI, const bool bCheckIfCloser = false) const;

	/** @return Interaction location suitable for AI. */
	virtual bool BuildAIInteractionLocation(ASCCharacterAIController* CharacterAI, FVector& OutLocation);

	/** Notification for when a pending move to this, from PendingCharacterAI, completes. Enough failures and they will be tossed.  */
	virtual void PendingCharacterAIMoveFinished(ASCCharacterAIController* CharacterAI, const EBTNodeResult::Type TaskResult, const bool bAlreadyAtGoal);

	/** Changes PendingCharacterAI to NULL and tracks the time if it matches CharacterAI. */
	virtual void RemovePendingCharacterAI(ASCCharacterAIController* CharacterAI);

	/** Assigns the PendingCharacterAI if it's NULL and tracks the time. */
	virtual void SetPendingCharacterAI(ASCCharacterAIController* CharacterAI, const bool bForce = false);

	/** @return Pending interacting character AI. */
	FORCEINLINE ASCCharacterAIController* GetPendingCharacterAI() const { return PendingCharacterAI; }

protected:
#if !UE_BUILD_SHIPPING
	/** Helper function for debugging AI failures. */
	virtual void DrawAIDebugLine(const FVector& StartLocation, const float Lifetime);
#endif

	// Override to always allow AI to interact
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable")
	bool bAlwaysAllowInteractionFromAI;

	// Pending character AI interaction, to avoid trampling
	UPROPERTY(Transient)
	ASCCharacterAIController* PendingCharacterAI;

	// Last character AI interaction, to avoid fixating
	UPROPERTY(Transient)
	ASCCharacterAIController* LastCharacterAI;

	// List of AI and the next time they can attempt to path to this interactble
	UPROPERTY(Transient)
	TMap<ASCCharacterAIController*, float> CharacterNextAllowedPathTime;

	// Last time an AI could not path to this interactable
	float LastFailedPathTime;

	// Last time an AI was forced to snap to face this interactable
	float LastForcedAISnapTime;

	// Next time the LastCharacterAI is allowed to interact with this due to being fixated
	float FixatedNextAllowedTime;

	// Number of times we have had the same ASCCharacterAIController in a row
	int32 CharacterAIAttempts;
};
