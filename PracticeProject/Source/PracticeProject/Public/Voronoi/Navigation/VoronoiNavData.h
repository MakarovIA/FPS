// By Polyakov Pavel

#pragma once

#include "VoronoiGraph.h"
#include "VoronoiQuerier.h"
#include "VoronoiNavigationPath.h"

#include "VoronoiNavData.generated.h"

class UVoronoiRenderingComponent;
class FVoronoiNavDataAttorney;

class FVoronoiNavDataGenerator;

/** Options on how Voronoi navigation mesh is generated */
USTRUCT()
struct PRACTICEPROJECT_API FVoronoiGenerationOptions
{
    GENERATED_USTRUCT_BODY()

    /** Indicates precision used to voxelize geometry */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "20.0", ClampMax = "40.0"))
    float CellSize;

    /** Indicates radius of navigation agent */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "20.0", ClampMax = "80.0"))
    float AgentRadius;

    /** Indicates height of navigation agent */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "100.0", ClampMax = "200.0"))
    float AgentHeight;

    /** Indicates crouched height of navigation agent */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "100.0", ClampMax = "200.0"))
    float AgentCrouchedHeight;

    /** Indicates step height of navigation agent */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "50.0"))
    float AgentStepHeight;

    /** Indicates walkable slope of navigation agent */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "60.0"))
    float AgentWalkableSlope;

    /** Indicates space between navigable area border and obstacle */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "50.0"))
    float BorderIndent;

    /** Indicates precision used to interpolate borders */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "40.0"))
    float MaxBorderDeviation;

    /** Indicates density of Voronoi sites */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SitesPerSquareMeter;

    /** Indicates whether to draw links */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1000.0"))
    float LinksSearchRadius;

    /** Indicates whether to draw surfaces */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "5000.0"))
    float VisibilityRadius;

    FORCEINLINE FVoronoiGenerationOptions()
        : CellSize(20.f), AgentRadius(42.f), AgentHeight(192.f), AgentCrouchedHeight(128.f), AgentStepHeight(35.f), AgentWalkableSlope(45.f)
        , BorderIndent(0.f), MaxBorderDeviation(25.f), SitesPerSquareMeter(0.4f), LinksSearchRadius(500.f), VisibilityRadius(2000.f) {}
};

/** Drawing modes */
UENUM(BlueprintType)
enum class EVoronoiFaceDrawingMode : uint8
{
    Standard                    UMETA(DisplayName = "Standard"),
    Visibility                  UMETA(DisplayName = "Visibility"),

    SVisibility                 UMETA(DisplayName = "South Visibility"),
    WVisibility                 UMETA(DisplayName = "West Visibility"),
    EVisibility                 UMETA(DisplayName = "East Visibility"),
    NVisibility                 UMETA(DisplayName = "North Visibility"),

    SEVisibility                UMETA(DisplayName = "South-East Visibility"),
    SWVisibility                UMETA(DisplayName = "South-West Visibility"),
    NEVisibility                UMETA(DisplayName = "North-East Visibility"),
    NWVisibility                UMETA(DisplayName = "North-West Visibility"),

	FragMapStatistic            UMETA(DisplayName = "Kill/Death statistic"),

    None                        UMETA(DisplayName = "None")
};

/** Options on how Voronoi navigation mesh is rendered */
USTRUCT()
struct PRACTICEPROJECT_API FVoronoiDrawingOptions
{
    GENERATED_USTRUCT_BODY()

    /** Indicates the way faces are rendered */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EVoronoiFaceDrawingMode FaceDrawingMode;

	/*  Indicates statistics for which EStatisticKey is displayed (if FaceDrawingMode == FragMapStatistic)*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EStatisticKey statkey;

    /** Indicates whether to draw edges */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bDrawEdges;

    /** Indicates whether to draw links */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bDrawLinks;

    /** Indicates whether to draw surfaces */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bDrawSurfaces;

    /** Material used to render Voronoi navigation mesh */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UMaterialInterface* VoronoiMaterial;

    FORCEINLINE FVoronoiDrawingOptions()
        : FaceDrawingMode(EVoronoiFaceDrawingMode::Standard), bDrawEdges(true), bDrawLinks(false), bDrawSurfaces(false)
        , VoronoiMaterial(static_cast<UMaterialInterface*>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/Voronoi/VoronoiMaterial.VoronoiMaterial"), nullptr, LOAD_None, nullptr))) {}
};

/** Navigation based on the use of Voronoi diagrams */
UCLASS(HideCategories = (Input, Rendering, Actor, Transform), NotPlaceable)
class PRACTICEPROJECT_API AVoronoiNavData : public ANavigationData, public IVoronoiQuerier
{
    GENERATED_BODY()

    /** Fallback values if an object making query is not IVoronoiQuerier */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi, meta = (AllowPrivateAccess = "true"))
    FVoronoiQuerierParameters VoronoiQuerierDefaults;

    /** Indicates heuristics scale used in pathfinding */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi, meta = (ClampMin = "0.1", ClampMax = "10.0", AllowPrivateAccess = "true"))
    float HeuristicsScale;

    /** Indicates the way Voronoi navigation mesh is generated  */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation, meta = (AllowPrivateAccess = "true"))
    FVoronoiGenerationOptions GenerationOptions;

    /** Indicates the way Voronoi navigation mesh is rendered  */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Display, meta = (AllowPrivateAccess = "true"))
    FVoronoiDrawingOptions DrawingOptions;

    bool bShowNavigation, bSkipInitialRebuild;
    TUniquePtr<FVoronoiGraph> VoronoiGraph;

	FORCEINLINE void CheckVoronoiGraph() const
	{
		if (!VoronoiGraph.IsValid())
		{
			NavDataGenerator->RebuildAll();
			NavDataGenerator->EnsureBuildCompletion();
		}
	}

    friend FVoronoiNavDataAttorney;
    friend FVoronoiNavDataGenerator;

public:
    AVoronoiNavData();

    virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
    virtual void Serialize(FArchive &Ar) override;

    virtual UPrimitiveComponent* ConstructRenderingComponent() override;

    virtual bool SupportsRuntimeGeneration() const override;
    virtual void ConditionalConstructGenerator() override;

    virtual void BeginDestroy() override;

    virtual bool ProjectPoint(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;
    virtual bool DoesNodeContainLocation(NavNodeRef NodeRef, const FVector& WorldSpaceLocation) const override { return false; }

    // -------------------------------------------------------------------------------------------------------
    // PATH COST AND LENGTH CALCULATIONS
    // -------------------------------------------------------------------------------------------------------

    virtual ENavigationQueryResult::Type CalcPathCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathCost, FSharedConstNavQueryFilter QueryFilter = NULL, const UObject* Querier = NULL) const override;
    virtual ENavigationQueryResult::Type CalcPathLength(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, FSharedConstNavQueryFilter QueryFilter = NULL, const UObject* Querier = NULL) const override;
    virtual ENavigationQueryResult::Type CalcPathLengthAndCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, float& OutPathCost, FSharedConstNavQueryFilter QueryFilter = NULL, const UObject* Querier = NULL) const override;

    // -------------------------------------------------------------------------------------------------------
    // RANDOM POINTS GETTERS
    // -------------------------------------------------------------------------------------------------------

	static FNavLocation GetRandomPointInFace(const FVoronoiFace* Face);
	static FNavLocation GetRandomPointInFaces(const TArray<const FVoronoiFace*> &Faces);

    virtual FNavLocation GetRandomPoint(FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;
    virtual bool GetRandomReachablePointInRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;
    virtual bool GetRandomPointInNavigableRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;
    
    // -------------------------------------------------------------------------------------------------------
    // EMPTY FUNCTIONS
    // -------------------------------------------------------------------------------------------------------

    virtual void BatchRaycast(TArray<FNavigationRaycastWork>& Workload, FSharedConstNavQueryFilter QueryFilter, const UObject* Querier = NULL) const override {};
    virtual void BatchProjectPoints(TArray<FNavigationProjectionWork>& Workload, const FVector& Extent, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override {};
    virtual void OnNavAreaAdded(const UClass* NavAreaClass, int32 AgentIndex) override {}
    virtual void OnNavAreaRemoved(const UClass* NavAreaClass) override {};

    // -------------------------------------------------------------------------------------------------------
    // IMPLEMENTATIONS
    // -------------------------------------------------------------------------------------------------------

    static FPathFindingResult FindPathVoronoi(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
    static bool TestPathVoronoi(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query, int32* NumVisitedNodes);
    static bool RaycastVoronoi(const ANavigationData* NavDataInstance, const FVector& RayStart, const FVector& RayEnd,
        FVector& HitLocation, FSharedConstNavQueryFilter QueryFilter, const UObject* Querier);
    
    /** Calculate cost of travel between two faces. If returned value is negative then there is no way */
    static float Distance(const IVoronoiQuerier* Querier, const FVoronoiFace *Zeroth, const FVoronoiFace *First, const FVoronoiFace *Second, bool bJumpRequired);

    // -------------------------------------------------------------------------------------------------------
    // NOT VIRTUAL FUNCIONS
    // -------------------------------------------------------------------------------------------------------

    FORCEINLINE bool ShouldSkipInitialRebuild() const { return bSkipInitialRebuild; }
    FORCEINLINE void OnInitialRebuildSkipped() { bSkipInitialRebuild = false; }

    void OnVoronoiNavDataGenerationFinished();

    const FVoronoiFace* GetFaceByPoint(const FVector& Point, bool bProjectPoint = true) const;
    FVoronoiFace* GetFaceByPoint(const FVector& Point, bool bProjectPoint = true);

    TArray<const FVoronoiFace*> GetFacesInRadius(const FVector& Origin, float Radius) const;
    TArray<const FVoronoiFace*> GetReachableFacesInRadius(const FVector& Origin, float Radius) const;

    FORCEINLINE void UpdateVoronoiGraphDrawing()
	{
		if (RenderingComp && RenderingComp->IsVisible()) 
			RenderingComp->MarkRenderStateDirty();
	}

    // -------------------------------------------------------------------------------------------------------
    // IVoronoiQuerier interface
    // -------------------------------------------------------------------------------------------------------

    virtual const FVoronoiQuerierParameters& GetVoronoiQuerierParameters() const override { return VoronoiQuerierDefaults; }

    // -------------------------------------------------------------------------------------------------------
    // BLUEPRINTS
    // -------------------------------------------------------------------------------------------------------

    /**
    * Retrieves AVoronoiNavData from Navigation System
    * @return VoronoiNavData or NULL if it is not present
    */
    UFUNCTION(BlueprintPure, Category = VoronoiPropertiesManager, meta = (WorldContext = "WorldContextObject"))
    static AVoronoiNavData* GetVoronoiNavData(const UObject* WorldContextObject);

    // -------------------------------------------------------------------------------------------------------
    // GET FUNCTIONS
    // -------------------------------------------------------------------------------------------------------

    FORCEINLINE const FVoronoiGenerationOptions& GetGenerationOptions() const { return GenerationOptions; }
    FORCEINLINE const FVoronoiDrawingOptions& GetDrawingOptions() const { return DrawingOptions; }

    FORCEINLINE float GetHeuristicsScale() const { return HeuristicsScale; }
    FORCEINLINE FVoronoiGraph* GetVoronoiGraph() const { CheckVoronoiGraph(); return VoronoiGraph.Get(); }
};

/** Proxy to access voronoi graph while it is generated */
class PRACTICEPROJECT_API FVoronoiNavDataAttorney
{
    friend UVoronoiRenderingComponent;
    static FORCEINLINE const FVoronoiGraph* GetVoronoiGraphUnchecked(const AVoronoiNavData* VoronoiNavData) { return VoronoiNavData->VoronoiGraph.Get(); }
};
