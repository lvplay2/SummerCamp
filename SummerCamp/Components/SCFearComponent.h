// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/ActorComponent.h"
#include "StatModifiedValue.h"
#include "ILLNetSerialization.h"
#include "SCFearComponent.generated.h"

/**
 * @class USCFearEventData
 */
UCLASS(Blueprintable, NotPlaceable)
class USCFearEventData
: public UDataAsset
{
	GENERATED_BODY()

public:
	// The amount per second of fear to add
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FearEvent")
	float IncreaseFearValue;

	// The amount per second of fear to remove
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FearEvent")
	float DecreaseFearValue;

	// The amount of time before fear starts being added
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FearEvent")
	float IncreaseDelay;

	// The amount of time before fear starts being removed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FearEvent")
	float DecreaseDelay;

	// The total amount of fear this event can add
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FearEvent")
	float MaxFearCap;

	// When true this event will decrease fear instead of increase it
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FearEvent")
	bool bDecreaseWhenActive;

	// The amount of time before this event deactivates after hitting the fear cap. If 0 will do nothing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FearEvent")
	float AutoDeactivateTimer;

	// Fear distance curve
	UPROPERTY(EditDefaultsOnly, Category = "FearEvent")
	UCurveFloat* FearDistanceCurve;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FSCFearEventActivatedDelegate, TSubclassOf<USCFearEventData>, FearEvent);

/**
* @struct FFearEvent
*/
USTRUCT()
struct FFearEvent
{
	GENERATED_USTRUCT_BODY()

	FFearEvent()
	: CurrentFearAdded(0.f)
	, CurrentFearRemoved(0.f)
	, CurrentIncreaseDelay(0.f)
	, CurrentDecreaseDelay(0.f)
	, Active(false)
	, CurrentDeactivateTimer(0.f)
	, CurveData(0.f) {}

	float CurrentFearAdded;
	float CurrentFearRemoved;
	float CurrentIncreaseDelay;
	float CurrentDecreaseDelay;
	bool Active;
	float CurrentDeactivateTimer;
	float CurveData;
};

/**
 * @class USCFearComponent
 */
UCLASS(ClassGroup = "Fear", meta = (BlueprintSpawnableComponent))
class SUMMERCAMP_API USCFearComponent 
	: public UActorComponent
{
	GENERATED_UCLASS_BODY()

	virtual ~USCFearComponent();

	// Begin UActorComponent interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// End UActorComponent interface

	/** @return Fear amount. */
	UFUNCTION(BlueprintPure, Category = "Fear")
	FORCEINLINE float GetFear() const { return FearAmount; }

	/** @return Adjusted fear amount for effects. */
	UFUNCTION(BlueprintPure, Category = "Fear")
	FORCEINLINE float GetCosmeticFear() const { return CosmeticFear; }

	/**
	 * Pushes a fear event on to the stack
	 * @param FearEvent Class reference to the fear event data
	 */
	UFUNCTION(BlueprintCallable, Category = "Fear")
	void PushFearEvent(TSubclassOf<USCFearEventData> FearEvent, float CurveData = 0.0f);

	/**
	 * Pops a fear event off of the stack
	 * @param FearEvent Class reference to the fear event data
	 */
	UFUNCTION(BlueprintCallable, Category = "Fear")
	void PopFearEvent(TSubclassOf<USCFearEventData> FearEvent);

	/**
	 * The stat modifier used to multiply against base fear being added
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	FStatModifiedValue FearModifier;

	UPROPERTY()
	FSCFearEventActivatedDelegate FearEventActivated;

protected:
	/** Helper function to quickly check if this has authority. */
	FORCEINLINE bool HasAuthority() const { return GetOwner() ? GetOwner()->Role == ROLE_Authority : false; }

	/** Helper function to quiclky get the owner as an ASCCounselorCharacter* */
	class ASCCounselorCharacter* GetCharacterOwner() const;

	/** Helper function to quickly tell if this is locally controlled. */
	bool IsLocallyControlled() const;

	/** The currently active fear amount. Is only modified from the server. */
	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	FFloat_NetQuantize FearAmount;

	UPROPERTY(BlueprintReadOnly, Transient)
	float CosmeticFear;

	// Time between TickComponent updates
	UPROPERTY()
	float UpdateFearDelay;

	// Time until TickComponent is allowed to update
	UPROPERTY(Transient)
	float TimeUntilFearUpdate;

	/** The stack of fear events being processed. */
	UPROPERTY()
	TMap<TSubclassOf<USCFearEventData>, FFearEvent> ActiveFearEvents;

public:

	class LockFearCMD
	{
		friend USCFearComponent;

	public:
		LockFearCMD() : Owner(nullptr), LockFearCmdHandle(nullptr) {}
		LockFearCMD(USCFearComponent* _Owner) : Owner(_Owner), LockFearCmdHandle(nullptr) {}
		void Register();
		void UnRegister();
		void LockFearToCMD(const TArray<FString>& Args);

		USCFearComponent* Owner;
		IConsoleCommand* LockFearCmdHandle;
		bool bRegistered;
	} LockFearCommand;

	/** Set fear to specified level and lock it there
	 * @param InFearLevel - The amount of fear to set the current fear value to.
	 * @param Lock - Locks the current fear value to the passed in value if set to true.
	 */
	UFUNCTION()
	void LockFearTo(float InFearLevel, bool Lock = true);

	/** Notifies the server to call SC_LockFearTo
	* @param InFearLevel - The amount of fear to set the current fear value to.
	* @param Lock - Locks the current fear value to the passed in value if set to true.
	*/
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_LockFearTo(float NewFear, bool Lock);

	/** Locks fear to its current setting and stops fear events from updating. */
	UPROPERTY(Replicated, Transient)
	bool bLockFearLevel;

	/** Register our Console command to lock fear level */
	UFUNCTION()
	void RegisterLockFearCMD();
};
