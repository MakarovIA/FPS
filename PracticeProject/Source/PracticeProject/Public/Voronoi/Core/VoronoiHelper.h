// By Polyakov Pavel

#pragma once

struct FVoronoiVertex;
struct FVoronoiEdge;
struct FVoronoiFace;

/** A collection of functions to use with a Voronoi diagram */
struct PRACTICEPROJECT_API FVoronoiHelper
{
    static TArray<const FVoronoiEdge*> GetAdjacentEdges(const FVoronoiVertex *Vertex);
    static TArray<const FVoronoiEdge*> GetAdjacentEdges(const FVoronoiFace *Face);
    static TArray<const FVoronoiFace*> GetAdjacentFaces(const FVoronoiVertex *Vertex);
    static TArray<const FVoronoiFace*> GetAdjacentFaces(const FVoronoiFace *Face);
    static TArray<const FVoronoiVertex*> GetAdjacentVertexes(const FVoronoiVertex *Vertex);
    static TArray<const FVoronoiVertex*> GetAdjacentVertexes(const FVoronoiFace *Face);

    static TArray<FVoronoiEdge*> GetAdjacentEdges(FVoronoiVertex *Vertex);
    static TArray<FVoronoiEdge*> GetAdjacentEdges(FVoronoiFace *Face);
    static TArray<FVoronoiFace*> GetAdjacentFaces(FVoronoiVertex *Vertex);
    static TArray<FVoronoiFace*> GetAdjacentFaces(FVoronoiFace *Face);
    static TArray<FVoronoiVertex*> GetAdjacentVertexes(FVoronoiVertex *Vertex);
    static TArray<FVoronoiVertex*> GetAdjacentVertexes(FVoronoiFace *Face);
};
