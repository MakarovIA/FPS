// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiGraph.h"

/** FVoronoiHelper */

TArray<const FVoronoiEdge*> FVoronoiHelper::GetAdjacentEdges(const FVoronoiVertex *Vertex)
{
    TArray<const FVoronoiEdge*> Result;
    Result.Add(Vertex->Edge);

    const FVoronoiEdge* EdgeToAdd = Vertex->Edge;
    while ((EdgeToAdd = (EdgeToAdd->FirstVertex == Vertex) ? EdgeToAdd->PreviousEdge : EdgeToAdd->NextEdge) != Vertex->Edge)
        Result.Add(EdgeToAdd);

    return Result;
}

TArray<const FVoronoiEdge*> FVoronoiHelper::GetAdjacentEdges(const FVoronoiFace *Face)
{
    TArray<const FVoronoiEdge*> Result;
    Result.Add(Face->Edge);

    const FVoronoiEdge* EdgeToAdd = Face->Edge;
    while ((EdgeToAdd = (EdgeToAdd->LeftFace == Face) ? EdgeToAdd->PreviousEdge : EdgeToAdd->NextEdge) != Face->Edge)
        Result.Add(EdgeToAdd);

    return Result;
}

TArray<const FVoronoiFace*> FVoronoiHelper::GetAdjacentFaces(const FVoronoiVertex *Vertex)
{
    TArray<const FVoronoiFace*> Result;

    const FVoronoiFace* FaceToAdd;
    for (const FVoronoiEdge* Edge : GetAdjacentEdges(Vertex))
        if ((FaceToAdd = Edge->FirstVertex == Vertex ? Edge->LeftFace : Edge->RightFace) != nullptr)
            Result.Add(FaceToAdd);
    
    return Result;
}

TArray<const FVoronoiFace*> FVoronoiHelper::GetAdjacentFaces(const FVoronoiFace *Face)
{
    TArray<const FVoronoiFace*> Result;

    const FVoronoiFace* FaceToAdd;
    for (const FVoronoiEdge* Edge : GetAdjacentEdges(Face))
        if ((FaceToAdd = Edge->LeftFace != Face ? Edge->LeftFace : Edge->RightFace) != nullptr)
            Result.Add(FaceToAdd);

    return Result;
}

TArray<const FVoronoiVertex*> FVoronoiHelper::GetAdjacentVertexes(const FVoronoiVertex *Vertex)
{
    TArray<const FVoronoiVertex*> Result;
    for (const FVoronoiEdge* Edge : GetAdjacentEdges(Vertex))
        Result.Add(Edge->FirstVertex == Vertex ? Edge->LastVertex : Edge->FirstVertex);
    
    return Result;
}

TArray<const FVoronoiVertex*> FVoronoiHelper::GetAdjacentVertexes(const FVoronoiFace *Face)
{
    TArray<const FVoronoiVertex*> Result;
    for (const FVoronoiEdge* Edge : GetAdjacentEdges(Face))
        Result.Add(Edge->LeftFace == Face ? Edge->LastVertex : Edge->FirstVertex);

    return Result;
}

TArray<FVoronoiEdge*> FVoronoiHelper::GetAdjacentEdges(FVoronoiVertex *Vertex) 
{ 
    TArray<FVoronoiEdge*> Result;
    Result.Add(Vertex->Edge);

    FVoronoiEdge* EdgeToAdd = Vertex->Edge;
    while ((EdgeToAdd = (EdgeToAdd->FirstVertex == Vertex) ? EdgeToAdd->PreviousEdge : EdgeToAdd->NextEdge) != Vertex->Edge)
        Result.Add(EdgeToAdd);

    return Result;
}

TArray<FVoronoiEdge*> FVoronoiHelper::GetAdjacentEdges(FVoronoiFace *Face)
{
    TArray<FVoronoiEdge*> Result;
    Result.Add(Face->Edge);

    FVoronoiEdge* EdgeToAdd = Face->Edge;
    while ((EdgeToAdd = (EdgeToAdd->LeftFace == Face) ? EdgeToAdd->PreviousEdge : EdgeToAdd->NextEdge) != Face->Edge)
        Result.Add(EdgeToAdd);

    return Result;
}

TArray<FVoronoiFace*> FVoronoiHelper::GetAdjacentFaces(FVoronoiVertex *Vertex)
{
    TArray<FVoronoiFace*> Result;

    FVoronoiFace* FaceToAdd;
    for (FVoronoiEdge* Edge : GetAdjacentEdges(Vertex))
        if ((FaceToAdd = Edge->FirstVertex == Vertex ? Edge->LeftFace : Edge->RightFace) != nullptr)
            Result.Add(FaceToAdd);

    return Result;
}

TArray<FVoronoiFace*> FVoronoiHelper::GetAdjacentFaces(FVoronoiFace *Face)
{
    TArray<FVoronoiFace*> Result;

    FVoronoiFace* FaceToAdd;
    for (FVoronoiEdge* Edge : GetAdjacentEdges(Face))
        if ((FaceToAdd = Edge->LeftFace != Face ? Edge->LeftFace : Edge->RightFace) != nullptr)
            Result.Add(FaceToAdd);

    return Result;
}

TArray<FVoronoiVertex*> FVoronoiHelper::GetAdjacentVertexes(FVoronoiVertex *Vertex)
{
    TArray<FVoronoiVertex*> Result;
    for (FVoronoiEdge* Edge : GetAdjacentEdges(Vertex))
        Result.Add(Edge->FirstVertex == Vertex ? Edge->LastVertex : Edge->FirstVertex);

    return Result;
}

TArray<FVoronoiVertex*> FVoronoiHelper::GetAdjacentVertexes(FVoronoiFace *Face)
{
    TArray<FVoronoiVertex*> Result;
    for (FVoronoiEdge* Edge : GetAdjacentEdges(Face))
        Result.Add(Edge->LeftFace == Face ? Edge->LastVertex : Edge->FirstVertex);

    return Result;
}
