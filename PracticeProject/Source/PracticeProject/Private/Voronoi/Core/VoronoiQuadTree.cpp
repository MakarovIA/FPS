// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiGraph.h"

/** FQuadTreeElement */

FQuadTreeElement::FQuadTreeElement(FVoronoiFace *InFace)
    : Face(InFace), Rect(FVector2D(FLT_MAX, FLT_MAX), FVector2D(-FLT_MAX, -FLT_MAX))
{
    for (const FVoronoiVertex* Vertex : FVoronoiHelper::GetAdjacentVertexes(InFace))
    {
        Rect.Min.X = FMath::Min(Rect.Min.X, Vertex->Location.X);
        Rect.Min.Y = FMath::Min(Rect.Min.Y, Vertex->Location.Y);

        Rect.Max.X = FMath::Max(Rect.Max.X, Vertex->Location.X);
        Rect.Max.Y = FMath::Max(Rect.Max.Y, Vertex->Location.Y);
    }
}

/** FQuadTreeNode */

void FQuadTreeNode::Insert(const FQuadTreeElement& NewElement)
{
    static const int32 NodeCapacity = 4;
    if (!bIsSubdivided && Elements.Num() == NodeCapacity)
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

void FQuadTreeNode::Subdivide()
{
    bIsSubdivided = true;

    const FVector2D Center = (Rect.Max + Rect.Min) / 2;

    LU = MakeUnique<FQuadTreeNode>(FBox2D(Rect.Min, Center));
    LD = MakeUnique<FQuadTreeNode>(FBox2D(FVector2D(Rect.Min.X, Center.Y), FVector2D(Center.X, Rect.Max.Y)));
    RU = MakeUnique<FQuadTreeNode>(FBox2D(FVector2D(Center.X, Rect.Min.Y), FVector2D(Rect.Max.X, Center.Y)));
    RD = MakeUnique<FQuadTreeNode>(FBox2D(Center, Rect.Max));

    for (const FQuadTreeElement& Element : Elements)
        Insert(Element);

    Elements.Empty();
}

/** FVoronoiQuadTree */

const FVoronoiFace* FVoronoiQuadTree::GetFaceByPoint(const FVector2D& Point) const
{
    const FQuadTreeNode *Temp = Root.Get();
    if (!Temp->GetRect().IsInside(Point))
        return nullptr;

    while (true)
    {
        for (const FQuadTreeElement& Element : Temp->IsSubdivided() ? Temp->GetStuckedElements() : Temp->GetElements())
            if (Element.GetRect().Min <= Point && Element.GetRect().Max >= Point && Element.GetFace()->Contains(Point))
                return Element.GetFace();

        if (!Temp->IsSubdivided())
            return nullptr;

        const FVector2D& Center = Temp->GetLU()->GetRect().Max;
        Temp = Point.X < Center.X ? (Point.Y < Center.Y ? Temp->GetLU() : Temp->GetLD()) : (Point.Y < Center.Y ? Temp->GetRU() : Temp->GetRD());
    }
}

TArray<const FVoronoiFace*> FVoronoiQuadTree::GetFacesInCircle(const FVector2D& Point, float Radius) const
{
    return GetFacesByPredicate([&Point, Radius](const FBox2D& InRect) -> bool
    {
        const float DistanceX = Point.X - FMath::Clamp(Point.X, InRect.Min.X, InRect.Max.X);
        const float DistanceY = Point.Y - FMath::Clamp(Point.Y, InRect.Min.Y, InRect.Max.Y);

        return DistanceX * DistanceX + DistanceY * DistanceY < Radius * Radius;
    });
}

TArray<const FVoronoiFace*> FVoronoiQuadTree::GetFacesInRectangle(const FBox2D& Rect) const
{
    return GetFacesByPredicate([&Rect](const FBox2D& InRect) -> bool { return Rect.Intersect(InRect); });
}

TArray<const FVoronoiFace*> FVoronoiQuadTree::GetFacesByPredicate(TFunction<bool(const FBox2D&)> Predicate) const
{
    TArray<const FVoronoiFace*> Result;

    TArray<const FQuadTreeNode*> Stack;
    if (Predicate(Root->GetRect()))
        Stack.Push(Root.Get());

    while (Stack.Num() > 0)
    {
        const FQuadTreeNode *Top = Stack.Pop();
        for (const FQuadTreeElement& Element : Top->GetElements())
            if (Predicate(Element.GetRect()))
                Result.Push(Element.GetFace());

        if (Top->IsSubdivided())
        {
            for (const FQuadTreeElement& Element : Top->GetStuckedElements())
                if (Predicate(Element.GetRect()))
                    Result.Push(Element.GetFace());

            if (Predicate(Top->GetLU()->GetRect())) Stack.Push(Top->GetLU());
            if (Predicate(Top->GetRU()->GetRect())) Stack.Push(Top->GetRU());
            if (Predicate(Top->GetLD()->GetRect())) Stack.Push(Top->GetLD());
            if (Predicate(Top->GetRD()->GetRect())) Stack.Push(Top->GetRD());
        }
    }

    return Result;
}

FVoronoiFace* FVoronoiQuadTree::GetFaceByPoint(const FVector2D& Point)
{
    FQuadTreeNode *Temp = Root.Get();
    if (!Temp->GetRect().IsInside(Point))
        return nullptr;

    while (true)
    {
        for (FQuadTreeElement& Element : Temp->IsSubdivided() ? Temp->GetStuckedElements() : Temp->GetElements())
            if (Element.GetRect().Min <= Point && Element.GetRect().Max >= Point && Element.GetFace()->Contains(Point))
                return Element.GetFace();

        if (!Temp->IsSubdivided())
            return nullptr;

        const FVector2D& Center = Temp->GetLU()->GetRect().Max;
        Temp = Point.X < Center.X ? (Point.Y < Center.Y ? Temp->GetLU() : Temp->GetLD()) : (Point.Y < Center.Y ? Temp->GetRU() : Temp->GetRD());
    }
}

TArray<FVoronoiFace*> FVoronoiQuadTree::GetFacesInCircle(const FVector2D& Point, float Radius)
{
    return GetFacesByPredicate([&Point, Radius](const FBox2D& InRect) -> bool
    {
        const float DistanceX = Point.X - FMath::Clamp(Point.X, InRect.Min.X, InRect.Max.X);
        const float DistanceY = Point.Y - FMath::Clamp(Point.Y, InRect.Min.Y, InRect.Max.Y);

        return DistanceX * DistanceX + DistanceY * DistanceY < Radius * Radius;
    });
}

TArray<FVoronoiFace*> FVoronoiQuadTree::GetFacesInRectangle(const FBox2D& Rect)
{
    return GetFacesByPredicate([&Rect](const FBox2D& InRect) -> bool { return Rect.Intersect(InRect); });
}

TArray<FVoronoiFace*> FVoronoiQuadTree::GetFacesByPredicate(TFunction<bool(const FBox2D&)> Predicate)
{
    TArray<FVoronoiFace*> Result;

    TArray<FQuadTreeNode*> Stack;
    if (Predicate(Root->GetRect()))
        Stack.Push(Root.Get());

    while (Stack.Num() > 0)
    {
        FQuadTreeNode *Top = Stack.Pop();
        for (FQuadTreeElement& Element : Top->GetElements())
            if (Predicate(Element.GetRect()))
                Result.Push(Element.GetFace());

        if (Top->IsSubdivided())
        {
            for (FQuadTreeElement& Element : Top->GetStuckedElements())
                if (Predicate(Element.GetRect()))
                    Result.Push(Element.GetFace());

            if (Predicate(Top->GetLU()->GetRect())) Stack.Push(Top->GetLU());
            if (Predicate(Top->GetRU()->GetRect())) Stack.Push(Top->GetRU());
            if (Predicate(Top->GetLD()->GetRect())) Stack.Push(Top->GetLD());
            if (Predicate(Top->GetRD()->GetRect())) Stack.Push(Top->GetRD());
        }
    }

    return Result;
}

TUniquePtr<FVoronoiQuadTree> FVoronoiQuadTree::ConstructFromSequence(TArray<TPreserveConstUniquePtr<FVoronoiFace>>& Faces)
{
    TArray<FQuadTreeElement> QuadTreeElements;
    QuadTreeElements.Reserve(Faces.Num());

    for (FVoronoiFace *Face : Faces)
        QuadTreeElements.Emplace(Face);

    FVector2D Min(FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX);
    for (const FQuadTreeElement& QuadTreeElement : QuadTreeElements)
    {
        Min.X = FMath::Min(Min.X, QuadTreeElement.GetRect().Min.X);
        Min.Y = FMath::Min(Min.Y, QuadTreeElement.GetRect().Min.Y);

        Max.X = FMath::Max(Max.X, QuadTreeElement.GetRect().Max.X);
        Max.Y = FMath::Max(Max.Y, QuadTreeElement.GetRect().Max.Y);
    }

    TUniquePtr<FVoronoiQuadTree> QuadTree = MakeUnique<FVoronoiQuadTree>(Min, Max);
    for (const FQuadTreeElement& QuadTreeElement : QuadTreeElements)
        QuadTree->Insert(QuadTreeElement);

    return QuadTree;
}
