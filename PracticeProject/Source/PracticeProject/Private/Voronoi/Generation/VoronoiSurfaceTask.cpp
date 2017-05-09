// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiSurfaceTask.h"

/** Types */

struct FFloodIndex
{
    int32 X, Y, Z;
    float Height;

    FORCEINLINE FFloodIndex(int32 InX, int32 InY, int32 InZ, float InHeight)
        : X(InX), Y(InY), Z(InZ), Height(InHeight) {}

    FORCEINLINE friend bool operator<(const FFloodIndex& Left, const FFloodIndex& Right)
    { 
        return Left.Height < Right.Height;
    }
};

struct FBorderCell
{
    int32 SurfaceId;
    float Height;

    bool Left, Right, Up, Down;

    FORCEINLINE FBorderCell(int32 InSurfaceId, float InHeight, bool InLeft, bool InRight, bool InUp, bool InDown)
        : SurfaceId(InSurfaceId), Height(InHeight), Left(InLeft), Right(InRight), Up(InUp), Down(InDown) {}
};

/** FVoronoiSurfaceTask */

void FVoronoiSurfaceTask::DoWork()
{
    const int32 XSize = GlobalHeightField.Num(), YSize = GlobalHeightField[0].Num();

    // ------------------------------------------------------------------------------------
    // CORRECT BY AGENT RADIUS AND BORDER INDENT
    // ------------------------------------------------------------------------------------

    for (int32 Pass = 0, Passes = FMath::CeilToInt((GenerationOptions.AgentRadius + GenerationOptions.BorderIndent) / GenerationOptions.CellSize); Pass < Passes + 2; ++Pass)
    {
        if (ShouldCancelBuild())
            return;

        TArray<TArray<float>> NewGlobalHeightTemp;
        NewGlobalHeightTemp.Init(TArray<float>(), YSize);

        TArray<TArray<TArray<float>>> NewGlobalHeightField;
        NewGlobalHeightField.Init(NewGlobalHeightTemp, XSize);

        for (int32 i = 1; i < XSize - 1; ++i)
        {
            for (int32 j = 1; j < YSize - 1; ++j)
            {
                for (int32 k = 0, ZSize = GlobalHeightField[i][j].Num(); k < ZSize; ++k)
                {
                    const auto Predicate = [StepHeight = GenerationOptions.AgentStepHeight, CurrentHeight = GlobalHeightField[i][j][k]](float InHeight) -> bool
                        { return FMath::Abs(InHeight - CurrentHeight) < StepHeight; };

                    const bool A = GlobalHeightField[i + 1][j].ContainsByPredicate(Predicate);
                    const bool B = GlobalHeightField[i - 1][j].ContainsByPredicate(Predicate);
                    const bool C = GlobalHeightField[i][j + 1].ContainsByPredicate(Predicate);
                    const bool D = GlobalHeightField[i][j - 1].ContainsByPredicate(Predicate);

                    if (A && B && C && D || (Pass >= Passes && (A || B) && (C || D)))
                        NewGlobalHeightField[i][j].Add(GlobalHeightField[i][j][k]);
                }
            }
        }

        GlobalHeightField = MoveTemp(NewGlobalHeightField);
    }

    // ------------------------------------------------------------------------------------
    // SPLIT LEVEL GEOMETRY INTO SURFACES
    // ------------------------------------------------------------------------------------

    TArray<FFloodIndex> FloodQueue;
    TArray<TArray<int32>> SurfaceIdTemp;
    TArray<TArray<TArray<int32>>> SurfaceId;

    SurfaceIdTemp.Init(TArray<int32>(), YSize);
    SurfaceId.Init(SurfaceIdTemp, XSize);

    for (int32 i = 1; i < XSize - 1; ++i)
    {
        for (int32 j = 1; j < YSize - 1; ++j)
        {
            SurfaceId[i][j].Init(-1, GlobalHeightField[i][j].Num());
            for (int32 k = 0, ZSize = GlobalHeightField[i][j].Num(); k < ZSize; ++k)
                FloodQueue.Emplace(i, j, k, GlobalHeightField[i][j][k]);
        }
    }

    FloodQueue.Heapify();
    while (FloodQueue.Num() > 0)
    {
        FFloodIndex Current = FloodQueue.HeapTop();
        FloodQueue.HeapRemoveAt(0, false);

        if (SurfaceId[Current.X][Current.Y][Current.Z] != -1)
            continue;

        Surfaces.Emplace();

        TArray<bool> bHeightVisitedTemp;
        TArray<TArray<bool>> bHeightVisited;

        bHeightVisitedTemp.Init(false, YSize);
        bHeightVisited.Init(bHeightVisitedTemp, XSize);

        TArray<FFloodIndex> Queue, CancelQueue;
        Queue.HeapPush(Current);

        while (Queue.Num() > 0)
        {
            if (ShouldCancelBuild())
                return;

            Current = Queue.HeapTop();
            Queue.HeapRemoveAt(0, false);

            if (bHeightVisited[Current.X][Current.Y] && SurfaceId[Current.X][Current.Y][Current.Z] == -1)
                CancelQueue.Push(Current);

            if (bHeightVisited[Current.X][Current.Y] || SurfaceId[Current.X][Current.Y][Current.Z] != -1)
                continue;

            bHeightVisited[Current.X][Current.Y] = true;
            SurfaceId[Current.X][Current.Y][Current.Z] = Surfaces.Num() - 1;

            int32 NeighboorIndex;
            const auto Predicate = [StepHeight = GenerationOptions.AgentStepHeight, CurrentHeight = GlobalHeightField[Current.X][Current.Y][Current.Z]](float InHeight)
                { return FMath::Abs(InHeight - CurrentHeight) < StepHeight; };

            if ((NeighboorIndex = GlobalHeightField[Current.X + 1][Current.Y].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[Current.X + 1][Current.Y][NeighboorIndex] == -1)
                Queue.HeapPush(FFloodIndex(Current.X + 1, Current.Y, NeighboorIndex, GlobalHeightField[Current.X + 1][Current.Y][NeighboorIndex]));

            if ((NeighboorIndex = GlobalHeightField[Current.X - 1][Current.Y].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[Current.X - 1][Current.Y][NeighboorIndex] == -1)
                Queue.HeapPush(FFloodIndex(Current.X - 1, Current.Y, NeighboorIndex, GlobalHeightField[Current.X - 1][Current.Y][NeighboorIndex]));

            if ((NeighboorIndex = GlobalHeightField[Current.X][Current.Y + 1].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[Current.X][Current.Y + 1][NeighboorIndex] == -1)
                Queue.HeapPush(FFloodIndex(Current.X, Current.Y + 1, NeighboorIndex, GlobalHeightField[Current.X][Current.Y + 1][NeighboorIndex]));

            if ((NeighboorIndex = GlobalHeightField[Current.X][Current.Y - 1].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[Current.X][Current.Y - 1][NeighboorIndex] == -1)
                Queue.HeapPush(FFloodIndex(Current.X, Current.Y - 1, NeighboorIndex, GlobalHeightField[Current.X][Current.Y - 1][NeighboorIndex]));
        }

        for (const FFloodIndex& CancelIndex : CancelQueue)
        {
            if (ShouldCancelBuild())
                return;

            int32 NeighboorIndex;
            const auto Predicate = [StepHeight = GenerationOptions.AgentStepHeight, CurrentHeight = GlobalHeightField[CancelIndex.X][CancelIndex.Y][CancelIndex.Z]](float InHeight)
                { return FMath::Abs(InHeight - CurrentHeight) < StepHeight; };

            if ((NeighboorIndex = GlobalHeightField[CancelIndex.X + 1][CancelIndex.Y].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[CancelIndex.X + 1][CancelIndex.Y][NeighboorIndex] == Surfaces.Num() - 1)
                SurfaceId[CancelIndex.X + 1][CancelIndex.Y][NeighboorIndex] = -1;

            if ((NeighboorIndex = GlobalHeightField[CancelIndex.X - 1][CancelIndex.Y].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[CancelIndex.X - 1][CancelIndex.Y][NeighboorIndex] == Surfaces.Num() - 1)
                SurfaceId[CancelIndex.X - 1][CancelIndex.Y][NeighboorIndex] = -1;

            if ((NeighboorIndex = GlobalHeightField[CancelIndex.X][CancelIndex.Y + 1].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[CancelIndex.X][CancelIndex.Y + 1][NeighboorIndex] == Surfaces.Num() - 1)
                SurfaceId[CancelIndex.X][CancelIndex.Y + 1][NeighboorIndex] = -1;

            if ((NeighboorIndex = GlobalHeightField[CancelIndex.X][CancelIndex.Y - 1].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[CancelIndex.X][CancelIndex.Y - 1][NeighboorIndex] == Surfaces.Num() - 1)
                SurfaceId[CancelIndex.X][CancelIndex.Y - 1][NeighboorIndex] = -1;
        }
    }

    // ------------------------------------------------------------------------------------
    // COLLECT SURFACES' BORDERS AND CELLS
    // ------------------------------------------------------------------------------------

    TArray<TArray<FBorderCell>> BorderCellsTemp;
    TArray<TArray<TArray<FBorderCell>>> BorderCells;

    BorderCellsTemp.Init(TArray<FBorderCell>(), YSize);
    BorderCells.Init(BorderCellsTemp, XSize);

    for (int32 i = 1; i < XSize - 1; ++i)
    {
        for (int32 j = 1; j < YSize - 1; ++j)
        {
            for (int32 k = 0, ZSize = GlobalHeightField[i][j].Num(); k < ZSize; ++k)
            {
                if (ShouldCancelBuild())
                    return;

                int32 NeighboorIndex;
                const auto Predicate = [StepHeight = GenerationOptions.AgentStepHeight, CurrentHeight = GlobalHeightField[i][j][k]](float InHeight)
                    { return FMath::Abs(InHeight - CurrentHeight) < StepHeight; };

                const bool Left  = (NeighboorIndex = GlobalHeightField[i - 1][j].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[i - 1][j][NeighboorIndex] == SurfaceId[i][j][k];
                const bool Right = (NeighboorIndex = GlobalHeightField[i + 1][j].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[i + 1][j][NeighboorIndex] == SurfaceId[i][j][k];
                const bool Up    = (NeighboorIndex = GlobalHeightField[i][j - 1].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[i][j - 1][NeighboorIndex] == SurfaceId[i][j][k];
                const bool Down  = (NeighboorIndex = GlobalHeightField[i][j + 1].IndexOfByPredicate(Predicate)) != INDEX_NONE && SurfaceId[i][j + 1][NeighboorIndex] == SurfaceId[i][j][k];

                if (!Left || !Right || !Up || !Down)
                    BorderCells[i][j].Emplace(SurfaceId[i][j][k], GlobalHeightField[i][j][k], Left, Right, Up, Down);
                else
                    Surfaces[SurfaceId[i][j][k]].Emplace(FMath::RoundToFloat(GlobalHeightFieldMin.X + (i + .5f) * GenerationOptions.CellSize),
                        FMath::RoundToFloat(GlobalHeightFieldMin.Y + (j + .5f) * GenerationOptions.CellSize), FMath::RoundToFloat(GlobalHeightField[i][j][k]));
            }
        }
    }

    // ------------------------------------------------------------------------------------
    // BUILD SURFACES' BORDERS
    // ------------------------------------------------------------------------------------

    SurfaceBorders.Init(TArray<TArray<FVector>>(), Surfaces.Num());

    for (int32 i = 1; i < XSize - 1; ++i)
    {
        for (int32 j = 1; j < YSize - 1; ++j)
        {
            for (int32 k = 0, ZSize = BorderCells[i][j].Num(); k < ZSize; ++k)
            {
                if (ShouldCancelBuild())
                    return;

                if (BorderCells[i][j][k].SurfaceId == -1)
                    continue;

                const int32 Id = BorderCells[i][j][k].SurfaceId;
                int32 CurrentX = i, CurrentY = j, CurrentZ = k;

                SurfaceBorders[Id].Emplace();

                while (BorderCells[CurrentX][CurrentY][CurrentZ].SurfaceId != -1)
                {
                    const float BorderX = !BorderCells[CurrentX][CurrentY][CurrentZ].Left ? .4f : (!BorderCells[CurrentX][CurrentY][CurrentZ].Right ? .6f : .5f);
                    const float BorderY = !BorderCells[CurrentX][CurrentY][CurrentZ].Up   ? .4f : (!BorderCells[CurrentX][CurrentY][CurrentZ].Down  ? .6f : .5f);

                    SurfaceBorders[Id].Last().Emplace(FMath::RoundToFloat(GlobalHeightFieldMin.X + (CurrentX + BorderX) * GenerationOptions.CellSize),
                        FMath::RoundToFloat(GlobalHeightFieldMin.Y + (CurrentY + BorderY) * GenerationOptions.CellSize), FMath::RoundToFloat(BorderCells[CurrentX][CurrentY][CurrentZ].Height));
                    BorderCells[CurrentX][CurrentY][CurrentZ].SurfaceId = -1;

                    const auto Predicate = [Id, StepHeight = GenerationOptions.AgentStepHeight, CurrentHeight = BorderCells[CurrentX][CurrentY][CurrentZ].Height](const FBorderCell &InCell) -> bool
                        { return InCell.SurfaceId == Id && FMath::Abs(InCell.Height - CurrentHeight) < 2 * StepHeight; };

                    const int32 Left  = BorderCells[CurrentX - 1][CurrentY].IndexOfByPredicate(Predicate);
                    const int32 Right = BorderCells[CurrentX + 1][CurrentY].IndexOfByPredicate(Predicate);
                    const int32 Up    = BorderCells[CurrentX][CurrentY - 1].IndexOfByPredicate(Predicate);
                    const int32 Down  = BorderCells[CurrentX][CurrentY + 1].IndexOfByPredicate(Predicate);

                    const int32 LeftUp    = BorderCells[CurrentX - 1][CurrentY - 1].IndexOfByPredicate(Predicate);
                    const int32 LeftDown  = BorderCells[CurrentX - 1][CurrentY + 1].IndexOfByPredicate(Predicate);
                    const int32 RightUp   = BorderCells[CurrentX + 1][CurrentY - 1].IndexOfByPredicate(Predicate);
                    const int32 RightDown = BorderCells[CurrentX + 1][CurrentY + 1].IndexOfByPredicate(Predicate);

                    if (LeftUp    != INDEX_NONE && !BorderCells[CurrentX][CurrentY][CurrentZ].Left)  { --CurrentX; --CurrentY; CurrentZ = LeftUp;    continue; }
                    if (LeftDown  != INDEX_NONE && !BorderCells[CurrentX][CurrentY][CurrentZ].Down)  { --CurrentX; ++CurrentY; CurrentZ = LeftDown;  continue; }
                    if (RightUp   != INDEX_NONE && !BorderCells[CurrentX][CurrentY][CurrentZ].Up)    { ++CurrentX; --CurrentY; CurrentZ = RightUp;   continue; }
                    if (RightDown != INDEX_NONE && !BorderCells[CurrentX][CurrentY][CurrentZ].Right) { ++CurrentX; ++CurrentY; CurrentZ = RightDown; continue; }

                    if (Left  != INDEX_NONE && !BorderCells[CurrentX][CurrentY][CurrentZ].Down)  { --CurrentX; CurrentZ = Left;  continue; }
                    if (Up    != INDEX_NONE && !BorderCells[CurrentX][CurrentY][CurrentZ].Left)  { --CurrentY; CurrentZ = Up;    continue; }
                    if (Right != INDEX_NONE && !BorderCells[CurrentX][CurrentY][CurrentZ].Up)    { ++CurrentX; CurrentZ = Right; continue; }
                    if (Down  != INDEX_NONE && !BorderCells[CurrentX][CurrentY][CurrentZ].Right) { ++CurrentY; CurrentZ = Down;  continue; }
                }
            }
        }
    }

    // ------------------------------------------------------------------------------------
    // INTERPOLATE SURFACES' BORDERS
    // ------------------------------------------------------------------------------------

    for (int32 i = 0, N = SurfaceBorders.Num(); i < N; ++i)
    {
        for (int32 j = 0, M = SurfaceBorders[i].Num(); j < M; ++j)
        {
            if (ShouldCancelBuild())
                return;

            for (int32 Iteration = 0, MaxIterations = 5; Iteration <= MaxIterations; ++Iteration)
            {
                const float MaxBorderDeviation = GenerationOptions.MaxBorderDeviation * Iteration / MaxIterations;

                TArray<int32> RelaxedBorder; RelaxedBorder.Add(0);
                for (int32 k = 1, sz = SurfaceBorders[i][j].Num(); k < sz; ++k)
                {
                    const FVector A = SurfaceBorders[i][j][RelaxedBorder.Last()];
                    const FVector B = SurfaceBorders[i][j][k + 1 < sz ? k + 1 : 0];

                    float MaxDeviation = 0;
                    for (int32 t = RelaxedBorder.Last() + 1; t <= k; ++t)
                        MaxDeviation = FMath::Max(MaxDeviation, FMath::PointDistToSegmentSquared(SurfaceBorders[i][j][t], A, B));

                    bool bSelfIntersection = false;
                    for (int32 t = 0; t < sz; ++t)
                    {
                        if ((t >= RelaxedBorder.Last() && t <= k + 1) || (t + 1 >= RelaxedBorder.Last() && t <= k))
                            continue;

                        FVector TempIntersection;
                        if (FMath::SegmentIntersection2D(A, B, SurfaceBorders[i][j][t], SurfaceBorders[i][j][t + 1 < sz ? t + 1 : 0], TempIntersection))
                        {
                            bSelfIntersection = true;
                            break;
                        }
                    }

                    if (MaxDeviation > MaxBorderDeviation * MaxBorderDeviation || bSelfIntersection)
                        RelaxedBorder.Add(k);
                }

                if (RelaxedBorder.Num() < 3)
                    break;

                for (int32 k = 0, sz = RelaxedBorder.Num(); k < sz; ++k)
                    SurfaceBorders[i][j][k] = SurfaceBorders[i][j][RelaxedBorder[k]];
                SurfaceBorders[i][j].SetNum(RelaxedBorder.Num());
            }
        }
    }
}
