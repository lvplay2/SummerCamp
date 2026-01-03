// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "Virtual_Cabin/SCGVCInteractibleWidget.h"
#include "SCGVCInteractible.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVCInteratibleStartInteractingDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVCInteratibleStopInteractingDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVCInteratibleItemUsedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVCInteratibleActionPressedDelegate);

UCLASS()
class SUMMERCAMP_API ASCGVCInteractible
: public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASCGVCInteractible(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;
	
	UMeshComponent* GetMeshComponent();

	virtual void ToggleLookedAtEffect(bool toggle, bool instant = false);

	UFUNCTION(BlueprintNativeEvent)
	void OnStartInteracting(ASCGVCCharacter* VCChar, ASCGVCPlayerController* VCController);

	UFUNCTION(BlueprintNativeEvent)
	void OnStopInteracting(ASCGVCCharacter* VCChar, ASCGVCPlayerController* VCController);

	UFUNCTION(BlueprintNativeEvent)
	void OnToggleProximityBP(bool Toggle, bool instant = false);

	UFUNCTION(BlueprintNativeEvent)
	void OnToggleLookedAtBP(bool Toggle, bool instant = false);

	UFUNCTION(BlueprintCallable, Category = "VC Interactible")
	void ToggleProximityIndicator(bool toggle, bool instant = false);

	UFUNCTION(BlueprintNativeEvent)
	void OnActionPressedWhenExamined(ASCGVCCharacter* VCChar, ASCGVCPlayerController* VCController);

	UFUNCTION(BlueprintNativeEvent)
	void OnItemUsed(ASCGVCCharacter* VCChar, ASCGVCPlayerController* VCController, ASCGVCItem* VCItem);

	UFUNCTION(BlueprintCallable, Category = "VC Interactible")
	void SetInteractibleInWorld(bool interactible);

	UFUNCTION(BlueprintCallable, Category = "VC Interactible")
	void ToggleHidden(bool hide);

	UFUNCTION(BlueprintCallable, Category = "VC Interactible")
	void ToggleCollision(bool collide);

	UFUNCTION(BlueprintCallable, Category = "VC Interactible")
	FText GetName();

	UPROPERTY(BlueprintAssignable, Category = "VC Control")
	FVCInteratibleStartInteractingDelegate OnStartInteractingDelegate;

	UPROPERTY(BlueprintAssignable, Category = "VC Control")
	FVCInteratibleStopInteractingDelegate OnStopInteractingDelegate;

	UPROPERTY(BlueprintAssignable, Category = "VC Control")
	FVCInteratibleItemUsedDelegate OnItemUsedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "VC Control")
	FVCInteratibleActionPressedDelegate OnActionPressedWhenExaminedDelegate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Interactible")
	FVector m_ExaminePosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Interactible")
	FRotator m_ExamineRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Interactible")
	bool m_ShowInteractibleEffect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Interactible")
	bool m_isExaminable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Interactible")
	bool b_isUsableWhileExamining = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Interactible")
	FText m_DescriptionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VC Interactible|UI")
	bool m_isPlayerInProximity = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VC Interactible")
	bool b_ItemUsableOnInteractible = false;

	UFUNCTION(BlueprintCallable, Category = "VC Interactible|UI")
	void SetItemUsableOnInteractible(bool newToggle);

	// button info panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Button Info Panel")
	FText m_UseButtonInfoText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Button Info Panel")
	FText m_BackButtonInfoText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Button Info Panel")
	bool b_ShowBackButtonInfo = true;

protected:
	UPROPERTY()
	UMeshComponent* m_MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Collision)
	UBoxComponent* m_CollisionComponent;

	UPROPERTY(EditAnywhere, Category = "VC Interactible Properties")
	FText m_Name;

	UPROPERTY(EditDefaultsOnly)
	USCGVCInteractibleWidget* m_InteractWidgetRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Interactible")
	bool m_IsInteractibleInWorld = true;
};
