// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCAfterimage.generated.h"

UCLASS()
class SUMMERCAMP_API ASCAfterimage : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASCAfterimage(const FObjectInitializer& ObjectInitializer);


	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* SkelMesh;
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* StatMesh;

	FORCEINLINE USkeletalMeshComponent* GetSkeletalMesh()
	{
		return SkelMesh;
	}
	FORCEINLINE UStaticMeshComponent* GetStaticMesh()
	{
		return StatMesh;
	}

	/** Set new display skeletal mesh */
	void ChangeAfterimageMesh(USkeletalMesh* newMesh);
	/** Set new display static mesh */
	void ChangeAfterimageMesh(UStaticMesh* newMesh);

	/** Slave to new Skeletal mesh (any master static mesh is removed) */
	void SlaveToMesh(USkeletalMeshComponent* OtherSkeletalMesh, bool follow = true);

	/** Slave to new static mesh (any master skeletal mesh is removed) */
	void SlaveToMesh(UStaticMeshComponent* OtherStaticMesh, bool follow = true);

	/** Stop mimicing the master mesh */
	UFUNCTION(BlueprintCallable, Category = "Skeletal Binding")
	void FreezeAnim();

	/** Resume mimicing the master mesh */
	UFUNCTION(BlueprintCallable, Category = "Skeletal Binding")
	void ResumeAnim();

	/** Stop following position of master mesh */
	UFUNCTION(BlueprintCallable, Category = "Skeletal Binding")
	void StopFollow();

	/** Resume following position of master mesh */
	UFUNCTION(BlueprintCallable, Category = "Skeletal Binding")
	void ResumeFollow();

	UFUNCTION()
	void SetDistanceTarget(AActor* Target, float MaxDistance);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	void SetVisibility(bool Skel, bool Static);

protected:
	/** Skeletal mesh to slave to */
	UPROPERTY()
	USkeletalMeshComponent* SkeletalMaster;

	/** Static mesh to slave to */
	UPROPERTY()
	UStaticMeshComponent* StaticMaster;

	bool bFollowMaster;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fade")
	bool bFadeIn;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fade")
	float FadeInTime;
	bool bFullyFadedIn;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fade")
	bool bFadeOut;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fade")
	float FadeOutTime;
	bool bBeginFadeOut;
	float FadeTimer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fade")
	UCurveFloat* FadeInCurve;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fade")
	UCurveFloat* FadeOutCurve;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fade")
	UCurveFloat* FadeDistanceCurve;

	UPROPERTY()
	AActor* DistanceTarget;
	float MaxDistanceFromTarget;

	UFUNCTION()
	void UpdateFade(float DeltaSeconds);
	
};
