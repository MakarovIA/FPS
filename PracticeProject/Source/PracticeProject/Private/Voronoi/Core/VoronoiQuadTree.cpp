// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiGraph.h"

/** FQuadTreeElement */

FVoronoiQuadTree::FQuadTreeElement::FQuadTreeElement(FVoronoiFace *InFace) : Face(InFace), Rect(FVector2D(FLT_MAX, FLT_MAX), FVector2D(-FLT_MAX, -FLT_MAX))
{
    TArray<FVoronoiVertex*> Vertexes = FVoronoiHelper::GetAdjacentVertexes(InFace);
    for (FVoronoiVertex* i : Vertexes)
    {
        const FVector& Location = i->GetLocation();

        Rect.Min.X = FMath::Min(Rect.Min.X, Location.X);
        Rect.Min.Y = FMath::Min(Rect.Min.Y, Location.Y);

        Rect.Max.X = FMath::Max(Rect.Max.X, Location.X);
        Rect.Max.Y = FMath::Max(Rect.Max.Y, Location.Y);
    }
}

/** FQuadTreeNode */

void FVoronoiQuadTree::FQuadTreeNode::Insert(const FQuadTreeElement& NewElement)
{
    static const int32 NodeCapacity = 4;
    if (!bIsSubdivided && Elements.Num() + 1 > NodeCapacity)
        Subdivide();

    if (!bIsSubdivided)
        Elements.Add(NewElement);
    else
    {
        const FBox2D& ElementRect = NewElement.GetRect();

        if (LU->Rect.IsInside(ElementRect)) { LU->Insert(NewElement); return; }
        if (LD->Rect.IsInside(ElementRect)) { LD->Insert(NewElement); return; }
        if (RU->Rect.IsInside(ElementRect)) { RU->Insert(NewElement); return; }
        if (RD->Rect.IsInside(ElementRect)) { RD->Insert(NewElement); return; }

        StuckedElements.Add(NewElement);
    }
}

void FVoronoiQuadTree::FQuadTreeNode::Subdivide()
{
    bIsSubdivided = true;

    const FVector2D Center = (Rect.Max + Rect.Min) / 2;

    LU = MakeUnique<FQuadTreeNode>(FBox2D(Rect.Min, Center));
    LD = MakeUnique<FQuadTreeNode>(FBox2D(FVector2D(Rect.Min.X, Center.Y), FVector2D(Center.X, Rect.Max.Y)));
    RU = MakeUnique<FQuadTreeNode>(FBox2D(FVector2D(Center.X, Rect.Min.Y), FVector2D(Rect.Max.X, Center.Y)));
    RD = MakeUnique<FQuadTreeNode>(FBox2D(Center, Rect.Max));

    for (const auto& i : Elements)
        Insert(i);

    Elements.Empty();
}

/** FVoronoiQuadTree */

void FVoronoiQuadTree::Insert(const FQuadTreeElement& NewElement)
{
    Root->Insert(NewElement);
}

FVoronoiFace* FVoronoiQuadTree::GetFaceByPoint(const FVector2D& Point) const
{
    if (!Root->GetRect().IsInside(Point))
        return nullptr;

    FVoronoiFace *Best = nullptr;
    float BestDistSquared = FLT_MAX;

    FQuadTreeNode *Temp = Root.Get();
    while (Temp->IsSubdivided())
    {
        // At first process stucked elements
        for (const auto& i : Temp->GetStuckedElements())
        {
            FVoronoiFace *const Face = i.GetFace();
            const float DistSquared = FVector2D::DistSquared(FVector2D(Face->GetLocation()), Point);

            if (DistSquared < BestDistSquared)
            {
                BestDistSquared = DistSquared;
                Best = Face;
            }
        }

        const FVector2D& Center = Temp->GetLU()->GetRect().Max;

        if (Point.X < Center.X)
            if (Point.Y < Center.Y) Temp = Temp->GetLU();
            else                    Temp = Temp->GetLD();
        else
            if (Point.Y < Center.Y) Temp = Temp->GetRU();
            else                    Temp = Temp->GetRD();
    }

    for (const auto& i : Temp->GetElements())
    {
        FVoronoiFace *const Face = i.GetFace();
        const float DistSquared = FVector2D::DistSquared(FVector2D(Face->GetLocation()), Point);

        if (DistSquared < BestDistSquared)
        {
            BestDistSquared = DistSquared;
            Best = Face;
        }
    }

    return Best;
}

TArray<FVoronoiFace*> FVoronoiQuadTree::GetFacesInCircle(const FVector2D& Point, float Radius) const
{
    return GetFacesByPredicate([&Point, &Radius](const FBox2D& InRect) -> bool
    {
        float DistanceX = Point.X - FMath::Clamp(Point.X, InRect.Min.X, InRect.Max.X);
        float DistanceY = Point.Y - FMath::Clamp(Point.Y, InRect.Min.Y, InRect.Max.Y);

        return DistanceX * DistanceX + DistanceY * DistanceY < (Radius * Radius);
    });
}

TArray<FVoronoiFace*> FVoronoiQuadTree::GetFacesInRectangle(const FBox2D& Rect) const
{
    return GetFacesByPredicate([&Rect](const FBox2D& InRect) -> bool { return Rect.Intersect(InRect); });
}

template<class Predicate>
TArray<FVoronoiFace*> FVoronoiQuadTree::GetFacesByPredicate(Predicate Intersect) const
{
    TArray<FVoronoiFace*> Result;

    TArray<const FQuadTreeNode*> Stack;
    if (Intersect(Root->GetRect()))
        Stack.Push(Root.Get());

    while (Stack.Num() > 0)
    {
        const auto Top = Stack.Last();
        Stack.Pop();

        for (const auto& i : Top->GetElements())
            if (Intersect(i.GetRect()))
                Result.Push(i.GetFace());

        if (Top->IsSubdivided())
        {
            for (const auto& i : Top->GetStuckedElements())
                if (Intersect(i.GetRect()))
                    Result.Push(i.GetFace());

            if (Intersect(Top->GetLU()->GetRect())) Stack.Push(Top->GetLU());
            if (Intersect(Top->GetRU()->GetRect())) Stack.Push(Top->GetRU());
            if (Intersect(Top->GetLD()->GetRect())) Stack.Push(Top->GetLD());
            if (Intersect(Top->GetRD()->GetRect())) Stack.Push(Top->GetRD());
        }
    }

    return Result;
}
