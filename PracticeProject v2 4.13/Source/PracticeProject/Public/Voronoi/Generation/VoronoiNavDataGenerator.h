// By Polyakov Pavel

#pragma once

#include "AI/NavDataGenerator.h"
#include "VoronoiMainGeneratorTask.h"
#include "VoronoiAdditionalGeneratorTask.h"

/** Class responsibe for managing a building process */
class PRACTICEPROJECT_API FVoronoiNavDataGenerator final : public FNavDataGenerator
{
    FVoronoiNavDataGenerator(const FVoronoiNavDataGenerator& NoCopy) { check(0); }
    FVoronoiNavDataGenerator& operator=(const FVoronoiNavDataGenerator& NoCopy) { check(0); return *this; }

    TWeakObjectPtr<AVoronoiNavData> VoronoiNavData;

    TArray<TUniquePtr<FVoronoiMainTask>> VoronoiMainTasks;
    TArray<TUniquePtr<FVoronoiAdditionalTask>> VoronoiAdditionalTasks;

    FQueuedThreadPool* VoronoiThreadPool;

    double GenerationStartTime;
    bool bBuildCanceled;

    void SyncGeneration();
    void FinishGeneration();

public:
    FVoronoiNavDataGenerator(AVoronoiNavData* InVoronoiNavData);
    ~FVoronoiNavDataGenerator();

    virtual bool RebuildAll() override;
    virtual void EnsureBuildCompletion() override;
    virtual void CancelBuild() override;
    virtual void TickAsyncBuild(float DeltaSeconds) override;

    virtual bool IsBuildInProgress(bool bCheckDirtyToo = false) const override { return VoronoiMainTasks.Num() + VoronoiAdditionalTasks.Num() > 0; }
    virtual int32 GetNumRemaningBuildTasks() const override { return VoronoiMainTasks.Num() > 0 ? VoronoiMainTasks.Num() + 2 : VoronoiAdditionalTasks.Num(); };
    virtual int32 GetNumRunningBuildTasks() const override { return VoronoiMainTasks.Num() + VoronoiAdditionalTasks.Num(); };

    virtual void RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas);

    FORCEINLINE AVoronoiNavData* GetVoronoiData() const { return VoronoiNavData.Get(); }
};
