// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCShoreItemSpawner.generated.h"

/**
 * @class ASCShoreItemSpawner
 */
UCLASS()
class ASCShoreItemSpawner
: public AActor
{
	GENERATED_BODY()

public:
	ASCShoreItemSpawner(const FObjectInitializer& ObjectInitializer);

	// Marked as used when a player uses this to spawn an item (SERVER only)
	bool bIsInUse;

#if WITH_EDITOR
	virtual void CheckForErrors() override;
#endif

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default", meta = (AllowPrivateAccess = "true"))
	class UBillboardComponent* SpriteComponent;
#endif
};
