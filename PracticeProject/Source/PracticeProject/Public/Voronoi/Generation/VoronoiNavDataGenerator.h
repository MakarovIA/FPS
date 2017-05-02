// By Polyakov Pavel

#pragma once

#include "AI/NavDataGenerator.h"
#include "VoronoiNavData.h"

class FVoronoiGeometryTask;
class FVoronoiSurfaceTask;
class FVoronoiDiagramTask;
class FVoronoiPropertiesTask;

const float TileSize = 1000;
const float CellSize = 20;

/** Class responsibe for managing a building process */
class PRACTICEPROJECT_API FVoronoiNavDataGenerator final : public FNavDataGenerator
{
    FVoronoiNavDataGenerator(const FVoronoiNavDataGenerator& NoCopy) { check(0); }
    FVoronoiNavDataGenerator& operator=(const FVoronoiNavDataGenerator& NoCopy) { check(0); return *this; }

    TWeakObjectPtr<AVoronoiNavData> VoronoiNavData;
    FBox TotalBounds;

    TArray<TUniquePtr<FAsyncTask<FVoronoiGeometryTask>>> VoronoiGeometryTasks;
    TUniquePtr<FAsyncTask<FVoronoiSurfaceTask>> VoronoiSurfaceTask;

    TArray<TUniquePtr<FAsyncTask<FVoronoiDiagramTask>>> VoronoiDiagramTasks;
    TUniquePtr<FAsyncTask<FVoronoiPropertiesTask>> VoronoiPropertiesTask;

    double GenerationStartTime;
    bool bBuildCanceled;

    void GeometryToSurface();
    void SurfaceToDiagram();
    void DiagramToProperties();

    void FinishGeneration();

public:
    FORCEINLINE FVoronoiNavDataGenerator(AVoronoiNavData* InVoronoiNavData)
        : VoronoiNavData(InVoronoiNavData), bBuildCanceled(false), GenerationStartTime(0.f) {}
    virtual ~FVoronoiNavDataGenerator() override { CancelBuild(); }

    virtual bool RebuildAll() override;
    virtual void EnsureBuildCompletion() override;
    virtual void CancelBuild() override;
    virtual void TickAsyncBuild(float DeltaSeconds) override;
    virtual void RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas) override;

    virtual bool IsBuildInProgress(bool bCheckDirtyToo = false) const override { return GetNumRemaningBuildTasks() > 0; }
    virtual int32 GetNumRemaningBuildTasks() const override { return GetNumRunningBuildTasks(); };
    virtual int32 GetNumRunningBuildTasks() const override;

    FORCEINLINE bool IsBuiltCanceled() const { return bBuildCanceled; }
    FORCEINLINE const AVoronoiNavData* GetVoronoiNavData() const { return VoronoiNavData.Get(); }
};

/** Base class for generator's tasks */
class PRACTICEPROJECT_API FVoronoiTask : public FNonAbandonableTask
{
    const FVoronoiNavDataGenerator& ParentGenerator;

protected:
    const FVoronoiGenerationOptions GenerationOptions;
    const FNavDataConfig AgentProperties;
    const UWorld *World;

public:
    FORCEINLINE FVoronoiTask(const FVoronoiNavDataGenerator& InParentGenerator)
        : ParentGenerator(InParentGenerator), GenerationOptions(InParentGenerator.GetVoronoiNavData()->GetGenerationOptions())
        , AgentProperties(InParentGenerator.GetVoronoiNavData()->GetConfig()), World(InParentGenerator.GetVoronoiNavData()->GetWorld()) {}

    FORCEINLINE bool ShouldCancelBuild() const { return ParentGenerator.IsBuiltCanceled(); }
    FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FVoronoiNavDataGenerator, STATGROUP_ThreadPoolAsyncTasks); }
};
