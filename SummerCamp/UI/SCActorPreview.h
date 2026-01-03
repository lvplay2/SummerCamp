// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCActorPreview.generated.h"

/**
 * @class ASCActorPreview
 */
UCLASS()
class SUMMERCAMP_API ASCActorPreview 
: public AActor
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Capture")
	USceneCaptureComponent2D* CaptureComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PreviewActor")
	TSoftClassPtr<AActor> PreviewClass;

	UFUNCTION(BlueprintCallable, Category = "PreviewActor")
	AActor* GetPreviewActor() const { return PreviewActor; }

	UFUNCTION(BlueprintCallable, Category = "PreviewActor")
	void SpawnPreviewActor();

	UFUNCTION(BlueprintCallable, Category = "PreviewActor")
	void DestroyPreviewActor();

	virtual void BeginDestroy();

private:
	UPROPERTY()
	AActor* PreviewActor;
};