// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiNavDataGenerator.h"
#include "VoronoiNavData.h"
#include "VoronoiSurface.h"
#include "VoronoiSite.h"

/** FVoronoiNavDataGenerator */

FVoronoiNavDataGenerator::FVoronoiNavDataGenerator(AVoronoiNavData* InVoronoiNavData)
    : VoronoiNavData(InVoronoiNavData), bBuildCanceled(false), GenerationStartTime(0.f)
{
    VoronoiThreadPool = FQueuedThreadPool::Allocate();
    verify(VoronoiThreadPool->Create(FPlatformMisc::NumberOfWorkerThreadsToSpawn()));
}

FVoronoiNavDataGenerator::~FVoronoiNavDataGenerator()
{
    CancelBuild();
    VoronoiThreadPool->Destroy();
}

bool FVoronoiNavDataGenerator::RebuildAll()
{
    // Cancel previous build
    CancelBuild();

    bool bEnsureBuildCompletion = false;
    if (!VoronoiNavData->VoronoiGraph.IsValid())
    {
        VoronoiNavData->VoronoiGraph = MakeUnique<FVoronoiGraph>();

        // We can't leave graph in this state as VoronoiNavData will consider it to be ready to use
        // Generally speaking, if we enter this branch it means serialization failed

        UE_LOG(LogNavigation, Error, TEXT("Voronoi graph does not exist! Trying to rebuild..."));
        bEnsureBuildCompletion = true;
    }

    // Ensure array is empty
    VoronoiNavData->VoronoiGraph->GeneratedSurfaces.Empty();

    // Get world and save current time for statistics
    UWorld* World = VoronoiNavData->GetWorld();
    GenerationStartTime = FPlatformTime::Seconds();

    for (TActorIterator<AVoronoiSurface> It(World); It; ++It)
    {
        // Create native copies of AVoronoiSurface
        const auto ActorSurface = static_cast<const AVoronoiSurface*>(*It);

        VoronoiNavData->VoronoiGraph->GeneratedSurfaces.Emplace(new FVoronoiSurface(ActorSurface->GetActorLocation(), ActorSurface->GetSize()));
        FVoronoiSurface *Surface = VoronoiNavData->VoronoiGraph->GeneratedSurfaces.Last().Get();
        
        // Get Voronoi sites for this surface
        TArray<AActor*> VoronoiSites;
        ActorSurface->GetAttachedActors(VoronoiSites);

        for (const AActor* Site : VoronoiSites)
        {
            const AVoronoiSite* VoronoiSite = static_cast<const AVoronoiSite*>(Site);
            if (Site->IsPendingKill() || VoronoiSite == nullptr)
                continue;
            
            Surface->Faces.Emplace(new FVoronoiFace(Surface, Site->GetActorLocation()));
            Surface->Faces.Last()->Properties.BaseCost = FMath::Max(1.f, VoronoiSite->GetBaseCost());
            Surface->Faces.Last()->Properties.BaseEnterCost = FMath::Max(0.f, VoronoiSite->GetBaseEnterCost());
            Surface->Faces.Last()->Properties.bNoWay = VoronoiSite->IsNoWay();
        }

        // Launch a task on surface
        VoronoiMainTasks.Emplace(new FVoronoiMainTask(Surface, VoronoiNavData.Get(), &bBuildCanceled));
        VoronoiMainTasks.Last()->StartBackgroundTask(VoronoiThreadPool);
    }

    // No Voronoi surfaces are present
    if (VoronoiMainTasks.Num() == 0)
    {
        VoronoiNavData->VoronoiGraph->Surfaces = MoveTemp(VoronoiNavData->VoronoiGraph->GeneratedSurfaces);
        VoronoiNavData->OnVoronoiNavDataGenerationFinished(FPlatformTime::Seconds() - GenerationStartTime);
    }
    
    else if (bEnsureBuildCompletion)
        EnsureBuildCompletion();

    return true;
}

void FVoronoiNavDataGenerator::EnsureBuildCompletion()
{
    if (VoronoiMainTasks.Num() != 0)
    {
        for (int32 i = 0, sz = VoronoiMainTasks.Num(); i < sz; ++i)
            VoronoiMainTasks[i]->EnsureCompletion();

        VoronoiMainTasks.Empty();
        SyncGeneration();
    }

    if (VoronoiAdditionalTasks.Num() != 0)
    {
        for (int32 i = 0, sz = VoronoiAdditionalTasks.Num(); i < sz; ++i)
            VoronoiAdditionalTasks[i]->EnsureCompletion();

        VoronoiAdditionalTasks.Empty();
        FinishGeneration();
    }
}

void FVoronoiNavDataGenerator::CancelBuild()
{
    bBuildCanceled = true;

    for (int32 i = 0, sz = VoronoiMainTasks.Num(); i < sz; ++i)
        VoronoiMainTasks[i]->EnsureCompletion();
    VoronoiMainTasks.Empty();

    for (int32 i = 0, sz = VoronoiAdditionalTasks.Num(); i < sz; ++i)
        VoronoiAdditionalTasks[i]->EnsureCompletion();
    VoronoiAdditionalTasks.Empty();

    bBuildCanceled = false;

    // Rendering related
    if (VoronoiNavData.IsValid() && VoronoiNavData->VoronoiGraph.IsValid())
    {
        if (VoronoiNavData->VoronoiGraph->bCanRenderGenerated)
            VoronoiNavData->VoronoiGraph->RenderedSurfaces = MoveTemp(VoronoiNavData->VoronoiGraph->GeneratedSurfaces);
        VoronoiNavData->VoronoiGraph->bCanRenderGenerated = false;
    }
}

void FVoronoiNavDataGenerator::TickAsyncBuild(float DeltaSeconds)
{
    for (int32 i = 0; i < VoronoiMainTasks.Num(); ++i)
    {
        if (VoronoiMainTasks[i]->IsDone())
        {
            VoronoiMainTasks.RemoveAtSwap(i--);
            if (VoronoiMainTasks.Num() == 0)
                SyncGeneration();
        }
    }

    if (VoronoiNavData->VoronoiGraph && VoronoiNavData->VoronoiGraph->bCanRenderGenerated)
        VoronoiNavData->UpdateVoronoiGraphDrawing();

    for (int32 i = 0; i < VoronoiAdditionalTasks.Num(); ++i)
    {
        if (VoronoiAdditionalTasks[i]->IsDone())
        {
            VoronoiAdditionalTasks.RemoveAtSwap(i--);
            if (VoronoiAdditionalTasks.Num() == 0)
                FinishGeneration();
        }
    }
}

void FVoronoiNavDataGenerator::RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas)
{
    bool bRebuild = false;
    for (const auto& area : DirtyAreas)
        bRebuild |= area.HasFlag(ENavigationDirtyFlag::Geometry);

    if (bRebuild)
        RebuildAll();
}

void FVoronoiNavDataGenerator::SyncGeneration()
{
    // Transfer precomputed properties from old surfaces (draft result)
    for (const auto& Surface : VoronoiNavData->VoronoiGraph->GeneratedSurfaces)
        for (const auto& Face : Surface->GetFaces())
            if (FVoronoiFace *OldFace = VoronoiNavData->GetFaceByPoint(Face->GetLocation(), false))
                Face->TacticalProperties = OldFace->TacticalProperties;

    // Allow to render intermediate results
    VoronoiNavData->VoronoiGraph->bCanRenderGenerated = true;

    // Continue links and properties calculating
    VoronoiAdditionalTasks.Reserve(2);
    VoronoiAdditionalTasks.Emplace(new FVoronoiAdditionalTask(VoronoiNavData.Get(), EVoronoiTaskType::VTT_Links, &bBuildCanceled));
    VoronoiAdditionalTasks.Emplace(new FVoronoiAdditionalTask(VoronoiNavData.Get(), EVoronoiTaskType::VTT_Properties, &bBuildCanceled));

    VoronoiAdditionalTasks[0]->StartBackgroundTask(VoronoiThreadPool);
    VoronoiAdditionalTasks[1]->StartBackgroundTask(VoronoiThreadPool);
}

void FVoronoiNavDataGenerator::FinishGeneration()
{
    VoronoiNavData->VoronoiGraph->bCanRenderGenerated = false;
    VoronoiNavData->VoronoiGraph->RenderedSurfaces.Empty();

    VoronoiNavData->VoronoiGraph->Surfaces = MoveTemp(VoronoiNavData->VoronoiGraph->GeneratedSurfaces);
    VoronoiNavData->OnVoronoiNavDataGenerationFinished(FPlatformTime::Seconds() - GenerationStartTime);
}
