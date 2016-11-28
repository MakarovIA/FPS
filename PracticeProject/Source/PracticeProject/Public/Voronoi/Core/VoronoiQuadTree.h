// By Polyakov Pavel

#pragma once

class FVoronoiFace;
class FVoronoiGraph;

/** BSP tree for storing Voronoi faces */
class PRACTICEPROJECT_API FVoronoiQuadTree final
{
public:
    /** A face and rectangle that this face lies in */
    class PRACTICEPROJECT_API FQuadTreeElement final
    {
        FVoronoiFace *Face;
        FBox2D Rect;

    public:
        FQuadTreeElement(FVoronoiFace *InFace);

        /** Get face assosiated with this element */
        FORCEINLINE FVoronoiFace* GetFace() const { return Face; }

        /** Get bounds of this element */
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

        friend FVoronoiGraph;

    public:
        FQuadTreeNode(const FBox2D& InRect) : Rect(InRect), bIsSubdivided(false) {}

        /** Inserts a new element and subdivide if needed */
        void Insert(const FQuadTreeElement& NewElement);

        /** Subdivides this node in four parts */
        void Subdivide();

        /** Get bounds of this node */
        FORCEINLINE const FBox2D& GetRect() const { return Rect; }

        /** Get an array of elements inside this node */
        FORCEINLINE const TArray<FQuadTreeElement>& GetElements() const { return Elements; }

        /** Get an array of stucked elements inside this node */
        FORCEINLINE const TArray<FQuadTreeElement>& GetStuckedElements() const { return StuckedElements; }

        /** Returns whether this node is subdivided */
        FORCEINLINE bool IsSubdivided() const { return bIsSubdivided; }

        FORCEINLINE FQuadTreeNode* GetLU() const { return LU.Get(); }
        FORCEINLINE FQuadTreeNode* GetLD() const { return LD.Get(); }
        FORCEINLINE FQuadTreeNode* GetRU() const { return RU.Get(); }
        FORCEINLINE FQuadTreeNode* GetRD() const { return RD.Get(); }
    };

private:
    TUniquePtr<FQuadTreeNode> Root;
    friend FVoronoiGraph;

public:
    /** Init quadtree to work on a rectangle represented by Min and Max points */
    FVoronoiQuadTree(const FVector2D& Min, const FVector2D& Max) : Root(MakeUnique<FQuadTreeNode>(FBox2D(Min, Max))) {}

    /** Insert a new element into a quadtree */
    void Insert(const FQuadTreeElement& NewElement);

    /** Get a face by a given point */
    FVoronoiFace* GetFaceByPoint(const FVector2D& Point) const;

    /** Gets an array of faces within a given radius from a given point */
    TArray<FVoronoiFace*> GetFacesInCircle(const FVector2D& Point, float Radius) const;

    /** Gets an array of faces belonging to a given rectangle */
    TArray<FVoronoiFace*> GetFacesInRectangle(const FBox2D& Rect) const;

    /** Gets an array of faces which intersect (predicate returns true) with an area determined by a given predicate. Function expects: bool (*)(const FBox2D& InRect) */
    template<class Predicate>
    TArray<FVoronoiFace*> GetFacesByPredicate(Predicate Intersect) const;

    /** Retrieve tree's root */
    FORCEINLINE const FQuadTreeNode* GetRoot() const { return Root.Get(); }
};
