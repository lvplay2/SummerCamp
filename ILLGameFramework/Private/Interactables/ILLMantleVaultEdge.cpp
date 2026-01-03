// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLMantleVaultEdge.h"

#if WITH_EDITOR
# include "Editor.h"
#endif
#include "UObjectToken.h"
#include "MapErrors.h"
#include "MessageLog.h"

#include "ILLCharacter.h"
#include "ILLInteractableManagerComponent.h"
#include "ILLSpecialMove_Mantle.h"
#include "ILLSpecialMove_Vault.h"

static const float CAPSULE_RADIUS_EDGE_CORNER_OFFSET = .9f;
static const float CAPSULE_RADIUS_FORWARD_SCALE = .5f;

FCollisionShape FloorSearchCollisionShape = FCollisionShape::MakeSphere(4.f);
static const float POINT_GROUND_IDEAL_DISTANCE = 10.f;

UILLMantleVaultEdgeRenderingComponent::UILLMantleVaultEdgeRenderingComponent(const FObjectInitializer& ObjInitializer)
: Super(ObjInitializer)
{
}

FPrimitiveSceneProxy* UILLMantleVaultEdgeRenderingComponent::CreateSceneProxy()
{
	struct FMantleVaultPointDrawing
	{
		FVector Location;
		const FVector* GroundSpot;
		const bool* bGrounded;
	};

	class FMantleVaultEdgeSceneProxy
	: public FPrimitiveSceneProxy
	{
	public:
		FMantleVaultEdgeSceneProxy(const UPrimitiveComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
		{
			AILLMantleVaultEdge* OwnerEdge = Cast<AILLMantleVaultEdge>(InComponent->GetOwner());
			if (OwnerEdge)
			{
				InteractableComponent = OwnerEdge->GetInteractableComponent();
				for (FMantleVaultEdgePoint& Other : OwnerEdge->EdgePoints)
				{
					FMantleVaultPointDrawing Entry;
					Entry.Location = Other.Location;
					Entry.GroundSpot = &Other.GroundSpot;
					Entry.bGrounded = &Other.bGrounded;
					EdgePoints.Push(Entry);
				}
			}

			bWillEverBeLit = false;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			const FMatrix& LTW = GetLocalToWorld();
			static const FColor RadiusColor(150, 160, 150, 48);
			FMaterialRenderProxy* const MeshColorInstance = new(FMemStack::Get()) FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(false), RadiusColor);
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
				if (VisibilityMap & (1 << ViewIndex))
				{
					for (int32 Index = 0; Index < EdgePoints.Num()-1; ++Index)
					{
						DrawEdge(PDI, LTW, Index, Index+1, MeshColorInstance, ViewIndex, Collector);
					}
				}
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = IsShown(View) && IsSelected();
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = IsShadowCast(View);
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
			return Result;
		}
		virtual uint32 GetMemoryFootprint() const override { return sizeof(*this) + GetAllocatedSize(); }
		uint32 GetAllocatedSize() const { return FPrimitiveSceneProxy::GetAllocatedSize() ; }

	private:
		enum class EPointState : uint8
		{
			Ungrounded,
			Grounded,
			NearlyGrounded,
		};

		void DrawEdge(FPrimitiveDrawInterface* PDI, const FMatrix& LTW, const int32 StartIndex, const int32 EndIndex, FMaterialRenderProxy* const MeshColorInstance, const int32 ViewIndex, FMeshElementCollector& Collector) const
		{
			const FMantleVaultPointDrawing& FirstPoint = EdgePoints[StartIndex];
			const bool bFirstPtGrounded = *FirstPoint.bGrounded;
			const FVector RealFirstLocation = LTW.TransformPosition(FirstPoint.Location);
			const FLinearColor FirstPtColor = bFirstPtGrounded ? ((RealFirstLocation - *FirstPoint.GroundSpot).IsNearlyZero(POINT_GROUND_IDEAL_DISTANCE) ? FLinearColor::Green : FLinearColor::Yellow) : FLinearColor::Red;

			const FMantleVaultPointDrawing& SecondPoint = EdgePoints[EndIndex];
			const bool bSecondPtGrounded = *SecondPoint.bGrounded;
			const FVector RealSecondLocation = LTW.TransformPosition(SecondPoint.Location);
			const FLinearColor SecondPtColor = bSecondPtGrounded ? ((RealSecondLocation - *SecondPoint.GroundSpot).IsNearlyZero(POINT_GROUND_IDEAL_DISTANCE) ? FLinearColor::Green : FLinearColor::Yellow) : FLinearColor::Red;

			// Points
			DrawPoint(PDI, FirstPoint, RealFirstLocation, FirstPtColor, MeshColorInstance, ViewIndex, Collector);
			DrawPoint(PDI, SecondPoint, RealSecondLocation, SecondPtColor, MeshColorInstance, ViewIndex, Collector);

			// Point connection
			DrawDashedLine(PDI, RealFirstLocation, RealSecondLocation, FLinearColor::Blue, 4.f, SDPG_World, 10.f);

			// Point mid-sections
			const FVector FirstLocation = bFirstPtGrounded ? *FirstPoint.GroundSpot : RealFirstLocation;
			const FVector SecondLocation = bSecondPtGrounded ? *SecondPoint.GroundSpot : RealSecondLocation;
			const FVector FirstToSecond = SecondLocation - FirstLocation;
			const FVector MidPoint = FirstLocation + FirstToSecond.GetSafeNormal()*FirstToSecond.Size()*.5f;
			PDI->DrawLine(FirstLocation, MidPoint, FirstPtColor, SDPG_World, 0.f, 10.f, true);
			PDI->DrawLine(MidPoint, SecondLocation, SecondPtColor, SDPG_World, 0.f, 10.f, true);

			// Point arrows along interaction direction, for the interaction distance
			const FRotator InteractionDirection = InteractableComponent->GetComponentTransform().GetUnitAxis(EAxis::X).Rotation();
			const FRotationTranslationMatrix FirstArrowTransform(InteractionDirection, FirstLocation);
			const FRotationTranslationMatrix SecondArrowTransform(InteractionDirection, SecondLocation);
			DrawDirectionalArrow(PDI, FirstArrowTransform, FirstPtColor, InteractableComponent->DistanceLimit, 1.f, SDPG_World);
			DrawDirectionalArrow(PDI, SecondArrowTransform, SecondPtColor, InteractableComponent->DistanceLimit, 1.f, SDPG_World);

			// Middle arrow
			const bool bEdgeIsGrounded = (bFirstPtGrounded && bSecondPtGrounded);
			const FVector ArrowEdgeLocation = FMath::ClosestPointOnLine(FirstLocation, SecondLocation, LTW.GetOrigin());
			const FRotationTranslationMatrix CenterArrowTransform(InteractionDirection, ArrowEdgeLocation);
			DrawDirectionalArrow(PDI, CenterArrowTransform, bEdgeIsGrounded ? FLinearColor::Green : FLinearColor::Yellow, InteractableComponent->DistanceLimit, 1.f, SDPG_World);
		}

		FORCEINLINE void DrawPoint(FPrimitiveDrawInterface* PDI, const FMantleVaultPointDrawing& Point, const FVector& WorldLocation, const FLinearColor& Color, FMaterialRenderProxy* const MeshColorInstance, const int32 ViewIndex, FMeshElementCollector& Collector) const
		{
			GetCylinderMesh(WorldLocation, *Point.GroundSpot, FloorSearchCollisionShape.GetSphereRadius(), 16, MeshColorInstance, SDPG_World, ViewIndex, Collector);
			PDI->DrawLine(WorldLocation, *Point.GroundSpot, Color, SDPG_World);

			if ((WorldLocation - *Point.GroundSpot).IsNearlyZero(POINT_GROUND_IDEAL_DISTANCE))
			{
				PDI->DrawPoint(WorldLocation, Color, 32.f, SDPG_World);
			}
			else if (*Point.bGrounded)
			{
				PDI->DrawPoint(WorldLocation, Color, 10.f, SDPG_World);
				PDI->DrawPoint(*Point.GroundSpot, Color, 20.f, SDPG_World);
			}
			else
			{
				PDI->DrawPoint(WorldLocation, Color, 20.f, SDPG_World);
			}
		}

		UILLMantleVaultInteractableComponent* InteractableComponent;
		TArray<FMantleVaultPointDrawing> EdgePoints;
	};

	return new FMantleVaultEdgeSceneProxy(this);
}

FBoxSphereBounds UILLMantleVaultEdgeRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (AILLMantleVaultEdge* OwnerEdge = Cast<AILLMantleVaultEdge>(GetOwner()))
	{
		FBox BoundingBox(EForceInit::ForceInitToZero);
		for (const FMantleVaultEdgePoint& Entry : OwnerEdge->EdgePoints)
		{
			BoundingBox += Entry.Location;
		}

		return FBoxSphereBounds(BoundingBox).TransformBy(LocalToWorld);
	}

	return FBoxSphereBounds(EForceInit::ForceInitToZero);
}

AILLMantleVaultEdge::AILLMantleVaultEdge(const FObjectInitializer& ObjInitializer)
: Super(ObjInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	bHidden = true;

	InteractableComponent = CreateDefaultSubobject<UILLMantleVaultInteractableComponent>(TEXT("InteractableComponent"));
	InteractableComponent->bInteractWhenOwnerHidden = true;
	InteractableComponent->DistanceLimit = 150.f;
	InteractableComponent->bUseYawLimit = true;
	InteractableComponent->MaxYawOffset = 45.f;
	InteractableComponent->bUsePitchLimit = true;
	InteractableComponent->SetupAttachment(RootComponent);

#if WITH_EDITORONLY_DATA
	EdRenderComp = CreateDefaultSubobject<UILLMantleVaultEdgeRenderingComponent>(TEXT("EdRenderComp"));
	EdRenderComp->PostPhysicsComponentTick.bCanEverTick = false;
	EdRenderComp->SetupAttachment(RootComponent);

	USelection::SelectionChangedEvent.AddUObject(this, &ThisClass::OnObjectSelected);
	USelection::SelectObjectEvent.AddUObject(this, &ThisClass::OnObjectSelected);
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (!IsRunningCommandlet() && (SpriteComponent != NULL))
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/Engine/EditorResources/AI/S_NavLink"));

		SpriteComponent->Sprite = SpriteTexture.Get();
		SpriteComponent->RelativeScale3D = FVector(0.5f, 0.5f, 0.5f);
		SpriteComponent->bHiddenInGame = true;
		SpriteComponent->bVisible = true;

		SpriteComponent->SetupAttachment(RootComponent);
		SpriteComponent->SetAbsolute(false, false, true);
		SpriteComponent->bIsScreenSizeScaled = true;
	}
#endif

	EdgePoints.Add(FMantleVaultEdgePoint(FVector(0,-100,0)));
	EdgePoints.Add(FMantleVaultEdgePoint(FVector(0,100,0)));
	SetActorEnableCollision(false);

	bCanBeDamaged = false;
}

void AILLMantleVaultEdge::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Bind component delegates
	InteractableComponent->OverrideInteractionLocation.BindDynamic(this, &ThisClass::OverrideInteractionLocation);
	InteractableComponent->OnBecameBest.AddDynamic(this, &ThisClass::OnBecameBest);
	InteractableComponent->OnInteract.AddDynamic(this, &ThisClass::OnInteract);
}

#if WITH_EDITOR
void AILLMantleVaultEdge::CheckForErrors()
{
	Super::CheckForErrors();

	// Check that each point is grounded
	CheckGroundedness(true);

	// Output an error if any of them aren't
	FMessageLog MapCheck("MapCheck");
	if (EdgePoints.Num() == 2)
	{
		for (const FMantleVaultEdgePoint& Point : EdgePoints)
		{
			if (!Point.bGrounded)
			{
				static FName NAME_MantleVaultEdgeFloating(TEXT("MantleVaultEdgeFloating"));
				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
				MapCheck.Warning()
					->AddToken(FUObjectToken::Create(this))
					->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} has point(s) that are floating or encroaching geometry")), Arguments)))
					->AddToken(FMapErrorToken::Create(NAME_MantleVaultEdgeFloating));
				break;
			}
		}
	}
	else
	{
		static FName NAME_MantleVaultTooManyEdgePoints(TEXT("MantleVaultTooManyEdgePoints"));
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} has too many edge point(s)! Behavior is undefined!")), Arguments)))
			->AddToken(FMapErrorToken::Create(NAME_MantleVaultTooManyEdgePoints));
	}
}

void AILLMantleVaultEdge::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	CheckGroundedness(true);
}

void AILLMantleVaultEdge::PostEditUndo()
{
	Super::PostEditUndo();

	CheckGroundedness(true);
}

void AILLMantleVaultEdge::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	CheckGroundedness(true);
}
#endif // WITH_EDITOR

void AILLMantleVaultEdge::CheckGroundedness(const bool bForce/* = false*/)
{
	if (!bForce && bCheckedGroundedness)
		return;

	if (UWorld* World = GetWorld())
	{
		static FName NAME_MantleVaultErrorCheck(TEXT("MantleVaultErrorCheck"));
		FCollisionQueryParams TraceParams(NAME_MantleVaultErrorCheck, true, this);
		TraceParams.bTraceAsyncScene = true;
		for (FMantleVaultEdgePoint& Point : EdgePoints)
		{
			// Sweep to find the floor
			FHitResult SweepResult(ForceInit);
			const FVector WorldLocation = GetTransform().TransformPosition(Point.Location);
			const FVector StartSweep = WorldLocation + FVector(0.f, 0.f, FloorSearchCollisionShape.GetSphereRadius());
			const FVector EndSweep = WorldLocation - FVector(0.f, 0.f, POINT_GROUND_IDEAL_DISTANCE*2.f);
			World->SweepSingleByChannel(SweepResult, StartSweep, EndSweep, FQuat::Identity, ECC_WorldStatic, FloorSearchCollisionShape, TraceParams);

			Point.bGrounded = false;
			bool bCompensateForSphere = true;
			if (SweepResult.bStartPenetrating)
			{
				// Started by penetrating, so fall back to a simple line trace
				FHitResult LineResult(ForceInit);
				World->LineTraceSingleByChannel(LineResult, StartSweep, EndSweep, ECC_WorldStatic, TraceParams);
				if (LineResult.bBlockingHit)
				{
					SweepResult = LineResult;
					bCompensateForSphere = false;
				}
			}

			if (SweepResult.bBlockingHit)
			{
				// Sweep result hit something, check if we are inside of something
				FHitResult EncroachResult(ForceInit);
				const FVector StartEncroach = SweepResult.Location + FVector(0.f, 0.f, FloorSearchCollisionShape.GetSphereRadius()*2.f);
				const FVector EndEncroach = SweepResult.Location + FVector(0.f, 0.f, FloorSearchCollisionShape.GetSphereRadius() + 1.f); // +1.f to compensate for low precision
				World->SweepSingleByChannel(EncroachResult, StartEncroach, EndEncroach, FQuat::Identity, ECC_WorldStatic, FloorSearchCollisionShape, TraceParams);
				if (!EncroachResult.bStartPenetrating)
				{
					Point.bGrounded = true;
					Point.GroundSpot = EncroachResult.bBlockingHit ? EncroachResult.Location : SweepResult.Location;
					if (bCompensateForSphere)
						Point.GroundSpot.Z -= FloorSearchCollisionShape.GetSphereRadius();
				}
			}

			// Fallback to the bottom of the sweep location
			if (!Point.bGrounded)
				Point.GroundSpot = EndSweep;
		}

		bCheckedGroundedness = true;
	}
}

#if WITH_EDITOR
void AILLMantleVaultEdge::OnObjectSelected(UObject* Object)
{
	if (Object == this)
	{
		CheckGroundedness(true);
	}
	else if (USelection* Selection = Cast<USelection>(Object))
	{
		TArray<UObject*> SelectedEdges;
		if (Selection->GetSelectedObjects(AILLMantleVaultEdge::StaticClass(), SelectedEdges) > 0)
		{
			for (UObject* SelectedObject : SelectedEdges)
			{
				if (AILLMantleVaultEdge* Edge = Cast<AILLMantleVaultEdge>(SelectedObject))
				{
					Edge->CheckGroundedness(true);
				}
			}
		}
	}
}
#endif // WITH_EDITOR

void AILLMantleVaultEdge::EdgeEvaluator(class AILLCharacter* Character, const int32 FirstIndex, const int32 SecondIndex, float* OutClosestDistanceSq/* = nullptr*/, FVector* OutClosestEdgePoint/* = nullptr*/) const
{
	auto GetPointLocation = [this](const FMantleVaultEdgePoint& Point) -> FVector
	{
		if (Point.bGrounded)
			return Point.GroundSpot;

		return GetTransform().TransformPosition(Point.Location);
	};

	// Transform the corner locations into world space to build a line
	const FVector FirstLocation = GetPointLocation(EdgePoints[FirstIndex]);
	const FVector SecondLocation = GetPointLocation(EdgePoints[SecondIndex]);

	// Determine how far to offset from the corners
	const float CapsuleRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector FirstSecondDelta = SecondLocation - FirstLocation;
	const float CornerOffset = FMath::Min(CapsuleRadius*CAPSULE_RADIUS_EDGE_CORNER_OFFSET, FirstSecondDelta.Size()*.5f);

	// Now adjust the corners inward by the CornerOffset, so you will not mantle/vault at the very edge point
	const FVector FirstToSecond = FirstSecondDelta.GetSafeNormal();
	const FVector PaddedFirstLocation = FirstLocation + FirstToSecond * CornerOffset;
	const FVector PaddedSecondLocation = SecondLocation + FirstToSecond * -CornerOffset;

	// Find the closest point on the Character's capsule to the edge
	const FVector ActorLocation = GetActorLocation();
	const FVector ClosestToCharacter = FMath::ClosestPointOnLine(PaddedFirstLocation, PaddedSecondLocation, Character->GetActorLocation());
	const FVector CharacterToEdgeDirection = (ClosestToCharacter - Character->GetActorLocation()).GetSafeNormal();
	const FVector InteractFromLocation = Character->GetActorLocation() + CharacterToEdgeDirection*CapsuleRadius*CAPSULE_RADIUS_FORWARD_SCALE;

	// Now project that onto the edge to find our destination
	const FVector ClosestToInteract = FMath::ClosestPointOnLine(PaddedFirstLocation, PaddedSecondLocation, InteractFromLocation);
	const float DistToPointSq = (ClosestToInteract - InteractFromLocation).SizeSquared();
	const FVector EdgeCenter = FMath::ClosestPointOnLine(PaddedFirstLocation, PaddedSecondLocation, ActorLocation);
	const FVector EdgeDirection = InteractableComponent->GetComponentTransform().GetUnitAxis(EAxis::X);
	if (OutClosestDistanceSq && *OutClosestDistanceSq > DistToPointSq)
	{
		*OutClosestDistanceSq = DistToPointSq;
		if (OutClosestEdgePoint)
		{
			*OutClosestEdgePoint = ClosestToInteract;
		}
	}
	
	if (CVarILLDebugInteractables.GetValueOnGameThread())
	{
		UWorld* World = GetWorld();
		DrawDebugLine(World, PaddedFirstLocation, PaddedSecondLocation, FColor::Blue, false, -1.f, SDPG_World, 1.f);
		DrawDebugDirectionalArrow(World, ClosestToInteract, ClosestToInteract-EdgeDirection*100.f, 100.f, FColor::Yellow, false, -1.f, SDPG_World, 1.f);

		if (OutClosestEdgePoint)
		{
			DrawDebugLine(World, *OutClosestEdgePoint - FVector(0,0,50.f), *OutClosestEdgePoint + FVector(0,0,50.f), FColor::Cyan, false, -1.f, SDPG_World, 1.f);
		}
	}
}

void AILLMantleVaultEdge::EvaluateEdges(class AILLCharacter* Character, float* OutClosestDistanceSq/* = nullptr*/, FVector* OutClosestEdgePoint/* = nullptr*/) const
{
	for (int32 Index = 0; Index < EdgePoints.Num()-1; ++Index)
	{
		EdgeEvaluator(Character, Index, Index+1, OutClosestDistanceSq, OutClosestEdgePoint);
	}
}

void AILLMantleVaultEdge::OnBecameBest(AActor* Interactor)
{
	// Update our grounded-ness
	CheckGroundedness();
}

void AILLMantleVaultEdge::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	// Update our grounded-ness
	CheckGroundedness();

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		if (AILLCharacter* Character = Cast<AILLCharacter>(Interactor))
		{
			if (InteractionType == EILLMantleVaultType::Vault)
			{
				Character->StartSpecialMove(UILLSpecialMove_Vault::StaticClass(), this);
			}
			else
			{
				Character->StartSpecialMove(UILLSpecialMove_Mantle::StaticClass(), this);
			}
		}
		break;
	}
}

FVector AILLMantleVaultEdge::OverrideInteractionLocation(AILLCharacter* Character)
{
	if (EdgePoints.Num() > 1)
	{
		FVector ClosestEdgePoint;
		float ClosestDistanceSq = FLT_MAX;
		EvaluateEdges(Character, &ClosestDistanceSq, &ClosestEdgePoint);

		if (ClosestDistanceSq != FLT_MAX)
		{
			return ClosestEdgePoint;
		}
	}

	return InteractableComponent->GetComponentLocation();
}
