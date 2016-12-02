// By Polyakov Pavel

#include "PracticeProject.h"
#include "BezierCurve.h"

/** FBezierCurve */

int32 FBezierCurve::Combination(int32 n, int32 k)
{
    static const int32 Limit = FBezierCurveBuildParams::CurveDergeeTrueLimit;
    static int32 Cash[Limit + 1][Limit + 1] = { { 0 } };

    if (Cash[n][k] != 0) return Cash[n][k];
    if (n < k)           return 0;
    if (k == 0)          return 1;

    return Cash[n][k] = Combination(n - 1, k - 1) + Combination(n - 1, k);
}

FVector FBezierCurve::GetValue(int32 InPiece, double InParameter) const
{
    FVector Result(0, 0, 0);

    int32 N = ControlPoints[InPiece].Num();
    for (int32 i = 0; i < N; ++i)
        Result += Combination(N - 1, i) * ControlPoints[InPiece][i] * FMath::Pow(InParameter, i) * FMath::Pow(1 - InParameter, N - 1 - i);

    return Result;
}

int32 FBezierCurve::GetPieceNumber() const
{ 
    return ControlPoints.Num();
}

/** FBezierCurveBuilder */

void FBezierCurveBuilder::RetrivePathPoints(const TArray<FNavPathPoint>& NavPathPoints, const FBezierCurveBuildParams &BuildParams,
    const FNavAgentProperties& AgentProperties, const UWorld* World, TArray<FVector> &PathPoints)
{
    PathPoints.Empty();
    PathPoints.Reserve(NavPathPoints.Num() * 2);

    for (const auto& i : NavPathPoints)
    {
        const FVector& NextPoint = i.Location;

        /** Don't paste additional points if NextPoint can't be seen */
        if (PathPoints.Num() > 0 && BuildParams.AdditionalPathPointsNumber > 0 &&
            !SmartCollisionCheck(PathPoints.Last(), NextPoint, AgentProperties, World))
        {
            const FVector Difference = NextPoint - PathPoints.Last();
            const float Distance = Difference.Size2D();

            const int32 TrueAdditionalPathPointsNumber = FMath::Min(BuildParams.AdditionalPathPointsNumber,
                FMath::FloorToInt(Distance / BuildParams.AdditionalPathPointsOffset / 2));

            const FVector Step = Difference / Distance * BuildParams.AdditionalPathPointsOffset;

            for (int32 j = 1; j <= TrueAdditionalPathPointsNumber; ++j)
                PathPoints.Add(PathPoints.Last() + Step * j);

            for (int32 j = TrueAdditionalPathPointsNumber; j >= 1; --j)
                PathPoints.Add(NextPoint - Step * j);
        }

        PathPoints.Add(NextPoint);
    }
}

void FBezierCurveBuilder::CalculateSubsequencesArray(const TArray<FVector>& PathPoints, const FNavAgentProperties& AgentProperties, const UWorld* World, TArray<int32> &SubsequencesArray)
{
    // Get number of path points
    int32 N = PathPoints.Num();

    // Declare arrays
    TArray<TArray<bool>> CollideArray;
    TArray<TArray<ETuple>> E;

    // Initialize arrays
    TArray<bool>   Temp1; Temp1.Init(false, N);    CollideArray.Init(Temp1, N);
    TArray<ETuple> Temp2; Temp2.Init(ETuple(), N); E.Init(Temp2, N);

    // Start calculation
    for (int32 k = 1; k < N; ++k)
    {
        for (int32 i = 0; i < N - k; ++i)
        {
            // Get second index
            int32 j = i + k;

            // Fill Collide Array
            if (k != 1)
                if (CollideArray[i][j - 1] || CollideArray[i + 1][j] ||
                    SmartCollisionCheck(PathPoints[i], PathPoints[j], AgentProperties, World) ||
                    !SimpleCollisionCheck((PathPoints[i] + PathPoints[j]) / 2, (PathPoints[i] + PathPoints[j]) / 2 + FVector(0, 0, -100), World))
                    CollideArray[i][j] = true;

            // Find the best segmentation
            if (!CollideArray[i][j])
            {
                E[i][j].Value = (PathPoints[j] - PathPoints[i]).Size2D();
                E[i][j].Parts = 1;
                E[i][j].End = j;
            } else
                E[i][j].Value = FLT_MAX / 2;

            for (int32 l = i + 1; l < j; ++l)
            {
                float ValuesSum = E[i][l].Value + E[l][j].Value;
                int32 PartsSum = E[i][l].Parts + E[l][j].Parts;

                if (ValuesSum + 20 < E[i][j].Value ||
                    (FMath::IsNearlyEqual(ValuesSum, E[i][j].Value, 20) && PartsSum < E[i][j].Parts))
                {
                    E[i][j].Value = ValuesSum;
                    E[i][j].Parts = PartsSum;
                    E[i][j].End = l;
                }
            }
        }
    }

    // Empty result array just in case there is something already
    SubsequencesArray.Empty();

    // Backtrack to find solution
    Backtracking(E, 0, N - 1, SubsequencesArray);
}

void FBezierCurveBuilder::Backtracking(const TArray<TArray<ETuple>> &E, int32 i, int32 j, TArray<int32> &SubsequencesArray)
{
    if (j == E[i][j].End)
        SubsequencesArray.Add(j);
    else
    {
        Backtracking(E, i, E[i][j].End, SubsequencesArray);
        Backtracking(E, E[i][j].End, j, SubsequencesArray);
    }
}

TUniquePtr<FBezierCurve> FBezierCurveBuilder::BuildBezierCurve(const TArray<FNavPathPoint>& NavPathPoints, const FVector& CurrentMoveDirection,
    const FBezierCurveBuildParams &BuildParams, const FNavAgentProperties& AgentProperties, const UWorld* World)
{
    // Declare arrays
    TArray<FVector> PathPoints;
    TArray<int32> SubsequencesArray;
    TArray<TArray<FVector>> ControlPoints;

    // Initialize arrays
    RetrivePathPoints(NavPathPoints, BuildParams, AgentProperties, World, PathPoints);
    CalculateSubsequencesArray(PathPoints, AgentProperties, World, SubsequencesArray);
    ControlPoints.Init(TArray<FVector>(), SubsequencesArray.Num());

    // Calculate limit
    int32 DegreeLimit = FMath::Min<int32>(BuildParams.CurveDergeeLimit, BuildParams.CurveDergeeTrueLimit);

    // Start building
    for (int32 i = 0, j = 0; i < SubsequencesArray.Num(); ++i)
    {
        ControlPoints[i].Add(PathPoints[j++]);

        // ------------------------------------------------------------------------------------------------------------------------
        // Derivative repair
        // ------------------------------------------------------------------------------------------------------------------------

        FVector Derivative = (i > 0 ? (ControlPoints[i - 1].Last(0) - ControlPoints[i - 1].Last(1)) : CurrentMoveDirection * 50);
        if (Derivative.Size() > 100)
            Derivative = Derivative.GetSafeNormal() * 100;

        while (SmartCollisionCheck(ControlPoints[i][0], ControlPoints[i][0] + Derivative, AgentProperties, World))
            Derivative /= 2;

        ControlPoints[i].Add(ControlPoints[i][0] + Derivative);

        // ------------------------------------------------------------------------------------------------------------------------
        // Minimal distance filter
        // ------------------------------------------------------------------------------------------------------------------------

        float MinDistSquared = BuildParams.MinDistanceBetweenControlPoints * BuildParams.MinDistanceBetweenControlPoints;

        while (j < SubsequencesArray[i])
        {
            const FVector& NextPoint = PathPoints[j++];
            if (FVector::DistSquaredXY(ControlPoints[i].Last(), NextPoint) >= MinDistSquared)
                ControlPoints[i].Add(NextPoint);
        }

        if (ControlPoints[i].Num() + 2 > DegreeLimit)
        {
            MinDistSquared = FVector::DistSquaredXY(ControlPoints[i].Last(), ControlPoints[i][0]) / (DegreeLimit - 2) / (DegreeLimit - 2);
            int32 last = 1;

            for (int32 t = 2; t < ControlPoints[i].Num() - 1; ++t)
                if (FVector::DistSquaredXY(ControlPoints[i][last], ControlPoints[i][t]) > MinDistSquared)
                    Swap(ControlPoints[i][++last], ControlPoints[i][t]);

            ControlPoints[i].SetNum(last + 1);
        }

        ControlPoints[i].Add(PathPoints[j]);
    }

    return TUniquePtr<FBezierCurve>(new FBezierCurve(ControlPoints));
}
