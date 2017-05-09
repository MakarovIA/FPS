// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiNavData.h"

#include "VoronoiNavDataGenerator.h"
#include "VoronoiRenderingComponent.h"

/** AVoronoiNavData */

AVoronoiNavData::AVoronoiNavData()
    : HeuristicsScale(.999f), bShowNavigation(false), bSkipInitialRebuild(false)
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

void AVoronoiNavData::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
    Super::TickActor(DeltaTime, TickType, ThisTickFunction);

    const bool bShowNavigationTemp = UVoronoiRenderingComponent::IsNavigationShowFlagSet(GetWorld());
    if (bShowNavigation != bShowNavigationTemp)
    {
        bShowNavigation = bShowNavigationTemp;
        UpdateVoronoiGraphDrawing();
    }
}

void AVoronoiNavData::Serialize(FArchive &Ar)
{
    Super::Serialize(Ar);

    if (NavDataGenerator.IsValid() && NavDataGenerator->IsBuildInProgress())
        NavDataGenerator->EnsureBuildCompletion();

    if (!VoronoiGraph.IsValid())
        VoronoiGraph = MakeUnique<FVoronoiGraph>();

    const bool bSerializationSuccess = VoronoiGraph->Serialize(Ar);
    bSkipInitialRebuild = Ar.IsLoading() ? bSerializationSuccess : bSkipInitialRebuild;
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

void AVoronoiNavData::BeginDestroy()
{
    Super::BeginDestroy();
    CancelBuild();
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
FNavLocation AVoronoiNavData::GetRandomPointInFace(const FVoronoiFace* Face)
{
	TArray<FVector> Points;
	for (const FVoronoiVertex* Vertex : FVoronoiHelper::GetAdjacentVertexes(Face))
		Points.Add(Vertex->Location);

	return FNavLocation(FMathExtended::GetRandomPointInPolygon(Points));
}
FNavLocation AVoronoiNavData::GetRandomPointInFaces(const TArray<const FVoronoiFace*> &Faces)
{
	float TotalArea = 0;
	for (const FVoronoiFace* Face : Faces)
		TotalArea += Face->TacticalProperties.Area;

	float ChosenArea = FMath::FRandRange(0, TotalArea);

	for (const FVoronoiFace* Face : Faces)
		if ((ChosenArea -= Face->TacticalProperties.Area) < 0)
			return GetRandomPointInFace(Face);

	return FNavLocation();
}

FNavLocation AVoronoiNavData::GetRandomPoint(FSharedConstNavQueryFilter Filter, const UObject* Querier) const
{
    CheckVoronoiGraph();

    TArray<const FVoronoiFace*> Faces;
    for (const TPreserveConstUniquePtr<FVoronoiSurface>& Surface : VoronoiGraph->Surfaces)
        Faces.Append(Surface->Faces);

	return GetRandomPointInFaces(Faces);
}
bool AVoronoiNavData::GetRandomReachablePointInRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, FSharedConstNavQueryFilter Filter, const UObject* Querier) const
{
    CheckVoronoiGraph();

    TArray<const FVoronoiFace*> Faces = GetReachableFacesInRadius(Origin, Radius);
    OutResult = GetRandomPointInFaces(Faces);

    return Faces.Num() != 0;
}
bool AVoronoiNavData::GetRandomPointInNavigableRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, FSharedConstNavQueryFilter Filter, const UObject* Querier) const
{
    CheckVoronoiGraph();

    TArray<const FVoronoiFace*> Faces = GetFacesInRadius(Origin, Radius);
    OutResult = GetRandomPointInFaces(Faces);

    return Faces.Num() != 0;
}

// IMPLEMENTATIONS
FPathFindingResult AVoronoiNavData::FindPathVoronoi(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
    struct FAStarNode
    {
        const FVoronoiFace *Previous, *Current;
        float Weight, Heuristics;

        bool bJumpRequired;

        FORCEINLINE FAStarNode(const FVoronoiFace *InPrevious, const FVoronoiFace *InCurrent, float InWeight, float InHeuristics, bool InJumpRequired)
            : Previous(InPrevious), Current(InCurrent), Weight(InWeight), Heuristics(InHeuristics), bJumpRequired(InJumpRequired) {}

        FORCEINLINE bool operator<(const FAStarNode &Right) const { return Weight + Heuristics < Right.Weight + Right.Heuristics; }
    };

    const AVoronoiNavData* Self = static_cast<const AVoronoiNavData*>(Query.NavData.Get()); Self->CheckVoronoiGraph();
    FVoronoiNavPathPtr VoronoiPath = StaticCastSharedPtr<FVoronoiNavigationPath>(Self->CreatePathInstance<FVoronoiNavigationPath>(Query));

    const IVoronoiQuerier* VoronoiQuerier = Cast<const IVoronoiQuerier>(Query.Owner.Get());
    if (VoronoiQuerier == nullptr || !VoronoiQuerier->IsVoronoiQuerier())
        VoronoiQuerier = Self;

    const FVoronoiFace *FirstVoronoi = Self->GetFaceByPoint(Query.StartLocation);
    const FVoronoiFace *LastVoronoi = Self->GetFaceByPoint(Query.EndLocation);
    
    if (FirstVoronoi == nullptr || LastVoronoi == nullptr)
        return ENavigationQueryResult::Fail;

    VoronoiPath->AddPathPoint(FirstVoronoi, Query.StartLocation, false);

    TSet<const FVoronoiFace*> Visited;
    TSet<const FVoronoiFace*> JumpRequired;
    TMap<const FVoronoiFace*, const FVoronoiFace*> Previous;

    TArray<FAStarNode> Queue;
    Queue.HeapPush(FAStarNode(nullptr, FirstVoronoi, 0, FVector::Dist(FirstVoronoi->Location, Query.EndLocation) * Self->GetHeuristicsScale(), false));

    while (Queue.Num() > 0)
    {
        FAStarNode Top = Queue.HeapTop();
        Queue.HeapRemoveAt(0, false);

        bool AlreadyVisited;
        if (Visited.Add(Top.Current, &AlreadyVisited), AlreadyVisited)
            continue;

        if (Top.bJumpRequired) JumpRequired.Add(Top.Current);
        if (Top.Previous)      Previous.Add(Top.Current, Top.Previous);

        if (Top.Current == LastVoronoi)
            break;

        TArray<FVoronoiLink> LinkedFaces(Top.Current->Links);
        for (const FVoronoiFace* Face : FVoronoiHelper::GetAdjacentFaces(Top.Current))
            LinkedFaces.Emplace(Face, false);

        float Distance;
        for (const FVoronoiLink& Link : LinkedFaces)
            if (!Visited.Contains(Link.Face) && (Distance = Self->Distance(VoronoiQuerier, Top.Previous, Top.Current, Link.Face, Top.bJumpRequired)) > 0)
                Queue.HeapPush(FAStarNode(Top.Current, Link.Face, Top.Weight + Distance, FVector::Dist(Link.Face->Location, Query.EndLocation) * Self->GetHeuristicsScale(), Link.bJumpRequired != 0));
    }

    if (!Previous.Contains(LastVoronoi))
    {
        if (!Query.bAllowPartialPaths)
            return ENavigationQueryResult::Fail;
        
        VoronoiPath->SetIsPartial(true);

        float Best = FLT_MAX, Temp;
        for (const FVoronoiFace *Face : Visited)
            if ((Temp = FVector::DistSquared(Face->Location, Query.EndLocation)) < Best)
                LastVoronoi = Face, Best = Temp;
    }

    TArray<const FVoronoiFace*> InversePath;
    for (const FVoronoiFace *Temp = LastVoronoi; Previous.Contains(Temp); Temp = Previous[Temp])
        InversePath.Add(Temp);

    for (int32 i = InversePath.Num() - 1; i >= 0; --i)
    {
        FVector Location = InversePath[i]->Location;

		const FVoronoiEdge* ConnectingEdge = nullptr;
        for (const FVoronoiEdge* Edge : FVoronoiHelper::GetAdjacentEdges(Previous[InversePath[i]]))
            if (Edge->LeftFace == InversePath[i] || Edge->RightFace == InversePath[i])
                ConnectingEdge = Edge;

        if (ConnectingEdge != nullptr && (i == 0 || !JumpRequired.Contains(InversePath[i - 1])))
        {
            const FVector2D FirstPoint(ConnectingEdge->FirstVertex->Location);
            const FVector2D SecondPoint(ConnectingEdge->LastVertex->Location);
                
            FVector A = VoronoiPath->GetPathPoints().Last().Location;
            FVector B = (i == 0) ? (VoronoiPath->IsPartial() ? LastVoronoi->Location : Query.EndLocation) : InversePath[i - 1]->Location;
            A.Z = B.Z = 0;

            const float FirstWeight = FMath::FRandRange(1., 2.) / FMath::PointDistToLine(FVector(FirstPoint, 0), B - A, A);
            const float SecondWeight = FMath::FRandRange(1., 2.) / FMath::PointDistToLine(FVector(SecondPoint, 0), B - A, A);

            Location = FVector((FirstPoint * FirstWeight + SecondPoint * SecondWeight) / (FirstWeight + SecondWeight), InversePath[i]->Location.Z);
        }

        VoronoiPath->AddPathPoint(InversePath[i], Location, JumpRequired.Contains(InversePath[i]));
    }

    VoronoiPath->AddPathPoint(LastVoronoi, VoronoiPath->IsPartial() ? LastVoronoi->Location : Query.EndLocation, false);
    VoronoiPath->MarkReady();

	FPathFindingResult Result(ENavigationQueryResult::Success);
	Result.Path = VoronoiPath;
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

float AVoronoiNavData::Distance(const IVoronoiQuerier* Querier, const FVoronoiFace *Zeroth, const FVoronoiFace *First, const FVoronoiFace *Second, bool bJumpRequired)
{
    float Penalty;
    if ((Penalty = Querier->GetAdditionalPenalty(First, Second)) < 0)
        return -1;

    if (Zeroth != nullptr || !Querier->GetInitialRotation().IsNearlyZero())
    {
        const FVector2D Direction = FVector2D(Second->Location - First->Location).GetSafeNormal();
        const FVector2D PreviousDirection = Zeroth != nullptr ? FVector2D(First->Location - Zeroth->Location).GetSafeNormal() : Querier->GetInitialRotation();
        
        Penalty += (1 - FVector2D::DotProduct(Direction, PreviousDirection)) * FMath::Max(0.f, Querier->GetVoronoiQuerierParameters().PenaltyForRotation);
    }

    if (First->Flags.bCrouchedOnly || Second->Flags.bCrouchedOnly)
        Penalty += FVector::Dist(First->Location, Second->Location) * FMath::Max(1.f, Querier->GetVoronoiQuerierParameters().PenaltyMultiplierForCrouch);

    if (bJumpRequired)
        Penalty += FMath::Max(0.f, Querier->GetVoronoiQuerierParameters().PenaltyForJump);

    return Penalty;
}

// NOT VIRTUAL FUNCTIONS
void AVoronoiNavData::OnVoronoiNavDataGenerationFinished()
{
	if (GetWorld() && GetWorld()->GetNavigationSystem())
		GetWorld()->GetNavigationSystem()->OnNavigationGenerationFinished(*this);

    for (FNavPathWeakPtr& Path : ActivePaths.FilterByPredicate([](const FNavPathWeakPtr& InPath) { return InPath.IsValid(); }))
		RequestRePath(Path.Pin(), ENavPathUpdateType::NavigationChanged);

	if (UVoronoiRenderingComponent::IsNavigationShowFlagSet(GetWorld()))
		UpdateVoronoiGraphDrawing();
}

const FVoronoiFace* AVoronoiNavData::GetFaceByPoint(const FVector& Point, bool bProjectPoint) const
{
    CheckVoronoiGraph();

	const float PointZ = bProjectPoint ? ::ProjectPoint(Point, GetWorld()).Z : Point.Z;

	const FVoronoiFace* Answer = nullptr;
	for (const TPreserveConstUniquePtr<FVoronoiSurface>& Surface : VoronoiGraph->Surfaces)
		if (const FVoronoiFace *Candidate = Surface->QuadTree->GetFaceByPoint(FVector2D(Point)))
			if (FMath::Abs(Candidate->Location.Z - PointZ) < 100.f && (Answer == nullptr || Answer->Location.Z > Candidate->Location.Z))
				Answer = Candidate;

    return Answer;
}

FVoronoiFace* AVoronoiNavData::GetFaceByPoint(const FVector& Point, bool bProjectPoint)
{
    CheckVoronoiGraph();

    const float PointZ = bProjectPoint ? ::ProjectPoint(Point, GetWorld()).Z : Point.Z;

    FVoronoiFace* Answer = nullptr;
    for (TPreserveConstUniquePtr<FVoronoiSurface>& Surface : VoronoiGraph->Surfaces)
        if (FVoronoiFace *Candidate = Surface->QuadTree->GetFaceByPoint(FVector2D(Point)))
            if (FMath::Abs(Candidate->Location.Z - PointZ) < 100.f && (Answer == nullptr || Answer->Location.Z > Candidate->Location.Z))
                Answer = Candidate;

    return Answer;
}

TArray<const FVoronoiFace*> AVoronoiNavData::GetFacesInRadius(const FVector& Origin, float Radius) const
{
    TArray<const FVoronoiFace*> Result;
    for (const TPreserveConstUniquePtr<FVoronoiSurface> &Surface : VoronoiGraph->Surfaces)
        Result.Append(Surface->QuadTree->GetFacesInCircle(FVector2D(Origin), Radius));

    return Result;
}

TArray<const FVoronoiFace*> AVoronoiNavData::GetReachableFacesInRadius(const FVector& Origin, float Radius) const
{
	const FVoronoiFace *Top;
    if ((Top = GetFaceByPoint(Origin)) == nullptr)
        return TArray<const FVoronoiFace*>();

    TSet<const FVoronoiFace*> Visited;
    TQueue<const FVoronoiFace*> Queue;

    Queue.Enqueue(Top);
    while (Queue.Dequeue(Top))
    {
        if (Visited.Contains(Top))
            continue;

        Visited.Add(Top);

        TArray<FVoronoiLink> LinkedFaces(Top->Links);
        for (const FVoronoiFace* Face : FVoronoiHelper::GetAdjacentFaces(Top))
            LinkedFaces.Emplace(Face, false);

        for (const FVoronoiLink& Link : LinkedFaces)
            if (!Visited.Contains(Link.Face))
                Queue.Enqueue(Link.Face);
    }

    TArray<const FVoronoiFace*> Result;
    for (const FVoronoiFace* Face : Visited)
        if (FVector::DistSquaredXY(Origin, Face->Location) <= Radius * Radius)
            Result.Add(Face);

    return Result;
}

// BLUEPRINTS
AVoronoiNavData* AVoronoiNavData::GetVoronoiNavData(const UObject* WorldContextObject)
{
    if (UWorld* World = WorldContextObject->GetWorld())
        if (TActorIterator<AVoronoiNavData> Iter = TActorIterator<AVoronoiNavData>(World))
            return *Iter;

    return nullptr;
}
