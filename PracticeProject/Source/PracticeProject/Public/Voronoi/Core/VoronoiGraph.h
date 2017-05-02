// By Polyakov Pavel

#pragma once

#include "PreserveConstPointer.h"
#include "PreserveConstUniquePtr.h"
#include "MathExtended.h"

#include "VoronoiQuadTree.h"
#include "VoronoiHelper.h"

struct FVoronoiVertex;
struct FVoronoiEdge;
struct FVoronoiFace;
struct FVoronoiSurface;

class FVoronoiGraph;
class FVoronoiNavDataGenerator;
class UVoronoiRenderingComponent;
class FVoronoiRenderAttorney;

/** Geometry and construction related flags */
struct PRACTICEPROJECT_API FVoronoiFlags final
{
    uint8 bCrouchedOnly = false, bNoJump = false, bBorder = false, bReserved = false;
};

/** Tactics related properties Voronoi faces have */
struct PRACTICEPROJECT_API FVoronoiTacticalProperties final
{
    float Area = 0.f, Luminance = 0.f, Visibility[8] = { 0.f };

    FORCEINLINE float GetNEVisibility() const { return Visibility[0] + Visibility[1]; }
    FORCEINLINE float GetNWVisibility() const { return Visibility[2] + Visibility[3]; }
    FORCEINLINE float GetSWVisibility() const { return Visibility[4] + Visibility[5]; }
    FORCEINLINE float GetSEVisibility() const { return Visibility[6] + Visibility[7]; }

    FORCEINLINE float GetNVisibility() const { return Visibility[1] + Visibility[2]; }
    FORCEINLINE float GetWVisibility() const { return Visibility[3] + Visibility[4]; }
    FORCEINLINE float GetSVisibility() const { return Visibility[5] + Visibility[6]; }
    FORCEINLINE float GetEVisibility() const { return Visibility[7] + Visibility[0]; }

    FORCEINLINE float GetFullVisibility() const { return GetNEVisibility() + 
                                                         GetNWVisibility() +
                                                         GetSWVisibility() +
                                                         GetSEVisibility(); }
};

/** Link to another Voronoi face */
struct PRACTICEPROJECT_API FVoronoiLink final
{
    const FVoronoiFace *Face;
    bool bJumpRequired;

    FORCEINLINE FVoronoiLink() {}
    FORCEINLINE FVoronoiLink(const FVoronoiFace *InFace, bool InJumpRequired)
        : Face(InFace), bJumpRequired(InJumpRequired) {}
};

/** Vertex of Voronoi diagram */
struct PRACTICEPROJECT_API FVoronoiVertex final
{
    FVector Location;
    TPreserveConstPointer<FVoronoiEdge> Edge;

    FORCEINLINE FVoronoiVertex() {}
    FORCEINLINE FVoronoiVertex(const FVector &InLocation, FVoronoiEdge *InEdge)
        : Location(InLocation), Edge(InEdge) {}
};

/** Edge of Voronoi diagram */
struct PRACTICEPROJECT_API FVoronoiEdge final
{
    TPreserveConstPointer<FVoronoiVertex> FirstVertex, LastVertex;
    TPreserveConstPointer<FVoronoiEdge> PreviousEdge, NextEdge;
    TPreserveConstPointer<FVoronoiFace> LeftFace, RightFace;

    FORCEINLINE FVoronoiEdge() {}
    FORCEINLINE FVoronoiEdge(FVoronoiFace *InLeftFace, FVoronoiFace *InRightFace)
        : LeftFace(InLeftFace), RightFace(InRightFace) {}
};

/** Face of Voronoi diagram */
struct PRACTICEPROJECT_API FVoronoiFace final
{
    FVector Location;

    TPreserveConstPointer<FVoronoiSurface> Surface;
    TPreserveConstPointer<FVoronoiEdge> Edge;

    TArray<FVoronoiLink> Links;
    FVoronoiFlags Flags;

    FVoronoiTacticalProperties TacticalProperties;

    FORCEINLINE FVoronoiFace() {}
    FORCEINLINE FVoronoiFace(FVoronoiSurface *InSurface, const FVector &InLocation)
        : Location(InLocation), Surface(InSurface) {}

    FORCEINLINE bool Contains(const FVector2D &Point) const
    {
        TArray<FVector2D> Points;
        for (const FVoronoiVertex* i : FVoronoiHelper::GetAdjacentVertexes(this))
            Points.Emplace(i->Location);

        return FMathExtended::IsPointInPolygon2D(Points, Point);
    }
};

/** Voronoi diagram limited by surface */
struct PRACTICEPROJECT_API FVoronoiSurface final
{
    TArray<TPreserveConstUniquePtr<FVoronoiVertex>> Vertexes;
    TArray<TPreserveConstUniquePtr<FVoronoiEdge>> Edges;
    TArray<TPreserveConstUniquePtr<FVoronoiFace>> Faces;

    TPreserveConstUniquePtr<FVoronoiQuadTree> QuadTree;
    TArray<TArray<FVector>> Borders;

    FORCEINLINE FVoronoiSurface() {}
    FORCEINLINE FVoronoiSurface(TArray<TArray<FVector>> InBorders)
        : Borders(MoveTemp(InBorders)) {}

private:
    FVoronoiSurface& operator=(const FVoronoiSurface&) = delete;
    FVoronoiSurface(const FVoronoiSurface&) = delete;
};

/** Storage for Voronoi-based navigation mesh */
class PRACTICEPROJECT_API FVoronoiGraph final
{
    TArray<TPreserveConstUniquePtr<FVoronoiSurface>> GeneratedSurfaces, RenderedSurfaces;
    bool bCanRenderGenerated;

    void SerializeInternal(FArchive &Ar);

    FVoronoiGraph& operator=(const FVoronoiGraph&) = delete;
    FVoronoiGraph(const FVoronoiGraph&) = delete;

    friend FVoronoiNavDataGenerator;
    friend FVoronoiRenderAttorney;

public:
    FORCEINLINE FVoronoiGraph()
        : bCanRenderGenerated(false) {}

    TArray<TPreserveConstUniquePtr<FVoronoiSurface>> Surfaces;
    void Serialize(FArchive &Ar);
};

/** Proxy class allowing generated surfaces to be rendered */
class PRACTICEPROJECT_API FVoronoiRenderAttorney final
{
    friend UVoronoiRenderingComponent;
    static FORCEINLINE const TArray<TPreserveConstUniquePtr<FVoronoiSurface>>& GetSurfacesToRender(const FVoronoiGraph &InVoronoiGraph)
    {
        if (InVoronoiGraph.bCanRenderGenerated) return InVoronoiGraph.GeneratedSurfaces;
        else return InVoronoiGraph.RenderedSurfaces.Num() > 0 ? InVoronoiGraph.RenderedSurfaces : InVoronoiGraph.Surfaces;
    }
};
