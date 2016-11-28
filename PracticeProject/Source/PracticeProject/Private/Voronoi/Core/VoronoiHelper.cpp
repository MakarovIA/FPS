// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiGraph.h"

/** FVoronoiHelper */

TArray<FVoronoiEdge*> FVoronoiHelper::GetAdjacentEdges(FVoronoiVertex *Vertex)
{
    TArray<FVoronoiEdge*> Result;
    Result.Add(Vertex->GetEdge());

    while (true)
    {
        FVoronoiEdge* EdgeToAdd = (Result.Last()->GetFirstVertex() == Vertex) ? Result.Last()->GetPreviousEdge() : Result.Last()->GetNextEdge();
        if (EdgeToAdd == nullptr || EdgeToAdd == Result[0])
            break;

        Result.Add(EdgeToAdd);
    }

    return Result;
}

TArray<FVoronoiEdge*> FVoronoiHelper::GetAdjacentEdges(FVoronoiFace *Face)
{
    TArray<FVoronoiEdge*> Result;
    if (Face->GetEdge())
    {
        Result.Add(Face->GetEdge());

        while (true)
        {
            FVoronoiEdge* EdgeToAdd = (Result.Last()->GetLeftFace() == Face) ? Result.Last()->GetPreviousEdge() : Result.Last()->GetNextEdge();
            if (EdgeToAdd == nullptr || EdgeToAdd == Result[0])
                break;

            Result.Add(EdgeToAdd);
        }
    }

    return Result;
}

TArray<FVoronoiFace*> FVoronoiHelper::GetAdjacentFaces(FVoronoiVertex *Vertex)
{
    TArray<FVoronoiFace*> Result;
    for (FVoronoiEdge* Edge : GetAdjacentEdges(Vertex))
        Result.Add(Edge->GetFirstVertex() == Vertex ? Edge->GetLeftFace() : Edge->GetRightFace());
    
    return Result;
}

TArray<FVoronoiFace*> FVoronoiHelper::GetAdjacentFaces(FVoronoiFace *Face)
{
    TArray<FVoronoiFace*> Result;
    for (FVoronoiEdge* Edge : GetAdjacentEdges(Face))
        Result.Add(Edge->GetLeftFace() != Face ? Edge->GetLeftFace() : Edge->GetRightFace());

    return Result;
}

TArray<FVoronoiVertex*> FVoronoiHelper::GetAdjacentVertexes(FVoronoiVertex *Vertex)
{
    TArray<FVoronoiVertex*> Result;
    for (FVoronoiEdge* Edge : GetAdjacentEdges(Vertex))
        Result.Add(Edge->GetFirstVertex() == Vertex ? Edge->GetLastVertex() : Edge->GetFirstVertex());
    
    return Result;
}

TArray<FVoronoiVertex*> FVoronoiHelper::GetAdjacentVertexes(FVoronoiFace *Face)
{
    TArray<FVoronoiVertex*> Result;
    for (FVoronoiEdge* Edge : GetAdjacentEdges(Face))
        Result.Add(Edge->GetLeftFace() == Face ? Edge->GetLastVertex() : Edge->GetFirstVertex());

    return Result;
}

FVector2D FVoronoiHelper::GetRandomPointInFace(FVoronoiFace *Face)
{
    TArray<FVector2D> Points;
    for (FVoronoiVertex* i : GetAdjacentVertexes(Face))
        Points.Add(FVector2D(i->GetLocation()));

    return GetRandomPointInPolygon(Points);
}

FVector2D FVoronoiHelper::GetRandomPointInPolygon(const TArray<FVector2D>& Points)
{
    const int32 N = Points.Num();
    const float RandomNumber = FMath::FRand();

    switch (N)
    {
        case 0: return FVector2D(0, 0);
        case 1: return Points[0];
        case 2: return Points[0] * RandomNumber + Points[1] * (1 - RandomNumber);

        default:
            TArray<float> Areas;
            Areas.Reserve(N - 2);

            for (int32 Index = 2; Index < N; ++Index)
            {
                const FVector2D AB = Points[Index] - Points[0];
                const FVector2D AC = Points[Index - 1] - Points[0];

                const float TriangleArea = FMath::Abs(AB.X * AC.Y - AC.X * AB.Y) / 2;
                Areas.Add((Index > 2 ? Areas.Last() : 0) + TriangleArea);
            }

            for (int32 Index = 2; Index < N; ++Index)
            {
                if (RandomNumber < Areas[Index - 2] / Areas.Last())
                {
                    const FVector2D AB = Points[Index] - Points[0];
                    const FVector2D AC = Points[Index - 1] - Points[0];

                    return Points[0] + (FMath::FRand() * AB + FMath::FRand() * AC) / 2;
                }
            }

            return FVector2D();
    }
}
