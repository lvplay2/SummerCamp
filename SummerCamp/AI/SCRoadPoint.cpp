// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCRoadPoint.h"

#include "MapErrors.h"
#include "MessageLog.h"
#include "UObjectToken.h"

USCRoadPointComponent::USCRoadPointComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

#if !UE_BUILD_SHIPPING
FPrimitiveSceneProxy* USCRoadPointComponent::CreateSceneProxy()
{
	class FRoadPointSceneProxy : public FPrimitiveSceneProxy
	{
	public:
		
		FRoadPointSceneProxy(const USCRoadPointComponent* InComponent)
			: FPrimitiveSceneProxy(InComponent)
			, ParentRoad(Cast<ASCRoadPoint>(InComponent->GetOwner()))
		{
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_RoadPointSceneProxy_GetDynamicMeshElements);

			if (IsSelected())
			{
				return;
			}

			for (int32 ViewIndex(0); ViewIndex < Views.Num(); ++ViewIndex)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];
					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

					const FMatrix& LocalToWorld = GetLocalToWorld();

					// Taking into account the min and maximum drawing distance
					const float DistanceSqr = (View->ViewMatrices.GetViewOrigin() - LocalToWorld.GetOrigin()).SizeSquared();
					if (DistanceSqr < FMath::Square(GetMinDrawDistance()) || DistanceSqr > FMath::Square(GetMaxDrawDistance()))
					{
						continue;
					}

					if (ParentRoad)
					{
						PDI->DrawPoint(ParentRoad->GetActorLocation(), FLinearColor::Blue, 15, SDPG_World);

						static const FLinearColor Purple(FColor::Purple);
						for (int32 NeighborIndex(0); NeighborIndex < ParentRoad->Neighbors.Num(); ++NeighborIndex)
						{
							if (ParentRoad->Neighbors[NeighborIndex])
								PDI->DrawLine(ParentRoad->GetActorLocation(), ParentRoad->Neighbors[NeighborIndex]->GetActorLocation(), Purple, SDPG_World, 3.f);
						}
					}
				}
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = !IsSelected() && IsShown(View) && View->Family->EngineShowFlags.Splines;
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = IsShadowCast(View);
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
			return Result;
		}

		virtual uint32 GetMemoryFootprint(void) const override { return sizeof *this + GetAllocatedSize(); }
		uint32 GetAllocatedSize(void) const { return FPrimitiveSceneProxy::GetAllocatedSize(); }

	private:
		ASCRoadPoint* ParentRoad;
	};

	return new FRoadPointSceneProxy(this);
}
#endif

ASCRoadPoint::ASCRoadPoint(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	Root = CreateDefaultSubobject<USCRoadPointComponent>(TEXT("Root"));
	RootComponent = Root;
}

#if WITH_EDITOR
void ASCRoadPoint::CheckForErrors()
{
	Super::CheckForErrors();

	static const FName NAME_RoadPointError(TEXT("RoadPointError"));
	if (Neighbors.Num() == 0)
	{
		FMessageLog MapCheck("MapCheck");
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		MapCheck.Error()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} has no neighbors!")), Arguments)))
			->AddToken(FMapErrorToken::Create(NAME_RoadPointError));
	}

	bool bSelfNeighbor = false;
	for (auto Neighbor : Neighbors)
	{
		if (Neighbor == this)
		{
			bSelfNeighbor = true;
			break;
		}
	}

	if (bSelfNeighbor)
	{
		FMessageLog MapCheck("MapCheck");
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		MapCheck.Error()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} references itself. Not allowed!")), Arguments)))
			->AddToken(FMapErrorToken::Create(NAME_RoadPointError));
	}
}

void ASCRoadPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		static const FName NeighborsName = GET_MEMBER_NAME_CHECKED(ASCRoadPoint, Neighbors);
		
		if (PropertyChangedEvent.Property->GetFName() == NeighborsName)
		{
			Root->MarkRenderStateDirty();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif