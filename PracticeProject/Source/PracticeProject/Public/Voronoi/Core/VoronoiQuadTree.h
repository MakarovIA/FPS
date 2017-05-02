// By Polyakov Pavel

#pragma once

template <typename T>
class TPreserveConstUniquePtr;
struct FVoronoiFace;

/** A face and rectangle this face lies in */
class PRACTICEPROJECT_API FQuadTreeElement final
{
    FVoronoiFace *Face;
    FBox2D Rect;

public:
    FQuadTreeElement(FVoronoiFace *InFace);

    FORCEINLINE const FVoronoiFace* GetFace() const { return Face; }
    FORCEINLINE FVoronoiFace* GetFace() { return Face; }

    FORCEINLINE const FBox2D& GetRect() const { return Rect; }
};

/** A node of a quad tree */
class PRACTICEPROJECT_API FQuadTreeNode final
{
    FBox2D Rect;
    TArray<FQuadTreeElement> Elements;
    TArray<FQuadTreeElement> StuckedElements;

    TUniquePtr<FQuadTreeNode> LU, LD, RU, RD;
    bool bIsSubdivided;

    void Subdivide();

public:
    FORCEINLINE FQuadTreeNode(const FBox2D& InRect)
        : Rect(InRect), bIsSubdivided(false) {}

    void Insert(const FQuadTreeElement& NewElement);

    FORCEINLINE const TArray<FQuadTreeElement>& GetElements() const { return Elements; }
    FORCEINLINE const TArray<FQuadTreeElement>& GetStuckedElements() const { return StuckedElements; }

    FORCEINLINE TArray<FQuadTreeElement>& GetElements() { return Elements; }
    FORCEINLINE TArray<FQuadTreeElement>& GetStuckedElements() { return StuckedElements; }

    FORCEINLINE bool IsSubdivided() const { return bIsSubdivided; }
    FORCEINLINE const FBox2D& GetRect() const { return Rect; }

    FORCEINLINE const FQuadTreeNode* GetLU() const { return LU.Get(); }
    FORCEINLINE const FQuadTreeNode* GetLD() const { return LD.Get(); }
    FORCEINLINE const FQuadTreeNode* GetRU() const { return RU.Get(); }
    FORCEINLINE const FQuadTreeNode* GetRD() const { return RD.Get(); }

    FORCEINLINE FQuadTreeNode* GetLU() { return LU.Get(); }
    FORCEINLINE FQuadTreeNode* GetLD() { return LD.Get(); }
    FORCEINLINE FQuadTreeNode* GetRU() { return RU.Get(); }
    FORCEINLINE FQuadTreeNode* GetRD() { return RD.Get(); }
};

/** BSP tree for storing Voronoi faces */
class PRACTICEPROJECT_API FVoronoiQuadTree final
{
    TUniquePtr<FQuadTreeNode> Root;

public:
    FORCEINLINE FVoronoiQuadTree(const FVector2D& Min, const FVector2D& Max)
        : Root(MakeUnique<FQuadTreeNode>(FBox2D(Min, Max))) {}

    FORCEINLINE void Insert(const FQuadTreeElement& NewElement) { Root->Insert(NewElement); }
    FORCEINLINE void Insert(FVoronoiFace* InFace) { Root->Insert(FQuadTreeElement(InFace)); }

    const FVoronoiFace* GetFaceByPoint(const FVector2D& Point) const;
    TArray<const FVoronoiFace*> GetFacesInCircle(const FVector2D& Point, float Radius) const;
    TArray<const FVoronoiFace*> GetFacesInRectangle(const FBox2D& Rect) const;
    TArray<const FVoronoiFace*> GetFacesByPredicate(TFunction<bool(const FBox2D&)> Predicate) const;

    FVoronoiFace* GetFaceByPoint(const FVector2D& Point);
    TArray<FVoronoiFace*> GetFacesInCircle(const FVector2D& Point, float Radius);
    TArray<FVoronoiFace*> GetFacesInRectangle(const FBox2D& Rect);
    TArray<FVoronoiFace*> GetFacesByPredicate(TFunction<bool(const FBox2D&)> Predicate);

    FORCEINLINE const FQuadTreeNode* GetRoot() const { return Root.Get(); }
    FORCEINLINE FQuadTreeNode* GetRoot() { return Root.Get(); }

    /** Constructs a quad tree for a surface using surface's faces */
    static TUniquePtr<FVoronoiQuadTree> ConstructFromSequence(TArray<TPreserveConstUniquePtr<FVoronoiFace>>& Faces);
};
