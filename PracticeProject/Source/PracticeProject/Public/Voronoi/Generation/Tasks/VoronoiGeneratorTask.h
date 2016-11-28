// By Polyakov Pavel

#pragma once

#include "VoronoiNavData.h"

/** Base class for generator's tasks */
class PRACTICEPROJECT_API FVoronoiGeneratorTask : public FNonAbandonableTask
{
    bool *bBuildCanceled;

protected:
    FORCEINLINE bool ShouldCancelBuild() const { return *bBuildCanceled; }

public:
    FVoronoiGeneratorTask(bool *InBuildCanceled) : bBuildCanceled(InBuildCanceled) {}

    /** Profiling */
    FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FVoronoiNavDataGenerator, STATGROUP_ThreadPoolAsyncTasks); }
};
