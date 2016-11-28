// By Polyakov Pavel

#pragma once

#include "VoronoiGraph.h"

typedef TSharedPtr<struct FVoronoiNavigationPath, ESPMode::ThreadSafe> FVoronoiNavPathPtr;

/** Path produced by AVoronoiNavData */
struct PRACTICEPROJECT_API FVoronoiNavigationPath : public FNavigationPath
{
    static const FNavPathType Type;

    TSet<FVoronoiFace*> JumpPositions;
    TArray<FVoronoiFace*> PathFaces;

    FVoronoiNavigationPath();
    void AddPathPoint(FVoronoiFace *InFace, const FVector& InLocation);
    virtual float GetCostFromIndex(int32 PathPointIndex) const override;
};
