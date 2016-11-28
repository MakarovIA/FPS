// By Polyakov Pavel

#pragma once

class FVoronoiVertex;
class FVoronoiEdge;
class FVoronoiFace;

/** A collection of functions to use with a Voronoi diagram */
class PRACTICEPROJECT_API FVoronoiHelper
{
public:
    /** Get adjacent edges to a vertex in counterclockwise order */
    static TArray<FVoronoiEdge*> GetAdjacentEdges(FVoronoiVertex *Vertex);

    /** Get adjacent edges to a face in clockwise order */
    static TArray<FVoronoiEdge*> GetAdjacentEdges(FVoronoiFace *Face);

    /** Get adjacent faces to a vertex in counterclockwise order */
    static TArray<FVoronoiFace*> GetAdjacentFaces(FVoronoiVertex *Vertex);

    /** Get adjacent faces to a face in clockwise order */
    static TArray<FVoronoiFace*> GetAdjacentFaces(FVoronoiFace *Face);

    /** Get adjacent vertexes to a vertex in counterclockwise order */
    static TArray<FVoronoiVertex*> GetAdjacentVertexes(FVoronoiVertex *Vertex);

    /** Get adjacent vertexes to a face in clockwise order */
    static TArray<FVoronoiVertex*> GetAdjacentVertexes(FVoronoiFace *Face);

    /** Get a random point in a given face */
    static FVector2D GetRandomPointInFace(FVoronoiFace *Face);

    /** Get a random point in a convex polygon described by a set of points */
    static FVector2D GetRandomPointInPolygon(const TArray<FVector2D>& Points);
};
