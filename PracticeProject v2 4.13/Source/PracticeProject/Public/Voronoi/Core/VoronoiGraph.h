// By Polyakov Pavel

#pragma once

#include "VoronoiQuadTree.h"
#include "VoronoiHelper.h"

class FVoronoiNavDataGenerator;
class FVoronoiAdditionalGeneratorTask;
class FVoronoiMainGeneratorTask;

class UVoronoiRenderingComponent;

class FVoronoiVertex;
class FVoronoiEdge;
class FVoronoiFace;

class FVoronoiSurface;
class FVoronoiGraph;

class FVoronoiRenderAttorney;

/** Geometry related flags that Voronoi Faces have */
struct PRACTICEPROJECT_API FVoronoiFlags final
{
    bool bCrouchedOnly, bNoJump, bObstacle, bBorder, bInvalid;

    FVoronoiFlags() : bCrouchedOnly(false), bNoJump(false), bObstacle(false), bBorder(false), bInvalid(false) {}

    const FVoronoiFlags& operator|=(const FVoronoiFlags& Other);
    const FVoronoiFlags& operator&=(const FVoronoiFlags& Other);

    FVoronoiFlags operator|(const FVoronoiFlags& Other);
    FVoronoiFlags operator&(const FVoronoiFlags& Other);
};

/** Cost related properties that Voronoi Faces have */
struct PRACTICEPROJECT_API FVoronoiProperties final
{
    float BaseCost, BaseEnterCost;
    bool bNoWay;

    FVoronoiProperties() : BaseCost(1.f), BaseEnterCost(0.f), bNoWay(false) {}
};

/** Tactics related properties that Voronoi Faces have */
struct PRACTICEPROJECT_API FVoronoiTacticalProperties final
{
    float Visibility, CloseRangeVisibility, FarRangeVisibility, SouthVisibility, WestVisibility, EastVisibility, NorthVisibility,
        SEVisibility, SWVisibility, NEVisibility, NWVisibility;

    FVoronoiTacticalProperties() : Visibility(0.5f), CloseRangeVisibility(0.5f), FarRangeVisibility(0.5f), SouthVisibility(0.5f), WestVisibility(0.5f), EastVisibility(0.5f), NorthVisibility(0.5f),
        SEVisibility(0.5f), SWVisibility(0.5f), NEVisibility(0.5f), NWVisibility(0.5f) {}
};

/** Link to Voronoi Face from another surface */
class PRACTICEPROJECT_API FVoronoiLink final
{
    FVoronoiFace *Face;
    bool bJumpRequired;

public:
    FVoronoiLink(FVoronoiFace *InFace, bool InJumpRequired) : Face(InFace), bJumpRequired(InJumpRequired) {}

    /** Get a Face this link leads to */
    FORCEINLINE FVoronoiFace* GetFace() const { return Face; }

    /** Is jump necessary to travel this link */
    FORCEINLINE bool IsJumpRequired() const { return bJumpRequired; }
};

/** Vertex of Voronoi diagram */
class PRACTICEPROJECT_API FVoronoiVertex final
{
    FVector Location;
    FVoronoiEdge *Edge;

    FVoronoiVertex(const FVector &InLocation, FVoronoiEdge *InEdge) : Location(InLocation), Edge(InEdge) {}

    friend FVoronoiMainGeneratorTask;
    friend FVoronoiGraph;

public:
    /** Get the location of the vertex */
    FORCEINLINE const FVector& GetLocation() const { return Location; }

    /** Get an edge adjacent to this vertex */
    FORCEINLINE FVoronoiEdge* GetEdge() const { return Edge; }
};

/** Edge of Voronoi diagram */
class PRACTICEPROJECT_API FVoronoiEdge final
{
    FVoronoiVertex *Begin, *End;
    FVoronoiFace *Left, *Right;
    FVoronoiEdge *Previous, *Next;

    FVoronoiEdge(FVoronoiFace *InLeft, FVoronoiFace *InRight) : Begin(nullptr), End(nullptr), Left(InLeft), Right(InRight), Previous(nullptr), Next(nullptr) {}

    friend FVoronoiMainGeneratorTask;
    friend FVoronoiGraph;

public:
    /** Get the vertex this edge begins in */
    FORCEINLINE FVoronoiVertex* GetFirstVertex() const { return Begin; }

    /** Get the vertex this edge ends in */
    FORCEINLINE FVoronoiVertex* GetLastVertex() const { return End; }

    /** Get the Face on the left side of this edge */
    FORCEINLINE FVoronoiFace* GetLeftFace() const { return Left; }

    /** Get the Face on the right side of this edge */
    FORCEINLINE FVoronoiFace* GetRightFace() const { return Right; }

    /** Get the "next" edge */
    FORCEINLINE FVoronoiEdge* GetNextEdge() const { return Next; }

    /** Get the "previous" edge */
    FORCEINLINE FVoronoiEdge* GetPreviousEdge() const { return Previous; }
};

/** Face of Voronoi diagram */
class PRACTICEPROJECT_API FVoronoiFace final
{
    FVector Location;

    FVoronoiSurface *Surface;
    FVoronoiEdge *Edge;

    TArray<FVoronoiLink> Links;
    FVoronoiFlags Flags;

    FVoronoiTacticalProperties TacticalProperties;

    FVoronoiFace(FVoronoiSurface *InSurface, const FVector &InLocation) : Location(InLocation), Surface(InSurface), Edge(nullptr) {}

    friend FVoronoiNavDataGenerator;
    friend FVoronoiAdditionalGeneratorTask;
    friend FVoronoiMainGeneratorTask;
    friend FVoronoiGraph;

public:
    /** Cost-related properties used in pathfinding */
    FVoronoiProperties Properties;

    /** Get the location of the Voronoi site */
    FORCEINLINE const FVector& GetLocation() const { return Location; }

    /** Get one edge adjacent to this Face */
    FORCEINLINE FVoronoiEdge* GetEdge() const { return Edge; }

    /** Get a surface this Face belongs to */
    FORCEINLINE FVoronoiSurface* GetSurface() const { return Surface; }

    /** Get links to faces that are in other surfaces */
    FORCEINLINE const TArray<FVoronoiLink>& GetLinks() const { return Links; }

    /** Get flags assosiated with this Face */
    FORCEINLINE const FVoronoiFlags& GetFlags() const { return Flags; }

    /** Get flags assosiated with this Face */
    FORCEINLINE const FVoronoiTacticalProperties& GetTacticalProperties() const { return TacticalProperties; }

    /** Returns whether a Face is navigable */
    FORCEINLINE bool IsNavigable() const { return !(Flags.bObstacle || Flags.bInvalid); }
};

/** Voronoi diagram limited by surface */
class PRACTICEPROJECT_API FVoronoiSurface final
{
    TArray<TUniquePtr<FVoronoiVertex>> Vertexes;
    TArray<TUniquePtr<FVoronoiEdge>> Edges;
    TArray<TUniquePtr<FVoronoiFace>> Faces;

    TUniquePtr<FVoronoiQuadTree> QuadTree;

    FVector Location;
    FVector2D Size;

    FVoronoiSurface(const FVector &InLocation, const FVector2D &InSize) : Location(InLocation), Size(InSize) {}

    FVoronoiSurface& operator=(const FVoronoiSurface&) = delete;
    FVoronoiSurface(const FVoronoiSurface&) = delete;

    friend FVoronoiNavDataGenerator;
    friend FVoronoiMainGeneratorTask;
    friend FVoronoiGraph;

public:
    /** Get an array of vertexes that belong to this surface */
    FORCEINLINE const TArray<TUniquePtr<FVoronoiVertex>>& GetVertexes() const { return Vertexes; }

    /** Get an array of edges that belong to this surface */
    FORCEINLINE const TArray<TUniquePtr<FVoronoiEdge>>& GetEdges() const { return Edges; }

    /** Get an array of faces that belong to this surface */
    FORCEINLINE const TArray<TUniquePtr<FVoronoiFace>>& GetFaces() const { return Faces; }

    /** Get quad tree */
    FORCEINLINE const FVoronoiQuadTree* GetQuadTree() const { return QuadTree.Get(); }

    /** Get the location of the Voronoi surface */
    FORCEINLINE const FVector& GetLocation() const { return Location; }

    /** Get the size of the Voronoi surface */
    FORCEINLINE const FVector2D& GetSize() const { return Size; }

    /** Returns whether a point is on surface */
    FORCEINLINE bool Contains(const FVector2D& Point) const { return Location.X <= Point.X && Location.X + Size.X >= Point.X && Location.Y <= Point.Y && Location.Y + Size.Y >= Point.Y; }
};

/** Storage for Voronoi-based navigation mesh */
class PRACTICEPROJECT_API FVoronoiGraph final
{
    TArray<TUniquePtr<FVoronoiSurface>> Surfaces, GeneratedSurfaces, RenderedSurfaces;
    bool bCanRenderGenerated;

    void SerializeInternal(FArchive &Ar);

    FVoronoiGraph& operator=(const FVoronoiGraph&) = delete;
    FVoronoiGraph(const FVoronoiGraph&) = delete;

    friend FVoronoiNavDataGenerator;
    friend FVoronoiAdditionalGeneratorTask;
    friend FVoronoiRenderAttorney;

public:
    FVoronoiGraph() : bCanRenderGenerated(false) {}

    /** Serialize Voronoi graph */
    void Serialize(FArchive &Ar);

    /** Returns an array of Voronoi surfaces */
    FORCEINLINE const TArray<TUniquePtr<FVoronoiSurface>>& GetSurfaces() const { return Surfaces; }
};

/** Proxy to access a single private field */
class PRACTICEPROJECT_API FVoronoiRenderAttorney
{
    friend UVoronoiRenderingComponent;
    static FORCEINLINE const TArray<TUniquePtr<FVoronoiSurface>>& GetSurfacesToRender(const FVoronoiGraph &InVoronoiGraph)
    { 
        return InVoronoiGraph.bCanRenderGenerated ? InVoronoiGraph.GeneratedSurfaces : (InVoronoiGraph.RenderedSurfaces.Num() > 0 ? InVoronoiGraph.RenderedSurfaces : InVoronoiGraph.Surfaces);
    }
};
