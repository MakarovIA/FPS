// By Polyakov Pavel

#pragma once

#include "VoronoiNavDataGenerator.h"

/** Collects geometry and constructs compressed height field */
class PRACTICEPROJECT_API FVoronoiSurfaceTask final : public FVoronoiTask
{
    TArray<TArray<TArray<float>>> GlobalHeightField;
    FVector GlobalHeightFieldMin;

public:
    TArray<TArray<TArray<FVector>>> SurfaceBorders;
    TArray<TArray<FVector>> Surfaces;

    FORCEINLINE FVoronoiSurfaceTask(const FVoronoiNavDataGenerator& InParentGenerator, TArray<TArray<TArray<float>>> InGlobalHeightField, FVector InGlobalHeightFieldMin)
        : FVoronoiTask(InParentGenerator), GlobalHeightField(MoveTemp(InGlobalHeightField)), GlobalHeightFieldMin(InGlobalHeightFieldMin) {}

    void DoWork();
};
