// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiGeometryTask.h"
#include "NavigationOctree.h"

/** Types */

struct FVoronoiGeometryCache
{
    struct FHeader
    {
        int32 NumVertexes;
        int32 NumFaces;
        FWalkableSlopeOverride SlopeOverride;
    };

    FHeader Header;

    float* Vertexes;
    int32* Indices;

    FORCEINLINE FVoronoiGeometryCache(const uint8* Memory)
        : Header(*((FHeader*)Memory)), Vertexes((float*)(Memory + sizeof(FVoronoiGeometryCache)))
        , Indices((int32*)(Memory + sizeof(FVoronoiGeometryCache) + sizeof(float) * Header.NumVertexes * 3)) {}
};

struct FVoronoiGeometryElement
{
    TArray<FVector> Vertexes;
    TArray<int32> Indices;

    FORCEINLINE FVoronoiGeometryElement(const FVoronoiGeometryCache& InCachedElement)
    {
        for (int32 i = 0, sz = InCachedElement.Header.NumVertexes * 3; i < sz; i += 3)
            Vertexes.Emplace(-InCachedElement.Vertexes[i], -InCachedElement.Vertexes[i + 2], InCachedElement.Vertexes[i + 1]);
        Indices.Append(InCachedElement.Indices, InCachedElement.Header.NumFaces * 3);
    }
};

enum class EHeightFieldState : uint8
{
    Empty,
    Obstacle,
    Walkable
};

/** FVoronoiGeometryTask */

FVoronoiGeometryTask::FVoronoiGeometryTask(const FVoronoiNavDataGenerator& InParentGenerator, const FBox& InTileBB)
    : FVoronoiTask(InParentGenerator), TileBB(InTileBB)
{
    const FNavigationOctree* NavOctree = World->GetNavigationSystem()->GetNavOctree();
    for (FNavigationOctree::TConstElementBoxIterator<FNavigationOctree::DefaultStackAllocator> It(*NavOctree, TileBB); It.HasPendingElements(); It.Advance())
    {
        const FNavigationOctreeElement& Element = It.GetCurrentElement();
        if (Element.Data->HasGeometry() && !Element.Data->IsPendingLazyGeometryGathering() && Element.Data->GetOwner())
            NavigationRelevantData.Add(Element.Data);
    }
}

void FVoronoiGeometryTask::DoWork()
{
    TArray<FVoronoiGeometryElement> Geometry;
    for (TSharedRef<FNavigationRelevantData, ESPMode::ThreadSafe>& ElementData : NavigationRelevantData)
    {
        if (ShouldCancelBuild())
            return;

        if (ElementData->CollisionData.Num() > 0)
            Geometry.Emplace(FVoronoiGeometryCache(ElementData->CollisionData.GetData()));
    }

    // ------------------------------------------------------------------------------------
    // USE COLLECTED GEOMETRY TO CONSTRUCT HEIGHTFIELD
    // ------------------------------------------------------------------------------------

    const FVector TileBBSize = TileBB.GetSize() / GenerationOptions.CellSize;
    const int32 TileBBSizeX = FMath::RoundToInt(TileBBSize.X), TileBBSizeY = FMath::RoundToInt(TileBBSize.Y), TileBBSizeZ = FMath::RoundToInt(TileBBSize.Z);

    static const TArray<FVector> TestBoxNormals = { FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1) };
    const TArray<FVector> TestBoxVertexes = { FVector(0, 0, 0), FVector(GenerationOptions.CellSize, 0, 0), FVector(0, GenerationOptions.CellSize, 0), FVector(0, 0, GenerationOptions.CellSize),
        FVector(GenerationOptions.CellSize, GenerationOptions.CellSize, 0), FVector(GenerationOptions.CellSize, 0, GenerationOptions.CellSize), FVector(0, GenerationOptions.CellSize, GenerationOptions.CellSize),
        FVector(GenerationOptions.CellSize, GenerationOptions.CellSize, GenerationOptions.CellSize) };

    TArray<EHeightFieldState> ColumnTemp;
    TArray<TArray<EHeightFieldState>> ProfileTemp;
    TArray<TArray<TArray<EHeightFieldState>>> HeightField;

    ColumnTemp.Init(EHeightFieldState::Empty, TileBBSizeZ);
    ProfileTemp.Init(ColumnTemp, TileBBSizeY);
    HeightField.Init(ProfileTemp, TileBBSizeX);

    for (const FVoronoiGeometryElement &Element : Geometry)
    {
        for (int32 i = 0, sz = Element.Indices.Num(); i < sz; i += 3)
        {
            if (ShouldCancelBuild())
                return;

            const TArray<FVector> TriangleVertexes = { Element.Vertexes[Element.Indices[i]], Element.Vertexes[Element.Indices[i + 1]], Element.Vertexes[Element.Indices[i + 2]] };
            const TArray<FVector> TriangleEdges = { TriangleVertexes[1] - TriangleVertexes[0], TriangleVertexes[2] - TriangleVertexes[0], TriangleVertexes[2] - TriangleVertexes[1] };

            const FVector TriangleNormal = FVector::CrossProduct(TriangleEdges[1], TriangleEdges[0]).GetSafeNormal();
            const bool bIsWalkable = TriangleNormal.Z > FMath::Cos(FMath::DegreesToRadians(GenerationOptions.AgentWalkableSlope));
            
            const FBox TriangleAABB = FBox(TriangleVertexes).ShiftBy(-TileBB.Min);
            const FVector TriangleAABBMin = (TriangleAABB.Min - FVector(1, 1, 1)) / GenerationOptions.CellSize;
            const FVector TriangleAABBMax = (TriangleAABB.Max + FVector(1, 1, 1)) / GenerationOptions.CellSize;

            const int32 MinX = FMath::Max(0, FMath::FloorToInt(TriangleAABBMin.X)), MaxX = FMath::Min(TileBBSizeX, FMath::CeilToInt(TriangleAABBMax.X));
            const int32 MinY = FMath::Max(0, FMath::FloorToInt(TriangleAABBMin.Y)), MaxY = FMath::Min(TileBBSizeY, FMath::CeilToInt(TriangleAABBMax.Y));
            const int32 MinZ = FMath::Max(0, FMath::FloorToInt(TriangleAABBMin.Z)), MaxZ = FMath::Min(TileBBSizeZ, FMath::CeilToInt(TriangleAABBMax.Z));

            for (int32 X = MinX; X < MaxX; ++X)
            {
                for (int32 Y = MinY; Y < MaxY; ++Y)
                {
                    for (int32 Z = MinZ; Z < MaxZ; ++Z)
                    {
                        if ((HeightField[X][Y][Z] == EHeightFieldState::Obstacle && !bIsWalkable) || HeightField[X][Y][Z] == EHeightFieldState::Walkable)
                            continue;

                        const FVector TestBoxMin = TileBB.Min + FVector(X * GenerationOptions.CellSize, Y * GenerationOptions.CellSize, Z * GenerationOptions.CellSize);
                        const TArray<FVector> TestTriangleVertexes = { TriangleVertexes[0] - TestBoxMin, TriangleVertexes[1] - TestBoxMin, TriangleVertexes[2] - TestBoxMin };

                        float BoxOffsetMin, BoxOffsetMax;
                        const float TriangleOffset = FVector::DotProduct(TriangleNormal, TestTriangleVertexes[0]);

                        FMathExtended::ProjectOnAxis(TestBoxVertexes, TriangleNormal, BoxOffsetMin, BoxOffsetMax);
                        if (BoxOffsetMax < TriangleOffset || BoxOffsetMin > TriangleOffset)
                            continue;

                        bool bIntersects = true;
                        for (int32 j = 0; j < 3; ++j)
                        {
                            for (int32 k = 0; k < 3; ++k)
                            {
                                float BoxProjectionMin, BoxProjectionMax, TriangleProjectionMin, TriangleProjectionMax;
                                const FVector Axis = TriangleEdges[j] ^ TestBoxNormals[k];

                                FMathExtended::ProjectOnAxis(TestBoxVertexes, Axis, BoxProjectionMin, BoxProjectionMax);
                                FMathExtended::ProjectOnAxis(TestTriangleVertexes, Axis, TriangleProjectionMin, TriangleProjectionMax);

                                if (BoxProjectionMax < TriangleProjectionMin || BoxProjectionMin > TriangleProjectionMax)
                                {
                                    bIntersects = false;
                                    j = 3, k = 3;
                                }
                            }
                        }

                        if (bIntersects)
                            HeightField[X][Y][Z] = bIsWalkable ? EHeightFieldState::Walkable : EHeightFieldState::Obstacle;
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------------------
    // COMPRESS HEIGHTFIELD
    // ------------------------------------------------------------------------------------

    TArray<TArray<float>> CompressedProfileTemp;
    CompressedProfileTemp.Init(TArray<float>(), TileBBSizeY);
    CompressedHeightField.Init(CompressedProfileTemp, TileBBSizeX);

    for (int32 i = 0; i < TileBBSizeX; ++i)
    {
        for (int32 j = 0; j < TileBBSizeY; ++j)
        {
            int32 LastWalkable = -1;
            float HeightAvaliable = 0;

            for (int32 k = 0; k < TileBBSizeZ; ++k)
            {
                if (ShouldCancelBuild())
                    return;

                switch (HeightField[i][j][k])
                {
                    case EHeightFieldState::Empty:
                        if (LastWalkable != -1 && (HeightAvaliable += GenerationOptions.CellSize) > GenerationOptions.AgentCrouchedHeight)
                            CompressedHeightField[i][j].Add(TileBB.Min.Z + (LastWalkable + 1) * GenerationOptions.CellSize), LastWalkable = -1, HeightAvaliable = 0;
                        break;

                    case EHeightFieldState::Obstacle:
                        LastWalkable = -1;
                        break;

                    case EHeightFieldState::Walkable:
                        LastWalkable = k;
                }
            }
        }
    }
}
