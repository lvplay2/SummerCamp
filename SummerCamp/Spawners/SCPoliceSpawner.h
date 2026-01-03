// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCPoliceSpawner.generated.h"

UCLASS()
class SUMMERCAMP_API ASCPoliceSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASCPoliceSpawner(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Police car blueprint to spawn */
	UPROPERTY(EditAnywhere, Category = "Default")
	TSubclassOf<AActor> PoliceSpawnClass;

	/** Attempt to spawn police at this location.
	 * @return true if police can be spawned, false if something(Jason) is blocing spawn
	 */
	UFUNCTION()
	bool ActivateSpawner();

	UFUNCTION(BlueprintImplementableEvent, Category = "Escape")
	void OnPoliceSpawned();

	/** The escape volume trigger assiciated with this police spawn */
	UPROPERTY(EditAnywhere, Category = "Default")
	class ASCEscapeVolume* EscapeVolume;

	/** The trigger volume that notifies when Jason is getting close to the escape volume */
	UPROPERTY(EditAnywhere, Category = "Default")
	class AVolume* JasonTriggerVolume;

	/** The location of ideal interception, Like the middle of the road. */
	UPROPERTY(EditAnywhere, Category = "Default")
	USceneComponent* InterceptDirection;

	/** Audio cue to play when Killer enters the JasonTriggerVolume */
	UPROPERTY(EditAnywhere, Category = "Default")
	USoundCue* PoliceSpotJasonAudio;
	
protected:
	/** Notify for when the EscapeVolume is triggered */
	UFUNCTION()
	void JasonTriggerOnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Notify for when the JasonTriggerVolume is triggered */
	UFUNCTION()
	void PoliceSpotJasonBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

};
