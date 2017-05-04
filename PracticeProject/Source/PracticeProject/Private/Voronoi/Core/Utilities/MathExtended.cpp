// By Polyakov Pavel

#include "PracticeProject.h"
#include "MathExtended.h"

/** FMathExtended */

void FMathExtended::ProjectOnAxis(const TArray<FVector> &Points, const FVector &Axis, float &Min, float &Max)
{
    Min = FLT_MAX, Max = -FLT_MAX;
    for (const FVector &Point : Points)
    {
        const float Value = FVector::DotProduct(Axis, Point);
        Min = FMath::Min(Min, Value), Max = FMath::Max(Max, Value);
    }
}

float FMathExtended::GetTriangleArea(const FVector &A, const FVector &B, const FVector &C)
{
    return FVector::CrossProduct(B - A, C - A).Size() / 2;
}

FVector FMathExtended::GetRandomPointInPolygon(const TArray<FVector>& Points)
{
    const int32 N = Points.Num();
    const float RandomNumber = FMath::FRand();

    switch (N)
    {
        case 0: return FVector(0);
        case 1: return Points[0];
        case 2: return Points[0] * RandomNumber + Points[1] * (1 - RandomNumber);

        default:
            TArray<float> Areas;
            Areas.Reserve(N - 2);

            for (int32 Index = 2; Index < N; ++Index)
                Areas.Add((Index > 2 ? Areas.Last() : 0) + GetTriangleArea(Points[0], Points[Index - 1], Points[Index]));

            int32 Triangle = 2;
            while (RandomNumber > Areas[Triangle - 2] / Areas.Last() && Triangle < N)
                ++Triangle;

            const FVector AB = Points[Triangle] - Points[0], AC = Points[Triangle - 1] - Points[0];
            return Points[0] + (FMath::FRand() * AB + FMath::FRand() * AC) / 2;
    }
}

bool FMathExtended::IsPointInPolygon2D(const TArray<FVector2D> &Polygon, const FVector2D& Point)
{
    bool Result = false;
    for (int32 i = 0, sz = Polygon.Num(), j = sz - 1; i < sz; j = i++)
        if ((Polygon[i].Y > Point.Y) != (Polygon[j].Y > Point.Y) && (Point.X < (Polygon[j].X - Polygon[i].X) * (Point.Y - Polygon[i].Y) / (Polygon[j].Y - Polygon[i].Y) + Polygon[i].X))
            Result = !Result;

    return Result;
}

float FMathExtended::GetParabolaCollisionX(const FVector2D &Focus1, const FVector2D &Focus2, float DirectrixY)
{
    if (FMath::IsNearlyEqual(Focus1.Y, Focus2.Y))
        return (Focus1.X + Focus2.X) / 2;
    if (FMath::IsNearlyEqual(Focus1.Y, DirectrixY))
        return Focus1.X;
    if (FMath::IsNearlyEqual(Focus2.Y, DirectrixY))
        return Focus2.X;

    const double dbl_DirectrixY = DirectrixY;
    const double dbl_Focus1_X = Focus1.X, dbl_Focus1_Y = Focus1.Y;
    const double dbl_Focus2_X = Focus2.X, dbl_Focus2_Y = Focus2.Y;

    const double C1 = 2 * (dbl_Focus1_Y - dbl_DirectrixY);
    const double C2 = 2 * (dbl_Focus2_Y - dbl_DirectrixY);

    const double D1 = dbl_Focus1_X * dbl_Focus1_X + dbl_Focus1_Y * dbl_Focus1_Y - dbl_DirectrixY * dbl_DirectrixY;
    const double D2 = dbl_Focus2_X * dbl_Focus2_X + dbl_Focus2_Y * dbl_Focus2_Y - dbl_DirectrixY * dbl_DirectrixY;

    const double A = C2 - C1;
    const double B = 2 * (dbl_Focus2_X * C1 - dbl_Focus1_X * C2);
    const double C = D1 * C2 - D2 * C1;

    const double D = FMath::Sqrt(B * B - 4 * A * C);

    const double X1 = (B < 0 ? -B + D : -B - D) / (2 * A);
    const double X2 = C / (A * X1);

    return Focus1.Y < Focus2.Y ? FMath::Min(X1, X2) : FMath::Max(X1, X2);
}

bool FMathExtended::GetParabolaIntersect(const FVector2D &Focus1, const FVector2D &Focus2, const FVector2D &Focus3, FVector2D &Circumcenter, float &DirectrixY)
{
    const FVector2D AB = Focus2 - Focus1;
    const FVector2D AC = Focus3 - Focus1;

    const float Determinant = FVector2D::CrossProduct(AB, AC);
    if (Determinant <= 0)
        return false;

    const float E = AB.X * (Focus1.X + Focus2.X) / 2 + AB.Y * (Focus1.Y + Focus2.Y) / 2;
    const float F = AC.X * (Focus1.X + Focus3.X) / 2 + AC.Y * (Focus1.Y + Focus3.Y) / 2;

    Circumcenter.X = (AC.Y * E - AB.Y * F) / Determinant, Circumcenter.Y = (AB.X * F - AC.X * E) / Determinant;
    DirectrixY = Circumcenter.Y + (Focus1 - Circumcenter).Size();

    return true;
}

int32 FMathExtended::Combination(int32 N, int32 K)
{
    static const int32 CacheSize = 20;
    static int32 Cache[CacheSize][CacheSize] = { { 0 } };

    if (Cache[N][K] != 0) return Cache[N][K];
    if (N < K)            return 0;
    if (K == 0)           return 1;

    return Cache[N][K] = Combination(N - 1, K - 1) + Combination(N - 1, K);
}
