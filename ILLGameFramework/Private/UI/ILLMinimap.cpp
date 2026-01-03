// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLMinimap.h"

#include "ILLCharacter.h"
#include "ILLHUD.h"
#include "ILLMinimapIconComponent.h"
#include "ILLMiniMapVolume.h"
#include "IllMinimapWidget.h"

#include "CanvasPanel.h"
#include "CanvasPanelSlot.h"
#include "Image.h"
#include "Overlay.h"
#include "SceneViewport.h"
#include "HittestGrid.h"

/** @return The FSlateRect location without window border and title bar. */
FORCEINLINE FSlateRect RemoveBorders(FSlateRect InGeoRect)
{
	if (GEngine)
	{
		TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
		if (Window.IsValid() && !GEngine->GameViewport->IsFullScreenViewport())
		{
			FMargin WindowBorders = Window->GetWindowBorderSize(true);
			if (Window->GetTitleBarSize().IsSet())
			{
				WindowBorders.Top += Window->GetTitleBarSize().Get();
			}
			// subtract the window border from the geometry rect
			// also subtract the window location as the cached geometry
			// is returned in desktop space
			InGeoRect.Top -= WindowBorders.Top;
			InGeoRect.Top -= Window->GetPositionInScreen().Y;
			InGeoRect.Bottom -= WindowBorders.Top;
			InGeoRect.Bottom -= Window->GetPositionInScreen().Y;
			InGeoRect.Left -= WindowBorders.Left;
			InGeoRect.Left -= Window->GetPositionInScreen().X;
			InGeoRect.Right -= WindowBorders.Left;
			InGeoRect.Right -= Window->GetPositionInScreen().X;
		}
	}

	return InGeoRect;
}

UILLMinimap::UILLMinimap(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, MinimapZoom(1.f)
, PreviousZoom(1.f)
, bRotateMap(true)
, bShowSelf(false)
, bCenterMap(true)
, bAutoAdjustSize(true)
, HiddenIconLifespan(2.f)
, SafeZoneExtent(1.f)
, bInitialized(false)
, Radius(10000.f)
{
}

void UILLMinimap::UpdateObjectLocations()
{
	if (InitializeMinimap() == false)
		return;

	AILLHUD* OwningHUD = GetOwningPlayerILLHUD();
	if (OwningHUD == nullptr)
		return;

	UCanvasPanelSlot* OverlaySlot = Cast<UCanvasPanelSlot>(Overlay->Slot);
	if (OverlaySlot == nullptr)
		return;

	UWorld* World = GetWorld();
	APawn* Player = GetOwningPlayerPawn();
	FVector PlayerLocation(FVector::ZeroVector);
	FRotator PlayerRotation(FRotator::ZeroRotator);
	if (Player)
	{
		PlayerLocation = Player->GetActorLocation();

		if (APlayerController* PlayerController = Cast<APlayerController>(Player->Controller))
		{
			if (PlayerController->PlayerCameraManager)
			{
				PlayerRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			}
		}
	}

	if (bAutoAdjustSize && PreviousZoom != MinimapZoom)
	{
		OverlaySlot->SetSize(MapSize * (MinimapZoom / 10.f));
		PreviousZoom = MinimapZoom;
	}

	const float CurrentTime = World->GetTimeSeconds();

	for (auto& MinimapIconDataPair : MinimapIcons)
	{
		UILLMinimapIconComponent* IconComponent = MinimapIconDataPair.Key;

		// If for some reason the component isn't valid anymore then skip it.
		if (!IsValid(IconComponent))
			continue;

		const bool bIsIconVisible = IconComponent->IsVisible();
		if (bIsIconVisible == false && MinimapIconDataPair.Value.IconWidget == nullptr)
			continue;

		if (bIsIconVisible)
		{
			// Make sure the widget is around (create if needed)
			if (MinimapIconDataPair.Value.IconWidget == nullptr)
			{
				// Create
				TSubclassOf<UILLMinimapWidget> IconClass = OwningHUD->GetMinimapIcon(IconComponent);

				// TODO: Find a better way of handling a widget that doesn't want to be shown on our HUD
				if (IconClass == nullptr)
					continue;

				MinimapIconDataPair.Value.IconWidget = CreateWidget<UILLMinimapWidget>(GetWorld(), IconClass);

				// Attach to minimap widget
				IconParent->AddChildToCanvas(MinimapIconDataPair.Value.IconWidget);
				MinimapIconDataPair.Value.IconWidget->InitializeWidget(IconComponent->GetOwner(), Player, this);
			}

			// Make sure the widget is visible
			if (MinimapIconDataPair.Value.bIsShown == false)
			{
				MinimapIconDataPair.Value.bIsShown = true;
				MinimapIconDataPair.Value.IconWidget->SetVisibility(ESlateVisibility::Visible);
			}

			// Orient/position the icon
			FVector ComponentLocation = IconComponent->GetComponentLocation();
			FRotator ComponentRotation = IconComponent->GetComponentRotation();

			// Rotate with the minimap volume effectively changing North axis.
			FVector CenterRot = ComponentLocation - MinimapVolumeCenter;
			CenterRot = CenterRot.RotateAngleAxis(-MinimapVolumeRotation.Yaw, FVector::UpVector);
			ComponentLocation = MinimapVolumeCenter + CenterRot;
			ComponentRotation -= MinimapVolumeRotation;

			// Transform the found player around the minimap center point to maintain their rotated position.
			if (bRotateMap)
			{
				const FVector Dir = (ComponentLocation - PlayerLocation).RotateAngleAxis(-PlayerRotation.Yaw, FVector::UpVector);
				ComponentLocation = PlayerLocation + Dir;
				ComponentRotation -= PlayerRotation;
			}

			// Perform widget tick.
			MinimapIconDataPair.Value.IconWidget->UpdateMapWidget(ComponentLocation, ComponentRotation);

			// Convert the location into the 0-1 space of the minimap.
			ComponentLocation.X = (ComponentLocation.X - TopLeft.X) / (BottomRight.X - TopLeft.X);
			ComponentLocation.Y = (ComponentLocation.Y - TopLeft.Y) / (BottomRight.Y - TopLeft.Y);

			UCanvasPanelSlot* WidgetSlot = Cast<UCanvasPanelSlot>(MinimapIconDataPair.Value.IconWidget->Slot);
			if (!WidgetSlot)
				continue;

			const FVector2D Size = OverlaySlot->GetSize();
			FVector2D FinalPos(ComponentLocation.Y * Size.X, ComponentLocation.X * Size.Y);
			const FVector2D Center(CenterPoint.Y * Size.X, CenterPoint.X * Size.Y);
			const FVector2D Distance = FinalPos - Center;

			if (Distance.Size() > Radius - SafeZoneExtent && MinimapIconDataPair.Value.IconWidget->bShowOnMinimapEdge)
			{
				MinimapIconDataPair.Value.IconWidget->SetVisibility(ESlateVisibility::Visible);
				FinalPos = Center + (Distance.GetSafeNormal() * (Radius - SafeZoneExtent));

				const FVector RotationVector(-Distance.Y, Distance.X, 0.f);
				ComponentRotation = RotationVector.Rotation();
				MinimapIconDataPair.Value.IconWidget->bIsOnMinimap = false;
			}
			else if (Distance.Size() > Radius - SafeZoneExtent)
			{
				MinimapIconDataPair.Value.IconWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				MinimapIconDataPair.Value.IconWidget->SetVisibility(ESlateVisibility::Visible);
				MinimapIconDataPair.Value.IconWidget->bIsOnMinimap = true;
			}

			WidgetSlot->SetPosition(FinalPos);
			MinimapIconDataPair.Value.IconWidget->SetRenderAngle(ComponentRotation.Yaw);

			// Update our render time
			MinimapIconDataPair.Value.LastRendered_Timestamp = CurrentTime;
		}
		else
		{
			// Make sure the widget is hidden
			if (MinimapIconDataPair.Value.bIsShown == true)
			{
				MinimapIconDataPair.Value.bIsShown = false;
				MinimapIconDataPair.Value.IconWidget->SetVisibility(ESlateVisibility::Collapsed);

				// Update our render time one last time
				MinimapIconDataPair.Value.LastRendered_Timestamp = CurrentTime;
			}

			// See if we should destory the widget (timestamp check)
			if (MinimapIconDataPair.Value.LastRendered_Timestamp - CurrentTime >= HiddenIconLifespan)
			{
				MinimapIconDataPair.Value.IconWidget->RemoveFromParent();
				MinimapIconDataPair.Value.IconWidget = nullptr;
			}
		}
	}
}

void UILLMinimap::PositionMinimap()
{
	if (bCenterMap == false || InitializeMinimap() == false)
		return;

	APawn* Player = GetOwningPlayerPawn();
	if (Player == nullptr)
		return;

	UCanvasPanelSlot* OverlaySlot = Cast<UCanvasPanelSlot>(Overlay->Slot);
	if (!Slot)
		return;

	const FVector PlayerPosition = Player->GetActorLocation();
	CenterPoint.X = (PlayerPosition.X - TopLeft.X) / (BottomRight.X - TopLeft.X);
	CenterPoint.Y = (PlayerPosition.Y - TopLeft.Y) / (BottomRight.Y - TopLeft.Y);

	const FVector2D Size = OverlaySlot->GetSize();
	const FVector2D FinalPos = FVector2D(-(CenterPoint.Y * Size.X), -(CenterPoint.X * Size.Y));

	OverlaySlot->SetPosition(FinalPos);
}

bool UILLMinimap::InitializeMinimap()
{
	if (bInitialized)
		return true;

	UWorld* World = GetWorld();
	if (World == nullptr)
		return false;

	AILLHUD* OwningHUD = GetOwningPlayerILLHUD();
	if (OwningHUD == nullptr)
		return false;

	AILLCharacter* Character = Cast<AILLCharacter>(GetOwningPlayerPawn());
	if (!IsValid(Character) || !Character->HasActorBegunPlay())
		return false;

	// Grab the first (and hopefully only) instance of AILLMinimapVolume in the world.
	TActorIterator<AILLMiniMapVolume> LandIt(World); // OPTIMIZE: pjackson: Fucking lazy ass actor iterator
	if (LandIt)
	{
		// Fill our map with the components already registered
		for (UILLMinimapIconComponent* MinimapIconComponent : OwningHUD->GetMinimapIconComponents())
		{
			MinimapIcons.Add(MinimapIconComponent);
		}

		// Bind registeration for new components
		OwningHUD->OnMinimapIconRegistered.AddDynamic(this, &UILLMinimap::OnIconComponentRegistered);
		OwningHUD->OnMinimapIconUnregistered.AddDynamic(this, &UILLMinimap::OnIconComponentUnregistered);

		MinimapZoom = LandIt->DefaultMinimapZoom;
		FVector Center = FVector::ZeroVector;
		FVector Extents(1.f, 1.f, 1.f);

		LandIt->GetActorBounds(false, Center, Extents);
		MinimapVolumeRotation = LandIt->GetActorRotation();
		MinimapVolumeCenter = Center;

		TopLeft.X = Center.X + Extents.X;
		TopLeft.Y = Center.Y - Extents.Y;
		BottomRight.X = Center.X - Extents.X;
		BottomRight.Y = Center.Y + Extents.Y;

		MapSize.X = Extents.Y * 2.f;
		MapSize.Y = Extents.X * 2.f;

		// Set The texture for the minimap
		UMaterialInstanceDynamic* DynamicMat = MapImage->GetDynamicMaterial();
		InitializeMapMaterial(DynamicMat, *LandIt);

		if (bAutoAdjustSize)
		{
			// the minimap will default to 1/10th the size of the minimap volume.
			FVector2D TextureSize = MapSize * (MinimapZoom * 0.1f);
			if (Slot)
			{
				// Get the radius of the minimap with an 1px buffer to hide widget popping under the compass.
				const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
				Radius = (CanvasSlot->GetSize().X * 0.5f) - SafeZoneExtent;
			}

			if (DynamicMat)
			{
				DynamicMat->SetScalarParameterValue(TEXT("MinimapRadius"), Radius / TextureSize.X);
			}

			UCanvasPanelSlot* OverlaySlot = Cast<UCanvasPanelSlot>(Overlay->Slot);
			if (!OverlaySlot)
			{
				OverlaySlot->SetSize(TextureSize);
				PreviousZoom = MinimapZoom;
			}

			OverlaySlot->SetSize(TextureSize);
			PreviousZoom = MinimapZoom;
		}

		bInitialized = true;
	}

	return bInitialized;
}

void UILLMinimap::RotateMinimap()
{
	if (InitializeMinimap() == false)
		return;

	APlayerController* PlayerController = GetOwningPlayer();

	FRotator Rot = FRotator::ZeroRotator;
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		Rot = PlayerController->PlayerCameraManager->GetCameraRotation();
	}

	if (bRotateMap == false)
	{
		UMaterialInstanceDynamic* DynamicMat = MapImage->GetDynamicMaterial();
		if (DynamicMat)
		{
			DynamicMat->SetScalarParameterValue(TEXT("Rotation"), 0);

			FLinearColor Center = FLinearColor(CenterPoint.Y, CenterPoint.X, 0, 0);
			DynamicMat->SetVectorParameterValue(TEXT("Center"), Center);
		}

		if (Compass)
		{
			Compass->SetRenderAngle(0);
		}

		if (PlayerViewIcon)
		{
			PlayerViewIcon->SetRenderAngle(Rot.Yaw);
		}
		return;
	}

	if (PlayerController == nullptr)
		return;

	UMaterialInstanceDynamic* DynamicMat = MapImage->GetDynamicMaterial();
	if (DynamicMat)
	{
		DynamicMat->SetScalarParameterValue(TEXT("Rotation"), Rot.Yaw / 360.f);

		FLinearColor Center = FLinearColor(CenterPoint.Y, CenterPoint.X, 0, 0);
		DynamicMat->SetVectorParameterValue(TEXT("Center"), Center);
	}

	if (Compass)
	{
		Compass->SetRenderAngle(-Rot.Yaw);
	}

	if (PlayerViewIcon)
	{
		PlayerViewIcon->SetRenderAngle(0);
	}
}

FVector UILLMinimap::GetClickWorldLocationForSlot(UILLUserWidget* UserWidget, FVector2D ScreenLocation)
{
	FGeometry Geo;
	if (UserWidget)
	{
		Geo = UserWidget->GetCachedGeometry();
	}

	FSlateRect GeoRect = RemoveBorders(BoundingRectForGeometry(Geo));

	if (GeoRect.ContainsPoint(ScreenLocation))
	{
		FVector2D AbsoluteLocation = (ScreenLocation - GeoRect.GetTopLeft()) / GeoRect.GetSize();
		AbsoluteLocation = FMath::Clamp(AbsoluteLocation, FVector2D::ZeroVector, FVector2D::UnitVector);

		// In world space X is vertical and Y is horizontal.
		const FVector2D WorldXY(TopLeft.X - (AbsoluteLocation.Y * MapSize.X), TopLeft.Y + (AbsoluteLocation.X * MapSize.Y));
		return FVector(WorldXY, 0);
	}

	return FVector::ZeroVector;
}

FVector UILLMinimap::GetClickWorldLocation(FGeometry Geo, const FPointerEvent& MouseEvt)
{
	FSlateRect GeoRect = RemoveBorders(BoundingRectForGeometry(Geo));
	FVector2D ScreenLocation = MouseEvt.GetScreenSpacePosition();

	if (GeoRect.ContainsPoint(ScreenLocation))
	{
		FVector2D AbsoluteLocation = (ScreenLocation - GeoRect.GetTopLeft()) / GeoRect.GetSize();
		AbsoluteLocation = FMath::Clamp(AbsoluteLocation, FVector2D::ZeroVector, FVector2D::UnitVector);

		// In world space X is vertical and Y is horizontal.
		const FVector2D WorldXY(TopLeft.X - (AbsoluteLocation.Y * MapSize.X), TopLeft.Y + (AbsoluteLocation.X * MapSize.Y));
		return FVector(WorldXY, 0);
	}

	return FVector::ZeroVector;
}

FVector2D UILLMinimap::GetClickLocalLocationForSlot(UCanvasPanelSlot* InSlot, FVector2D ScreenLocation)
{
	FGeometry Geo;
	if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(InSlot->Parent))
	{
		CanvasPanel->GetGeometryForSlot(InSlot, Geo);
	}

	FSlateRect GeoRect = RemoveBorders(BoundingRectForGeometry(Geo));

	if (GeoRect.ContainsPoint(ScreenLocation))
	{
		return ScreenLocation - GeoRect.GetTopLeft();
	}

	return -FVector2D::UnitVector;
}

FVector2D UILLMinimap::GetClickLocalLocation(FGeometry Geo, const FPointerEvent& MouseEvt)
{
	FSlateRect GeoRect = RemoveBorders(BoundingRectForGeometry(Geo));
	FVector2D ScreenLocation = MouseEvt.GetScreenSpacePosition();

	if (GeoRect.ContainsPoint(ScreenLocation))
	{
		return ScreenLocation - GeoRect.GetTopLeft();
	}

	return -FVector2D::UnitVector;
}

FVector2D UILLMinimap::GetRelativeSize(FVector ObjectExtents)
{
	UCanvasPanelSlot* OverlaySlot = Cast<UCanvasPanelSlot>(Overlay->Slot);
	if (!OverlaySlot)
	{
		return FVector2D::ZeroVector;
	}

	ObjectExtents *= 2.f;
	FVector2D NormalizedV = FVector2D(ObjectExtents.X / MapSize.X, ObjectExtents.Y / MapSize.Y);

	return FVector2D(NormalizedV.Y * OverlaySlot->GetSize().X, NormalizedV.X * OverlaySlot->GetSize().Y);
}

void UILLMinimap::OnIconComponentRegistered(UILLMinimapIconComponent* IconComponent)
{
	if (MinimapIcons.Find(IconComponent))
		return;

	MinimapIcons.Add(IconComponent);
}

void UILLMinimap::OnIconComponentUnregistered(UILLMinimapIconComponent* IconComponent)
{
	FMinimapIconData* Data = MinimapIcons.Find(IconComponent);
	if (Data == nullptr)
		return;

	// Make sure we clean up our widget before we remove this component from our list
	if (Data->IconWidget)
	{
		Data->IconWidget->RemoveFromParent();
		Data->IconWidget = nullptr;
	}

	MinimapIcons.Remove(IconComponent);
}

void UILLMinimap::InitializeMapMaterial(UMaterialInstanceDynamic* MID, AILLMiniMapVolume* Volume)
{
	if (MID)
	{
		Volume->MapTexture.LoadSynchronous();
		MID->SetTextureParameterValue(TEXT("Texture"), Volume->MapTexture.Get());
	}
}
