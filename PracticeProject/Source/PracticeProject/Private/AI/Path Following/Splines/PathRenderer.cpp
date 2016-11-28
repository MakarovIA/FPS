// By Polyakov Pavel

#include "PracticeProject.h"
#include "PathRenderer.h"
#include "Spline.h"

#ifdef WITH_EDITOR

/** FPathRenderer */

void FPathRenderer::DrawPiecewiseLinearPath(const UObject* WorldContextObject, const FNavigationPath *Path)
{
    const UWorld *World = WorldContextObject->GetWorld();
    const TArray<FNavPathPoint>& PathPoints = Path->GetPathPoints();

    for (int32 i = 1, sz = PathPoints.Num(); i < sz; ++i)
        DrawDebugLine(World, PathPoints[i - 1].Location, PathPoints[i].Location, FColor(255, 165, 0), true, -1, 0, 5.f);  
}

void FPathRenderer::DrawSpline(const UObject* WorldContextObject, const FSpline *Spline)
{
    const UWorld *World = WorldContextObject->GetWorld();
    const int32 N = Spline->GetPieceNumber();

    for (int32 i = 0; i < N; ++i)
        for (double Parameter = 0.01; Parameter < 1.001; Parameter += 0.01)
            DrawDebugLine(World, Spline->GetValue(i, Parameter - 0.01), Spline->GetValue(i, Parameter), FColor(0, 0, 255), true, -1, 0, 20.f);
}

#endif // WITH_EDITOR
