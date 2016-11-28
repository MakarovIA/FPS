// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiMainGeneratorTask.h"

/** VBreakPoint */

float FVoronoiMainGeneratorTask::VBreakPoint::GetCollisionX(float SweepLineY) const
{
    // Save Focuses
    FVector2D P1(LeftArc->Focus->Location);
    FVector2D P2(RightArc->Focus->Location);

    // Trivial cases
    if (FMath::IsNearlyEqual(P1.Y, P2.Y))
        return (P1.X + P2.X) / 2;
    if (FMath::IsNearlyEqual(P1.Y, SweepLineY))
        return P1.X;
    if (FMath::IsNearlyEqual(P2.Y, SweepLineY))
        return P2.X;

    // We need to be VERY PRECISE there so everything will be converted to double
    // Using floats there can result even in negative discriminant which is impossible
    const double dbl_SweepLineY = SweepLineY;
    const double dbl_P1_X = P1.X;
    const double dbl_P1_Y = P1.Y;
    const double dbl_P2_X = P2.X;
    const double dbl_P2_Y = P2.Y;

    const double c1 = 2 * (dbl_P1_Y - dbl_SweepLineY);
    const double c2 = 2 * (dbl_P2_Y - dbl_SweepLineY);

    const double d1 = dbl_P1_X * dbl_P1_X + dbl_P1_Y * dbl_P1_Y - dbl_SweepLineY * dbl_SweepLineY;
    const double d2 = dbl_P2_X * dbl_P2_X + dbl_P2_Y * dbl_P2_Y - dbl_SweepLineY * dbl_SweepLineY;

    const double a = c2 - c1;
    const double b = 2 * (dbl_P2_X * c1 - dbl_P1_X * c2);
    const double c = d1 * c2 - d2 * c1;

    const double D = FMath::Sqrt(b * b - 4 * a * c);

    const double X1 = (b < 0 ? -b + D : -b - D) / (2 * a);
    const double X2 = c / (a * X1);

    // Choose the point we need
    if (P1.Y < P2.Y)
        return FMath::Min(X1, X2);
    else
        return FMath::Max(X1, X2);
}

/** FVoronoiMainGeneratorTask */

FVoronoiMainGeneratorTask::VArc* FVoronoiMainGeneratorTask::GetArcByX(VTreeNode *Root, float X, float SweepLineY) const
{
    while (!Root->IsLeaf())
        if (static_cast<VBreakPoint*>(Root)->GetCollisionX(SweepLineY) > X)
            Root = Root->Left.Get();
        else
            Root = Root->Right.Get();

    return static_cast<VArc*>(Root);
}

bool FVoronoiMainGeneratorTask::Circle(const FVector2D &A, const FVector2D &B, const FVector2D &C, FVector2D &Circumcenter, float &SweepLineY) const
{
    const FVector2D AB = B - A;
    const FVector2D AC = C - A;

    const float Det = AB.X * AC.Y - AC.X * AB.Y;

    // In that case parabolas will never intersect in one point
    // AC should be clockwise from AB
    if (Det <= 0)
        return false;

    const float E = AB.X * (A.X + B.X) / 2 + AB.Y * (A.Y + B.Y) / 2;
    const float F = AC.X * (A.X + C.X) / 2 + AC.Y * (A.Y + C.Y) / 2;

    Circumcenter.X = (AC.Y * E - AB.Y * F) / Det;
    Circumcenter.Y = (AB.X * F - AC.X * E) / Det;

    SweepLineY = Circumcenter.Y + (A - Circumcenter).Size();
    return true;
}

void FVoronoiMainGeneratorTask::CheckCircleEvent(VArc *Arc, TArray<TUniquePtr<VEvent>> &EventQueue)
{
    InvalidateCircleEvent(Arc);

    if (Arc->LeftBP == nullptr || Arc->RightBP == nullptr)
        return;

    VArc *LeftArc = Arc->LeftBP->LeftArc;
    VArc *RightArc = Arc->RightBP->RightArc;

    if (LeftArc->Focus == RightArc->Focus)
        return;

    FVector2D Circumcenter;
    float NextSweepLineY;

    if (Circle(FVector2D(LeftArc->Focus->Location), FVector2D(Arc->Focus->Location), FVector2D(RightArc->Focus->Location), Circumcenter, NextSweepLineY))
        Arc->CircleEvent = static_cast<VCircleEvent*>(EventQueue[EventQueue.HeapPush(MakeUnique<VCircleEvent>(Circumcenter, NextSweepLineY, Arc))].Get());
}

void FVoronoiMainGeneratorTask::InvalidateCircleEvent(VArc *Arc)
{
    if (Arc != nullptr && Arc->CircleEvent != nullptr)
    {
        Arc->CircleEvent->IsValid = false;
        Arc->CircleEvent->Arc = nullptr;
        Arc->CircleEvent = nullptr;
    }
}

void FVoronoiMainGeneratorTask::AddVertexToEdge(FVoronoiEdge *Edge, FVoronoiVertex *Vertex, bool bOriginal)
{
    if (bOriginal) Edge->Begin = Vertex;
    else Edge->End = Vertex;

    FVoronoiEdge *FirstEdge = Vertex->GetEdge();
    if (FirstEdge == Edge)
        return;

    if (Vertex->GetEdge()->GetFirstVertex() == Vertex)
    {
        if (bOriginal && Edge->Right == FirstEdge->Left)
        {
            if (FirstEdge->Previous != nullptr)
                Edge->Previous = FirstEdge->Previous;
            FirstEdge->Previous = Edge;
        }

        if (!bOriginal && Edge->Left == FirstEdge->Left)
        {
            if (FirstEdge->Previous != nullptr)
                Edge->Next = FirstEdge->Previous;
            FirstEdge->Previous = Edge;
        }

        const bool Condition1 = bOriginal && Edge->Left == FirstEdge->Right;
        const bool Condition2 = !bOriginal && Edge->Right == FirstEdge->Right;

        if (Condition1 || Condition2)
        {
            if (FirstEdge->Previous == nullptr)
                FirstEdge->Previous = Edge;
            else
            {
                FVoronoiEdge *Temp = FirstEdge->Previous;
                if (Temp->Begin == Vertex)
                    Temp->Previous = Edge;
                else
                    Temp->Next = Edge;
            }

            if (Condition1)
                Edge->Previous = FirstEdge;
            else
                Edge->Next = FirstEdge;
        }
    }
    else
    {
        if (bOriginal && Edge->Right == FirstEdge->Right)
        {
            if (FirstEdge->Next != nullptr)
                Edge->Previous = FirstEdge->Next;
            FirstEdge->Next = Edge;
        }

        if (!bOriginal && Edge->Left == FirstEdge->Right)
        {
            if (FirstEdge->Next != nullptr)
                Edge->Next = FirstEdge->Next;
            FirstEdge->Next = Edge;
        }

        const bool Condition1 = bOriginal && Edge->Left == FirstEdge->Left;
        const bool Condition2 = !bOriginal && Edge->Right == FirstEdge->Left;

        if (Condition1 || Condition2)
        {
            if (FirstEdge->Next == nullptr)
                FirstEdge->Next = Edge;
            else
            {
                FVoronoiEdge *Temp = FirstEdge->Next;
                if (Temp->Begin == Vertex)
                    Temp->Previous = Edge;
                else
                    Temp->Next = Edge;
            }

            if (Condition1)
                Edge->Previous = FirstEdge;
            else
                Edge->Next = FirstEdge;
        }
    }
}

void FVoronoiMainGeneratorTask::DoWork()
{
    // -------------------------------------------------------------------------------------------------------
    // Preparation Stage (duplicate removal)
    // -------------------------------------------------------------------------------------------------------

    VoronoiSurface->Faces.Sort([](const TUniquePtr<FVoronoiFace> &Left, const TUniquePtr<FVoronoiFace> &Right)
    {
        const FVector2D A = FVector2D(Left->GetLocation());
        const FVector2D B = FVector2D(Right->GetLocation());

        return FMath::FloorToInt(A.Y) < FMath::FloorToInt(B.Y) || (FMath::FloorToInt(A.Y) == FMath::FloorToInt(B.Y) && FMath::FloorToInt(A.X) < FMath::FloorToInt(B.X));
    });

    for (int32 i = 1; i < VoronoiSurface->Faces.Num(); ++i)
    {
        const FVector2D A = FVector2D(VoronoiSurface->Faces[i - 1]->GetLocation());
        const FVector2D B = FVector2D(VoronoiSurface->Faces[i]->GetLocation());

        if (FMath::FloorToInt(A.Y) == FMath::FloorToInt(B.Y) && FMath::FloorToInt(A.X) == FMath::FloorToInt(B.X))
            VoronoiSurface->Faces.RemoveAtSwap(i--);
    }

    // -------------------------------------------------------------------------------------------------------
    // Fortune Algorithm Stage
    // -------------------------------------------------------------------------------------------------------

    // Sweep line
    float SweepLineY = 0;

    TArray<TUniquePtr<VEvent>> EventQueue;
    TUniquePtr<VTreeNode> Root = nullptr;

    // Push all site events to queue
    for (const auto& i : VoronoiSurface->Faces)
        EventQueue.Emplace(new VSiteEvent(i.Get()));

    EventQueue.Heapify();

    // Proccess events
    while (EventQueue.Num() > 0)
    {
        // Build was canceled
        if (ShouldCancelBuild())
            return;

        TUniquePtr<VEvent> Event = MoveTemp(EventQueue.HeapTop());
        EventQueue.HeapRemoveAt(0);

        if (!Event->IsValid)
            continue;

        SweepLineY = Event->Priority;

        // ------------------------------------------------------------------------------------------------------------------------------------
        // SITE EVENT
        // ------------------------------------------------------------------------------------------------------------------------------------

        if (Event->IsSiteEvent)
        {
            VSiteEvent *SiteEvent = static_cast<VSiteEvent*>(Event.Get());

            if (!Root.IsValid())
            {
                Root = MakeUnique<VArc>(SiteEvent->Site);
                continue;
            }

            /** --- TREE MODIFICATION ---
               NOTE: Every pointer has 2 versions: raw and unique.
                     This is because unique will be invalidated on attachment so for code clearance raw pointer without _Unique postfix is used everywhere except assignment */

            VArc* Arc = GetArcByX(Root.Get(), SiteEvent->Site->Location.X, SweepLineY);
            InvalidateCircleEvent(Arc);

            TUniquePtr<VBreakPoint> BP_Unique = MakeUnique<VBreakPoint>(Arc->Parent);
            VBreakPoint* BP = BP_Unique.Get();

            /** --- SPECIAL CASE: THERE SHOULD BE OLY ONE BREAK POINT BETWEEN ARCS --- */

            if (FMath::IsNearlyEqual(Arc->Focus->Location.Y, SweepLineY))
            {
                // Detach arc and reatach it immediately.
                // This is safe as no new allocations are made during this proccess

                if (Arc->Parent == nullptr)
                {
                    Root.Release();
                    Root = MoveTemp(BP_Unique);
                } 
                else
                {
                    if (Arc->Parent->Left.Get() == Arc)
                    {
                        Arc->Parent->Left.Release();
                        Arc->Parent->Left = MoveTemp(BP_Unique);
                    }
                    else
                    {
                        Arc->Parent->Right.Release();
                        Arc->Parent->Right = MoveTemp(BP_Unique);
                    }
                }

                if (Arc->Focus->Location.X < SiteEvent->Site->Location.X)
                    BP->Left = TUniquePtr<VArc>(Arc);
                else
                    BP->Right = TUniquePtr<VArc>(Arc);

                Arc->Parent = BP;

                TUniquePtr<VArc> NewArc_Unique = MakeUnique<VArc>(SiteEvent->Site, BP);
                VArc *NewArc = NewArc_Unique.Get();

                if (Arc->Focus->Location.X < SiteEvent->Site->Location.X)
                {
                    BP->LeftArc = Arc;
                    BP->Right = MoveTemp(NewArc_Unique);
                    BP->RightArc = NewArc;

                    NewArc->RightBP = Arc->RightBP;
                    NewArc->LeftBP = BP;

                    if (Arc->RightBP != nullptr)
                    {
                        Arc->RightBP->LeftArc = NewArc;
                        Arc->RightBP->Edge->Left = NewArc->Focus;
                    }

                    Arc->RightBP = BP;
                } 
                else
                {
                    BP->RightArc = Arc;
                    BP->Left = MoveTemp(NewArc_Unique);
                    BP->LeftArc = NewArc;

                    NewArc->LeftBP = Arc->LeftBP;
                    NewArc->RightBP = BP;

                    if (Arc->LeftBP != nullptr)
                    {
                        Arc->LeftBP->RightArc = NewArc;
                        Arc->LeftBP->Edge->Right = NewArc->Focus;
                    }

                    Arc->LeftBP = BP;
                }

                // Create an edge that will be drawn by a new breakpoint
                BP->Edge = VoronoiSurface->Edges[VoronoiSurface->Edges.Emplace(new FVoronoiEdge(BP->LeftArc->Focus, BP->RightArc->Focus))].Get();
                BP->bOriginal = true;

                // Check for circle events
                CheckCircleEvent(Arc, EventQueue);
                CheckCircleEvent(NewArc, EventQueue);

                continue;
            }

            // Split an arc under new voronoi site's x into tree arcs A, B and C
            TUniquePtr<VBreakPoint> BP2_Unique = MakeUnique<VBreakPoint>(BP);
            VBreakPoint* BP2 = BP2_Unique.Get();

            TUniquePtr<VArc> A_Unique = MakeUnique<VArc>(Arc->Focus, BP),
                B_Unique = MakeUnique<VArc>(SiteEvent->Site, BP2),
                C_Unique = MakeUnique<VArc>(Arc->Focus, BP2);

            VArc *A = A_Unique.Get(), *B = B_Unique.Get(), *C = C_Unique.Get();

            // Create an edge that will be drawn by new breakpoints
            BP->Edge = BP2->Edge = VoronoiSurface->Edges[VoronoiSurface->Edges.Emplace(new FVoronoiEdge(A->Focus, B->Focus))].Get();
            BP->bOriginal = true;
            BP2->bOriginal = false;

            /* Insert arcs and breakpoints into the tree so the old arc is replaced by the next subtree
                    BP
                   /  \
                  A   BP2
                     /   \
                    B     C
            */

            BP->Left = MoveTemp(A_Unique);
            BP->LeftArc = A;
            BP->Right = MoveTemp(BP2_Unique);
            BP->RightArc = B;

            BP2->Left = MoveTemp(B_Unique);
            BP2->LeftArc = B;
            BP2->Right = MoveTemp(C_Unique);
            BP2->RightArc = C;

            B->LeftBP = BP;
            B->RightBP = BP2;

            A->RightBP = BP;
            A->LeftBP = Arc->LeftBP;

            C->LeftBP = BP2;
            C->RightBP = Arc->RightBP;

            if (Arc->LeftBP != nullptr)
                Arc->LeftBP->RightArc = A;
            if (Arc->RightBP != nullptr)
                Arc->RightBP->LeftArc = C;

            if (Arc->Parent == nullptr)
            {
                Root = MoveTemp(BP_Unique);
                continue;
            }

            if (Arc->Parent->Left.Get() == Arc)
                Arc->Parent->Left = MoveTemp(BP_Unique);
            else
                Arc->Parent->Right = MoveTemp(BP_Unique);

            CheckCircleEvent(A, EventQueue);
            CheckCircleEvent(C, EventQueue);
        } 
        
        // ------------------------------------------------------------------------------------------------------------------------------------
        // CIRCLE EVENT
        // ------------------------------------------------------------------------------------------------------------------------------------

        else
        {
            VCircleEvent *CircleEvent = static_cast<VCircleEvent*>(Event.Get());

            VBreakPoint *LeftBP = CircleEvent->Arc->LeftBP;
            VBreakPoint *RightBP = CircleEvent->Arc->RightBP;

            VArc* LeftArc = LeftBP->LeftArc;
            VArc* RightArc = RightBP->RightArc;

            InvalidateCircleEvent(LeftArc);
            InvalidateCircleEvent(RightArc);

            // Create new vertex
            FVoronoiVertex *NewVertex = VoronoiSurface->Vertexes[VoronoiSurface->Vertexes.Emplace(new FVoronoiVertex(FVector(CircleEvent->Circumcenter, VoronoiSurface->Location.Z), LeftBP->Edge))].Get();

            // Add it to edges being drawn by left and right break points
            AddVertexToEdge(LeftBP->Edge, NewVertex, LeftBP->bOriginal);
            AddVertexToEdge(RightBP->Edge, NewVertex, RightBP->bOriginal);

            // Create new edge starting at Circumcenter
            FVoronoiEdge *NewEdge = VoronoiSurface->Edges[VoronoiSurface->Edges.Emplace(new FVoronoiEdge(RightArc->Focus, LeftArc->Focus))].Get();
            AddVertexToEdge(NewEdge, NewVertex, true);

            // Modify tree
            if (RightBP->Left.Get() == CircleEvent->Arc)
            {
                LeftBP->RightArc = RightArc;
                RightArc->LeftBP = LeftBP;

                VBreakPoint *Parent = RightBP->Parent;
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

                VBreakPoint *Parent = LeftBP->Parent;
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
    // Infinite Edges Fininishing Stage
    // -------------------------------------------------------------------------------------------------------

    for (const auto& Edge : VoronoiSurface->GetEdges())
    {
        // Get left and right faces
        FVoronoiFace *LeftFace = Edge->GetLeftFace();
        FVoronoiFace *RightFace = Edge->GetRightFace();

        // Set pointers to edges
        if (LeftFace->Edge == nullptr || Edge->End == nullptr)
            LeftFace->Edge = Edge.Get();
        if (RightFace->Edge == nullptr || Edge->Begin == nullptr)
            RightFace->Edge = Edge.Get();

        // Set invalid flag
        if (Edge->Begin == nullptr || Edge->End == nullptr ||
            !VoronoiSurface->Contains(FVector2D(Edge->GetFirstVertex()->GetLocation())) ||
            !VoronoiSurface->Contains(FVector2D(Edge->GetLastVertex()->GetLocation())))
            LeftFace->Flags.bInvalid = RightFace->Flags.bInvalid = 1;

        // Finish invalid edges
        if (Edge->Begin == nullptr)
            AddVertexToEdge(Edge.Get(), VoronoiSurface->Vertexes[VoronoiSurface->Vertexes.Emplace(new FVoronoiVertex(FVector(), Edge.Get()))].Get(), true);

        if (Edge->End == nullptr)
            AddVertexToEdge(Edge.Get(), VoronoiSurface->Vertexes[VoronoiSurface->Vertexes.Emplace(new FVoronoiVertex(FVector(), Edge.Get()))].Get(), false);
    }

    // -------------------------------------------------------------------------------------------------------
    // Voronoi Quad Tree Building Stage
    // -------------------------------------------------------------------------------------------------------

    FVector2D Min(FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX);

    TArray<FVoronoiQuadTree::FQuadTreeElement> QuadTreeElements;
    for (const auto& i : VoronoiSurface->GetFaces())
    {
        // Special case
        if (i->GetEdge() == nullptr)
            i->Flags.bInvalid = true;

        // There is no sense in holding invalid faces in a quad tree
        if (i->GetFlags().bInvalid)
            continue;

        // Add new face and retrieve calculated rectangle
        QuadTreeElements.Add(FVoronoiQuadTree::FQuadTreeElement(i.Get()));
        const FBox2D& Rect = QuadTreeElements.Last().GetRect();

        Min.X = FMath::Min(Min.X, Rect.Min.X);
        Min.Y = FMath::Min(Min.Y, Rect.Min.Y);

        Max.X = FMath::Max(Max.X, Rect.Max.X);
        Max.Y = FMath::Max(Max.Y, Rect.Max.Y);
    }

    VoronoiSurface->QuadTree = MakeUnique<FVoronoiQuadTree>(Min, Max);
    for (const auto& i : QuadTreeElements)
        VoronoiSurface->QuadTree->Insert(i);

    // -------------------------------------------------------------------------------------------------------
    // Projection and Obstacle Detection Stage
    // -------------------------------------------------------------------------------------------------------

    // Get World
    const UWorld* World = VoronoiNavData->GetWorld();

    // Get surface's elements
    const auto& Vertexes = VoronoiSurface->GetVertexes();
    const auto& Edges = VoronoiSurface->GetEdges();
    const auto& Faces = VoronoiSurface->GetFaces();

    // Get Agent properties
    const auto& AgentProperties = VoronoiNavData->GetConfig();

    const float JumpZVelocity = 450;
    const float Gravity = FMath::Abs(World->GetGravityZ());

    const FVector AgentCrouchedHeight(0, 0, AgentProperties.AgentHeight * 2 / 3);
    const FVector AgentFullHeight(0, 0, AgentProperties.AgentHeight);

    const FVector AgentJumpHeight(0, 0, JumpZVelocity * JumpZVelocity / 2 / Gravity);
    const FVector AgentJumpFullHeight = AgentFullHeight + AgentJumpHeight;

    const FVector AgentStepHeight(0, 0, AgentProperties.AgentStepHeight == -1 ? 30 : AgentProperties.AgentStepHeight);

    // Calculate Z of vertexes by projecting them to the ground
    for (auto& i : Vertexes)
    {
        if (ShouldCancelBuild())
            return;

        const float ProjectionZ = ProjectPoint(i->GetLocation(), World).Z;
        i->Location.Z = ProjectionZ != i->GetLocation().Z ? ProjectionZ + 30 : ProjectionZ;
    }

    // Adjust Z of faces by taking coordinates from nearby vertexes calculated just before it
    for (const auto& Face : Faces)
    {
        if (!Face->IsNavigable())
            continue;

        Face->Location.Z = 0;

        TArray<FVoronoiVertex*> AdjVertexes = FVoronoiHelper::GetAdjacentVertexes(Face.Get());
        for (FVoronoiVertex* j : AdjVertexes)
            Face->Location.Z += j->GetLocation().Z / AdjVertexes.Num();
    }

    // Calculate flags for faces
    for (auto& Edge : Edges)
    {
        if (ShouldCancelBuild())
            return;

        const FVector Start = Edge->GetFirstVertex()->GetLocation();
        const FVector End = Edge->GetLastVertex()->GetLocation();

        FVoronoiFlags Temp;

        Temp.bObstacle = SmartCollisionCheck(Start + AgentStepHeight, End + AgentStepHeight, AgentProperties, World) || SmartCollisionCheck(Start + AgentCrouchedHeight, End + AgentCrouchedHeight, AgentProperties, World);
        Temp.bCrouchedOnly = Temp.bObstacle || SmartCollisionCheck(Start + AgentFullHeight, End + AgentFullHeight, AgentProperties, World);
        Temp.bNoJump = Temp.bCrouchedOnly || SmartCollisionCheck(Start + AgentJumpFullHeight, End + AgentJumpFullHeight, AgentProperties, World);

        Edge->GetLeftFace()->Flags |= Temp;
        Edge->GetRightFace()->Flags |= Temp;
    }

    // Border flag
    for (const auto& Face : Faces)
        if (Face->IsNavigable())
            for (const auto& Adj : FVoronoiHelper::GetAdjacentFaces(Face.Get()))
                if (bool FORCE_COMPILE = Face->Flags.bBorder = Adj->GetFlags().bInvalid)
                    break;
}
