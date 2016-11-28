// By Polyakov Pavel

#pragma once

#include "VoronoiGraph.h"
#include "VoronoiQuerier.h"
#include "VoronoiPath.h"

#include "VoronoiNavData.generated.h"

class UVoronoiRenderingComponent;
class FVoronoiNavDataAttorney;

class FVoronoiNavDataGenerator;

/** Fallback values for the case of not IVoronoiQuerier */
USTRUCT()
struct PRACTICEPROJECT_API FVoronoiQuerierDefaults
{
    GENERATED_USTRUCT_BODY()

    /** Penalty value for rotation used in pathfinding */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi, meta = (ClampMin = "0.0"))
    float PenaltyForRotation;

    /** Penalty mutiplier for crouching on the way */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi, meta = (ClampMin = "1.0"))
    float PenaltyMultiplierForCrouch;

    /** Penalty value for jump */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi, meta = (ClampMin = "0.0"))
    float PenaltyForJump;

    FVoronoiQuerierDefaults() : PenaltyForRotation(200), PenaltyMultiplierForCrouch(1), PenaltyForJump(0) {}
};

/** Drawing modes */
UENUM(BlueprintType)
enum class EVoronoiFaceDrawingMode : uint8
{
    VFDM_Standard                    UMETA(DisplayName = "Standard"),
    VFDM_Visibility                  UMETA(DisplayName = "Visibility"),
    VFDM_CloseRangeVisibility        UMETA(DisplayName = "Close Range Visibility"),
    VFDM_FarRangeVisibility          UMETA(DisplayName = "Far Range Visibility"),
    VFDM_SouthVisibility             UMETA(DisplayName = "South Visibility"),
    VFDM_WestVisibility              UMETA(DisplayName = "West Visibility"),
    VFDM_EastVisibility              UMETA(DisplayName = "East Visibility"),
    VFDM_NorthVisibility             UMETA(DisplayName = "North Visibility"),
    VFDM_SEVisibility                UMETA(DisplayName = "South-East Visibility"),
    VFDM_SWVisibility                UMETA(DisplayName = "South-West Visibility"),
    VFDM_NEVisibility                UMETA(DisplayName = "North-East Visibility"),
    VFDM_NWVisibility                UMETA(DisplayName = "North-West Visibility"),

    VFDM_None                        UMETA(DisplayName = "None")
};

/** Options on how Voronoi navigation mesh is rendered */
USTRUCT()
struct PRACTICEPROJECT_API FVoronoiDrawingOptions
{
    GENERATED_USTRUCT_BODY()

    /** Indicates the way Faces are rendered */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Display)
    EVoronoiFaceDrawingMode FaceDrawingMode;

    /** Indicates whether to draw voronoi sites */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Display)
    bool bDrawSites;

    /** Indicates whether to draw surfaces */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Display)
    bool bDrawSurfaces;

    /** Indicates whether to draw edges */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Display)
    bool bDrawEdges;

    /** Indicates whether to draw links */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Display)
    bool bDrawLinks;

    /** Indicates whether to draw quadtrees */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Display)
    bool bDrawQuadTrees;

    FVoronoiDrawingOptions() : FaceDrawingMode(EVoronoiFaceDrawingMode::VFDM_Standard), bDrawSites(true), bDrawSurfaces(true), bDrawEdges(true), bDrawLinks(false), bDrawQuadTrees(false) {}
};

/** Navigation based on the use of Voronoi diagrams */
UCLASS(HideCategories = (Input, Rendering, Actor, Runtime, Transform), NotPlaceable)
class PRACTICEPROJECT_API AVoronoiNavData : public ANavigationData, public IVoronoiQuerier
{
    GENERATED_BODY()

    /** Fallback values if an object making query is not IVoronoiQuerier */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi, meta = (AllowPrivateAccess = "true"))
    FVoronoiQuerierDefaults VoronoiQuerierDefaults;

    /** Indicates whether to choose way points on edges or simply use site's locations */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = Voronoi, meta = (AllowPrivateAccess = "true"))
    bool bPostProcessPaths;

    /** Indicates whether to start automatic rebuild when voronoi sites were moved */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Generation, meta = (AllowPrivateAccess = "true"))
    bool bLiveGeneration;

    /** Indicates whether to log Voronoi graph info on successful generation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Generation, meta = (AllowPrivateAccess = "true"))
    bool bEnableLog;

    /** Indicates the way Voronoi navigation mesh is rendered  */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Display, meta = (AllowPrivateAccess = "true"))
    FVoronoiDrawingOptions DrawingOptions;

    // -------------------------------------------------------------------------
    // Internal variables
    // -------------------------------------------------------------------------

    TUniquePtr<FVoronoiGraph> VoronoiGraph;

    bool bShowNavigation;
    bool bUpdateDrawing;
    bool bUpdatePaths;
    
    void CheckVoronoi() const;

    friend FVoronoiNavDataAttorney;
    friend FVoronoiNavDataGenerator;

public:
    AVoronoiNavData();

    /** Tick function */
    virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

    /** Voronoi navigation data serialization */
    virtual void Serialize(FArchive &Ar) override;
    
    /** Construct rendering component for Voronoi navigation data */
    virtual UPrimitiveComponent* ConstructRenderingComponent() override;

    virtual bool SupportsRuntimeGeneration() const override;
    virtual void ConditionalConstructGenerator() override;

    /** Generator calls this when a graph is ready. Used for logs and debug drawing */
    virtual void OnVoronoiNavDataGenerationFinished(double TimeSpent);

    /** Simply assigns Point parameter to OutLocation */
    virtual bool ProjectPoint(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;
    
    /** Always returns false */
    virtual bool DoesNodeContainLocation(NavNodeRef NodeRef, const FVector& WorldSpaceLocation) const override { return false; }

    // -----------------------------------
    // PATH COST AND LENGTH CALCULATIONS
    // -----------------------------------
    virtual ENavigationQueryResult::Type CalcPathCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathCost, FSharedConstNavQueryFilter QueryFilter = NULL, const UObject* Querier = NULL) const override;
    virtual ENavigationQueryResult::Type CalcPathLength(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, FSharedConstNavQueryFilter QueryFilter = NULL, const UObject* Querier = NULL) const override;
    virtual ENavigationQueryResult::Type CalcPathLengthAndCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, float& OutPathCost, FSharedConstNavQueryFilter QueryFilter = NULL, const UObject* Querier = NULL) const override;

    // ----------------------------------
    // RANDOM POINTS GETTERS
    // ----------------------------------
    virtual FNavLocation GetRandomPoint(FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;
    virtual bool GetRandomReachablePointInRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;
    virtual bool GetRandomPointInNavigableRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;
    
    // ----------------------------------
    // EMPTY FUNCTIONS
    // ----------------------------------
    virtual void BatchRaycast(TArray<FNavigationRaycastWork>& Workload, FSharedConstNavQueryFilter QueryFilter, const UObject* Querier = NULL) const override {};
    virtual void BatchProjectPoints(TArray<FNavigationProjectionWork>& Workload, const FVector& Extent, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override {};
    virtual void OnNavAreaAdded(const UClass* NavAreaClass, int32 AgentIndex) override {}
    virtual void OnNavAreaRemoved(const UClass* NavAreaClass) override {};

    // ----------------------------------
    // IMPLEMENTATIONS
    // ----------------------------------
    static FPathFindingResult FindPathVoronoi(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
    static bool TestPathVoronoi(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query, int32* NumVisitedNodes);
    static bool RaycastVoronoi(const ANavigationData* NavDataInstance, const FVector& RayStart, const FVector& RayEnd,
        FVector& HitLocation, FSharedConstNavQueryFilter QueryFilter, const UObject* Querier);
    
    /** Calculate cost of travel between two faces. If returned value is negative then there is no way */
    static float Distance(const IVoronoiQuerier* Querier, FVoronoiFace *Zeroth, FVoronoiFace *First, FVoronoiFace *Second, bool bJumpRequired);

    // ----------------------------------
    // NOT VIRTUAL FUNCIONS
    // ----------------------------------

    /** Determine surface by a given point. Can be null */
    FVoronoiSurface* GetSurfaceByPoint(const FVector& Point, bool bProjectPoint = true) const;

    /** Determine Face by a given point. Can be null */
    FVoronoiFace* GetFaceByPoint(const FVector& Point, bool bProjectPoint = true) const;

    /** Get Faces in a given radius */
    TArray<FVoronoiFace*> GetFacesInRadius(const FVector& Origin, float Radius) const;

    /** Get Faces in a given radius that can be reached from the origin */
    TArray<FVoronoiFace*> GetReachableFacesInRadius(const FVector& Origin, float Radius) const;

    /** Causes drawing update */
    FORCEINLINE void UpdateVoronoiGraphDrawing() { bUpdateDrawing = true; }

    /** Mark all active paths as needing update */
    FORCEINLINE void MarkPathsAsNeedingUpdate() { bUpdatePaths = true; }

    // -----------------------------------
    // IVoronoiQuerier interface begin
    // -----------------------------------

    /** Get a penalty value for rotation used in pathfinding */
    virtual float GetPenaltyForRotation() const override { return VoronoiQuerierDefaults.PenaltyForRotation; }

    /** Get penalty mutiplier for crouching on the way */
    virtual float GetPenaltyMultiplierForCrouch() const override { return VoronoiQuerierDefaults.PenaltyMultiplierForCrouch; }

    /** Get penalty mutiplier for jumps on the way */
    virtual float GetPenaltyForJump() const override { return VoronoiQuerierDefaults.PenaltyForJump; }

    // -----------------------------------
    // GET FUNCTIONS
    // -----------------------------------

    FORCEINLINE FVoronoiGraph* GetVoronoiGraph() const { CheckVoronoi(); return VoronoiGraph.Get(); }

    FORCEINLINE bool ShouldPostProcessPaths() const { return bPostProcessPaths; }
    FORCEINLINE bool IsLiveGenerated() const { return bLiveGeneration; }
    FORCEINLINE bool IsLogEnabled() const { return bEnableLog; }

    FORCEINLINE const FVoronoiDrawingOptions& GetDrawingOptions() const { return DrawingOptions; }
};

/** Proxy to access a single private field */
class PRACTICEPROJECT_API FVoronoiNavDataAttorney
{
    friend UVoronoiRenderingComponent;
    static FORCEINLINE const FVoronoiGraph* GetVoronoiGraphUnchecked(const AVoronoiNavData* VoronoiNavData) { return VoronoiNavData->VoronoiGraph.Get(); }
};
