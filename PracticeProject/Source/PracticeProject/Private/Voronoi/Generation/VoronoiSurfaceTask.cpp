// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiSurfaceTask.h"

/** Types */

struct FBorderCell
{
    int32 OriginalIndex;
    bool Left, Right, Up, Down;

    FORCEINLINE FBorderCell(int32 InOriginalIndex, bool InLeft, bool InRight, bool InUp, bool InDown)
        : OriginalIndex(InOriginalIndex), Left(InLeft), Right(InRight), Up(InUp), Down(InDown) {}
};

/** FVoronoiSurfaceTask */

void FVoronoiSurfaceTask::DoWork()
{
    const int32 XSize = GlobalHeightField.Num();
    const int32 YSize = GlobalHeightField[0].Num();

    const float StepHeight = AgentProperties.AgentStepHeight != -1 ? AgentProperties.AgentStepHeight : 30.f;
    const float AgentRadius = AgentProperties.AgentRadius;

    // ------------------------------------------------------------------------------------
    // ARRAY INITIALIZATORS
    // ------------------------------------------------------------------------------------

    TArray<TArray<float>> FloatProfileTemp;
    FloatProfileTemp.Init(TArray<float>(), YSize);

    TArray<TArray<int32>> Int32ProfileTemp;
    Int32ProfileTemp.Init(TArray<int32>(), YSize);

    TArray<bool> BoolRowTemp;
    BoolRowTemp.Init(false, YSize);

    TArray<TArray<FBorderCell>> BorderCellProfileTemp;
    BorderCellProfileTemp.Init(TArray<FBorderCell>(), YSize);

    // ------------------------------------------------------------------------------------
    // CORRECT BY AGENT RADIUS AND BORDER INDENT
    // ------------------------------------------------------------------------------------

    for (int32 Pass = 0, Passes = FMath::CeilToInt((AgentRadius + GenerationOptions.BorderIndent) / GenerationOptions.CellSize); Pass < Passes + 2; ++Pass)
    {
        if (ShouldCancelBuild())
            return;

        TArray<TArray<TArray<float>>> NewGlobalHeightField;
        NewGlobalHeightField.Init(FloatProfileTemp, XSize);

        for (int32 i = 1; i < XSize - 1; ++i)
        {
            for (int32 j = 1; j < YSize - 1; ++j)
            {
                for (int32 k = 0, sz = GlobalHeightField[i][j].Num(); k < sz; ++k)
                {
                    const auto Predicate = [StepHeight, CurrentHeight = GlobalHeightField[i][j][k]](float Height) { return FMath::Abs(Height - CurrentHeight) < StepHeight; };

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

    TArray<TArray<TArray<int32>>> SurfaceId;
    SurfaceId.Init(Int32ProfileTemp, XSize);

    for (int32 i = 1; i < XSize - 1; ++i)
        for (int32 j = 1; j < YSize - 1; ++j)
            SurfaceId[i][j].Init(-1, GlobalHeightField[i][j].Num());

    for (int32 i = 1; i < XSize - 1; ++i)
    {
        for (int32 j = 1; j < YSize - 1; ++j)
        {
            for (int32 k = 0, sz = GlobalHeightField[i][j].Num(); k < sz; ++k)
            {
                if (ShouldCancelBuild())
                    return;

                if (SurfaceId[i][j][k] != -1)
                    continue;

                Surfaces.Emplace();

                TArray<TArray<bool>> bHeightVisited;
                bHeightVisited.Init(BoolRowTemp, XSize);

                TQueue<FIntVector> Queue;
                FIntVector Current(i, j, k);

                Queue.Enqueue(Current);
                while (Queue.Dequeue(Current))
                {
                    if (ShouldCancelBuild())
                        return;

                    if (bHeightVisited[Current.X][Current.Y])
                        continue;

                    bHeightVisited[Current.X][Current.Y] = true;
                    SurfaceId[Current.X][Current.Y][Current.Z] = Surfaces.Num() - 1;

                    const auto Predicate = [StepHeight, CurrentHeight = GlobalHeightField[Current.X][Current.Y][Current.Z]](float Height) { return FMath::Abs(Height - CurrentHeight) < StepHeight; };
                    int32 NeighboorIndex;

                    if ((NeighboorIndex = GlobalHeightField[Current.X + 1][Current.Y].IndexOfByPredicate(Predicate)) != INDEX_NONE)
                        Queue.Enqueue(FIntVector(Current.X + 1, Current.Y, NeighboorIndex));

                    if ((NeighboorIndex = GlobalHeightField[Current.X - 1][Current.Y].IndexOfByPredicate(Predicate)) != INDEX_NONE)
                        Queue.Enqueue(FIntVector(Current.X - 1, Current.Y, NeighboorIndex));

                    if ((NeighboorIndex = GlobalHeightField[Current.X][Current.Y + 1].IndexOfByPredicate(Predicate)) != INDEX_NONE)
                        Queue.Enqueue(FIntVector(Current.X, Current.Y + 1, NeighboorIndex));

                    if ((NeighboorIndex = GlobalHeightField[Current.X][Current.Y - 1].IndexOfByPredicate(Predicate)) != INDEX_NONE)
                        Queue.Enqueue(FIntVector(Current.X, Current.Y - 1, NeighboorIndex));
                }
            }
        }
    }

    // ------------------------------------------------------------------------------------
    // RECOLLECT SURFACES' BORDERS
    // ------------------------------------------------------------------------------------

    TArray<TArray<TArray<FBorderCell>>> BorderCells;
    BorderCells.Init(BorderCellProfileTemp, XSize);

    for (int32 i = 1; i < XSize - 1; ++i)
    {
        for (int32 j = 1; j < YSize - 1; ++j)
        {
            for (int32 k = 0, sz = SurfaceId[i][j].Num(); k < sz; ++k)
            {
                if (ShouldCancelBuild())
                    return;

                const auto Predicate = [CurrentId = SurfaceId[i][j][k]](int32 Id) { return CurrentId == Id; };

                const bool Left  = SurfaceId[i + 1][j].ContainsByPredicate(Predicate);
                const bool Right = SurfaceId[i - 1][j].ContainsByPredicate(Predicate);
                const bool Up    = SurfaceId[i][j - 1].ContainsByPredicate(Predicate);
                const bool Down  = SurfaceId[i][j + 1].ContainsByPredicate(Predicate);

                if (!Left || !Right || !Up || !Down)
                    BorderCells[i][j].Emplace(k, Right, Left, Up, Down);
                else
                    Surfaces[SurfaceId[i][j][k]].Emplace(FMath::RoundToFloat(GlobalHeightFieldMin.X + (i + .5f) * GenerationOptions.CellSize),
                        FMath::RoundToFloat(GlobalHeightFieldMin.Y + (j + .5f) * GenerationOptions.CellSize), FMath::RoundToFloat(GlobalHeightField[i][j][k]));
            }
        }
    }

    SurfaceBorders.Init(TArray<TArray<FVector>>(), Surfaces.Num());

    for (int32 i = 1; i < XSize - 1; ++i)
    {
        for (int32 j = 1; j < YSize - 1; ++j)
        {
            for (int32 k = 0, sz = BorderCells[i][j].Num(); k < sz; ++k)
            {
                if (ShouldCancelBuild())
                    return;

                if (BorderCells[i][j][k].OriginalIndex == -1)
                    continue;

                const int32 Id = SurfaceId[i][j][BorderCells[i][j][k].OriginalIndex];
                int32 CurrentX = i, CurrentY = j, CurrentZ = k;

                SurfaceBorders[Id].Emplace();

                while (BorderCells[CurrentX][CurrentY][CurrentZ].OriginalIndex != -1)
                {
                    const float BorderX = !BorderCells[CurrentX][CurrentY][CurrentZ].Left ? 0.f : (!BorderCells[CurrentX][CurrentY][CurrentZ].Right ? 1.f : .5f);
                    const float BorderY = !BorderCells[CurrentX][CurrentY][CurrentZ].Up   ? 0.f : (!BorderCells[CurrentX][CurrentY][CurrentZ].Down  ? 1.f : .5f);

                    SurfaceBorders[Id].Last().Emplace(FMath::RoundToFloat(GlobalHeightFieldMin.X + (CurrentX + BorderX) * GenerationOptions.CellSize), FMath::RoundToFloat(GlobalHeightFieldMin.Y + (CurrentY + BorderY) * GenerationOptions.CellSize),
                        FMath::RoundToFloat(GlobalHeightField[CurrentX][CurrentY][BorderCells[CurrentX][CurrentY][CurrentZ].OriginalIndex]));
                    BorderCells[CurrentX][CurrentY][CurrentZ].OriginalIndex = -1;

                    const int32 Left  = BorderCells[CurrentX - 1][CurrentY].IndexOfByPredicate([Id, &CellId = SurfaceId[CurrentX - 1][CurrentY]](const FBorderCell &Cell) { return Cell.OriginalIndex != -1 && CellId[Cell.OriginalIndex] == Id; });
                    const int32 Right = BorderCells[CurrentX + 1][CurrentY].IndexOfByPredicate([Id, &CellId = SurfaceId[CurrentX + 1][CurrentY]](const FBorderCell &Cell) { return Cell.OriginalIndex != -1 && CellId[Cell.OriginalIndex] == Id; });
                    const int32 Up    = BorderCells[CurrentX][CurrentY - 1].IndexOfByPredicate([Id, &CellId = SurfaceId[CurrentX][CurrentY - 1]](const FBorderCell &Cell) { return Cell.OriginalIndex != -1 && CellId[Cell.OriginalIndex] == Id; });
                    const int32 Down  = BorderCells[CurrentX][CurrentY + 1].IndexOfByPredicate([Id, &CellId = SurfaceId[CurrentX][CurrentY + 1]](const FBorderCell &Cell) { return Cell.OriginalIndex != -1 && CellId[Cell.OriginalIndex] == Id; });

                    const int32 LeftUp    = BorderCells[CurrentX - 1][CurrentY - 1].IndexOfByPredicate([Id, &CellId = SurfaceId[CurrentX - 1][CurrentY - 1]](const FBorderCell &Cell) { return Cell.OriginalIndex != -1 && CellId[Cell.OriginalIndex] == Id; });
                    const int32 LeftDown  = BorderCells[CurrentX - 1][CurrentY + 1].IndexOfByPredicate([Id, &CellId = SurfaceId[CurrentX - 1][CurrentY + 1]](const FBorderCell &Cell) { return Cell.OriginalIndex != -1 && CellId[Cell.OriginalIndex] == Id; });
                    const int32 RightUp   = BorderCells[CurrentX + 1][CurrentY - 1].IndexOfByPredicate([Id, &CellId = SurfaceId[CurrentX + 1][CurrentY - 1]](const FBorderCell &Cell) { return Cell.OriginalIndex != -1 && CellId[Cell.OriginalIndex] == Id; });
                    const int32 RightDown = BorderCells[CurrentX + 1][CurrentY + 1].IndexOfByPredicate([Id, &CellId = SurfaceId[CurrentX + 1][CurrentY + 1]](const FBorderCell &Cell) { return Cell.OriginalIndex != -1 && CellId[Cell.OriginalIndex] == Id; });

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

            TArray<int32> RelaxedBorder;
            for (float MaxBorderDeviation = GenerationOptions.MaxBorderDeviation; RelaxedBorder.Num() < 3; MaxBorderDeviation /= 2)
            {
                RelaxedBorder.Reset();
                RelaxedBorder.Add(0);

                for (int32 k = 1, sz = SurfaceBorders[i][j].Num(); k < sz; ++k)
                {
                    const FVector A = SurfaceBorders[i][j][RelaxedBorder.Last()];
                    const FVector B = SurfaceBorders[i][j][k + 1 < sz ? k + 1 : 0];

                    float MaxDeviation = 0;
                    for (int32 p = RelaxedBorder.Last() + 1; p <= k; ++p)
                        MaxDeviation = FMath::Max(MaxDeviation, FMath::PointDistToSegmentSquared(SurfaceBorders[i][j][p], A, B));

                    if (MaxDeviation > MaxBorderDeviation * MaxBorderDeviation)
                        RelaxedBorder.Add(k);
                }
            }
            
            for (int32 k = 0, sz = RelaxedBorder.Num(); k < sz; ++k)
                SurfaceBorders[i][j][k] = SurfaceBorders[i][j][RelaxedBorder[k]];

            SurfaceBorders[i][j].SetNum(RelaxedBorder.Num());
        }
    }
}
