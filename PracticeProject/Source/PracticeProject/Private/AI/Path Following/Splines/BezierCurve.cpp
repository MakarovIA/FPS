// By Polyakov Pavel

#include "PracticeProject.h"
#include "BezierCurve.h"

#include "MathExtended.h"

/** FBezierCurve */

FVector FBezierCurve::GetValue(int32 InPiece, double InParameter) const
{
    FVector Result(0, 0, 0);
    for (int32 i = 0, N = ControlPoints[InPiece].Num(); i < N; ++i)
        Result += FMathExtended::Combination(N - 1, i) * ControlPoints[InPiece][i] * FMath::Pow(InParameter, i) * FMath::Pow(1 - InParameter, N - 1 - i);

    return Result;
}

int32 FBezierCurve::GetPieceNumber() const
{ 
    return ControlPoints.Num();
}

/** FBezierCurveBuilder */

TArray<FVector> FBezierCurveBuilder::RetrivePathPoints(const TArray<FNavPathPoint>& NavPathPoints, const FBezierCurveBuildParams &BuildParams, const FNavAgentProperties& AgentProperties, const UWorld* World)
{
    TArray<FVector> PathPoints;
    PathPoints.Reserve(NavPathPoints.Num() * 2);

    for (const FNavPathPoint& PathPoint : NavPathPoints)
    {
        const FVector& NextPoint = PathPoint.Location;

        // Don't paste additional points if NextPoint can't be seen
        if (PathPoints.Num() > 0 && BuildParams.AdditionalPathPointsNumber > 0 &&
            !SmartCollisionCheck(PathPoints.Last(), NextPoint, AgentProperties.AgentRadius, World))
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

    return PathPoints;
}

TArray<int32> FBezierCurveBuilder::CalculateSubsequencesArray(const TArray<FVector>& PathPoints, const FNavAgentProperties& AgentProperties, const UWorld* World)
{
    struct FDynamics
    {
        float Value;
        int32 End, Parts;
    };

    const int32 N = PathPoints.Num();

    TArray<bool> Temp1;
    TArray<FDynamics> Temp2;

    Temp1.Init(false, N);
    Temp2.Init(FDynamics(), N);

    TArray<TArray<bool>> CollideArray;
    TArray<TArray<FDynamics>> Dynamics;

    CollideArray.Init(Temp1, N);
    Dynamics.Init(Temp2, N);

    for (int32 k = 1; k < N; ++k)
    {
        for (int32 i = 0; i < N - k; ++i)
        {
            const int32 j = i + k;
            if (k != 1 && (CollideArray[i][j - 1] || CollideArray[i + 1][j] || SmartCollisionCheck(PathPoints[i], PathPoints[j], AgentProperties.AgentRadius, World) ||
                !SimpleCollisionCheck((PathPoints[i] + PathPoints[j]) / 2, (PathPoints[i] + PathPoints[j]) / 2 + FVector(0, 0, -100), World)))
                CollideArray[i][j] = true;

            if (!CollideArray[i][j])
            {
                Dynamics[i][j].Value = (PathPoints[j] - PathPoints[i]).Size2D();
                Dynamics[i][j].Parts = 1;
                Dynamics[i][j].End = j;
            }
            else
                Dynamics[i][j].Value = FLT_MAX / 2;

            for (int32 l = i + 1; l < j; ++l)
            {
                const float ValuesSum = Dynamics[i][l].Value + Dynamics[l][j].Value;
                const int32 PartsSum = Dynamics[i][l].Parts + Dynamics[l][j].Parts;

                if (ValuesSum + 20 < Dynamics[i][j].Value || (FMath::IsNearlyEqual(ValuesSum, Dynamics[i][j].Value, 20) && PartsSum < Dynamics[i][j].Parts))
                {
                    Dynamics[i][j].Value = ValuesSum;
                    Dynamics[i][j].Parts = PartsSum;
                    Dynamics[i][j].End = l;
                }
            }
        }
    }

    static void (*Backtracking)(const TArray<TArray<FDynamics>> &Dynamics, int32 i, int32 j, TArray<int32> &SubsequencesArray) =
        [](const TArray<TArray<FDynamics>> &Dynamics, int32 i, int32 j, TArray<int32> &SubsequencesArray)
    {
        if (j == Dynamics[i][j].End)
            SubsequencesArray.Add(j);
        else
        {
            Backtracking(Dynamics, i, Dynamics[i][j].End, SubsequencesArray);
            Backtracking(Dynamics, Dynamics[i][j].End, j, SubsequencesArray);
        }
    };

    TArray<int32> SubsequencesArray;
    Backtracking(Dynamics, 0, N - 1, SubsequencesArray);

    return SubsequencesArray;
}

TUniquePtr<FBezierCurve> FBezierCurveBuilder::BuildBezierCurve(const TArray<FNavPathPoint>& NavPathPoints, const FVector& CurrentMoveDirection, const FBezierCurveBuildParams &BuildParams, const FNavAgentProperties& AgentProperties, const UWorld* World)
{
    TArray<FVector> PathPoints = RetrivePathPoints(NavPathPoints, BuildParams, AgentProperties, World);
    TArray<int32> SubsequencesArray = CalculateSubsequencesArray(PathPoints, AgentProperties, World);

    TArray<TArray<FVector>> ControlPoints;
    ControlPoints.Init(TArray<FVector>(), SubsequencesArray.Num());

    const int32 DegreeLimit = 15;
    for (int32 i = 0, j = 0, N = SubsequencesArray.Num(); i < N; ++i)
    {
        ControlPoints[i].Add(PathPoints[j++]);

        // ------------------------------------------------------------------------------------------------------------------------
        // Derivative repair
        // ------------------------------------------------------------------------------------------------------------------------

        FVector Derivative = (i > 0 ? (ControlPoints[i - 1].Last(0) - ControlPoints[i - 1].Last(1)) : CurrentMoveDirection * 50);
        if (Derivative.Size() > 100) Derivative = Derivative.GetSafeNormal() * 100;

        while (SmartCollisionCheck(ControlPoints[i][0], ControlPoints[i][0] + Derivative, AgentProperties.AgentRadius, World))
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

    return MakeUnique<FBezierCurve>(ControlPoints);
}
