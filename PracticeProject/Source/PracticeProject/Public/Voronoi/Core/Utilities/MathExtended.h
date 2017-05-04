// By Polyakov Pavel

#pragma once

/** A collection of math functions */
struct PRACTICEPROJECT_API FMathExtended
{
    static void ProjectOnAxis(const TArray<FVector> &Points, const FVector &Axis, float &Min, float &Max);
    static float GetTriangleArea(const FVector &A, const FVector &B, const FVector &C);
    static FVector GetRandomPointInPolygon(const TArray<FVector>& Points);

    static bool IsPointInPolygon2D(const TArray<FVector2D> &Polygon, const FVector2D& Point);
    static float GetParabolaCollisionX(const FVector2D &Focus1, const FVector2D &Focus2, float DirectrixY);
    static bool GetParabolaIntersect(const FVector2D &Focus1, const FVector2D &Focus2, const FVector2D &Focus3, FVector2D &Circumcenter, float &DirectrixY);

    static int32 Combination(int32 N, int32 K);
};
