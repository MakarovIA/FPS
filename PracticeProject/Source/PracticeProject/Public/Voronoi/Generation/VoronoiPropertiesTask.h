// By Polyakov Pavel

#pragma once

#include "VoronoiNavDataGenerator.h"

/** Builds Voronoi links and calculates properties of faces */
class PRACTICEPROJECT_API FVoronoiPropertiesTask final : public FVoronoiTask
{
    TArray<TPreserveConstUniquePtr<FVoronoiSurface>> *GeneratedSurfaces;

public:
    FORCEINLINE FVoronoiPropertiesTask(const FVoronoiNavDataGenerator& InParentGenerator, TArray<TPreserveConstUniquePtr<FVoronoiSurface>> *InGeneratedSurfaces)
        : FVoronoiTask(InParentGenerator), GeneratedSurfaces(InGeneratedSurfaces) {}

    void DoWork();
};
