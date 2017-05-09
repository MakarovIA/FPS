// By Polyakov Pavel

#pragma once

#include "VoronoiNavDataGenerator.h"

/** Collects geometry and constructs compressed height field */
class PRACTICEPROJECT_API FVoronoiGeometryTask final : public FVoronoiTask
{
    TArray<TSharedRef<FNavigationRelevantData, ESPMode::ThreadSafe>> NavigationRelevantData;

public:
    TArray<TArray<TArray<float>>> CompressedHeightField;
    FBox TileBB;

    FVoronoiGeometryTask(const FVoronoiNavDataGenerator& InParentGenerator, const FBox& InTileBB);
    void DoWork();
};
