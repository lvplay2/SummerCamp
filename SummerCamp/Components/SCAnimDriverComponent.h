// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/ActorComponent.h"
#include "SCAnimDriverComponent.generated.h"

/**
 * @struct FAnimNotifyData
 */
USTRUCT(BlueprintType)
struct FAnimNotifyData
{
	GENERATED_BODY();

public:
	// Total anim play time
	UPROPERTY(BlueprintReadOnly)
	float TotalAnimTime;

	// Anim time at current frame
	UPROPERTY(BlueprintReadOnly)
	float AnimFrameTime;

	// Frame time scaled to 0 - 1
	UPROPERTY(BlueprintReadOnly)
	float AnimScaledTime;

	// Delta time since last anim tick
	UPROPERTY(BlueprintReadOnly)
	float AnimDeltaTime;

	// Anim Instance notify was called on
	UPROPERTY(BlueprintReadOnly)
	UAnimInstance* AnimInstance;

	// Skeletal mesh the notify was called on
	UPROPERTY(BlueprintReadOnly)
	USkeletalMeshComponent* CharacterMesh;

	FAnimNotifyData()
	{
		TotalAnimTime = 0;
		AnimFrameTime = 0;
		AnimScaledTime = 0;
		AnimDeltaTime = 0;
		AnimInstance = nullptr;
		CharacterMesh = nullptr;
	}

	FAnimNotifyData(float InTotalAnimTime, float InAnimFrameTime, float InAnimScaledTime, float InAnimDeltaTime, UAnimInstance* InAnimInstance, USkeletalMeshComponent* InMesh)
	{
		TotalAnimTime = InTotalAnimTime;
		AnimFrameTime = InAnimFrameTime;
		AnimScaledTime = InAnimScaledTime;
		AnimDeltaTime = InAnimDeltaTime;
		AnimInstance = InAnimInstance;
		CharacterMesh = InMesh;
	}
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FAnimNotifyEventDelegate, FAnimNotifyData, NotifyData);

/**
 * @class USCAnimDriverComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCAnimDriverComponent
: public UActorComponent
{
	GENERATED_BODY()

public:
	USCAnimDriverComponent(const FObjectInitializer& ObjectInitializer);

	/** Called from SCAnimNotifyState */
	virtual bool BeginAnimDriver(AActor* InNotifier, FAnimNotifyData InNotifyData);

	/** Called from SCAnimNotifyState */
	virtual void TickAnimDriver(FAnimNotifyData InNotifyData);

	/** Called from SCAnimNotifyState */
	virtual void EndAnimDriver(FAnimNotifyData InNotifyData);

	/** Setup notify owner and Notify delegates */
	UFUNCTION(BlueprintCallable, Category = "AnimDriver")
	virtual void InitializeAnimDriver(AActor* InNotifyOwner, FAnimNotifyEventDelegate InBeginDelegate, FAnimNotifyEventDelegate InTickDelegate, FAnimNotifyEventDelegate InEndDelegate);

	/** Initialize all notify events */
	UFUNCTION(BlueprintCallable, Category = "AnimDriver")
	virtual void InitializeNotifies(FAnimNotifyEventDelegate InBeginDelegate, FAnimNotifyEventDelegate InTickDelegate, FAnimNotifyEventDelegate InEndDelegate);

	/** Initialize begin and tick notify events */
	UFUNCTION(BlueprintCallable, Category = "AnimDriver")
	virtual void InitializeBeginTick(FAnimNotifyEventDelegate InBeginDelegate, FAnimNotifyEventDelegate InTickDelegate);

	/** Initialize begin and end notify events */
	UFUNCTION(BlueprintCallable, Category = "AnimDriver")
	virtual void InitializeBeginEnd(FAnimNotifyEventDelegate InBeginDelegate, FAnimNotifyEventDelegate InEndDelegate);

	/** Initialize tick and end notify events */
	UFUNCTION(BlueprintCallable, CAtegory = "AnimDriver")
	virtual void InitializeTickEnd(FAnimNotifyEventDelegate InTickDelegate, FAnimNotifyEventDelegate InEndDelegate);

	/** Initialize the begin notify event */
	UFUNCTION(BlueprintCallable, Category = "AnimDriver")
	virtual void InitializeBegin(FAnimNotifyEventDelegate InBeginDelegate);

	/** INitialize the end notify event */
	UFUNCTION(BlueprintCallable, Category = "AnimDriver")
	virtual void InitializeEnd(FAnimNotifyEventDelegate InEndDelegate);

	/** Initialize the tick notify event */
	UFUNCTION(BlueprintCallable, Category = "AnimDriver")
	virtual void InitializeTick(FAnimNotifyEventDelegate InTickDelegate);

	/** Set Notify Owner to new actor */
	UFUNCTION(BlueprintCallable, Category = "AnimDriver")
	virtual void SetNotifyOwner(AActor* InNotifyOwner) { NotifyOwner = InNotifyOwner; }

	UFUNCTION(BlueprintPure, Category = "AnimDriver")
	virtual AActor* GetNotifyOwner() const { return NotifyOwner; }

	/** Return notify name */
	UFUNCTION()
	FName GetNotifyName() { return NotifyName; };

	UFUNCTION()
	void SetNotifyName(FName InName) { NotifyName = InName; }

	/** If set, will clear the driver owner when the anim driver ends */
	void SetAutoClear(bool bAutoClear) { bAutoClearOwner = bAutoClear; }

protected:
	// If set, will clear the driver owner when the anim driver ends
	UPROPERTY(EditDefaultsOnly)
	bool bAutoClearOwner;

	// The actor we want to listen for notifies from
	UPROPERTY()
	AActor* NotifyOwner;

	// The notify name we want to listen for
	UPROPERTY(EditDefaultsOnly, Category = "AnimDriver")
	FName NotifyName;
	
	UPROPERTY()
	FAnimNotifyEventDelegate BeginNotify;

	UPROPERTY()
	FAnimNotifyEventDelegate TickNotify;

	UPROPERTY()
	FAnimNotifyEventDelegate EndNotify;
};
