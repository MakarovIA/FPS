// By Polyakov Pavel

#pragma once

#ifdef WITH_EDITOR

struct FSpline;

/** This class exists for debug purpose only */
struct PRACTICEPROJECT_API FPathRenderer
{
    static void DrawPiecewiseLinearPath(const UObject* WorldContextObject, const FNavigationPath *Path);
    static void DrawSpline(const UObject* WorldContextObject, const FSpline *Spline);
};

#endif // WITH_EDITOR
