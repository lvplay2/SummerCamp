// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Volume.h"
#include "SCPowerGridVolume.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCPowerGridUpdated, bool, bPowered);

class ASCCounselorCharacter;
class ASCFuseBox;

class USCFearEventData;

/**
 * @class ASCPowerGridVolume
 */
UCLASS()
class SUMMERCAMP_API ASCPowerGridVolume
: public AActor
{
	GENERATED_BODY()

public:
	ASCPowerGridVolume(const FObjectInitializer& ObjectInitializer);

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	// End AActor interface

	// Area to search for powered objects
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	UBoxComponent* PoweredArea;

	// Fear event to apply to any counselors inside the grid when power goes out
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fear")
	TSubclassOf<USCFearEventData> BlackOutFearEvent;

	// When a light is turned on it will randomly select one of these curves to adjust its intensity over time before turning fully on
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lights")
	TArray<UCurveFloat*> PowerOnCurves;

	// When a light is turned off it will randomly select one of these curves to adjust its intensity over time before turning fully off
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lights")
	TArray<UCurveFloat*> PowerOffCurves;

	/** Called from the power plant */
	void AddPoweredObject(UObject* PoweredObject);

	/** If true, this grid has the power on */
	UFUNCTION(BlueprintPure, Category = "Power")
	bool GetIsPowered() const { return !IsDeactivated(); }

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void RequestDeactivateLights(const bool bDeactivateLights, const bool bShouldSkipFlicker = false);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_DeactivateLights(const bool bDeactivateLights, const bool bShouldSkipFlicker = false);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_DeactivateLights(const bool bDeactivateLights, const bool bShouldSkipFlicker = false);

	FORCEINLINE bool IsDeactivated() const { return bDeactivated; }
	FORCEINLINE int32 GetPoweredObjectCount() const { return PoweredObjects.Num(); }
	FORCEINLINE const TArray<UObject*>& GetPoweredObjects() const { return PoweredObjects; }
	FORCEINLINE bool ContainsPoweredObject(const UObject* PoweredObject) const { return PoweredObjects.Contains(PoweredObject); }
	FORCEINLINE void SetFuseBox(ASCFuseBox* FuseBox) { ConnectedFuseBox = FuseBox; }
	FORCEINLINE ASCFuseBox* GetFuseBox() const { return ConnectedFuseBox; }
	FORCEINLINE bool ContainsCounselor(const ASCCounselorCharacter* Counselor) const { return Counselors.Contains(Counselor); }

protected:
	void DeactivateLights(const bool bDeactivateLights, const bool bShouldSkipFlicker = false);

	/** Gets a random power on/off curve */
	UCurveFloat* GetRandomCurve(const bool bPowerOn) const;

	// Counselors inside of the grid
	UPROPERTY(transient)
	TArray<ASCCounselorCharacter*> Counselors;

	// Fuse box that powers this grid
	UPROPERTY(transient, BlueprintReadOnly, Category = "Power")
	ASCFuseBox* ConnectedFuseBox;

	// Powered objects inside of the grid (filled out by the PowerPlant)
	UPROPERTY(transient)
	TArray<UObject*> PoweredObjects;

	// Blueprint callback for when power is turned on or off
	UPROPERTY(BlueprintAssignable, Category = "Power")
	FSCPowerGridUpdated OnPowerChanged;

	// If true, the current blackout doesn't care about flickering the lights on or off
	bool bSkipFlicker;

	// Number of objects we're going to blackout per wave
	int32 MaxBlackoutObjectsPerWave;

	// Index into powered objects that we last left off at when turning the objects on/off
	int32 CurrentBlackoutPosition;

	// Timestamp to perform the next wave of turning objects on/off
	float NextBlackoutWave_Timestamp;

	// Longest time (in seconds) for all power on curves
	float LongestPowerOnCurve;

	// Longest time (in seconds) for all power off curves
	float LongestPowerOffCurve;

	// If true, the power is out!
	bool bDeactivated;
};
