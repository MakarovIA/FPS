// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiGraph.h"

/** FVoronoiFlags */

const FVoronoiFlags& FVoronoiFlags::operator|=(const FVoronoiFlags& Other)
{
    bCrouchedOnly |= Other.bCrouchedOnly;
    bNoJump |= Other.bNoJump;

    bObstacle |= Other.bObstacle;
    bBorder |= Other.bBorder;

    bInvalid |= Other.bInvalid;

    return *this;
}

const FVoronoiFlags& FVoronoiFlags::operator&=(const FVoronoiFlags& Other)
{
    bCrouchedOnly &= Other.bCrouchedOnly;
    bNoJump &= Other.bNoJump;

    bObstacle &= Other.bObstacle;
    bBorder &= Other.bBorder;

    bInvalid &= Other.bInvalid;

    return *this;
}

FVoronoiFlags FVoronoiFlags::operator|(const FVoronoiFlags& Other)
{
    return FVoronoiFlags(*this) |= Other;
}

FVoronoiFlags FVoronoiFlags::operator&(const FVoronoiFlags& Other)
{
    return FVoronoiFlags(*this) &= Other;
}

/** FVoronoiGraph */

void FVoronoiGraph::Serialize(FArchive &Ar)
{
    static int32 VERSION = 3;
    static FName FRIENDLY_NAME = TEXT("VoronoiVersion");
    static FGuid GUID(0x8335131F, 0x7EA94C36, 0xBDAD5D6D, 0xB93EAD9F);
    static FCustomVersionRegistration GRegisterVoronoiCustomVersion(GUID, 1, FRIENDLY_NAME);

    // Called during generation
    check(GeneratedSurfaces.Num() == 0);

    // Check if archive contains no data
    const FCustomVersion *VoronoiVersion = Ar.GetCustomVersions().GetVersion(GUID);
    if (Ar.IsLoading() && VoronoiVersion == nullptr)
        return; 

    // Remember position of size variable
    const int64 SizePosition = Ar.Tell();

    // If saving this value will be corrected later
    int32 SizeInBytes = 0;
    Ar << SizeInBytes;

    // Check version
    int32 ArchiveVersion = VERSION;
    Ar << ArchiveVersion;

    // Version has changed so we can not use the data stored in the archive
    if (Ar.IsLoading() && ArchiveVersion != VERSION)
    {
        Ar.Seek(Ar.Tell() + SizeInBytes - sizeof(SizeInBytes) - sizeof(ArchiveVersion));
        return;
    }

    // Save GUID
    if (Ar.IsSaving())
        Ar.SetCustomVersion(GUID, 1, FRIENDLY_NAME);

    // Actual serialization
    SerializeInternal(Ar);

    // Size correction
    if (Ar.IsSaving())
    {
        const int64 CurrentPosition = Ar.Tell();
        Ar.Seek(SizePosition);

        SizeInBytes = CurrentPosition - SizePosition;
        Ar << SizeInBytes;

        Ar.Seek(CurrentPosition);
    }
}

void FVoronoiGraph::SerializeInternal(FArchive &Ar)
{
    int32 SurfaceNumber = Surfaces.Num();
    Ar << SurfaceNumber;

    if (Ar.IsLoading())
    {
        Surfaces.Reset();
        Surfaces.Reserve(SurfaceNumber);
    }

    TMap<FVoronoiSurface*, int32> SurfacesIndexes;
    SurfacesIndexes.Reserve(SurfaceNumber);

    TArray<TMap<FVoronoiFace*, int32>> FacesIndexes;
    FacesIndexes.Init(TMap<FVoronoiFace*, int32>(), SurfaceNumber);

    for (int32 i = 0; i < SurfaceNumber; ++i)
    {
        FVector SurfaceLocation = Ar.IsSaving() ? Surfaces[i]->GetLocation() : FVector();
        FVector2D SurfaceSize = Ar.IsSaving() ? Surfaces[i]->GetSize() : FVector2D();
        Ar << SurfaceLocation << SurfaceSize;

        if (Ar.IsLoading())
            Surfaces.Emplace(new FVoronoiSurface(SurfaceLocation, SurfaceSize));

        if (Ar.IsSaving())
            SurfacesIndexes.Add(Surfaces[i].Get(), i);

        // --------------------------------------------------------------------------------------------------------
        // Serialize faces, edges and vertexes
        // --------------------------------------------------------------------------------------------------------

        int32 FacesNumber = Surfaces[i]->GetFaces().Num();
        int32 EdgesNumber = Surfaces[i]->GetEdges().Num();
        int32 VertexesNumber = Surfaces[i]->GetVertexes().Num();

        Ar << FacesNumber << EdgesNumber << VertexesNumber;

        if (Ar.IsLoading())
        {
            Surfaces[i]->Faces.Reserve(FacesNumber);
            Surfaces[i]->Edges.Reserve(EdgesNumber);
            Surfaces[i]->Vertexes.Reserve(VertexesNumber);
        }

        TMap<FVoronoiEdge*, int32> EdgesIndexes;
        EdgesIndexes.Reserve(EdgesNumber);

        TMap<FVoronoiVertex*, int32> VertexesIndexes;
        VertexesIndexes.Reserve(VertexesNumber);

        for (int32 j = 0; j < FacesNumber; ++j)
        {
            FVector Location = Ar.IsSaving() ? Surfaces[i]->GetFaces()[j]->GetLocation() : FVector();
            Ar << Location;

            if (Ar.IsLoading())
                Surfaces[i]->Faces.Emplace(new FVoronoiFace(Surfaces[i].Get(), Location));

            Ar.Serialize(&Surfaces[i]->GetFaces()[j]->Flags, sizeof(FVoronoiFlags));
            Ar.Serialize(&Surfaces[i]->GetFaces()[j]->Properties, sizeof(FVoronoiProperties));
            Ar.Serialize(&Surfaces[i]->GetFaces()[j]->TacticalProperties, sizeof(FVoronoiTacticalProperties));

            if (Ar.IsSaving())
                FacesIndexes[i].Add(Surfaces[i]->GetFaces()[j].Get(), j);
        }

        for (int32 j = 0; j < EdgesNumber; ++j)
        {
            int32 LeftFaceID = Ar.IsSaving() ? FacesIndexes[i][Surfaces[i]->GetEdges()[j]->GetLeftFace()] : 0;
            int32 RightFaceID = Ar.IsSaving() ? FacesIndexes[i][Surfaces[i]->GetEdges()[j]->GetRightFace()] : 0;

            Ar << LeftFaceID << RightFaceID;

            if (Ar.IsLoading())
                Surfaces[i]->Edges.Emplace(new FVoronoiEdge(Surfaces[i]->GetFaces()[LeftFaceID].Get(), Surfaces[i]->GetFaces()[RightFaceID].Get()));

            if (Ar.IsSaving())
                EdgesIndexes.Add(Surfaces[i]->GetEdges()[j].Get(), j);
        }

        for (int32 j = 0; j < VertexesNumber; ++j)
        {
            FVector Location = Ar.IsSaving() ? Surfaces[i]->GetVertexes()[j]->GetLocation() : FVector();
            int32 EdgeID = Ar.IsSaving() ? EdgesIndexes[Surfaces[i]->GetVertexes()[j]->GetEdge()] : 0;

            Ar << Location << EdgeID;

            if (Ar.IsLoading())
                Surfaces[i]->Vertexes.Emplace(new FVoronoiVertex(Location, Surfaces[i]->GetEdges()[EdgeID].Get()));

            if (Ar.IsSaving())
                VertexesIndexes.Add(Surfaces[i]->GetVertexes()[j].Get(), j);
        }

        for (int32 j = 0; j < FacesNumber; ++j)
        {
            int32 EdgeID = -1;

            if (Ar.IsSaving())
                if (FVoronoiEdge *Edge = Surfaces[i]->GetFaces()[j]->GetEdge())
                    EdgeID = EdgesIndexes[Edge];

            Ar << EdgeID;

            if (Ar.IsLoading() && EdgeID != -1)
                Surfaces[i]->GetFaces()[j]->Edge = Surfaces[i]->GetEdges()[EdgeID].Get();
        }

        for (int32 j = 0; j < EdgesNumber; ++j)
        {
            int32 BeginID = Ar.IsSaving() ? VertexesIndexes[Surfaces[i]->GetEdges()[j]->GetFirstVertex()] : 0;
            int32 EndID = Ar.IsSaving() ? VertexesIndexes[Surfaces[i]->GetEdges()[j]->GetLastVertex()] : 0;

            int32 NextID = -1;
            int32 PreviousID = -1;

            if (Ar.IsSaving())
            {
                FVoronoiEdge *Next = Surfaces[i]->GetEdges()[j]->GetNextEdge();
                FVoronoiEdge *Previous = Surfaces[i]->GetEdges()[j]->GetPreviousEdge();

                if (Next != nullptr)
                    NextID = EdgesIndexes[Next];

                if (Previous != nullptr)
                    PreviousID = EdgesIndexes[Previous];
            }

            Ar << BeginID << EndID << NextID << PreviousID;

            if (Ar.IsLoading())
            {
                Surfaces[i]->GetEdges()[j]->Begin = Surfaces[i]->GetVertexes()[BeginID].Get();
                Surfaces[i]->GetEdges()[j]->End = Surfaces[i]->GetVertexes()[EndID].Get();

                Surfaces[i]->GetEdges()[j]->Next = NextID != -1 ? Surfaces[i]->GetEdges()[NextID].Get() : nullptr;
                Surfaces[i]->GetEdges()[j]->Previous = PreviousID != -1 ? Surfaces[i]->GetEdges()[PreviousID].Get() : nullptr;
            }
        }

        // --------------------------------------------------------------------------------------------------------
        // Serialize quad tree
        // --------------------------------------------------------------------------------------------------------

        FVector2D RootMin = Ar.IsSaving() ? Surfaces[i]->GetQuadTree()->GetRoot()->GetRect().Min : FVector2D();
        FVector2D RootMax = Ar.IsSaving() ? Surfaces[i]->GetQuadTree()->GetRoot()->GetRect().Max : FVector2D();

        Ar << RootMin << RootMax;

        if (Ar.IsLoading())
            Surfaces[i]->QuadTree = MakeUnique<FVoronoiQuadTree>(RootMin, RootMax);

        TArray<FVoronoiQuadTree::FQuadTreeNode*> Stack;
        Stack.Push(Surfaces[i]->GetQuadTree()->Root.Get());

        while (Stack.Num() > 0)
        {
            const auto Top = Stack.Last();
            Stack.Pop();

            Ar << Top->bIsSubdivided;

            int32 StuckedNumber = Top->GetStuckedElements().Num();
            Ar << StuckedNumber;

            if (Ar.IsLoading())
                Top->StuckedElements.Reserve(StuckedNumber);

            for (int32 j = 0; j < StuckedNumber; ++j)
            {
                int32 FaceID = Ar.IsSaving() ? FacesIndexes[i][Top->GetStuckedElements()[j].GetFace()] : 0;
                Ar << FaceID;

                if (Ar.IsLoading())
                    Top->StuckedElements.Emplace(Surfaces[i]->GetFaces()[FaceID].Get());
            }

            if (Top->bIsSubdivided)
            {
                FVector2D LUMin = Ar.IsSaving() ? Top->GetLU()->GetRect().Min : FVector2D();
                FVector2D LUMax = Ar.IsSaving() ? Top->GetLU()->GetRect().Max : FVector2D();

                FVector2D LDMin = Ar.IsSaving() ? Top->GetLD()->GetRect().Min : FVector2D();
                FVector2D LDMax = Ar.IsSaving() ? Top->GetLD()->GetRect().Max : FVector2D();

                FVector2D RUMin = Ar.IsSaving() ? Top->GetRU()->GetRect().Min : FVector2D();
                FVector2D RUMax = Ar.IsSaving() ? Top->GetRU()->GetRect().Max : FVector2D();

                FVector2D RDMin = Ar.IsSaving() ? Top->GetRD()->GetRect().Min : FVector2D();
                FVector2D RDMax = Ar.IsSaving() ? Top->GetRD()->GetRect().Max : FVector2D();

                Ar << LUMin << LUMax << LDMin << LDMax << RUMin << RUMax << RDMin << RDMax;

                if (Ar.IsLoading())
                {
                    Top->LU = MakeUnique<FVoronoiQuadTree::FQuadTreeNode>(FBox2D(LUMin, LUMax));
                    Top->LD = MakeUnique<FVoronoiQuadTree::FQuadTreeNode>(FBox2D(LDMin, LDMax));
                    Top->RU = MakeUnique<FVoronoiQuadTree::FQuadTreeNode>(FBox2D(RUMin, RUMax));
                    Top->RD = MakeUnique<FVoronoiQuadTree::FQuadTreeNode>(FBox2D(RDMin, RDMax));
                }

                Stack.Push(Top->GetLU());
                Stack.Push(Top->GetLD());
                Stack.Push(Top->GetRU());
                Stack.Push(Top->GetRD());
            } 
            else
            {
                int32 ElementsNumber = Top->GetElements().Num();
                Ar << ElementsNumber;

                if (Ar.IsLoading())
                    Top->Elements.Reserve(StuckedNumber);

                for (int32 j = 0; j < ElementsNumber; ++j)
                {
                    int32 FaceID = Ar.IsSaving() ? FacesIndexes[i][Top->GetElements()[j].GetFace()] : 0;
                    Ar << FaceID;

                    if (Ar.IsLoading())
                        Top->Elements.Emplace(Surfaces[i]->GetFaces()[FaceID].Get());
                }
            }
        }
    }

    // Links serialation
    for (int32 i = 0; i < SurfaceNumber; ++i)
    {
        for (int32 j = 0, FacesNumber = Surfaces[i]->GetFaces().Num(); j < FacesNumber; ++j)
        {
            int32 LinksNumber = Surfaces[i]->GetFaces()[j]->Links.Num();
            Ar << LinksNumber;

            if (Ar.IsLoading())
                Surfaces[i]->GetFaces()[j]->Links.Reserve(LinksNumber);

            for (int32 t = 0; t < LinksNumber; ++t)
            {
                int32 SurfaceID = Ar.IsSaving() ? SurfacesIndexes[Surfaces[i]->GetFaces()[j]->GetLinks()[t].GetFace()->GetSurface()] : 0;
                int32 FaceID = Ar.IsSaving() ? FacesIndexes[SurfaceID][Surfaces[i]->GetFaces()[j]->GetLinks()[t].GetFace()] : 0;
                bool bJumpRequired = Ar.IsSaving() ? Surfaces[i]->GetFaces()[j]->GetLinks()[t].IsJumpRequired() : false;

                Ar << SurfaceID << FaceID << bJumpRequired;

                if (Ar.IsLoading())
                    Surfaces[i]->GetFaces()[j]->Links.Emplace(Surfaces[SurfaceID]->GetFaces()[FaceID].Get(), bJumpRequired);
            }
        }
    }
}
