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

    /** Indicates density of Voronoi sites */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SitesPerSquareMeter;

    /** Indicates precision used to interpolate borders */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float MaxBorderDeviation;

    /** Indicates whether to draw links */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1000.0"))
    float LinksSearchRadius;

    /** Indicates whether to draw surfaces */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "10000.0"))
    float VisibilityRadius;

    FORCEINLINE FVoronoiGenerationOptions()
        : SitesPerSquareMeter(0.5f), MaxBorderDeviation(20.f), LinksSearchRadius(500.f), VisibilityRadius(2000.f) {}
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
        , VoronoiMaterial(Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/Voronoi/VoronoiMaterial.VoronoiMaterial"), nullptr, LOAD_None, nullptr))) {}
};

/** Navigation based on the use of Voronoi diagrams */
UCLASS(HideCategories = (Input, Rendering, Actor, Runtime, Transform), NotPlaceable)
class PRACTICEPROJECT_API AVoronoiNavData : public ANavigationData, public IVoronoiQuerier
{
    GENERATED_BODY()

    /** Fallback values if an object making query is not IVoronoiQuerier */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi, meta = (AllowPrivateAccess = "true"))
    FVoronoiQuerierParameters VoronoiQuerierDefaults;

    /** Indicates the way Voronoi navigation mesh is generated  */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation, meta = (AllowPrivateAccess = "true"))
    FVoronoiGenerationOptions GenerationOptions;

    /** Indicates the way Voronoi navigation mesh is rendered  */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Display, meta = (AllowPrivateAccess = "true"))
    FVoronoiDrawingOptions DrawingOptions;

    bool bShowNavigation = false;
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

    // -----------------------------------
    // PATH COST AND LENGTH CALCULATIONS
    // -----------------------------------

    virtual ENavigationQueryResult::Type CalcPathCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathCost, FSharedConstNavQueryFilter QueryFilter = NULL, const UObject* Querier = NULL) const override;
    virtual ENavigationQueryResult::Type CalcPathLength(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, FSharedConstNavQueryFilter QueryFilter = NULL, const UObject* Querier = NULL) const override;
    virtual ENavigationQueryResult::Type CalcPathLengthAndCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, float& OutPathCost, FSharedConstNavQueryFilter QueryFilter = NULL, const UObject* Querier = NULL) const override;

    // ----------------------------------
    // RANDOM POINTS GETTERS
    // ----------------------------------

	static FNavLocation GetRandomPointInFace(const FVoronoiFace* Face);
	static FNavLocation GetRandomPointInFaces(const TArray<const FVoronoiFace*> &Faces);

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
    static float Distance(const IVoronoiQuerier* Querier, const FVoronoiFace *Zeroth, const FVoronoiFace *First, const FVoronoiFace *Second, bool bJumpRequired);

    // ----------------------------------
    // NOT VIRTUAL FUNCIONS
    // ----------------------------------

	/** Generator calls this when a graph is ready. Used for logs and debug drawing */
	void OnVoronoiNavDataGenerationFinished(double TimeSpent);

    const FVoronoiFace* GetFaceByPoint(const FVector& Point, bool bProjectPoint = true) const;
    FVoronoiFace* GetFaceByPoint(const FVector& Point, bool bProjectPoint = true);

    TArray<const FVoronoiFace*> GetFacesInRadius(const FVector& Origin, float Radius) const;
    TArray<const FVoronoiFace*> GetReachableFacesInRadius(const FVector& Origin, float Radius) const;

    /** Causes rendering update */
    FORCEINLINE void UpdateVoronoiGraphDrawing()
	{
		if (RenderingComp && RenderingComp->IsVisible()) 
			RenderingComp->MarkRenderStateDirty();
	}

    // -----------------------------------
    // IVoronoiQuerier interface begin
    // -----------------------------------

    /** Get penalty modification parameters for pathfinding */
    virtual const FVoronoiQuerierParameters& GetVoronoiQuerierParameters() const override { return VoronoiQuerierDefaults; }

    // -----------------------------------
    // BLUEPRINTS
    // -----------------------------------

    /**
    * Retrieves AVoronoiNavData from Navigation System
    * @return VoronoiNavData or NULL if it is not present
    */
    UFUNCTION(BlueprintPure, Category = VoronoiPropertiesManager, meta = (WorldContext = "WorldContextObject"))
    static AVoronoiNavData* GetVoronoiNavData(const UObject* WorldContextObject);

    // -----------------------------------
    // GET FUNCTIONS
    // -----------------------------------

    FORCEINLINE const FVoronoiGenerationOptions& GetGenerationOptions() const { return GenerationOptions; }
    FORCEINLINE const FVoronoiDrawingOptions& GetDrawingOptions() const { return DrawingOptions; }

    FORCEINLINE FVoronoiGraph* GetVoronoiGraph() const { CheckVoronoiGraph(); return VoronoiGraph.Get(); }
};

/** Proxy to access voronoi graph while it is generated */
class PRACTICEPROJECT_API FVoronoiNavDataAttorney
{
    friend UVoronoiRenderingComponent;
    static FORCEINLINE const FVoronoiGraph* GetVoronoiGraphUnchecked(const AVoronoiNavData* VoronoiNavData) { return VoronoiNavData->VoronoiGraph.Get(); }
};
