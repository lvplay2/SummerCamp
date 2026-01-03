// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCAnimDriverComponent.h"
#include "SCContextKillActor.generated.h"

enum class EILLInteractMethod : uint8;

class ASCKillerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSCKillEvent, ASCCharacter*, Killer, ASCCharacter*, Victim);

/**
 * @class ASCContextKillActor
 */
UCLASS()
class SUMMERCAMP_API ASCContextKillActor
: public AActor
{
	GENERATED_BODY()

public:
	ASCContextKillActor(const FObjectInitializer& ObjectInitialzier);

	// BEGIN AActor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	// END AActor interface

	/** Activate the ContextKillComponent */
	UFUNCTION()
	virtual void ActivateKill(AActor* Interactor, EILLInteractMethod InteractMethod);

	/**
	 * Can the given character interact with this component.
	 * @param Character - AILLCharacter reference requesting interaction.
	 * @param ViewLocation - The position of the character or camera.
	 * @param ViewRotation - The rotation of the character or camera.
	 */
	UFUNCTION()
	virtual int32 CanInteractWith(class AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	class USCContextKillComponent* ContextKillComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	class USCInteractComponent* InteractComponent;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	USceneComponent* CameraFocalPoint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bOnlyUseableOnce;

	UPROPERTY(Replicated)
	bool bUsed;

	/////////////////////////////////////////////////////////
	// Optional Animated Kill Mesh
protected:
	// Mesh we'll play an animation on while performing the kill
	UPROPERTY()
	USkeletalMeshComponent* KillMesh;

	// Animation to play on the mesh above
	UPROPERTY()
	UAnimSequenceBase* KillMeshAnimation;

	// Timer to turn skeletal mesh updates back off
	FTimerHandle TimerHandle_AnimationPlaying;

	/**
	 * Sets our animating mesh and tells it to stop ticking
	 * TODO: Add support for meshes that animate normally?
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void SetAnimatedKillMesh(USkeletalMeshComponent* InKillMesh, UAnimSequenceBase* InKillAnimation);

	/** Callback for when the kill starts (plays kill animation on the kill mesh, handles skeleton updates) */
	UFUNCTION()
	void KillStarted(ASCCharacter* Interactor, ASCCharacter* Victim);

	/** Callback for when the kill is completed */
	UFUNCTION()
	void KillCompleted(ASCCharacter* Interactor, ASCCharacter* Victim);

	// Blueprint callback for when the kill is started
	UPROPERTY(BlueprintAssignable)
	FSCKillEvent OnKillStarted;

	// Blueprint callback for when the kill is completed
	UPROPERTY(BlueprintAssignable)
	FSCKillEvent OnKillCompleted;

	/////////////////////////////////////////////////////////
	// Context FX driver and delegates
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	USCAnimDriverComponent* FXDriver;

	UPROPERTY()
	FAnimNotifyEventDelegate FXDriverBeginDelegate;
	UPROPERTY()
	FAnimNotifyEventDelegate FXDriverTickDelegate;
	UPROPERTY()
	FAnimNotifyEventDelegate FXDriverEndDelegate;

	UPROPERTY(EditDefaultsOnly)
	bool bUseFocalPointForKillCam;

	UFUNCTION()
	void FXDriverBegin(FAnimNotifyData NotifyData);
	UFUNCTION(BlueprintImplementableEvent, Category = "FX")
	void FXBegin(FAnimNotifyData NotifyData);

	UFUNCTION()
	void FXDriverTick(FAnimNotifyData NotifyData);
	UFUNCTION(BlueprintImplementableEvent, Category = "FX")
	void FXTick(FAnimNotifyData NotifyData);

	UFUNCTION()
	void FXDriverEnd(FAnimNotifyData NotifyData);
	UFUNCTION(BlueprintImplementableEvent, Category = "FX")
	void FXEnd(FAnimNotifyData NotifyData);

	UFUNCTION()
	USceneComponent* GetCameraFocalPoint();

	/////////////////////////////////////////////////////////
	// UI

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	UTexture2D* GetContextKillHUDIcon() const { return ContextKillHUDIcon; }

	UFUNCTION(BlueprintCallable, Category = "UI")
	UTexture2D* GetContextKillMenuIcon() const { return ContextKillMenuIcon; }

	UFUNCTION(BlueprintCallable, Category = "UI")
	TSoftClassPtr<ASCWeapon> GetContextWeaponType() const { return WeaponClass; }

	/** Gets the friend name text for this kill. */
	FORCEINLINE const FText& GetFriendlyName() const { return FriendlyName; }

	/** Gets the description for this kill. */
	FORCEINLINE const FText& GetDescription() const { return Description; }

	/** Gets the cost of this context kill. */
	int32 GetCost() const { return Cost; }

	// CP cost to buy this kill
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meta Game")
	int32 Cost;

	// Level needed to be to unlock this kill.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meta Game")
	int32 LevelRequirement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meta Game")
	FString DLCEntitlement;

protected:
	// Kill Friendly Name
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FText FriendlyName;

	// Kill Description
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	UTexture2D* ContextKillHUDIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	UTexture2D* ContextKillMenuIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<ASCWeapon> WeaponClass;

	/////////////////////////////////////////////////////////
	// Orientation
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	bool bShouldOrientTowardKiller;

	/** List of components that should ignore any orientation rotations we may do (applies inverse of calculated rotation), should be called once on construction */
	UFUNCTION(BlueprintCallable, Category = "Default")
	void AddIgnoredRotationComponents(TArray<USceneComponent*> Components);

protected:
	// List of components that should ignore any orientation rotations we may do (applies inverse of calculated rotation)
	UPROPERTY()
	TArray<USceneComponent*> IgnoredRotationComponents;

	/** Returns the nearest killer, if any (may be nullptr) */
	ASCKillerCharacter* TryGetKiller() const;
};
