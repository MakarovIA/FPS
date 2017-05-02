// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiDiagramTask.h"

/** Types */

struct FVoronoiDiagramTask::FTreeNode
{
    FBreakPoint *Parent;
    TUniquePtr<FTreeNode> Left, Right;

    FORCEINLINE FTreeNode(FBreakPoint *InParent)
        : Parent(InParent), Left(nullptr), Right(nullptr) {}
    virtual ~FTreeNode() {}

    FORCEINLINE bool IsLeaf() const { return !(Left.IsValid() || Right.IsValid()); }
};

struct FVoronoiDiagramTask::FBreakPoint final : public FTreeNode
{
    FVoronoiEdge *Edge;
    FArc *LeftArc, *RightArc;

    bool bOriginal;

    FORCEINLINE FBreakPoint(FVoronoiDiagramTask::FBreakPoint *InParent = nullptr)
        : FTreeNode(InParent), Edge(nullptr), LeftArc(nullptr), RightArc(nullptr) {}
};

struct FVoronoiDiagramTask::FArc final : public FTreeNode
{
    FVoronoiFace *Focus;

    FCircleEvent *CircleEvent;
    FBreakPoint *LeftBP, *RightBP;

    FORCEINLINE FArc(FVoronoiFace *InFocus, FBreakPoint *InParent = nullptr)
        : FTreeNode(InParent), Focus(InFocus), CircleEvent(nullptr), LeftBP(nullptr), RightBP(nullptr) {}
};

struct FVoronoiDiagramTask::FEvent
{
    const float Priority;
    const bool IsSiteEvent;

    bool IsValid;

    FORCEINLINE FEvent(float InPriority, bool InSiteEvent)
        : Priority(InPriority), IsSiteEvent(InSiteEvent), IsValid(true) {}
    virtual ~FEvent() {}

    FORCEINLINE friend bool operator<(const TUniquePtr<FEvent> &Left, const TUniquePtr<FEvent> &Right)
    {
        return Left->Priority < Right->Priority;
    }
};

struct FVoronoiDiagramTask::FSiteEvent final : public FEvent
{
    FVoronoiFace *Site;
    FORCEINLINE FSiteEvent(FVoronoiFace *InSite)
        : FEvent(InSite->Location.Y, true), Site(InSite) {}
};

struct FVoronoiDiagramTask::FCircleEvent final : public FEvent
{
    FVector2D Circumcenter;
    float CircleBottomY;

    FArc *Arc;

    FORCEINLINE FCircleEvent(const FVector2D& InCircumcenter, float InCircleBottomY, FArc *InArc)
        : FEvent(InCircleBottomY, false), Circumcenter(InCircumcenter), CircleBottomY(InCircleBottomY), Arc(InArc) {}
};

struct FVoronoiEdgeIntersect
{
    FVoronoiEdge *Edge;
    FVector Intersection;

    FORCEINLINE FVoronoiEdgeIntersect(FVoronoiEdge *InEdge, const FVector& InIntersection)
        : Edge(InEdge), Intersection(InIntersection) {}
};

/** FVoronoiDiagramTask */

FVoronoiDiagramTask::FArc* FVoronoiDiagramTask::GetArcByX(FTreeNode *Root, float X, float SweepLineY)
{
    while (!Root->IsLeaf())
        Root = FMathExtended::GetParabolaCollisionX(FVector2D(static_cast<FBreakPoint*>(Root)->LeftArc->Focus->Location),
            FVector2D(static_cast<FBreakPoint*>(Root)->RightArc->Focus->Location), SweepLineY) > X ? Root->Left.Get() : Root->Right.Get();

    return static_cast<FArc*>(Root);
}

void FVoronoiDiagramTask::CheckCircleEvent(FArc *Arc, TArray<TUniquePtr<FEvent>> &EventQueue)
{
    InvalidateCircleEvent(Arc);

    if (Arc->LeftBP == nullptr || Arc->RightBP == nullptr)
        return;

    FArc *LeftArc = Arc->LeftBP->LeftArc, *RightArc = Arc->RightBP->RightArc;
    if (LeftArc->Focus == RightArc->Focus)
        return;

    FVector2D Circumcenter;
    float NextSweepLineY;

    if (FMathExtended::GetParabolaIntersect(FVector2D(LeftArc->Focus->Location), FVector2D(Arc->Focus->Location), FVector2D(RightArc->Focus->Location), Circumcenter, NextSweepLineY))
        Arc->CircleEvent = static_cast<FCircleEvent*>(EventQueue[EventQueue.HeapPush(MakeUnique<FCircleEvent>(Circumcenter, NextSweepLineY, Arc))].Get());
}

void FVoronoiDiagramTask::InvalidateCircleEvent(FArc *Arc)
{
    if (Arc != nullptr && Arc->CircleEvent != nullptr)
    {
        Arc->CircleEvent->IsValid = false;
        Arc->CircleEvent->Arc = nullptr;
        Arc->CircleEvent = nullptr;
    }
}

void FVoronoiDiagramTask::AddVertexToEdge(FVoronoiEdge *Edge, FVoronoiVertex *Vertex, bool bOriginal)
{
    if (bOriginal) Edge->FirstVertex = Vertex;
    else           Edge->LastVertex = Vertex;

    FVoronoiEdge *FirstEdge = Vertex->Edge;
    if (FirstEdge == Edge)
        return;

    FVoronoiFace *ClockwiseFace = bOriginal ? Edge->RightFace : Edge->LeftFace;
    FVoronoiFace *FirstCouterClockwiseFace = FirstEdge->FirstVertex != Vertex ? FirstEdge->RightFace : FirstEdge->LeftFace;

    TPreserveConstPointer<FVoronoiEdge> &PreviousEdge = bOriginal ? Edge->PreviousEdge : Edge->NextEdge;
    TPreserveConstPointer<FVoronoiEdge> &FirstPreviousEdge = FirstEdge->FirstVertex == Vertex ? FirstEdge->PreviousEdge : FirstEdge->NextEdge;

    if (ClockwiseFace == FirstCouterClockwiseFace)
    {
        PreviousEdge = FirstPreviousEdge ? FirstPreviousEdge : FirstEdge;
        FirstPreviousEdge = Edge;
    }
    else
    {
        if (!FirstPreviousEdge)
            FirstPreviousEdge = Edge;
        else
        {
            TPreserveConstPointer<FVoronoiEdge> &FirstPreviousPreviousEdges = FirstPreviousEdge->FirstVertex == Vertex ? FirstPreviousEdge->PreviousEdge : FirstPreviousEdge->NextEdge;
            FirstPreviousPreviousEdges = Edge;
        }

        PreviousEdge = FirstEdge;
    }
}

void FVoronoiDiagramTask::AdjustFaceLocation(FVoronoiFace *Face)
{
    TArray<FVoronoiVertex*> AdjVertexes = FVoronoiHelper::GetAdjacentVertexes(Face);
    const int32 Count = AdjVertexes.Num();

    Face->Location.X = Face->Location.Y = 0;
    for (FVoronoiVertex* Vertex : AdjVertexes)
        Face->Location.X += Vertex->Location.X / Count,
        Face->Location.Y += Vertex->Location.Y / Count;
}

void FVoronoiDiagramTask::DoWork()
{
    float SweepLineY = 0;
    TUniquePtr<FTreeNode> Root = nullptr;

    TArray<TUniquePtr<FEvent>> EventQueue;
    for (TPreserveConstUniquePtr<FVoronoiFace> &i : VoronoiSurface->Faces)
        EventQueue.Add(MakeUnique<FSiteEvent>(i));
    EventQueue.Heapify();

    while (EventQueue.Num() > 0)
    {
        if (ShouldCancelBuild())
            return;

        TUniquePtr<FEvent> Event = MoveTemp(EventQueue.HeapTop());
        EventQueue.HeapRemoveAt(0);

        if (!Event->IsValid)
            continue;

        SweepLineY = Event->Priority;

        // ------------------------------------------------------------------------------------------------------------------------------------
        // SITE EVENT
        // ------------------------------------------------------------------------------------------------------------------------------------

        if (Event->IsSiteEvent)
        {
            FSiteEvent *SiteEvent = static_cast<FSiteEvent*>(Event.Get());

            if (!Root.IsValid())
            {
                Root = MakeUnique<FArc>(SiteEvent->Site);
                continue;
            }

            FArc* Arc = GetArcByX(Root.Get(), SiteEvent->Site->Location.X, SweepLineY);
            InvalidateCircleEvent(Arc);

            TUniquePtr<FBreakPoint> BP_Unique = MakeUnique<FBreakPoint>(Arc->Parent);
            FBreakPoint* BP = BP_Unique.Get();

            /** --- SPECIAL CASE: THERE SHOULD BE ONLY ONE BREAK POINT BETWEEN ARCS --- */

            if (FMath::IsNearlyEqual(Arc->Focus->Location.Y, SweepLineY))
            {
                TUniquePtr<FTreeNode>& OldArcLocation = Arc->Parent ? (Arc->Parent->Left.Get() == Arc ? Arc->Parent->Left : Arc->Parent->Right) : Root;
                TUniquePtr<FTreeNode>& NewArcLocation = Arc->Focus->Location.X < SiteEvent->Site->Location.X ? BP->Left : BP->Right;

                NewArcLocation = MoveTemp(OldArcLocation);
                OldArcLocation = MoveTemp(BP_Unique);

                Arc->Parent = BP;

                TUniquePtr<FArc> NewArc_Unique = MakeUnique<FArc>(SiteEvent->Site, BP);
                FArc *NewArc = NewArc_Unique.Get();

                if (Arc->Focus->Location.X < SiteEvent->Site->Location.X)
                {
                    BP->Right = MoveTemp(NewArc_Unique);

                    BP->LeftArc = Arc, BP->RightArc = NewArc;
                    NewArc->RightBP = Arc->RightBP, NewArc->LeftBP = BP;

                    if (Arc->RightBP != nullptr)
                    {
                        Arc->RightBP->LeftArc = NewArc;
                        Arc->RightBP->Edge->LeftFace = NewArc->Focus;
                    }

                    Arc->RightBP = BP;
                } 
                else
                {
                    BP->Left = MoveTemp(NewArc_Unique);

                    BP->RightArc = Arc, BP->LeftArc = NewArc;
                    NewArc->LeftBP = Arc->LeftBP, NewArc->RightBP = BP;

                    if (Arc->LeftBP != nullptr)
                    {
                        Arc->LeftBP->RightArc = NewArc;
                        Arc->LeftBP->Edge->RightFace = NewArc->Focus;
                    }

                    Arc->LeftBP = BP;
                }

                BP->Edge = VoronoiSurface->Edges[VoronoiSurface->Edges.Add(MakeUnique<FVoronoiEdge>(BP->LeftArc->Focus, BP->RightArc->Focus))];
                BP->bOriginal = true;

                CheckCircleEvent(Arc, EventQueue);
                CheckCircleEvent(NewArc, EventQueue);

                continue;
            }

            /** --- USUAL CASE: SPLIT ARC UNDER VORONOI SITE INTO THREE ARCS --- 
                    BP : [A, BP2]; BP2 : [B, C] */

            TUniquePtr<FBreakPoint> BP2_Unique = MakeUnique<FBreakPoint>(BP);
            FBreakPoint* BP2 = BP2_Unique.Get();

            TUniquePtr<FArc> A_Unique = MakeUnique<FArc>(Arc->Focus, BP),
                B_Unique = MakeUnique<FArc>(SiteEvent->Site, BP2),
                C_Unique = MakeUnique<FArc>(Arc->Focus, BP2);

            FArc *A = A_Unique.Get(), *B = B_Unique.Get(), *C = C_Unique.Get();

            BP->Left = MoveTemp(A_Unique), BP->Right = MoveTemp(BP2_Unique);
            BP2->Left = MoveTemp(B_Unique), BP2->Right = MoveTemp(C_Unique);

            BP->LeftArc = A, BP->RightArc = B;
            BP2->LeftArc = B, BP2->RightArc = C;

            A->LeftBP = Arc->LeftBP, A->RightBP = BP;
            B->LeftBP = BP, B->RightBP = BP2;
            C->LeftBP = BP2, C->RightBP = Arc->RightBP;

            if (Arc->LeftBP != nullptr)
                Arc->LeftBP->RightArc = A;
            if (Arc->RightBP != nullptr)
                Arc->RightBP->LeftArc = C;

            TUniquePtr<FTreeNode>& OldArcLocation = Arc->Parent ? (Arc->Parent->Left.Get() == Arc ? Arc->Parent->Left : Arc->Parent->Right) : Root;
            OldArcLocation = MoveTemp(BP_Unique);

            BP->Edge = BP2->Edge = VoronoiSurface->Edges[VoronoiSurface->Edges.Add(MakeUnique<FVoronoiEdge>(A->Focus, B->Focus))];
            BP->bOriginal = true; BP2->bOriginal = false;

            CheckCircleEvent(A, EventQueue);
            CheckCircleEvent(C, EventQueue);
        }
        
        // ------------------------------------------------------------------------------------------------------------------------------------
        // CIRCLE EVENT
        // ------------------------------------------------------------------------------------------------------------------------------------

        else
        {
            FCircleEvent *CircleEvent = static_cast<FCircleEvent*>(Event.Get());

            FBreakPoint *LeftBP = CircleEvent->Arc->LeftBP;
            FBreakPoint *RightBP = CircleEvent->Arc->RightBP;

            FArc* LeftArc = LeftBP->LeftArc;
            FArc* RightArc = RightBP->RightArc;

            InvalidateCircleEvent(LeftArc);
            InvalidateCircleEvent(RightArc);

            FVoronoiVertex *NewVertex = VoronoiSurface->Vertexes[VoronoiSurface->Vertexes.Add(MakeUnique<FVoronoiVertex>(
                FVector(FMath::RoundToFloat(CircleEvent->Circumcenter.X), FMath::RoundToFloat(CircleEvent->Circumcenter.Y), 0), LeftBP->Edge))];

            AddVertexToEdge(LeftBP->Edge, NewVertex, LeftBP->bOriginal);
            AddVertexToEdge(RightBP->Edge, NewVertex, RightBP->bOriginal);

            FVoronoiEdge *NewEdge = VoronoiSurface->Edges[VoronoiSurface->Edges.Add(MakeUnique<FVoronoiEdge>(RightArc->Focus, LeftArc->Focus))];
            AddVertexToEdge(NewEdge, NewVertex, true);

            if (RightBP->Left.Get() == CircleEvent->Arc)
            {
                LeftBP->RightArc = RightArc;
                RightArc->LeftBP = LeftBP;

                FBreakPoint *Parent = RightBP->Parent;
                RightBP->Right->Parent = Parent;

                if (Parent->Left.Get() == RightBP)
                    Parent->Left = MoveTemp(RightBP->Right);
                else
                    Parent->Right = MoveTemp(RightBP->Right);

                LeftBP->Edge = NewEdge;
                LeftBP->bOriginal = false;
            }
            else
            {
                RightBP->LeftArc = LeftArc;
                LeftArc->RightBP = RightBP;

                FBreakPoint *Parent = LeftBP->Parent;
                LeftBP->Left->Parent = Parent;

                if (Parent->Right.Get() == LeftBP)
                    Parent->Right = MoveTemp(LeftBP->Left);
                else
                    Parent->Left = MoveTemp(LeftBP->Left);

                RightBP->Edge = NewEdge;
                RightBP->bOriginal = false;
            }

            CheckCircleEvent(LeftArc, EventQueue);
            CheckCircleEvent(RightArc, EventQueue);
        }
    }

    // -------------------------------------------------------------------------------------------------------
    // SET FACES' POINTERS TO EDGES
    // -------------------------------------------------------------------------------------------------------

    for (TPreserveConstUniquePtr<FVoronoiEdge>& Edge : VoronoiSurface->Edges)
        Edge->RightFace->Edge = Edge->LeftFace->Edge = Edge;

    // -------------------------------------------------------------------------------------------------------
    // FINISIH INFINITE EDGES
    // -------------------------------------------------------------------------------------------------------

    for (TPreserveConstUniquePtr<FVoronoiEdge>& Edge : VoronoiSurface->Edges)
    {
        if (Edge->FirstVertex == nullptr || Edge->LastVertex == nullptr)		
         {		
             const FVector& A = Edge->LeftFace->Location;
             const FVector& B = Edge->RightFace->Location;		
 		
             const FVector Normal = FVector(A.Y - B.Y, B.X - A.X, 0).GetSafeNormal();
             const FVector Center = Edge->FirstVertex ? Edge->FirstVertex->Location : (Edge->LastVertex ? Edge->LastVertex->Location : (A + B) / 2);
             	
             if (Edge->FirstVertex == nullptr)
                 AddVertexToEdge(Edge, VoronoiSurface->Vertexes[VoronoiSurface->Vertexes.Add(MakeUnique<FVoronoiVertex>(Center + Normal * 1e6f, Edge))], true);
             
             if (Edge->LastVertex == nullptr)
                 AddVertexToEdge(Edge, VoronoiSurface->Vertexes[VoronoiSurface->Vertexes.Add(MakeUnique<FVoronoiVertex>(Center - Normal * 1e6f, Edge))], false);
         }
    }

    // -------------------------------------------------------------------------------------------------------
    // MOVE VERTEXES AWAY FROM BORDERS
    // -------------------------------------------------------------------------------------------------------

    for (TPreserveConstUniquePtr<FVoronoiVertex>& Vertex : VoronoiSurface->Vertexes)
    {
        for (int32 i = 0, M = VoronoiSurface->Borders.Num(); i < M; ++i)
        {
            for (int32 j = 0, N = VoronoiSurface->Borders[i].Num(); j < N; ++j)
            {
                const FVector2D Temp(FMath::ClosestPointOnSegment2D(FVector2D(Vertex->Location), FVector2D(VoronoiSurface->Borders[i][j]), FVector2D(VoronoiSurface->Borders[i][j + 1 < N ? j + 1 : 0])));
                if (FVector2D::DistSquared(Temp, FVector2D(Vertex->Location)) < 1.f)
                {
                    const FVector Direction = (Vertex->Edge->LastVertex->Location - Vertex->Edge->FirstVertex->Location).GetSafeNormal();
                    Vertex->Location += Direction;
                }
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------
    // APPLY SURFACE BORDERS TO VORONOI DIAGRAM
    // -------------------------------------------------------------------------------------------------------
    
    for (int32 k = 0, M = VoronoiSurface->Borders.Num(); k < M; ++k)
    {
        FVoronoiEdge *FirstEdge = nullptr, *LastEdge = nullptr;
        for (int32 i = 0, N = VoronoiSurface->Borders[k].Num(); i < N; ++i)
        {
            if (ShouldCancelBuild())
                return;

            // -------------------------------------------------------------------------------------------------------
            // COLLECT EDGES INTERSECTING BORDER EDGE
            // -------------------------------------------------------------------------------------------------------

            const FVector A = VoronoiSurface->Borders[k][i];
            const FVector B = VoronoiSurface->Borders[k][(i + 1) % N];
            const FVector BorderDirection = (B - A).GetSafeNormal();

            const double BorderLineX = (double)B.X - (double)A.X;
            const double BorderLineY = (double)B.Y - (double)A.Y;

            TArray<FVoronoiEdge*> Candidates;
            for (int32 j = 0, sz = VoronoiSurface->Edges.Num(); j < sz; ++j)
            {
                if (!VoronoiSurface->Edges[j]->LeftFace)
                    continue;

                const FVector C = VoronoiSurface->Edges[j]->FirstVertex->Location;
                const FVector D = VoronoiSurface->Edges[j]->LastVertex->Location;

                const double CToAX = (double)C.X - (double)A.X, CToAY = (double)C.Y - (double)A.Y;
                const double DToAX = (double)D.X - (double)A.X, DToAY = (double)D.Y - (double)A.Y;

                const bool IsCLeftFromBorderLine = BorderLineX * CToAY - BorderLineY * CToAX <= 0;
                const bool IsDLeftFromBorderLine = BorderLineX * DToAY - BorderLineY * DToAX <= 0;

                if (IsCLeftFromBorderLine)
                {
                    if (IsDLeftFromBorderLine)
                        continue;

                    Swap(VoronoiSurface->Edges[j]->FirstVertex, VoronoiSurface->Edges[j]->LastVertex);
                    Swap(VoronoiSurface->Edges[j]->PreviousEdge, VoronoiSurface->Edges[j]->NextEdge);
                    Swap(VoronoiSurface->Edges[j]->LeftFace, VoronoiSurface->Edges[j]->RightFace);
                }

                Candidates.Add(VoronoiSurface->Edges[j]);
            }

            TArray<FVoronoiEdgeIntersect> IntersectingEdges;
            for (int32 j = 0, sz = Candidates.Num(); j < sz; ++j)
            {
                const FVector C = Candidates[j]->FirstVertex->Location;
                const FVector D = Candidates[j]->LastVertex->Location;

                FVector Intersection;
                if (FMath::SegmentIntersection2D(A, VoronoiSurface->Borders[k][(i + 1) % N], C, D, Intersection))
                    IntersectingEdges.Emplace(Candidates[j], Intersection);
            }

            for (int32 j = 0, sz = IntersectingEdges.Num(); j < sz; ++j)
            {
                for (int32 p = j + 1; p < sz; ++p)
                {
                    if (IntersectingEdges[p].Edge->LeftFace == IntersectingEdges[j].Edge->RightFace)
                    {
                        Swap(IntersectingEdges[p], IntersectingEdges[j + 1]);
                        break;
                    }

                    if (IntersectingEdges[p].Edge->RightFace == IntersectingEdges[0].Edge->LeftFace)
                    {
                        FVoronoiEdgeIntersect Temp = IntersectingEdges[p];
                        IntersectingEdges.RemoveAtSwap(p);
                        IntersectingEdges.Insert(Temp, 0);
                        break;
                    }
                }
            }

            // -------------------------------------------------------------------------------------------------------
            // APPLY BORDER TO INTERSECTING EDGES
            // -------------------------------------------------------------------------------------------------------

            for (int32 j = 0, sz = IntersectingEdges.Num(); j < sz; ++j)
            {
                if (!LastEdge)
                {
                    LastEdge = FirstEdge = VoronoiSurface->Edges[VoronoiSurface->Edges.Add(MakeUnique<FVoronoiEdge>(nullptr, IntersectingEdges[j].Edge->LeftFace))];
                    FVoronoiVertex *FirstVertex = VoronoiSurface->Vertexes[VoronoiSurface->Vertexes.Add(MakeUnique<FVoronoiVertex>(VoronoiSurface->Borders[k][0], LastEdge))];

                    LastEdge->FirstVertex = FirstVertex;

                    for (int32 p = 1; p <= i; ++p)
                    {
                        FVoronoiVertex *TempVertex = VoronoiSurface->Vertexes[VoronoiSurface->Vertexes.Add(MakeUnique<FVoronoiVertex>(VoronoiSurface->Borders[k][p], LastEdge))];
                        FVoronoiEdge *TempEdge = VoronoiSurface->Edges[VoronoiSurface->Edges.Add(MakeUnique<FVoronoiEdge>(nullptr, LastEdge->RightFace))];

                        LastEdge->LastVertex = TempVertex;
                        LastEdge->NextEdge = TempEdge;

                        TempEdge->PreviousEdge = LastEdge;
                        TempEdge->FirstVertex = TempVertex;

                        LastEdge = TempEdge;
                    }
                }

                FVoronoiVertex *NewVertex = VoronoiSurface->Vertexes[VoronoiSurface->Vertexes.Add(MakeUnique<FVoronoiVertex>(IntersectingEdges[j].Intersection, LastEdge))];
                FVoronoiEdge *NewEdge = VoronoiSurface->Edges[VoronoiSurface->Edges.Add(MakeUnique<FVoronoiEdge>(IntersectingEdges[j].Edge->LeftFace, IntersectingEdges[j].Edge->RightFace))];

                NewEdge->PreviousEdge = IntersectingEdges[j].Edge->PreviousEdge;
                NewEdge->FirstVertex = IntersectingEdges[j].Edge->FirstVertex;
                NewEdge->FirstVertex->Edge = NewEdge;
                NewEdge->LastVertex = NewVertex;

                if (NewEdge->PreviousEdge)
                {
                    FVoronoiEdge *PreviousPreviousEdge = IntersectingEdges[j].Edge->PreviousEdge->RightFace == IntersectingEdges[j].Edge->LeftFace ?
                        IntersectingEdges[j].Edge->PreviousEdge->PreviousEdge : IntersectingEdges[j].Edge->PreviousEdge->NextEdge;
                    TPreserveConstPointer<FVoronoiEdge> &PtrToChange = PreviousPreviousEdge->PreviousEdge == IntersectingEdges[j].Edge ? PreviousPreviousEdge->PreviousEdge : PreviousPreviousEdge->NextEdge;

                    PtrToChange = NewEdge;
                }

                IntersectingEdges[j].Edge->FirstVertex = NewVertex;

                LastEdge->LastVertex = NewVertex;
                LastEdge->NextEdge = NewEdge;

                FVoronoiEdge *BorderEdge = VoronoiSurface->Edges[VoronoiSurface->Edges.Add(MakeUnique<FVoronoiEdge>(nullptr, IntersectingEdges[j].Edge->RightFace))];
                BorderEdge->PreviousEdge = LastEdge;
                BorderEdge->FirstVertex = NewVertex;

                LastEdge = NewEdge->NextEdge = BorderEdge;
            }

            // -------------------------------------------------------------------------------------------------------
            // CONTINUE TO THE NEXT BORDER EDGE
            // -------------------------------------------------------------------------------------------------------

            if (LastEdge)
            {
                if (i + 1 < N)
                {
                    FVoronoiVertex *NewVertex = VoronoiSurface->Vertexes[VoronoiSurface->Vertexes.Add(MakeUnique<FVoronoiVertex>(VoronoiSurface->Borders[k][i + 1], LastEdge))];
                    FVoronoiEdge *BorderEdge = VoronoiSurface->Edges[VoronoiSurface->Edges.Add(MakeUnique<FVoronoiEdge>(nullptr, LastEdge->RightFace))];

                    LastEdge->LastVertex = NewVertex;
                    LastEdge->NextEdge = BorderEdge;

                    BorderEdge->PreviousEdge = LastEdge;
                    BorderEdge->FirstVertex = NewVertex;

                    LastEdge = BorderEdge;
                }
                else
                {
                    LastEdge->LastVertex = FirstEdge->FirstVertex;
                    LastEdge->NextEdge = FirstEdge;

                    LastEdge->LastVertex->Edge = LastEdge;
                    FirstEdge->PreviousEdge = LastEdge;
                }
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------
    // CLEANUP ELEMENTS OUTSIDE NAVIGABLE AREA
    // -------------------------------------------------------------------------------------------------------

    TSet<FVoronoiEdge*> VisitedEdges;
    for (TPreserveConstUniquePtr<FVoronoiEdge>& Edge : VoronoiSurface->Edges)
    {
        if (ShouldCancelBuild())
            return;

        if (Edge->LeftFace || VisitedEdges.Contains(Edge))
            continue;

        FVoronoiFace* OldFace = Edge->RightFace;

        TArray<FVoronoiEdge*> FaceEdges;
        FaceEdges.Add(Edge);

        FVoronoiEdge* EdgeToAdd;
        while ((EdgeToAdd = (FaceEdges.Last()->LeftFace == OldFace) ? FaceEdges.Last()->PreviousEdge : FaceEdges.Last()->NextEdge) != FaceEdges[0])
            FaceEdges.Add(EdgeToAdd);

        VisitedEdges.Append(FaceEdges);

        FVoronoiFace *NewFace = VoronoiSurface->Faces[VoronoiSurface->Faces.Add(MakeUnique<FVoronoiFace>(VoronoiSurface, OldFace->Location))];
        NewFace->Flags.bBorder = true, NewFace->Edge = Edge;

        for (FVoronoiEdge* FaceEdge : FaceEdges)
        {
            TPreserveConstPointer<FVoronoiFace> &FaceToSet = FaceEdge->LeftFace == OldFace ? FaceEdge->LeftFace : FaceEdge->RightFace;
            FaceToSet = NewFace;
        }

        AdjustFaceLocation(NewFace);
    }

    TSet<FVoronoiFace*> VisitedFaces;
    TSet<FVoronoiVertex*> VisitedVertexes;

    VisitedEdges.Empty();
    for (TPreserveConstUniquePtr<FVoronoiEdge>& Edge : VoronoiSurface->Edges)
    {
        if (ShouldCancelBuild())
            return;

        if (Edge->LeftFace || VisitedEdges.Contains(Edge))
            continue;

        FVoronoiFace *Top = Edge->RightFace;
        TQueue<FVoronoiFace*> Queue;

        Queue.Enqueue(Top);
        while (Queue.Dequeue(Top))
        {
            if (VisitedFaces.Contains(Top))
                continue;

            VisitedFaces.Add(Top);
            VisitedEdges.Append(FVoronoiHelper::GetAdjacentEdges(Top));
            VisitedVertexes.Append(FVoronoiHelper::GetAdjacentVertexes(Top));

            for (FVoronoiFace *Adj : FVoronoiHelper::GetAdjacentFaces(Top))
                if (!VisitedFaces.Contains(Adj))
                    Queue.Enqueue(Adj);
        }
    }
    
    for (int32 i = 0; i < VoronoiSurface->Faces.Num(); ++i)    if (!VisitedFaces.Contains(VoronoiSurface->Faces[i]))       VoronoiSurface->Faces.RemoveAtSwap(i--);
    for (int32 i = 0; i < VoronoiSurface->Edges.Num(); ++i)    if (!VisitedEdges.Contains(VoronoiSurface->Edges[i]))       VoronoiSurface->Edges.RemoveAtSwap(i--);
    for (int32 i = 0; i < VoronoiSurface->Vertexes.Num(); ++i) if (!VisitedVertexes.Contains(VoronoiSurface->Vertexes[i])) VoronoiSurface->Vertexes.RemoveAtSwap(i--);

    // -------------------------------------------------------------------------------------------------------
    // SPLIT CONCAVE FACES INTO CONVEX ONES
    // -------------------------------------------------------------------------------------------------------

    for (int32 i = 0; i < VoronoiSurface->Faces.Num(); ++i)
    {
        if (ShouldCancelBuild())
            return;

        if (!VoronoiSurface->Faces[i]->Flags.bBorder)
            continue;

        TArray<FVoronoiVertex*> AdjVertexes = FVoronoiHelper::GetAdjacentVertexes(VoronoiSurface->Faces[i]);
        bool bSplitFound = false;

        for (int32 Start = 0, j = 1, sz = AdjVertexes.Num(); j <= sz + Start && Start < sz; ++j)
        {
            const FVector& Previous = AdjVertexes[(j - 1) % sz]->Location;
            const FVector& Current = AdjVertexes[j % sz]->Location;
            const FVector& Next = AdjVertexes[(j + 1) % sz]->Location;

            if ((FVector2D(Next - Previous) ^ FVector2D(Next - Current)) < 0)
            {
                for (int32 k = Start; k <= j - 2; ++k)
                {
                    const FVector& KLocation = AdjVertexes[k % sz]->Location;
                    if (j % sz == k || (j + 1) % sz == k || (j - 1) % sz == k ||
                        (FVector2D(KLocation - Previous) ^ FVector2D(KLocation - Current)) < 0)
                        continue;

                    bool bIntersects = false;
                    for (int32 t = j + 1; (t + 1) % sz != Start; ++t)
                    {
                        FVector Intersection;
                        if (FMath::SegmentIntersection2D(Current, KLocation, AdjVertexes[t % sz]->Location, AdjVertexes[(t + 1) % sz]->Location, Intersection))
                        {
                            bIntersects = true;
                            break;
                        }
                    }

                    if (!bIntersects)
                    {
                        FVoronoiFace *NewFace = VoronoiSurface->Faces[VoronoiSurface->Faces.Add(MakeUnique<FVoronoiFace>(VoronoiSurface, VoronoiSurface->Faces[i]->Location))];
                        FVoronoiEdge* NewEdge = VoronoiSurface->Edges[VoronoiSurface->Edges.Add(MakeUnique<FVoronoiEdge>(VoronoiSurface->Faces[i], NewFace))];

                        NewEdge->FirstVertex = AdjVertexes[k % sz];
                        NewEdge->LastVertex = AdjVertexes[j % sz];

                        NewFace->Flags.bBorder = true;
                        NewFace->Edge = NewEdge;

                        VoronoiSurface->Faces[i]->Edge = NewEdge;

                        const auto Predicate = [&OldFace = VoronoiSurface->Faces[i]](FVoronoiEdge* InEdge) { return InEdge->LeftFace == OldFace || InEdge->RightFace == OldFace; };

                        TArray<FVoronoiEdge*> AdjFirst = FVoronoiHelper::GetAdjacentEdges(NewEdge->FirstVertex).FilterByPredicate(Predicate);
                        TArray<FVoronoiEdge*> AdjLast = FVoronoiHelper::GetAdjacentEdges(NewEdge->LastVertex).FilterByPredicate(Predicate);

                        if (!(AdjFirst[0]->NextEdge == AdjFirst[1] || AdjFirst[0]->PreviousEdge == AdjFirst[1])) Swap(AdjFirst[0], AdjFirst[1]);
                        if (!(AdjLast[0]->NextEdge  == AdjLast[1]  || AdjLast[0]->PreviousEdge  == AdjLast[1]))  Swap(AdjLast[0], AdjLast[1]);

                        if (AdjFirst[0]->NextEdge == AdjFirst[1]) AdjFirst[0]->NextEdge = NewEdge; else AdjFirst[0]->PreviousEdge = NewEdge;
                        if (AdjLast[0]->PreviousEdge == AdjLast[1]) AdjLast[0]->PreviousEdge = NewEdge; else AdjLast[0]->NextEdge = NewEdge;

                        NewEdge->PreviousEdge = AdjFirst[1];
                        NewEdge->NextEdge = AdjLast[1];
                       
                        TArray<FVoronoiEdge*> FaceEdges;
                        FaceEdges.Add(NewEdge->NextEdge);

                        FVoronoiEdge* EdgeToAdd;
                        while ((EdgeToAdd = (FaceEdges.Last()->LeftFace == VoronoiSurface->Faces[i]) ? FaceEdges.Last()->PreviousEdge : FaceEdges.Last()->NextEdge) != NewEdge)
                            FaceEdges.Add(EdgeToAdd);

                        for (FVoronoiEdge* FaceEdge : FaceEdges)
                        {
                            TPreserveConstPointer<FVoronoiFace> &FaceToSet = FaceEdge->LeftFace == VoronoiSurface->Faces[i] ? FaceEdge->LeftFace : FaceEdge->RightFace;
                            FaceToSet = NewFace;
                        }

                        AdjustFaceLocation(NewFace);
                        AdjustFaceLocation(VoronoiSurface->Faces[i]);

                        bSplitFound = true;
                        break;
                    }
                }

                j = ++Start;
            }

            if (bSplitFound)
                break;
        }
    }

    // -------------------------------------------------------------------------------------------------------
    // CONSTRUCT VORONOI QUADTREE
    // -------------------------------------------------------------------------------------------------------

    VoronoiSurface->QuadTree = FVoronoiQuadTree::ConstructFromSequence(VoronoiSurface->Faces);

    // -------------------------------------------------------------------------------------------------------
    // SET Z-LOCATION OF VERTEXES AND HEIGHT FLAGS OF FACES
    // -------------------------------------------------------------------------------------------------------

    const float JumpZVelocity = 450;
    const float Gravity = FMath::Abs(World->GetGravityZ());

    const FVector AgentFullHeight(0, 0, AgentProperties.AgentHeight);
    const FVector AgentJumpFullHeight = AgentFullHeight + FVector(0, 0, JumpZVelocity * JumpZVelocity / 2 / Gravity);

    for (TPreserveConstUniquePtr<FVoronoiVertex>& Vertex : VoronoiSurface->Vertexes)
    {
        if (ShouldCancelBuild())
            return;

        if (Vertex->Edge->LeftFace)
        {
            TArray<FVoronoiFace*> AdjFaces = FVoronoiHelper::GetAdjacentFaces(Vertex);
            const int32 Count = AdjFaces.Num();

            Vertex->Location.Z = 0;
            for (FVoronoiFace* j : AdjFaces)
                Vertex->Location.Z += j->Location.Z / Count;
        }

        Vertex->Location.Z -= 2.5f;
    }

    for (TPreserveConstUniquePtr<FVoronoiFace>& Face : VoronoiSurface->Faces)
    {
        if (ShouldCancelBuild())
            return;

        Face->Flags.bCrouchedOnly = SimpleCollisionCheck(Face->Location, Face->Location + AgentFullHeight, World);
        Face->Flags.bNoJump = Face->Flags.bCrouchedOnly || SimpleCollisionCheck(Face->Location, Face->Location + AgentJumpFullHeight, World);
    }
}
