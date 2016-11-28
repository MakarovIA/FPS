// By Polyakov Pavel

#pragma once

#ifdef WITH_EDITOR

class FSpline;

/** This class exists for debug purpose only */
class PRACTICEPROJECT_API FPathRenderer
{
public:
    static void DrawPiecewiseLinearPath(const UObject* WorldContextObject, const FNavigationPath *Path);
    static void DrawSpline(const UObject* WorldContextObject, const FSpline *Spline);
};

#endif // WITH_EDITOR
