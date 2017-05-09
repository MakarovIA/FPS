// By Polyakov Pavel

#pragma once

#include "VoronoiGraph.h"

typedef TSharedPtr<struct FVoronoiNavigationPath, ESPMode::ThreadSafe> FVoronoiNavPathPtr;

/** Path produced by AVoronoiNavData */
struct PRACTICEPROJECT_API FVoronoiNavigationPath : public FNavigationPath
{
    static const FNavPathType Type;

    TSet<const FVoronoiFace*> JumpPositions;
    TArray<const FVoronoiFace*> PathFaces;

    FORCEINLINE FVoronoiNavigationPath()
    {
        PathType = FVoronoiNavigationPath::Type;
    }

    void AddPathPoint(const FVoronoiFace *InFace, const FVector& InLocation, bool bIsJumpRequired);
    virtual float GetCostFromIndex(int32 PathPointIndex) const override;
};
