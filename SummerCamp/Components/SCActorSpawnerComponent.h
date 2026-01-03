// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/ActorComponent.h"
#include "SCActorSpawnerComponent.generated.h"

/**
 * @class USCActorSpawnerComponent
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SUMMERCAMP_API USCActorSpawnerComponent
: public UChildActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UActorComponent Interface
	virtual void InitializeComponent() override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void CheckForErrors() override;
#endif
	// End UActorComponent Interface

	UPROPERTY(EditAnywhere, Category="Default")
	TArray<TSubclassOf<AActor>> ActorList;

	UFUNCTION()
	void DestroySpawnedActor();

	UFUNCTION(BlueprintPure, Category="Default")
	AActor* GetSpawnedActor() const { return SpawnedActor; }

	UFUNCTION(BlueprintCallable, Category="Default")
	bool SpawnActor(TSubclassOf<AActor> ClassOverride = nullptr);

	bool SupportsType(const TSubclassOf<AActor> Type) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Default")
	bool bSpawnOnInitialize;

private:
	UPROPERTY()
	UBillboardComponent* Billboard;

	UPROPERTY(Transient)
	AActor* SpawnedActor;
};
