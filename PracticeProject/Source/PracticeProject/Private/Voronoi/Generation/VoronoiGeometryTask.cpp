// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiGeometryTask.h"
#include "NavigationOctree.h"

/** Types */

struct FVoronoiGeometryCache
{
    struct FHeader
    {
        int32 NumVerts;
        int32 NumFaces;
        FWalkableSlopeOverride SlopeOverride;
    };

    FHeader Header;

    float* Verts;
    int32* Indices;

    FORCEINLINE FVoronoiGeometryCache(const uint8* Memory)
        : Header(*((FHeader*)Memory))
        , Verts((float*)(Memory + sizeof(FVoronoiGeometryCache)))
        , Indices((int32*)(Memory + sizeof(FVoronoiGeometryCache) + (sizeof(float) * Header.NumVerts * 3))) {}
};

struct FVoronoiRawGeometryElement
{
    TArray<float> GeomCoords;
    TArray<int32> GeomIndices;

    TArray<FTransform> PerInstanceTransform;
};

struct FVoronoiGeometryElement
{
    TArray<FVector> Vertexes;
    TArray<int32> Indices;

    FORCEINLINE FVoronoiGeometryElement(FVoronoiRawGeometryElement InElement)
        : Indices(MoveTemp(InElement.GeomIndices))
    {
        for (int32 i = 0, sz = InElement.GeomCoords.Num(); i < sz; i += 3)
            Vertexes.Emplace(-InElement.GeomCoords[i], -InElement.GeomCoords[i + 2], InElement.GeomCoords[i + 1]);

        for (const FTransform& Transform : InElement.PerInstanceTransform)
            for (int32 i = 0, sz = InElement.GeomCoords.Num(); i < sz; i += 3)
                Vertexes[i] = Transform.TransformPosition(Vertexes[i]);
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
        if (Element.ShouldUseGeometry(AgentProperties) && Element.Data->HasGeometry() && !Element.Data->IsPendingLazyGeometryGathering() && Element.Data->GetOwner() != nullptr)
            NavigationRelevantData.Add(Element.Data);
    }
}

void FVoronoiGeometryTask::AppendGeometry(const FNavigationRelevantData& ElementData, TArray<FVoronoiGeometryElement> &Geometry)
{
    if (ElementData.CollisionData.Num() == 0)
        return;

    FVoronoiRawGeometryElement GeometryElement;
    FVoronoiGeometryCache CollisionCache(ElementData.CollisionData.GetData());

    if (ElementData.NavDataPerInstanceTransformDelegate.IsBound())
    {
        ElementData.NavDataPerInstanceTransformDelegate.Execute(TileBB, GeometryElement.PerInstanceTransform);
        if (GeometryElement.PerInstanceTransform.Num() == 0)
            return;
    }

    const int32 NumCoords = CollisionCache.Header.NumVerts * 3;
    const int32 NumIndices = CollisionCache.Header.NumFaces * 3;

    if (NumIndices > 0)
    {
        GeometryElement.GeomCoords.SetNumUninitialized(NumCoords);
        GeometryElement.GeomIndices.SetNumUninitialized(NumIndices);

        FMemory::Memcpy(GeometryElement.GeomCoords.GetData(), CollisionCache.Verts, sizeof(float) * NumCoords);
        FMemory::Memcpy(GeometryElement.GeomIndices.GetData(), CollisionCache.Indices, sizeof(int32) * NumIndices);

        Geometry.Emplace(MoveTemp(GeometryElement));
    }
}

void FVoronoiGeometryTask::DoWork()
{
    TArray<FVoronoiGeometryElement> Geometry;
    for (TSharedRef<FNavigationRelevantData, ESPMode::ThreadSafe>& ElementData : NavigationRelevantData)
    {
        if (ShouldCancelBuild())
            return;

        AppendGeometry(ElementData.Get(), Geometry);
    }

    // ------------------------------------------------------------------------------------
    // USE COLLECTED GEOMETRY TO CONSTRUCT HEIGHTFIELD
    // ------------------------------------------------------------------------------------

    const FVector TileBBSize = TileBB.GetSize();

    const int32 TileBBSizeX = FMath::RoundToInt(TileBBSize.X / CellSize);
    const int32 TileBBSizeY = FMath::RoundToInt(TileBBSize.Y / CellSize);
    const int32 TileBBSizeZ = FMath::RoundToInt(TileBBSize.Z / CellSize);

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

            const FVector& A = Element.Vertexes[Element.Indices[i]];
            const FVector& B = Element.Vertexes[Element.Indices[i + 1]];
            const FVector& C = Element.Vertexes[Element.Indices[i + 2]];

            const FVector TriangleNormal = FVector::CrossProduct(B - A, C - A);
            const bool bIsWalkable = FMath::Abs(TriangleNormal.GetSafeNormal().Z) > 0.75;

            const TArray<FVector> TriangleEdges = { B - A, C - A, B - C };
            const TArray<FVector> TriangleVertexes = { A, B, C };

            const FBox TriangleAABB = FBox(TriangleVertexes).ShiftBy(-TileBB.Min);

            const int32 MinX = FMath::Max(0, FMath::FloorToInt((TriangleAABB.Min.X - 1) / CellSize)), MaxX = FMath::Min(TileBBSizeX, FMath::CeilToInt((TriangleAABB.Max.X + 1) / CellSize));
            const int32 MinY = FMath::Max(0, FMath::FloorToInt((TriangleAABB.Min.Y - 1) / CellSize)), MaxY = FMath::Min(TileBBSizeY, FMath::CeilToInt((TriangleAABB.Max.Y + 1) / CellSize));
            const int32 MinZ = FMath::Max(0, FMath::FloorToInt((TriangleAABB.Min.Z - 1) / CellSize)), MaxZ = FMath::Min(TileBBSizeZ, FMath::CeilToInt((TriangleAABB.Max.Z + 1) / CellSize));

            for (int32 X = MinX; X < MaxX; ++X)
            {
                for (int32 Y = MinY; Y < MaxY; ++Y)
                {
                    for (int32 Z = MinZ; Z < MaxZ; ++Z)
                    {
                        const FVector TestBoxMin = TileBB.Min + FVector(X * CellSize, Y * CellSize, Z * CellSize);
                        const TArray<FVector> TestBoxNormals = { FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1) };
                        const TArray<FVector> TestBoxVertexes = { TestBoxMin, TestBoxMin + FVector(CellSize, 0, 0), TestBoxMin + FVector(0, CellSize, 0), TestBoxMin + FVector(0, 0, CellSize),
                            TestBoxMin + FVector(CellSize, CellSize, 0), TestBoxMin + FVector(CellSize, 0, CellSize), TestBoxMin + FVector(0, CellSize, CellSize), TestBoxMin + FVector(CellSize, CellSize, CellSize) };

                        EHeightFieldState Temp = HeightField[X][Y][Z];
                        HeightField[X][Y][Z] = EHeightFieldState::Obstacle;

                        for (int32 t = 0; t < 3; ++t)
                        {
                            for (int32 j = 0; j < 3; ++j)
                            {
                                float BoxProjectionMin, BoxProjectionMax, TriangleProjectionMin, TriangleProjectionMax;
                                const FVector Axis = TriangleEdges[t] ^ TestBoxNormals[j];

                                FMathExtended::ProjectOnAxis(TestBoxVertexes, Axis, BoxProjectionMin, BoxProjectionMax);
                                FMathExtended::ProjectOnAxis(TriangleVertexes, Axis, TriangleProjectionMin, TriangleProjectionMax);

                                if (BoxProjectionMax < TriangleProjectionMin || BoxProjectionMin > TriangleProjectionMax)
                                    HeightField[X][Y][Z] = Temp, t = 3, j = 3;
                            }
                        }

                        if (bIsWalkable && Z + 1 < TileBBSizeZ && HeightField[X][Y][Z] == EHeightFieldState::Obstacle && HeightField[X][Y][Z + 1] == EHeightFieldState::Empty)
                            HeightField[X][Y][Z + 1] = EHeightFieldState::Walkable;
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------------------
    // COMPRESS HEIGHTFIELD
    // ------------------------------------------------------------------------------------

    const float AgentCrouchedHeight = AgentProperties.AgentHeight * 2 / 3;

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

                if (HeightField[i][j][k] == EHeightFieldState::Empty && (HeightAvaliable += CellSize) > AgentCrouchedHeight && LastWalkable != -1)
                    CompressedHeightField[i][j].Add(TileBB.Min.Z + LastWalkable * CellSize), LastWalkable = -1;

                if (HeightField[i][j][k] == EHeightFieldState::Obstacle)
                    LastWalkable = -1;

                if (HeightField[i][j][k] == EHeightFieldState::Walkable)
                    HeightAvaliable = CellSize, LastWalkable = k;
            }
        }
    }
}
