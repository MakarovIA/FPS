// By Polyakov Pavel

#pragma once

#include "VoronoiNavDataGenerator.h"

/** Builds Voronoi diagram */
class PRACTICEPROJECT_API FVoronoiDiagramTask final : public FVoronoiTask
{
    struct FTreeNode;
    struct FBreakPoint;
    struct FArc;

    struct FEvent;
    struct FSiteEvent;
    struct FCircleEvent;

    FVoronoiSurface *VoronoiSurface;

    static FArc* GetArcByX(FTreeNode *Root, float X, float SweepLineY);

    static void CheckCircleEvent(FArc *Arc, TArray<TUniquePtr<FEvent>> &EventQueue);
    static void InvalidateCircleEvent(FArc *Arc);

    static void AddVertexToEdge(FVoronoiEdge *Edge, FVoronoiVertex *Vertex, bool bOriginal);
    static void AdjustFaceLocation(FVoronoiFace *Face);

public:
    FORCEINLINE FVoronoiDiagramTask(const FVoronoiNavDataGenerator& InParentGenerator, FVoronoiSurface *InVoronoiSurface)
        : FVoronoiTask(InParentGenerator), VoronoiSurface(InVoronoiSurface) {}

    void DoWork();
};
