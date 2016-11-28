// By Polyakov Pavel

#pragma once

#include "VoronoiGeneratorTask.h"

typedef FAsyncTask<class FVoronoiAdditionalGeneratorTask> FVoronoiAdditionalTask;

/** Determines what FVoronoiAdditionalGeneratorTask is supposed to do */
enum class PRACTICEPROJECT_API EVoronoiTaskType
{
    VTT_Links,
    VTT_Properties
};

/** Class responsible for building Voronoi links or calculating properties of faces */
class PRACTICEPROJECT_API FVoronoiAdditionalGeneratorTask final : public FVoronoiGeneratorTask
{
    AVoronoiNavData *VoronoiNavData;
    EVoronoiTaskType TaskType;

public:
    FVoronoiAdditionalGeneratorTask(AVoronoiNavData* InVoronoiNavData, EVoronoiTaskType InTaskType, bool *InBuildCanceled)
        : FVoronoiGeneratorTask(InBuildCanceled), VoronoiNavData(InVoronoiNavData), TaskType(InTaskType) {}

    void DoWork();
};
