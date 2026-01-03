// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLUserWidget.h"
#include "ILLMinimap.generated.h"

class AILLMiniMapVolume;
class UILLMinimapWidget;

/**
 * @struct FMinimapIconData
 */
USTRUCT()
struct FMinimapIconData
{
	GENERATED_BODY()

public:
	/** Widget gets created/destroyed by ILLMinimap when the object is no longer relevant */
	UPROPERTY()
	UILLMinimapWidget* IconWidget;

	/** Internal reference to if the widget is visible or not */
	bool bIsShown;

	/** Timestamp to track how long a widget has been hidden before we destroy it */
	float LastRendered_Timestamp;
};

/**
 * @class UILLMinimap
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLMinimap
: public UILLUserWidget
{
	GENERATED_UCLASS_BODY()

public:

	// Update positions and rotation of all actor widgets represented on the minimap.
	UFUNCTION(BlueprintCallable, Category = "Minimap|Transform")
	virtual void UpdateObjectLocations();

	/**
	 * Returns the world location for a click on the minimap
	 * @param geo - UMG widget's geometry, obtained from click events in BP.
	 * @param mouseEvt - the mouse event (click)
	 * @return - FVector world position of minimap click with Z = 0
	 */
	UFUNCTION(BlueprintPure, Category = "Minimap|Transform")
	virtual FVector GetClickWorldLocation(FGeometry Geo, const FPointerEvent& MouseEvt);

	/**
	 * Returns the world location for a click on the minimap
	 * @param InSlot - The widget slot to test the click on.
	 * @param ScreenPosition - The position o the mouse curson in screen space.
	 * @return - FVector world position of minimap click with Z = 0
	 */
	UFUNCTION(BlueprintPure, Category = "Minimap|Transform")
	virtual FVector GetClickWorldLocationForSlot(UILLUserWidget* UserWidget, FVector2D ScreenPosition);

	/**
	 * Returns the UV location of the minimap click in local (0-1) coordinates.
	 * @param geo - UMG widget's geometry, obtained from click events in BP.
	 * @param mouseEvt - the mouse event (click)
	 * @return - UV coordinate of the click.
	 */
	UFUNCTION(BlueprintPure, Category = "Minimap|Transform")
	virtual FVector2D GetClickLocalLocation(FGeometry Geo, const FPointerEvent& MouseEvt);

	/**
	 * Returns the UV location of the minimap click in local (0-1) coordinates.
	 * @param InSlot - The widget slot to test the click on.
	 * @param ScreenPosition - The position o the mouse curson in screen space.
	 * @return - UV coordinate of the click.
	 */
	UFUNCTION(BlueprintPure, Category = "Minimap|Transform")
	virtual FVector2D GetClickLocalLocationForSlot(UCanvasPanelSlot* InSlot, FVector2D ScreenPosition);

	UFUNCTION()
	virtual bool InitializeMinimap();

	// Center minimap on owning character.
	UFUNCTION(BlueprintCallable, Category = "Minimap|Transform")
	virtual void PositionMinimap();

	// Rotates the minimap material.
	UFUNCTION(BlueprintCallable, Category = "Minimap|Transform")
	virtual void RotateMinimap();

	/**
	 * Returns the relative size of the object in comparison to the minimap volume in UI space.
	 * @param objectExtents - The extents of the object bounding area.
	 * return - relative size of the given bounds in pixel size.
	 */
	UFUNCTION(BlueprintPure, Category = "Minimap|Transform")
	FVector2D GetRelativeSize(FVector ObjectExtents);

	UPROPERTY(BlueprintReadOnly, Category = "Minimap|Data")
	FVector2D TopLeft;

	UPROPERTY(BlueprintReadOnly, Category = "Minimap|Data")
	FVector2D BottomRight;

	UPROPERTY(BlueprintReadOnly, Category = "Minimap|Data")
	FVector2D MapSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Data")
	float MinimapZoom;
	float PreviousZoom;

	// The Image component to use for the map background.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Data")
	class UImage* MapImage;

	// The panel to draw our icons to, should be laid over the MapImage.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Data")
	class UCanvasPanel* IconParent;

	// The overlay is the root canvas the image and icon parent scale to. All scale manipulation should be done on this object.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Data")
	class UCanvasPanel* Overlay;

	// The image component for the compas overlay on the minimap.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Data")
	class UCanvasPanel* Compass;

	// The image component for the player view frustum on the minimap.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Data")
	class UImage* PlayerViewIcon;

	// Rotate the minimap with the player camera?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Data")
	bool bRotateMap;

	// Draw an Icon for the owning actor if it is in the IconArray.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Data")
	bool bShowSelf;

	// Center the map on the owning character?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Data")
	bool bCenterMap;

	// Automatically adjust the scale of the Minimap based on zoom and ILLMinimapVolume size?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Data")
	bool bAutoAdjustSize;

	/** Time in seconds for a widget to remain while hidden before being destroyed entirely */
	UPROPERTY(EditAnywhere, Category = "Minimap|Data")
	float HiddenIconLifespan;

	// The distance from every side of the minimap that is designated "Safe Zone" to handle clipping issues
	UPROPERTY(EditAnywhere, Category = "Minimap|Data")
	float SafeZoneExtent;

	UFUNCTION()
	void OnIconComponentRegistered(UILLMinimapIconComponent* IconComponent);

	UFUNCTION()
	void OnIconComponentUnregistered(UILLMinimapIconComponent* IconComponent);

protected:
	virtual void InitializeMapMaterial(UMaterialInstanceDynamic* MID, AILLMiniMapVolume* Volume);

	bool bInitialized;

	// used to set the rotation point for the map material.
	FVector2D CenterPoint;
	float Radius;

	// Used to rotate icons around the center of the minimap when +X is not north.
	FRotator MinimapVolumeRotation;
	FVector MinimapVolumeCenter;

	/** List of icons we're tracking for the minimap widget*/
	UPROPERTY()
	TMap<UILLMinimapIconComponent*, FMinimapIconData> MinimapIcons;
};
