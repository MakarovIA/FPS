// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiNavDataGenerator.h"

#include "VoronoiGeometryTask.h"
#include "VoronoiSurfaceTask.h"
#include "VoronoiDiagramTask.h"
#include "VoronoiPropertiesTask.h"

/** FVoronoiNavDataGenerator */

bool FVoronoiNavDataGenerator::RebuildAll()
{
    CancelBuild();

    VoronoiNavData->VoronoiGraph->GeneratedSurfaces.Empty();
    GenerationStartTime = FPlatformTime::Seconds();

    GenerationOptions = VoronoiNavData->GetGenerationOptions();
    AgentProperties = VoronoiNavData->GetConfig();

    // ------------------------------------------------------------------------------------
    // COLLECT NAVIGATION BOUNDING BOXES
    // ------------------------------------------------------------------------------------

    const UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World.Get());
    const TSet<FNavigationBounds>& NavigationBoundsSet = NavSys->GetNavigationBounds();
    const int32 AgentIndex = NavSys->GetSupportedAgentIndex(VoronoiNavData.Get());

    TArray<FBox> InclusionBounds;
    if (NavSys->ShouldGenerateNavigationEverywhere())
        InclusionBounds.Add(NavSys->GetWorldBounds());
    else
        for (const FNavigationBounds& NavigationBounds : NavigationBoundsSet)
            if (NavigationBounds.SupportedAgents.Contains(AgentIndex))
                InclusionBounds.Add(NavigationBounds.AreaBox);

    for (int32 i = 0, sz = InclusionBounds.Num(); i < sz; ++i)
        InclusionBounds[i].Min = FVector(
            FMath::FloorToFloat(InclusionBounds[i].Min.X / GenerationOptions.CellSize) * GenerationOptions.CellSize,
            FMath::FloorToFloat(InclusionBounds[i].Min.Y / GenerationOptions.CellSize) * GenerationOptions.CellSize,
            FMath::FloorToFloat(InclusionBounds[i].Min.Z / GenerationOptions.CellSize) * GenerationOptions.CellSize),
        InclusionBounds[i].Max = FVector(
            FMath::FloorToFloat(InclusionBounds[i].Max.X / GenerationOptions.CellSize) * GenerationOptions.CellSize,
            FMath::FloorToFloat(InclusionBounds[i].Max.Y / GenerationOptions.CellSize) * GenerationOptions.CellSize,
            FMath::FloorToFloat(InclusionBounds[i].Max.Z / GenerationOptions.CellSize) * GenerationOptions.CellSize);

    for (int32 i = 0; i < InclusionBounds.Num(); ++i)
    {
        for (int32 j = i + 1; j < InclusionBounds.Num(); ++j)
        {
            const FBox Overlap = InclusionBounds[i].Overlap(InclusionBounds[j]);
            if (Overlap.Max.X == Overlap.Min.X || Overlap.Max.Y == Overlap.Min.Y || Overlap.Max.Z == Overlap.Min.Z)
                continue;

            if (Overlap.Min.X > InclusionBounds[j].Min.X)
                InclusionBounds.Emplace(InclusionBounds[j].Min, FVector(Overlap.Min.X, InclusionBounds[j].Max.Y, InclusionBounds[j].Max.Z));
            
            if (Overlap.Max.X < InclusionBounds[j].Max.X)
                InclusionBounds.Emplace(FVector(Overlap.Max.X, InclusionBounds[j].Min.Y, InclusionBounds[j].Min.Z), InclusionBounds[j].Max);
            
            if (Overlap.Min.Y > InclusionBounds[j].Min.Y)
                InclusionBounds.Emplace(FVector(Overlap.Min.X, InclusionBounds[j].Min.Y, InclusionBounds[j].Min.Z), FVector(Overlap.Max.X, Overlap.Min.Y, InclusionBounds[j].Max.Z));

            if (Overlap.Max.Y < InclusionBounds[j].Max.Y)
                InclusionBounds.Emplace(FVector(Overlap.Min.X, Overlap.Max.Y, InclusionBounds[j].Min.Z), FVector(Overlap.Max.X, InclusionBounds[j].Max.Y, InclusionBounds[j].Max.Z));

            if (Overlap.Min.Z > InclusionBounds[j].Min.Z)
                InclusionBounds.Emplace(FVector(Overlap.Min.X, Overlap.Min.Y, InclusionBounds[j].Min.Z), FVector(Overlap.Max.X, Overlap.Max.Y, Overlap.Min.Z));

            if (Overlap.Max.Z < InclusionBounds[j].Max.Z)
                InclusionBounds.Emplace(FVector(Overlap.Min.X, Overlap.Min.Y, Overlap.Max.Z), FVector(Overlap.Max.X, Overlap.Max.Y, InclusionBounds[j].Max.Z));

            InclusionBounds.RemoveAt(j--);
        }
    }

    // ------------------------------------------------------------------------------------
    // LAUNCH GEOMETRY COLLECTION
    // ------------------------------------------------------------------------------------

    TotalBounds = 0;
    for (int32 i = 0, sz = InclusionBounds.Num(); i < sz; ++i)
        TotalBounds += InclusionBounds[i];

    const int32 TileSizeInCells = 50;
    const double TileSize = GenerationOptions.CellSize * TileSizeInCells;

    for (const FBox& Bounds : InclusionBounds)
    {
        for (int32 i = 0, isz = FMath::CeilToInt((Bounds.Max.X - Bounds.Min.X) / TileSize); i < isz; ++i)
        {
            for (int32 j = 0, jsz = FMath::CeilToInt((Bounds.Max.Y - Bounds.Min.Y) / TileSize); j < jsz; ++j)
            {
                const FVector TileBBMin(Bounds.Min.X + TileSize * i, Bounds.Min.Y + TileSize * j, Bounds.Min.Z);
                const FVector TileBBMax(i + 1 < isz ? Bounds.Min.X + TileSize * (i + 1) : Bounds.Max.X, j + 1 < jsz ? Bounds.Min.Y + TileSize * (j + 1) : Bounds.Max.Y, Bounds.Max.Z);

                VoronoiGeometryTasks.Add(MakeUnique<FAsyncTask<FVoronoiGeometryTask>>(*this, FBox(TileBBMin, TileBBMax)));
                VoronoiGeometryTasks.Last()->StartBackgroundTask();
            }
        }
    }

    if (VoronoiGeometryTasks.Num() == 0)
        FinishGeneration();

    return true;
}

void FVoronoiNavDataGenerator::EnsureBuildCompletion()
{
    if (VoronoiGeometryTasks.Num() != 0)
    {
        for (int32 i = 0, sz = VoronoiGeometryTasks.Num(); i < sz; ++i)
            VoronoiGeometryTasks[i]->EnsureCompletion();

        GeometryToSurface();
        VoronoiGeometryTasks.Empty();
    }

    if (VoronoiSurfaceTask.IsValid())
    {
        VoronoiSurfaceTask->EnsureCompletion();

        SurfaceToDiagram();
        VoronoiSurfaceTask.Release();
    }

    if (VoronoiDiagramTasks.Num() != 0)
    {
        for (int32 i = 0, sz = VoronoiDiagramTasks.Num(); i < sz; ++i)
            VoronoiDiagramTasks[i]->EnsureCompletion();

        DiagramToProperties();
        VoronoiDiagramTasks.Empty();
    }

    if (VoronoiPropertiesTask.IsValid())
    {
        VoronoiPropertiesTask->EnsureCompletion();
        VoronoiPropertiesTask.Release();

        FinishGeneration();
    }
}

void FVoronoiNavDataGenerator::CancelBuild()
{
    bBuildCanceled = true;

    for (int32 i = 0, sz = VoronoiGeometryTasks.Num(); i < sz; ++i)
        VoronoiGeometryTasks[i]->EnsureCompletion();
    VoronoiGeometryTasks.Empty();

    if (VoronoiSurfaceTask.IsValid())
    {
        VoronoiSurfaceTask->EnsureCompletion();
        VoronoiSurfaceTask.Release();
    }

    for (int32 i = 0, sz = VoronoiDiagramTasks.Num(); i < sz; ++i)
        VoronoiDiagramTasks[i]->EnsureCompletion();
    VoronoiDiagramTasks.Empty();

    if (VoronoiPropertiesTask.IsValid())
    {
        VoronoiPropertiesTask->EnsureCompletion();
        VoronoiPropertiesTask.Release();
    }

    bBuildCanceled = false;

    if (VoronoiNavData.IsValid() && VoronoiNavData->VoronoiGraph.IsValid())
    {
        if (VoronoiNavData->VoronoiGraph->bCanRenderGenerated)
            VoronoiNavData->VoronoiGraph->RenderedSurfaces = MoveTemp(VoronoiNavData->VoronoiGraph->GeneratedSurfaces);
        VoronoiNavData->VoronoiGraph->bCanRenderGenerated = false;
    }
}

void FVoronoiNavDataGenerator::TickAsyncBuild(float DeltaSeconds)
{
    if (VoronoiGeometryTasks.Num() != 0)
    {
        int32 CompletedTasks = 0;
        for (int32 i = 0, sz = VoronoiGeometryTasks.Num(); i < sz; ++i)
            CompletedTasks += VoronoiGeometryTasks[i]->IsDone();

        if (CompletedTasks == VoronoiGeometryTasks.Num())
        {
            GeometryToSurface();
            VoronoiGeometryTasks.Empty();
        }
    }

    if (VoronoiSurfaceTask.IsValid() && VoronoiSurfaceTask->IsDone())
    {
        SurfaceToDiagram();
        VoronoiSurfaceTask.Release();
    }

    if (VoronoiDiagramTasks.Num() != 0)
    {
        int32 CompletedTasks = 0;
        for (int32 i = 0, sz = VoronoiDiagramTasks.Num(); i < sz; ++i)
            CompletedTasks += VoronoiDiagramTasks[i]->IsDone();

        if (CompletedTasks == VoronoiDiagramTasks.Num())
        {
            DiagramToProperties();
            VoronoiDiagramTasks.Empty();
        }
    }

#ifdef WITH_EDITOR
    if (VoronoiNavData->VoronoiGraph && VoronoiNavData->VoronoiGraph->bCanRenderGenerated)
        VoronoiNavData->UpdateVoronoiGraphDrawing();
#endif // WITH_EDITOR

    if (VoronoiPropertiesTask.IsValid() && VoronoiPropertiesTask->IsDone())
    {
        VoronoiPropertiesTask.Release();
        FinishGeneration();
    }
}

void FVoronoiNavDataGenerator::RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas)
{
    if (DirtyAreas.ContainsByPredicate([](const FNavigationDirtyArea& Area) { return Area.HasFlag(ENavigationDirtyFlag::Geometry) || Area.HasFlag(ENavigationDirtyFlag::NavigationBounds); }))
        RebuildAll();
}

int32 FVoronoiNavDataGenerator::GetNumRunningBuildTasks() const
{
    int32 RunningTasks = 0;
    for (int32 i = 0; i < VoronoiGeometryTasks.Num(); ++i)
        RunningTasks += !VoronoiGeometryTasks[i]->IsDone();

    RunningTasks += VoronoiSurfaceTask.IsValid();
    for (int32 i = 0; i < VoronoiDiagramTasks.Num(); ++i)
        RunningTasks += !VoronoiDiagramTasks[i]->IsDone();

    RunningTasks += VoronoiPropertiesTask.IsValid();
    return RunningTasks;
}

void FVoronoiNavDataGenerator::GeometryToSurface()
{
    UE_LOG(LogNavigation, Log, TEXT("Voronoi geometry collection finished in %d ms"), (int32)((FPlatformTime::Seconds() - GenerationStartTime) * 1000));

    const FVector TotalBBSize = TotalBounds.GetSize();
    const int32 TotalBBSizeX = FMath::RoundToInt(TotalBBSize.X / GenerationOptions.CellSize);
    const int32 TotalBBSizeY = FMath::RoundToInt(TotalBBSize.Y / GenerationOptions.CellSize);

    TArray<TArray<float>> ProfileTemp;
    TArray<TArray<TArray<float>>> GlobalHeightField;

    ProfileTemp.Init(TArray<float>(), TotalBBSizeY);
    GlobalHeightField.Init(ProfileTemp, TotalBBSizeX);

    for (int32 k = 0, sz = VoronoiGeometryTasks.Num(); k < sz; ++k)
    {
        FVoronoiGeometryTask &Task = VoronoiGeometryTasks[k]->GetTask();

        const FVector TileBBSize = Task.TileBB.GetSize();
        const int32 TileBBSizeX = FMath::RoundToInt(TileBBSize.X / GenerationOptions.CellSize);
        const int32 TileBBSizeY = FMath::RoundToInt(TileBBSize.Y / GenerationOptions.CellSize);

        const FVector HeightFieldOffset = Task.TileBB.Min - TotalBounds.Min;
        const int32 GlobalX = FMath::RoundToInt(HeightFieldOffset.X / GenerationOptions.CellSize);
        const int32 GlobalY = FMath::RoundToInt(HeightFieldOffset.Y / GenerationOptions.CellSize);

        for (int32 i = 0; i < TileBBSizeX; ++i)
            for (int32 j = 0; j < TileBBSizeY; ++j)
                if (GlobalHeightField[GlobalX + i][GlobalY + j].Num() == 0)
                    GlobalHeightField[GlobalX + i][GlobalY + j] = MoveTemp(Task.CompressedHeightField[i][j]);
                else
                    GlobalHeightField[GlobalX + i][GlobalY + j].Append(MoveTemp(Task.CompressedHeightField[i][j]));
    }

    VoronoiSurfaceTask = MakeUnique<FAsyncTask<FVoronoiSurfaceTask>>(*this, MoveTemp(GlobalHeightField), TotalBounds.Min);
    VoronoiSurfaceTask->StartBackgroundTask();
}

void FVoronoiNavDataGenerator::SurfaceToDiagram()
{
    UE_LOG(LogNavigation, Log, TEXT("Voronoi surface generation finished in %d ms"), (int32)((FPlatformTime::Seconds() - GenerationStartTime) * 1000));

    FVoronoiSurfaceTask &Task = VoronoiSurfaceTask->GetTask();
    VoronoiNavData->VoronoiGraph->GeneratedSurfaces.Reserve(Task.Surfaces.Num());

    const float CellArea = GenerationOptions.CellSize * GenerationOptions.CellSize / 1e4f;
    const float SitesDensity = CellArea * VoronoiNavData->GetGenerationOptions().SitesPerSquareMeter;

    for (int32 i = 0, sz = Task.Surfaces.Num(); i < sz; ++i)
    {
        VoronoiNavData->VoronoiGraph->GeneratedSurfaces.Add(MakeUnique<FVoronoiSurface>(Task.SurfaceBorders[i]));
        for (int32 j = 0, SitesToPlace = FMath::Min(Task.Surfaces[i].Num(), FMath::FloorToInt(Task.Surfaces[i].Num() * SitesDensity)); j < SitesToPlace; ++j)
        {
            const int32 ChosenCell = FMath::RandRange(0, Task.Surfaces[i].Num() - 1);
            VoronoiNavData->VoronoiGraph->GeneratedSurfaces[i]->Faces.Add(MakeUnique<FVoronoiFace>(VoronoiNavData->VoronoiGraph->GeneratedSurfaces[i], Task.Surfaces[i][ChosenCell]));
            Task.Surfaces[i].RemoveAtSwap(ChosenCell);
        }

        for (int32 j = 0, N = Task.SurfaceBorders[i].Num(); j < N; ++j)
            for (int32 k = 0, M = Task.SurfaceBorders[i][j].Num(); k < M; ++k)
                VoronoiNavData->VoronoiGraph->GeneratedSurfaces[i]->Faces.Add(MakeUnique<FVoronoiFace>(VoronoiNavData->VoronoiGraph->GeneratedSurfaces[i], Task.SurfaceBorders[i][j][k]));

        VoronoiDiagramTasks.Add(MakeUnique<FAsyncTask<FVoronoiDiagramTask>>(*this, VoronoiNavData->VoronoiGraph->GeneratedSurfaces[i]));
        VoronoiDiagramTasks.Last()->StartBackgroundTask();
    }

    if (VoronoiDiagramTasks.Num() == 0)
        FinishGeneration();
}

void FVoronoiNavDataGenerator::DiagramToProperties()
{
    UE_LOG(LogNavigation, Log, TEXT("Voronoi diagrams generation finished in %d ms"), (int32)((FPlatformTime::Seconds() - GenerationStartTime) * 1000));

#ifdef WITH_EDITOR
    for (TPreserveConstUniquePtr<FVoronoiSurface>& Surface : VoronoiNavData->VoronoiGraph->GeneratedSurfaces)
        for (TPreserveConstUniquePtr<FVoronoiFace>& Face : Surface->Faces)
            if (FVoronoiFace *OldFace = VoronoiNavData->GetFaceByPoint(Face->Location, false))
                Face->TacticalProperties = OldFace->TacticalProperties;

    VoronoiNavData->VoronoiGraph->bCanRenderGenerated = true;
#endif // WITH_EDITOR

    // Continue links and properties calculation
    VoronoiPropertiesTask = MakeUnique<FAsyncTask<FVoronoiPropertiesTask>>(*this, &VoronoiNavData->VoronoiGraph->GeneratedSurfaces);
    VoronoiPropertiesTask->StartBackgroundTask();
}

void FVoronoiNavDataGenerator::FinishGeneration()
{
    UE_LOG(LogNavigation, Log, TEXT("Voronoi properties calculation finished in %d ms"), (int32)((FPlatformTime::Seconds() - GenerationStartTime) * 1000));

    VoronoiNavData->VoronoiGraph->bCanRenderGenerated = false;
    VoronoiNavData->VoronoiGraph->RenderedSurfaces.Empty();

    VoronoiNavData->VoronoiGraph->Surfaces = MoveTemp(VoronoiNavData->VoronoiGraph->GeneratedSurfaces);
    VoronoiNavData->OnVoronoiNavDataGenerationFinished();
}
