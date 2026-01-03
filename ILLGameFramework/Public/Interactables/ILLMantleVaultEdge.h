// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/Actor.h"
#include "ILLMantleVaultInteractableComponent.h"
#include "ILLMantleVaultEdgeRenderingComponent.h"
#include "ILLMantleVaultEdge.generated.h"

/**
 * @struct FMantleVaultEdgePoint
 */
USTRUCT(BlueprintType)
struct ILLGAMEFRAMEWORK_API FMantleVaultEdgePoint
{
	GENERATED_USTRUCT_BODY()

	// Location of this point along an edge
	UPROPERTY(EditAnywhere, Category=Default, BlueprintReadWrite, meta=(MakeEditWidget=""))
	FVector Location;

	// Ground location, or location where the ground search ended
	FVector GroundSpot;

	// Is this edge point grounded to a surface?
	bool bGrounded;

	FMantleVaultEdgePoint()
	: Location(0, 0, 0) {}

	FMantleVaultEdgePoint(const FVector& InLocation) 
	: Location(InLocation) {}
};

/**
 * @enum EILLMantleVaultType
 */
UENUM(BlueprintType)
enum class EILLMantleVaultType : uint8
{
	Mantle,
	Vault
};

/**
 * @class AILLMantleVaultEdge
 */
UCLASS()
class ILLGAMEFRAMEWORK_API AILLMantleVaultEdge
: public AActor
{
	GENERATED_UCLASS_BODY()
		
	// Begin AActor interface
	virtual void PostInitializeComponents() override;
#if WITH_EDITOR
	virtual void CheckForErrors() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
	virtual void PostEditMove(bool bFinished) override;
#endif
	// End AActor interface

	//////////////////////////////////////////////////
	// Edge and interaction info

	// Edge points, should be a minimum of two of these for obvious reasons
	UPROPERTY(EditAnywhere, Category="Edges")
	TArray<FMantleVaultEdgePoint> EdgePoints;

	// Should we mantle or vault?
	UPROPERTY(EditAnywhere, Category="Edges")
	EILLMantleVaultType InteractionType;

private:
	// Helper function to update bGrounded on each of the EdgePoints
	void CheckGroundedness(const bool bForce = false);

#if WITH_EDITORONLY_DATA
	// Delegate fired when an object is selected in the editor
	void OnObjectSelected(UObject* Object);
#endif

	/** Edge evaluator used by GetDistanceCullLocation and GetInteractionLocation. */
	void EdgeEvaluator(class AILLCharacter* Character, const int32 FirstIndex, const int32 SecondIndex, float* OutClosestDistanceSq = nullptr, FVector* OutClosestEdgePoint = nullptr) const;

	/** Helper to call the EdgeEvaluator on all edges in this loop. */
	void EvaluateEdges(class AILLCharacter* Character, float* OutClosestDistanceSq = nullptr, FVector* OutClosestEdgePoint = nullptr) const;

	// Have we ever checked grounded-ness?
	bool bCheckedGroundedness;

	//////////////////////////////////////////////////
	// Interactable component
public:
	/** @return Interactable component. */
	FORCEINLINE UILLMantleVaultInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

protected:
	// Interactable component
	UPROPERTY(EditAnywhere, Category="Interaction")
	UILLMantleVaultInteractableComponent* InteractableComponent;

	/** Callback for UILLInteractableComponent::OnBecameBest. */
	UFUNCTION()
	void OnBecameBest(AActor* Interactor);

	/** Callback for UILLInteractableComponent::OnInteract. */
	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/** @return Interaction location override for InteractableComponent. */
	UFUNCTION()
	FVector OverrideInteractionLocation(AILLCharacter* Character);

	//////////////////////////////////////////////////
	// Editor components
private:
#if WITH_EDITORONLY_DATA
	// Editor preview
	UPROPERTY()
	UILLMantleVaultEdgeRenderingComponent* EdRenderComp;

	// Editor sprite
	UPROPERTY()
	UBillboardComponent* SpriteComponent;
#endif // WITH_EDITORONLY_DATA
};
