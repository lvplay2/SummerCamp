// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Character.h"

#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "SCGVCInteractibleWidgetManager.h"
#include "SCGVCPlayerState.h"
#include "ILLCharacter.h"
#include "SCGVCCharacter.generated.h"

#define VC_MAX_INVENTORY_ITEMS 50

class USCGVCInteractibleWidgetManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVCCharacterChangedStateDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVCCharacterPickUpDelegate, FString, ItemID);

UENUM(BlueprintType)
enum class EVCPlayerCharacterControlState : uint8
{
	EVCPC_Walk						UMETA(DisplayName="Walk"),
	EVCPC_ExamineItem				UMETA(DisplayName="ExamineItem"),
	EVCPC_InteractingWithObject		UMETA(DisplayName="InteractingWithObject"),
	EVCPC_InInventory				UMETA(DisplayName="InInventory"),
	EVCPC_InPauseMenu				UMETA(DisplayName="InPauseMenu"),
	EVCPC_InDebugMenu				UMETA(DisplayName="InDebugMenu"),
	EVCPC_CameraOnly				UMETA(DisplayName="CameraOnly"),
	EVCPC_InCutscene				UMETA(DisplayName="InCutscene")
};

UCLASS()
class SUMMERCAMP_API ASCGVCCharacter
: public AILLCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCGVCCharacter(const FObjectInitializer& ObjectInitializer);

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* m_FirstPersonCameraComponent;

	/** The reach of the trace to identify interactibles*/
	UPROPERTY(EditAnywhere, Category = "VC Camera")
	float m_TraceReach = 180;

	/** Player speed coefficient (capped at 1.0) */
	UPROPERTY(EditAnywhere, Category = "VC Player Movement")
	float m_WalkSpeed = 1;

	/** Player look sensitivity */
	UPROPERTY(EditAnywhere, Category = "VC Player Movement")
	float m_LookSpeed = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Enum")
	EVCPlayerCharacterControlState m_ControlState;

	UPROPERTY(BlueprintAssignable, Category = "VC Control")
	FVCCharacterChangedStateDelegate OnChangedState;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void OnReceivedPlayerState() override;

	/** get the player state for this character. */
	virtual ASCGVCPlayerState* GetPlayerState() const;

	//handles moving forward/backward
	UFUNCTION()
	void MoveForward(float Amt);

	//handles strafing
	UFUNCTION()
	void MoveHorizontal(float Amt);

	UFUNCTION(BlueprintNativeEvent, Category = "VC Character Logic")
	void OnVCStartCrouch();

	UFUNCTION(BlueprintNativeEvent, Category = "VC Character Logic")
	void OnVCStopCrouch();

	UFUNCTION(BlueprintnativeEvent, Category = "VC Character Logic")
	void OnTryFootstepSound(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "VC Character Logic")
	void ToggleCrouch();

	UFUNCTION(BlueprintCallable, Category = "VC Character Logic")
	void PerformAction();

	UFUNCTION(BlueprintCallable, Category = "VC Character Logic")
	void CancelAction();

	UFUNCTION(BlueprintCallable, Category = "VC Character Logic")
	void DoInventoryDown();

	UFUNCTION(BlueprintCallable, Category = "VC Character Logic")
	void DoInventoryUp();

	UFUNCTION(BlueprintCallable, Category = "VC Character Logic")
	void DoInventoryLeft();

	UFUNCTION(BlueprintCallable, Category = "VC Character Logic")
	void DoInventoryRight();

	void DoDebugCodeInput(TCHAR charentered);

	template<TCHAR charentered> void DoDebugCodeInput() { DoDebugCodeInput(charentered); }

	UFUNCTION()
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "VC Character Logic")
	void ToggleDebugMenu();

	UFUNCTION()
	void ToggleDescription();

	UFUNCTION()
	void OnPause();

	/**
	 * Add input (affecting Pitch) to the Controller's ControlRotation, if it is a local PlayerController.
	 * This value is multiplied by the PlayerController's InputPitchScale value.
	 * @param Val Amount to add to Pitch. This value is multiplied by the PlayerController's InputPitchScale value.
	 * @see PlayerController::InputPitchScale
	 */
	UFUNCTION(BlueprintCallable, Category = "VC Pawn|Input", meta = (Keywords = "up down addpitch"))
	virtual void AddControllerPitchInput(float Amt) override;

	/**
	 * Add input (affecting Yaw) to the Controller's ControlRotation, if it is a local PlayerController.
	 * This value is multiplied by the PlayerController's InputYawScale value.
	 * @param Val Amount to add to Yaw. This value is multiplied by the PlayerController's InputYawScale value.
	 * @see PlayerController::InputYawScale
	 */
	UFUNCTION(BlueprintCallable, Category = "VC Pawn|Input", meta = (Keywords = "left right turn addyaw"))
	virtual void AddControllerYawInput(float Amt) override;

	UFUNCTION(BlueprintCallable, Category = "VC Pawn|Logic", meta = (Keywords = "Item, Inventory, Pickup"))
	void PickUpItem(ASCGVCItem* NewItem);

	UFUNCTION(BlueprintCallable, Category = "VC Pawn|Logic", meta = (Keywords = "Item, Inventory, Pickup"))
	void DropItem(ASCGVCItem* DroppedItem);

	UFUNCTION(BlueprintCallable, Category = "VC Pawn|Logic", meta = (Keywords = "Item, Inventory, Pickup"))
	bool HasItemInInventory(const FString& itemID);

	UFUNCTION(BlueprintPure, Category = "VC Pawn|Logic", meta = (Keywords = "Item, Inventory, Pickup"))
	int GetInventoryLength() const { return m_InventoryLength; };
	
	UFUNCTION(BlueprintCallable, Category = "VC System|Achievements", meta = (Keywords = "Achievement, Achievements"))
	void SetVCAchievementProgress();

	UFUNCTION(BlueprintCallable, Category = "VC UI", meta = (Keywords = "UI, Save"))
	void PlayUISaveAnimation();

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "VC Pawn|Logic")
	FVCCharacterPickUpDelegate OnPickUpItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Pawn|Logic")
	bool m_InventoryEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Pawn|Logic")
	bool m_DebugMenuEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Pawn|Logic")
	bool m_CrouchingEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Item Rotation")
	float m_RotationDeadZone = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Item Rotation")
	float m_PitchSpeed = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Item Rotation")
	float m_YawSpeed = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Character Logic")
	bool b_isCrouching = false;

	void RotateExaminedItemYaw(float Amt);
	void RotateExaminedItemPitch(float Amt);
	void RotateEquippedItemYaw(float Amt);
	void RotateEquippedItemPitch(float Amt);

	UFUNCTION(BlueprintCallable, Category = "VC Pawn|Trace")
	bool TraceFromCurrentCamera(FHitResult& OutHitResult);

	TArray<ASCGVCItem*> GetInventory();

	void SetEquippedItem(ASCGVCItem* newItem);

	void SendActionOnEquippedItem();

	UFUNCTION(BlueprintCallable, Category = "VC Pawn|Trace")
	void ClosedPauseMenuCallback();

	bool CanInteractAtAll() const;

	virtual USCGVCInteractibleWidgetManager* GetInteractibleManager() const { return m_InteractibleWidgetManager; }

	UFUNCTION(BlueprintCallable, Category = "VC Pawn|Trace")
	void SetControlState(EVCPlayerCharacterControlState newstate);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int m_InventoryLength = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<ASCGVCItem*> m_Inventory;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "VC Character", meta = (AllowPrivateAccess = "true"))
	USCGVCInteractibleWidgetManager* m_InteractibleWidgetManager;

private:
	UPROPERTY()
	class ASCGVCInteractible* m_LookedAtInteractible;
	UPROPERTY()
	class ASCGVCInteractible* m_ExaminedInteractible;
	UPROPERTY()
	class ASCGVCItem* m_EquippedItem;
	UPROPERTY()
	class ASCGVCItem* m_ExaminedItem;

	void CheckIfDebugCodeString(TCHAR newchar);

	static const FString CorrectDebugCode;
	static const FName VC_ACHIEVEMENT_NAME;
	FString m_curDebugCode;

};
