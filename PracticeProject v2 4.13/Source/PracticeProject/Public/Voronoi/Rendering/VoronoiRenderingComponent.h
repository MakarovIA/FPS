// By Polyakov Pavel

#pragma once

#include "VoronoiNavData.h"
#include "VoronoiRenderingComponent.generated.h"

/** Component for Voronoi navigation mesh rendering */
UCLASS()
class PRACTICEPROJECT_API UVoronoiRenderingComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

    FColor SelectFaceColor(FVoronoiFace *Face, const FVoronoiDrawingOptions &Options) const;

public:
    UVoronoiRenderingComponent();

    virtual bool NeedsLoadForServer() const override;
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;

    static bool IsNavigationShowFlagSet(const UWorld* World);
};
