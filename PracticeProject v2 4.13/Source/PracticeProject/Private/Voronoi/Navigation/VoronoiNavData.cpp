// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiNavData.h"

#include "VoronoiNavDataGenerator.h"
#include "VoronoiRenderingComponent.h"

/** AVoronoiNavData */

AVoronoiNavData::AVoronoiNavData() : bPostProcessPaths(true), bLiveGeneration(true), bEnableLog(true), bShowNavigation(true), bUpdateDrawing(false), bUpdatePaths(false)
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        FindPathImplementation = FindPathVoronoi;
        FindHierarchicalPathImplementation = FindPathVoronoi;

        TestPathImplementation = TestPathVoronoi;
        TestHierarchicalPathImplementation = TestPathVoronoi;

        RaycastImplementation = RaycastVoronoi;
    }

    bEnableDrawing = true;
}

void AVoronoiNavData::CheckVoronoi() const
{
    if (!VoronoiGraph.IsValid())
    {
        NavDataGenerator->RebuildAll();
        NavDataGenerator->EnsureBuildCompletion();
    }
}

void AVoronoiNavData::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
    Super::TickActor(DeltaTime, TickType, ThisTickFunction);

    // Manage paths
    if (bUpdatePaths)
    {
        for (FNavPathWeakPtr& i : ActivePaths)
            if (i.IsValid())
                RequestRePath(i.Pin(), ENavPathUpdateType::NavigationChanged);

        bUpdatePaths = false;
    }

    // Manage rendering
    const bool bShowNavigationTemp = UVoronoiRenderingComponent::IsNavigationShowFlagSet(GetWorld());
    if (bShowNavigation != bShowNavigationTemp)
    {
        UpdateVoronoiGraphDrawing();
        bShowNavigation = bShowNavigationTemp;
    }

    if (bUpdateDrawing && RenderingComp != nullptr && RenderingComp->bVisible)
    {
        RenderingComp->MarkRenderStateDirty();
        bUpdateDrawing = false;
    }
}

void AVoronoiNavData::Serialize(FArchive &Ar)
{
    Super::Serialize(Ar);

    if (NavDataGenerator.IsValid() && NavDataGenerator->IsBuildInProgress())
        NavDataGenerator->EnsureBuildCompletion();

    if (!VoronoiGraph.IsValid())
        VoronoiGraph = MakeUnique<FVoronoiGraph>();

    VoronoiGraph->Serialize(Ar);

    if (Ar.IsLoading())
        UpdateVoronoiGraphDrawing();
}

UPrimitiveComponent* AVoronoiNavData::ConstructRenderingComponent()
{
    return NewObject<UVoronoiRenderingComponent>(this, TEXT("VoronoiRenderingComponent"), RF_Transient);
}

bool AVoronoiNavData::SupportsRuntimeGeneration() const
{
    return true;
}

void AVoronoiNavData::ConditionalConstructGenerator()
{
    if (!NavDataGenerator.IsValid())
        NavDataGenerator = MakeShareable(new FVoronoiNavDataGenerator(this));
}

void AVoronoiNavData::OnVoronoiNavDataGenerationFinished(double TimeSpent)
{
    if (bEnableLog)
    {
        int32 Faces = 0, NavigableFaces = 0, Edges = 0, Vertexes = 0;
        for (const auto& Surface : VoronoiGraph->GetSurfaces())
        {
            Faces += Surface->GetFaces().Num();
            
            for (const auto& Face : Surface->GetFaces())
                if (Face->IsNavigable())
                    ++NavigableFaces;

            Edges += Surface->GetEdges().Num();
            Vertexes += Surface->GetVertexes().Num();
        }

        UE_LOG(LogNavigation, Warning, TEXT("Voronoi navigation data successfully generated (Surfaces: %d, Faces: %d / %d, Edges: %d, Vertexes: %d) in %d ms"),
            VoronoiGraph->GetSurfaces().Num(), NavigableFaces, Faces, Edges, Vertexes, (int)(TimeSpent * 1000));
    }

    if (GetWorld() && GetWorld()->GetNavigationSystem())
        GetWorld()->GetNavigationSystem()->OnNavigationGenerationFinished(*this);

    // Draw changes
    UpdateVoronoiGraphDrawing();

    // Request paths rebuild
    MarkPathsAsNeedingUpdate();
}

bool AVoronoiNavData::ProjectPoint(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent, FSharedConstNavQueryFilter Filter, const UObject* Querier) const
{ 
    OutLocation = FNavLocation(::ProjectPoint(Point, GetWorld()));
    return true;
}

// PATH COST AND LENGTH CALCULATIONS
ENavigationQueryResult::Type AVoronoiNavData::CalcPathCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathCost, FSharedConstNavQueryFilter QueryFilter, const UObject* Querier) const
{
    static float TempLength = 0;
    return CalcPathLengthAndCost(PathStart, PathEnd, TempLength, OutPathCost, QueryFilter, Querier);
}
ENavigationQueryResult::Type AVoronoiNavData::CalcPathLength(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, FSharedConstNavQueryFilter QueryFilter, const UObject* Querier) const
{
    static float TempCost = 0;
    return CalcPathLengthAndCost(PathStart, PathEnd, OutPathLength, TempCost, QueryFilter, Querier);
}
ENavigationQueryResult::Type AVoronoiNavData::CalcPathLengthAndCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, float& OutPathCost, FSharedConstNavQueryFilter QueryFilter, const UObject* Querier) const
{ 
    FPathFindingQuery Query(Querier, *this, PathStart, PathEnd, QueryFilter);
    const FPathFindingResult& Result = FindPath(NavDataConfig, Query);
    if (Result.IsSuccessful())
    {
        OutPathLength = Query.PathInstanceToFill->GetLength();
        OutPathCost = Query.PathInstanceToFill->GetCost();
    }

    return Result.Result;
}

// RANDOM POINTS GETTERS
FNavLocation AVoronoiNavData::GetRandomPoint(FSharedConstNavQueryFilter Filter, const UObject* Querier) const
{
    CheckVoronoi();

    TArray<FVoronoiFace*> Faces;
    for (const auto& Surface : VoronoiGraph->GetSurfaces())
        for (const auto& Face : Surface->GetFaces())
        if (Face->IsNavigable())
            Faces.Add(Face.Get());

    if (Faces.Num() == 0)
        return FNavLocation();

    const auto& ChosenFace = Faces[FMath::RandRange(0, Faces.Num() - 1)];
    return FNavLocation(FVector(FVoronoiHelper::GetRandomPointInFace(ChosenFace), ChosenFace->GetLocation().Z));
}
bool AVoronoiNavData::GetRandomReachablePointInRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, FSharedConstNavQueryFilter Filter, const UObject* Querier) const
{
    CheckVoronoi();

    // Return at least origin in case we fail
    OutResult = FNavLocation(Origin);

    TArray<FVoronoiFace*> FacesToChooseFrom = GetReachableFacesInRadius(Origin, Radius);
    if (FacesToChooseFrom.Num() == 0)
        return false;

    FVoronoiFace *ChosenFace = FacesToChooseFrom[FMath::RandRange(0, FacesToChooseFrom.Num() - 1)];

    TArray<FVector2D> Points;
    for (const auto& i : FVoronoiHelper::GetAdjacentVertexes(ChosenFace))
        if (FVector2D::DistSquared(FVector2D(Origin), FVector2D(i->GetLocation())) <= Radius * Radius)
            Points.Add(FVector2D(i->GetLocation()));

    if (Points.Num() < 3)
        OutResult = FNavLocation(FVector(FVoronoiHelper::GetRandomPointInFace(ChosenFace), ChosenFace->GetLocation().Z));
    else
        OutResult = FNavLocation(FVector(FVoronoiHelper::GetRandomPointInPolygon(Points), ChosenFace->GetLocation().Z));

    return true;
}
bool AVoronoiNavData::GetRandomPointInNavigableRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, FSharedConstNavQueryFilter Filter, const UObject* Querier) const
{
    CheckVoronoi();

    // Return at least origin in case we fail
    OutResult = FNavLocation(Origin);

    TArray<FVoronoiFace*> FacesToChooseFrom = GetFacesInRadius(Origin, Radius);
    if (FacesToChooseFrom.Num() == 0)
        return false;

    FVoronoiFace *ChosenFace = FacesToChooseFrom[FMath::RandRange(0, FacesToChooseFrom.Num() - 1)];

    TArray<FVector2D> Points;
    for (const auto& i : FVoronoiHelper::GetAdjacentVertexes(ChosenFace))
        if (FVector2D::DistSquared(FVector2D(Origin), FVector2D(i->GetLocation())) <= Radius * Radius)
            Points.Add(FVector2D(i->GetLocation()));

    if (Points.Num() < 3)
        OutResult = FNavLocation(FVector(FVoronoiHelper::GetRandomPointInFace(ChosenFace), ChosenFace->GetLocation().Z));
    else
        OutResult = FNavLocation(FVector(FVoronoiHelper::GetRandomPointInPolygon(Points), ChosenFace->GetLocation().Z));

    return true;
}

// IMPLEMENTATIONS
FPathFindingResult AVoronoiNavData::FindPathVoronoi(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
    const AVoronoiNavData* Self = static_cast<const AVoronoiNavData*>(Query.NavData.Get());
    Self->CheckVoronoi();

    // Create path
    FPathFindingResult Result;
    Result.Path = Query.PathInstanceToFill.IsValid() ? Query.PathInstanceToFill : Self->CreatePathInstance<FVoronoiNavigationPath>(Query);

    FVoronoiNavPathPtr VPath = StaticCastSharedPtr<FVoronoiNavigationPath>(Result.Path);

    // TODO: Incremental algorithm may be used instead
    VPath->GetPathPoints().Reset();
    VPath->JumpPositions.Reset();

    // Get Voronoi faces start and end locations are in
    FVoronoiFace *FirstVoronoi = Self->GetFaceByPoint(Query.StartLocation);
    FVoronoiFace *LastVoronoi = Self->GetFaceByPoint(Query.EndLocation);
    
    // No voronoi faces found
    if (FirstVoronoi == nullptr || LastVoronoi == nullptr)
        return ENavigationQueryResult::Fail;

    // Get Voronoi querier or fall back to default if failed
    const IVoronoiQuerier* VoronoiQuerier = Cast<const IVoronoiQuerier>(Query.Owner.Get());
    if (VoronoiQuerier == nullptr || !VoronoiQuerier->IsVoronoiQuerier())
        VoronoiQuerier = Self;

    // Add start location
    VPath->AddPathPoint(FirstVoronoi, Query.StartLocation);

    // -------------------------------------------------------------------------------------------------------------
    // A* Algorithm
    // 
    // Pathfinding is performed along Voronoi faces considering their properties, flags and querier's preferences in rotation rate, visibility and etc.
    // 
    // There are two ways faces may be connected: by edge and by link. In the first case adjacent faces
    // can be found using FVoronoiHelper method, in the second one array of linked faces is stored in each face.
    //
    // After the path is found it needs to be postprocessed. That means a sequence of faces needs to be converted into
    // a sequence of FVector. It actually requires a special care - we should choose only that points which lie on
    // an edge between connected faces or that are sites in case of faces being linked because it is the only way
    // to guarantee that a path can be travelled without any raycasts on this stage.
    //
    // Once path is ready it is returned to querier which is (not necassary) supposed to be BotAIController...
    // ------------------------------------------------------------------------------------------------------------

    /** AStar Node */
    struct PRACTICEPROJECT_API FAStarNode final
    {
        FVoronoiFace *Previous, *Current;
        float Weight, Heuristics;

        bool bJumpRequired;

        FAStarNode(FVoronoiFace *InPrevious, FVoronoiFace *InCurrent, float InWeight, float InHeuristics, bool InJumpRequired)
            : Previous(InPrevious), Current(InCurrent), Weight(InWeight), Heuristics(InHeuristics), bJumpRequired(InJumpRequired) {}

        FORCEINLINE bool operator<(const FAStarNode &Right) const { return Weight + Heuristics < Right.Weight + Right.Heuristics; }
    };
    
    // Declare heuristics
    static float(*HeuristicsFunc)(FVoronoiFace *InFace, const FVector &InEnd) = [](FVoronoiFace *InFace, const FVector &InEnd) -> float
    {
        return FVector::Dist(InFace->GetLocation(), InEnd);
    };

    TSet<FVoronoiFace*> Visited;
    TMap<FVoronoiFace*, FVoronoiFace*> Previous;

    // Set of faces that need a jump to travel IN them
    TSet<FVoronoiFace*> JumpRequired;

    TArray<FAStarNode> Queue;
    Queue.HeapPush(FAStarNode(nullptr, FirstVoronoi, 0, HeuristicsFunc(FirstVoronoi, Query.EndLocation), false));

    while (Queue.Num() > 0) 
    {
        FAStarNode Top = Queue.HeapTop();
        Queue.HeapRemoveAt(0);

        if (Visited.Contains(Top.Current))
            continue;
        
        Visited.Add(Top.Current);

        if (Top.bJumpRequired)
            JumpRequired.Add(Top.Current);

        if (Top.Previous != nullptr)
            Previous.Add(Top.Current, Top.Previous);
        if (Top.Current == LastVoronoi)
            break;

        FVoronoiFace *CurrentFace = Top.Current;

        // Collect all faces bot can travel in
        TArray<FVoronoiLink> LinkedFaces(CurrentFace->GetLinks());
        for (FVoronoiFace* i : FVoronoiHelper::GetAdjacentFaces(CurrentFace))
            LinkedFaces.Emplace(i, false);

        for (const FVoronoiLink& Link : LinkedFaces) 
        {
            FVoronoiFace *i = Link.GetFace();
            if (!i->IsNavigable() || Visited.Contains(i))
                continue;

            const float Distance = Self->Distance(VoronoiQuerier, Top.Previous, Top.Current, i, Top.bJumpRequired);
            if (Distance > 0)
                Queue.HeapPush(FAStarNode(Top.Current, i, Top.Weight + Distance, HeuristicsFunc(i, Query.EndLocation), Link.IsJumpRequired()));
        }
    }

    if (!Previous.Contains(LastVoronoi))
    {
        if (!Query.bAllowPartialPaths)
            return ENavigationQueryResult::Fail;
        
        VPath->SetIsPartial(true);

        float Best = FLT_MAX;
        for (FVoronoiFace *i : Visited)
        {
            const float temp = HeuristicsFunc(i, Query.EndLocation);
            if (temp < Best)
                LastVoronoi = i,
                Best = temp;
        }

        // Trivial case
        if (FirstVoronoi == LastVoronoi)
            return ENavigationQueryResult::Success;
    }

    TArray<FVoronoiFace*> InversePath;
    for (FVoronoiFace *Temp = LastVoronoi; Previous.Contains(Temp); Temp = Previous[Temp])
        InversePath.Add(Temp);

    for (int32 i = InversePath.Num() - 1; i >= 0; --i)
    {
        FVector Location = InversePath[i]->GetLocation();

        if (Self->ShouldPostProcessPaths()) 
        {
            // Find edge between faces in path
            FVoronoiEdge* ConnectingEdge = nullptr;
            for (FVoronoiEdge* Edge : FVoronoiHelper::GetAdjacentEdges(Previous[InversePath[i]]))
                if (Edge->GetLeftFace() == InversePath[i] || Edge->GetRightFace() == InversePath[i])
                    ConnectingEdge = Edge;

            if (ConnectingEdge != nullptr)
            {
                const FVector2D FirstPoint(ConnectingEdge->GetFirstVertex()->GetLocation());
                const FVector2D SecondPoint(ConnectingEdge->GetLastVertex()->GetLocation());
                
                /** We should choose point on edge in a dependent from neighbours way in order to get rid of redundant noise on straight areas and moving too far from a center of a turn.
                    Weights of vertexes will be modified according to their distance to a line connecting previous chosen point and a center of the next face */

                FVector A = VPath->GetPathPoints().Last().Location;
                FVector B = (i == 0) ? (VPath->IsPartial() ? LastVoronoi->GetLocation() : Query.EndLocation) : InversePath[i - 1]->GetLocation();
                A.Z = B.Z = 0;

                const float FirstWeight = FMath::FRandRange(1., 2.) / FMath::PointDistToLine(FVector(FirstPoint, 0), B - A, A);
                const float SecondWeight = FMath::FRandRange(1., 2.) / FMath::PointDistToLine(FVector(SecondPoint, 0), B - A, A);

                Location = FVector((FirstPoint * FirstWeight + SecondPoint * SecondWeight) / (FirstWeight + SecondWeight), InversePath[i]->GetLocation().Z);
            }
        }

        if (JumpRequired.Contains(InversePath[i]))
            VPath->JumpPositions.Add(VPath->PathFaces.Last());
        VPath->AddPathPoint(InversePath[i], Location);
    }

    // Add end location
    VPath->AddPathPoint(LastVoronoi, VPath->IsPartial() ? LastVoronoi->GetLocation() : Query.EndLocation);
    VPath->MarkReady();

    Result.Result = ENavigationQueryResult::Success;
    return Result;
}

bool AVoronoiNavData::TestPathVoronoi(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query, int32* NumVisitedNodes)
{
    FPathFindingResult Result = FindPathVoronoi(AgentProperties, Query);
    *NumVisitedNodes = Result.Path->GetPathPoints().Num();
    return Result.IsSuccessful();
}

bool AVoronoiNavData::RaycastVoronoi(const ANavigationData* NavDataInstance, const FVector& RayStart, const FVector& RayEnd, FVector& HitLocation, FSharedConstNavQueryFilter QueryFilter, const UObject* Querier)
{
    return false;
}

float AVoronoiNavData::Distance(const IVoronoiQuerier* Querier, FVoronoiFace *Zeroth, FVoronoiFace *First, FVoronoiFace *Second, bool bJumpRequired)
{
    const auto& FirstFlags = First->GetFlags();
    const auto& SecondFlags = Second->GetFlags();

    const auto& FirstProperties = First->Properties;
    const auto& SecondProperties = Second->Properties;

    if (!Second->IsNavigable() || SecondProperties.bNoWay)
        return -1;

    float Penalty = Querier->GetAdditionalPenalty(First, Second);
    if (Penalty < 0)
        return -1;

    if (Zeroth != nullptr || !Querier->GetInitialRotation().IsNearlyZero())
    {
        const FVector2D Direction = FVector2D(Second->GetLocation() - First->GetLocation()).GetSafeNormal();
        const FVector2D PreviousDirection = Zeroth != nullptr ? FVector2D(First->GetLocation() - Zeroth->GetLocation()).GetSafeNormal() : Querier->GetInitialRotation();
        
        Penalty += (1 - FVector2D::DotProduct(Direction, PreviousDirection)) * FMath::Max(0.f, Querier->GetPenaltyForRotation());
    }

    const float Distance = FVector::Dist(First->GetLocation(), Second->GetLocation());

    if (FirstFlags.bCrouchedOnly || SecondFlags.bCrouchedOnly)
        Penalty += Distance * FMath::Max(1.f, Querier->GetPenaltyMultiplierForCrouch()) * (FirstProperties.BaseCost + SecondProperties.BaseCost) / 2;
    else
        Penalty += Distance * (FirstProperties.BaseCost + SecondProperties.BaseCost) / 2;

    if (bJumpRequired)
        Penalty += FMath::Max(0.f, Querier->GetPenaltyForJump());

    Penalty += SecondProperties.BaseEnterCost;

    return Penalty;
}

// NOT VIRTUAL FUNCTIONS
FVoronoiSurface* AVoronoiNavData::GetSurfaceByPoint(const FVector& Point, bool bProjectPoint) const
{
    CheckVoronoi();

    const float PointZ = bProjectPoint ? ::ProjectPoint(Point, GetWorld()).Z : Point.Z;

    FVoronoiSurface* Answer = nullptr;
    for (const auto& Surface : VoronoiGraph->GetSurfaces())
        if (Surface->Contains(FVector2D(Point)) && Surface->GetLocation().Z > PointZ && (Answer == nullptr || Answer->GetLocation().Z > Surface->GetLocation().Z))
            Answer = Surface.Get();

    return Answer;
}

FVoronoiFace* AVoronoiNavData::GetFaceByPoint(const FVector& Point, bool bProjectPoint) const
{
    CheckVoronoi();

    if (FVoronoiSurface* Surface = GetSurfaceByPoint(Point, bProjectPoint))
        return Surface->GetQuadTree()->GetFaceByPoint(FVector2D(Point));

    return nullptr;
}

TArray<FVoronoiFace*> AVoronoiNavData::GetFacesInRadius(const FVector& Origin, float Radius) const
{
    FVoronoiFace *Top = GetFaceByPoint(Origin);

    if (Top == nullptr)
        return TArray<FVoronoiFace*>();

    TSet<FVoronoiFace*> Visited;

    TQueue<FVoronoiFace*> Queue;
    Queue.Enqueue(Top);

    while (Queue.Dequeue(Top))
    {
        if (Visited.Contains(Top))
            continue;

        Visited.Add(Top);

        TArray<FVoronoiLink> LinkedFaces(Top->GetLinks());
        for (FVoronoiFace* Adj : FVoronoiHelper::GetAdjacentFaces(Top))
            LinkedFaces.Add(FVoronoiLink(Adj, false));

        for (const FVoronoiLink& Link : LinkedFaces)
        {
            FVoronoiFace* j = Link.GetFace();

            if (!Visited.Contains(j))
                Queue.Enqueue(j);
        }
    }

    TArray<FVoronoiFace*> Result;
    for (FVoronoiFace* i : Visited)
        if (i->IsNavigable() && FVector2D::DistSquared(FVector2D(Origin), FVector2D(i->GetLocation())) <= Radius * Radius)
            Result.Add(i);

    return Result;
}

TArray<FVoronoiFace*> AVoronoiNavData::GetReachableFacesInRadius(const FVector& Origin, float Radius) const
{
    FVoronoiFace *Top = GetFaceByPoint(Origin);

    if (Top == nullptr)
        return TArray<FVoronoiFace*>();

    TSet<FVoronoiFace*> Visited;

    TQueue<FVoronoiFace*> Queue;
    Queue.Enqueue(Top);

    while (Queue.Dequeue(Top))
    {
        if (Visited.Contains(Top))
            continue;

        Visited.Add(Top);

        TArray<FVoronoiLink> LinkedFaces(Top->GetLinks());
        for (FVoronoiFace* Adj : FVoronoiHelper::GetAdjacentFaces(Top))
            LinkedFaces.Add(FVoronoiLink(Adj, false));

        for (const FVoronoiLink& Link : LinkedFaces)
        {
            FVoronoiFace *j = Link.GetFace();

            if (j->IsNavigable() && !Visited.Contains(j))
                Queue.Enqueue(j);
        }
    }

    TArray<FVoronoiFace*> Result;
    for (FVoronoiFace* i : Visited)
        if (FVector2D::DistSquared(FVector2D(Origin), FVector2D(i->GetLocation())) <= Radius * Radius)
            Result.Add(i);

    return Result;
}
