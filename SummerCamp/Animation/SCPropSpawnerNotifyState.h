// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SCPropSpawnerNotifyState.generated.h"

class ASCCharacter;

/**
 * @class UILLPropSpawnerNotifyState
 */
UCLASS(meta=(DisplayName="SC Prop Spawner"))
class SUMMERCAMP_API USCPropSpawnerNotifyState
: public UAnimNotifyState
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UObject interface
	virtual void BeginDestroy() override;
	// End UObject interface

	// Begin UAnimNotifyState interface
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual FString GetNotifyName_Implementation() const override;
	// TODO: Add support for properties changing so aligning objects can be less painful
	// End UAnimNotifyState interface

	// If set to true, will hide any currently equipped item
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	bool bHideEquippedItem;

	// Skeletal mesh to attach to character mesh
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	USkeletalMesh* SkeletalPropMesh;

	// Animation for the skeletal prop to play
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	UAnimationAsset* PropAnimation;

	// Should this animation loop for the life of the prop?
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	bool bLoopPropAnimation;

	// TODO: Add support for static mesh props

	// Socket or bone to attach to
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	FName SocketName;

	// Extra offset to apply from socket position
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	FVector LocationOffset;

	// Extra rotation to apply from socket orientation
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	FRotator RotationOffset;

	// If true, will remove the mesh when the notify state completes, if false you will need to clean it up yourself!
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	bool bAutoDestroy;

private:
	// Internal handle for the skeletal mesh component
	TArray<TWeakObjectPtr<USkeletalMeshComponent>> SkeletalMeshCompList;

	// Used to notify the owner of this prop spawner
	UPROPERTY(Transient)
	ASCCharacter* OwningCharacter;

	// Don't un-hide an item that was already hidden
	bool bItemWasVisible;

	/** Restores item visiiblity for the currently equipped item when this notify ends */
	void RestoreItemVisiblity();

	/** Will remove the skeletal mesh if set and if bAutoDestroy is true */
	void TryCleanupSkeletalMeshes();
};
