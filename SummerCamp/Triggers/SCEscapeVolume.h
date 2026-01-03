// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCEscapeVolume.generated.h"

class ASCCounselorCharacter;

class USCMinimapIconComponent;

class USplineComponent;

/**
* @enum ESCEscapeType
*/
UENUM(BlueprintType)
enum class ESCEscapeType : uint8
{
	Police,
	Car,
	Boat,
	EscapePod,
	GrendelBridge,
	MAX UMETA(Hidden)
};

UCLASS()
class SUMMERCAMP_API ASCEscapeVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASCEscapeVolume(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor Interface
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END AActor Interface

	/** The collision volume for detecting if a counselor has escaped. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	USphereComponent* EscapeVolume;

	/** The path the character should follow while leaving the area
	 * This path is automatically adjusted to take the characters position upon entering into account.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	USplineComponent* ExitPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	class USCCameraSplineComponent* EscapeCamSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	class USCSplineCamera* EscapeCam;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	USceneComponent* SpectatorStartPoint;

	UPROPERTY(EditAnywhere, Category = "Default")
	USCMinimapIconComponent* MinimapIconComponent;

	UPROPERTY(EditAnywhere, Category = "Default")
	float CameraLerpTime;

	/** Called to activate the volume if it is not available by default */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_ActivateVolume();

	/** Deactivate the volume if it is rendered unuseable or inactive on start. */
	UFUNCTION(NetMulticast, Reliable)
	void DeactivateVolume();

	/** @return true if this escape volume is enabled and counselors can escape with it */
	bool CanEscapeHere() const;

	/** Check that there is nothing in the volume that shouldn't be before spawning.
	 * @return true if there are no obstructions
	 */
	UFUNCTION()
	bool IsUnobstructed();

	// Escape via Police, Car, or Boat?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default")
	ESCEscapeType EscapeType;

protected:
	/** Set our counselor escaped immediately after the camera spline finishes? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default")
	bool bUseSplineCameraDurationAsDelay;

	/** if not UseSplineCameraDirationAsDelay, use this value */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default")
	float EscapeDelay;

	// Used to ignore attempting to follow an exit spline.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default")
	bool bIgnoreExitSpline;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void SetupEscapeCamera(ASCCounselorCharacter* Counselor);

private:
	FTimerHandle TimerHandle_SetDriverCamera;
};
