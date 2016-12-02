// By Polyakov Pavel

#pragma once

#include "VoronoiGeneratorTask.h"

typedef FAsyncTask<class FVoronoiMainGeneratorTask> FVoronoiMainTask;

/** Class responsible for building FVoronoiGraph (Voronoi diagram) */
class PRACTICEPROJECT_API FVoronoiMainGeneratorTask final : public FVoronoiGeneratorTask
{
    struct VBreakPoint;
    struct VCircleEvent;

    /** Represents a node in a binary tree used in Fortune's Algorithm. If it is a leaf then it is VArc, in other case it is VBreakPoint */
    struct PRACTICEPROJECT_API VTreeNode
    {
        VBreakPoint *Parent;
        TUniquePtr<VTreeNode> Left, Right;

        VTreeNode(VBreakPoint *InParent) : Parent(InParent), Left(nullptr), Right(nullptr) {}
        virtual ~VTreeNode() {}

        FORCEINLINE bool IsLeaf() const { return !(Left.IsValid() || Right.IsValid()); }
    };

    /** A part of parabola */
    struct PRACTICEPROJECT_API VArc final : public VTreeNode
    {
        /** Focus of parabola */
        FVoronoiFace *Focus;

        /** Event that will remove this arc */
        VCircleEvent *CircleEvent;
        VBreakPoint *LeftBP, *RightBP;

        VArc(FVoronoiFace *InFocus, VBreakPoint *InParent = nullptr) : VTreeNode(InParent), Focus(InFocus), CircleEvent(nullptr), LeftBP(nullptr), RightBP(nullptr) {}
    };

    /** A point where two arcs collide */
    struct PRACTICEPROJECT_API VBreakPoint final : public VTreeNode
    {
        /** Edge being drawn by this break point */
        FVoronoiEdge *Edge;
        VArc *LeftArc, *RightArc;

        /** Whether this break point is responsible for begin or end vertex */
        bool bOriginal;

        /** Get x-location of the point where left and right arcs collide */
        float GetCollisionX(float SweepLineY) const;

        VBreakPoint(VBreakPoint *InParent = nullptr) : VTreeNode(InParent), Edge(nullptr), LeftArc(nullptr), RightArc(nullptr) {}
    };

    /** Represents a site or circle event */
    struct PRACTICEPROJECT_API VEvent
    {
        const float Priority;
        const bool IsSiteEvent;

        bool IsValid;

        VEvent(float InPriority, bool InSiteEvent) : Priority(InPriority), IsSiteEvent(InSiteEvent), IsValid(true) {}
        virtual ~VEvent() {}

        friend bool operator<(const TUniquePtr<VEvent> &Left, const TUniquePtr<VEvent> &Right)
        {
            return Left->Priority < Right->Priority;
        }
    };

    /** Happens when the sweep line goes through a new voronoi site */
    struct PRACTICEPROJECT_API VSiteEvent final : public VEvent
    {
        FVoronoiFace *Site;
        VSiteEvent(FVoronoiFace *InSite) : VEvent(InSite->GetLocation().Y, true), Site(InSite) {}
    };

    /** Happens when three arcs intersect in one point and one of them is removed */
    struct PRACTICEPROJECT_API VCircleEvent final : public VEvent
    {
        /** Point where arcs collide */
        FVector2D Circumcenter;

        /** Sweep line's y when arcs will collide */
        float CircleBottomY;

        /** Arc to remove */
        VArc *Arc;

        VCircleEvent(const FVector2D& InCircumcenter, float InCircleBottomY, VArc *InArc)
            : VEvent(InCircleBottomY, false), Circumcenter(InCircumcenter), CircleBottomY(InCircleBottomY), Arc(InArc) {}
    };

    /** A surface we a working with */
    FVoronoiSurface *VoronoiSurface;

    /** Voronoi navigation data */
    const AVoronoiNavData* VoronoiNavData;

    /** Returns an arc that is under given x */
    VArc* GetArcByX(VTreeNode *Root, float X, float SweepLineY) const;

    /** Finds the center of a circle that goes through three points */
    bool Circle(const FVector2D &A, const FVector2D &B, const FVector2D &C, FVector2D &Circumcenter, float &SweepLineY) const;

    /** Check if any circle event is going to happen to this arc and push it to the event queue */
    void CheckCircleEvent(VArc *Arc, TArray<TUniquePtr<VEvent>> &EventQueue);

    /** Invalidate circle event for this arc if one exists*/
    void InvalidateCircleEvent(VArc *Arc);

    /** Add a vertex to edge */
    void AddVertexToEdge(FVoronoiEdge *Edge, FVoronoiVertex *Vertex, bool bOriginal);

public:
    FVoronoiMainGeneratorTask(FVoronoiSurface *InVoronoiSurface, const AVoronoiNavData* InVoronoiNavData, bool *InBuildCanceled)
        : FVoronoiGeneratorTask(InBuildCanceled), VoronoiSurface(InVoronoiSurface), VoronoiNavData(InVoronoiNavData) {}

    void DoWork();
};
