// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Virtual_Cabin/SCGVCInteractible.h"
#include "SCGVCItem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVCItemPickedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVCItemDroppedDelegate);

UCLASS()
class SUMMERCAMP_API ASCGVCItem
: public ASCGVCInteractible
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASCGVCItem(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	void SetToForwardRendering();

	void SetToWorldRendering();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Item")
	bool m_IsInventoryItem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Item")
	FVector m_WorldAnchorPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Item")
	FRotator m_WorldAnchorRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Item Examine")
	float m_ScaleWhenExamining = 1.0f;

	UPROPERTY()
	FVector m_OrigScale;

	UPROPERTY(EditAnywhere)
	UTexture2D* m_IconTexture;
	
	FORCEINLINE UTexture2D* GetItemIcon() { return m_IconTexture; }

	UFUNCTION(BlueprintCallable, Category = "VC Item")
	FString GetItemID();

	UFUNCTION(BlueprintCallable, Category = "VC Item")
	void SnapToAnchor();

	UFUNCTION(BlueprintNativeEvent)
	void OnDropped();

	UFUNCTION(BlueprintNativeEvent)
	void OnPickedUp();

	UFUNCTION(BlueprintNativeEvent)
	void OnInvokePickUpCue();

	UFUNCTION(BlueprintNativeEvent)
	void OnInvokeDropCue();

	UFUNCTION(BlueprintNativeEvent)
	void OnInvokeSnapToAnchorCue();

	UFUNCTION(BlueprintNativeEvent)
	void OnInvokeEnteredExamineModeCue();

	UFUNCTION()
	void SetItemToExamineRotationPivot();

	UFUNCTION()
	void SetItemToNormalLocationAndPivot();

	UPROPERTY(BlueprintAssignable, Category = "VC Control")
	FVCItemPickedDelegate OnItemPickedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "VC Control")
	FVCItemDroppedDelegate OnItemDroppedDelegate;

	UFUNCTION()
	UArrowComponent* GetRotationPivot();
protected:
	UPROPERTY(EditAnywhere, Category = "VC Item Properties")
	FString m_ItemID;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "VC Item Examine")
	UArrowComponent* m_RotationOffsetArrow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Item Examine")
	FVector m_MeshPrevRelativePosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Item Examine")
	FRotator m_MeshPrevRelativeRotation;
};