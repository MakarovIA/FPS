// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiGraph.h"

/** FVoronoiGraph */

int32 VERSION = 7;
FName FRIENDLY_NAME = TEXT("VoronoiVersion");
FGuid GUID(0x8335131F, 0x7EA94C36, 0xBDAD5D6D, 0xB93EAD9F);
FCustomVersionRegistration GRegisterVoronoiCustomVersion(GUID, 1, FRIENDLY_NAME);

double FVoronoiFace::getKills(EStatisticKey key) const
{
	double result = 0;
	if (this->onlineStats.kills.find(EStatisticKey::SKT_Bots_1) != this->onlineStats.kills.end())
	{
		result = this->onlineStats.kills.at(EStatisticKey::SKT_Bots_1);
	}
	return result;
}

double FVoronoiFace::getDeaths(EStatisticKey key) const
{
	double result = 0;
	if (this->onlineStats.deaths.find(EStatisticKey::SKT_Bots_1) != this->onlineStats.deaths.end())
	{
		result = this->onlineStats.deaths.at(EStatisticKey::SKT_Bots_1);
	}
	return result;
}

TPreserveConstUniquePtr<FVoronoiFace>* FVoronoiGraph::GetNearestFaceToLoc(FVector location)
{
	TPreserveConstUniquePtr<FVoronoiFace>* result = nullptr;

	for (auto& i : this->Surfaces)
		for (auto& FacePtr : i->Faces) 
		{
			FVector faceLoc = FacePtr->Location;
			if (!result || FVector::Dist(faceLoc, location) < FVector::Dist((*result)->Location, location))
				result = &FacePtr;
		}

	return result;
}

void FVoronoiGraph::addOnlineStatisticInFromOldGraph(const FVoronoiGraph * oldGraph) 
{
	for (int i = 0; i < oldGraph->journal.kills.size(); i++) 
	{
		this->addKill(oldGraph->journal.kills[i].second, oldGraph->journal.kills[i].first);
	}

	for (int i = 0; i < oldGraph->journal.deaths.size(); i++) 
	{
		this->addDeath(oldGraph->journal.deaths[i].second, oldGraph->journal.deaths[i].first);
	}
}

void FVoronoiGraph::diffusion(double& valueA, double& valueB, const double diffusion_coef) 
{
	const double a = valueA;
	const double b = valueB;

	valueA += diffusion_coef * (b - a);
	valueB += diffusion_coef * (a - b);
}

void FVoronoiGraph::diffuseStatistic(EStatisticKey key, const double diffusion_coef)
{
	/*
	struct cmpFVector 
	{
		bool operator()(const FVector& a, const FVector& b) const 
		{
			return a.X < b.X || (a.X == b.X && a.Y < b.Y) || (a.X == b.X && a.Y == b.Y && a.Z < b.Z);
		}
	};

	std::map<FVector, std::pair<double, double>, cmpFVector> result;

	for (const auto& surface : this->Surfaces)
		for (const auto& face : surface->Faces) 
		{
			result[face->Location] = std::make_pair(face->onlineStats.deaths[key], face->onlineStats.kills[key]);
		}


	for (const auto& surface : this->Surfaces)
		for (const auto& edge : surface->Edges) 
		{
			FVector faceALocation = edge->LeftFace->Location;
			FVector faceBLocation = edge->RightFace->Location;
			diffusion(result[faceALocation].first, result[faceBLocation].first, diffusion_coef);
			diffusion(result[faceALocation].second, result[faceBLocation].second, diffusion_coef);
		}


	for (const auto& surface : this->Surfaces)
		for (const auto& face : surface->Faces) 
		{
			face->onlineStats.deaths[key] = result[face->Location].first;
			face->onlineStats.kills[key] = result[face->Location].second;
		}
	*/
}

void FVoronoiGraph::addKill(EStatisticKey entity, FVector location) 
{
	this->journal.kills.push_back(std::make_pair(location, entity));
	(*GetNearestFaceToLoc(location))->onlineStats.kills[entity]++;
}

void FVoronoiGraph::addDeath(EStatisticKey entity, FVector location) 
{
	this->journal.kills.push_back(std::make_pair(location, entity));
	(*GetNearestFaceToLoc(location))->onlineStats.deaths[entity]++;
}

bool FVoronoiGraph::Serialize(FArchive &Ar)
{
    check(GeneratedSurfaces.Num() == 0);

    const FCustomVersion *VoronoiVersion = Ar.GetCustomVersions().GetVersion(GUID);
    if (Ar.IsLoading() && VoronoiVersion == nullptr)
        return false; 

    const int64 SizePosition = Ar.Tell();
    int32 SizeInBytes = 0, ArchiveVersion = VERSION;
    Ar << SizeInBytes << ArchiveVersion;

    if (Ar.IsLoading() && ArchiveVersion != VERSION)
    {
        Ar.Seek(Ar.Tell() + SizeInBytes - sizeof(SizeInBytes) - sizeof(ArchiveVersion));
        return false;
    }

    SerializeInternal(Ar);

    if (Ar.IsSaving())
    {
        const int64 CurrentPosition = Ar.Tell();
        SizeInBytes = CurrentPosition - SizePosition;

        Ar.Seek(SizePosition);
        Ar << SizeInBytes;
        Ar.Seek(CurrentPosition);

        Ar.SetCustomVersion(GUID, 1, FRIENDLY_NAME);
    }

    return true;
}

void FVoronoiGraph::SerializeInternal(FArchive &Ar)
{
    TMap<FVoronoiSurface*, int32> SurfacesIndexes;
    TArray<TMap<FVoronoiFace*, int32>> FacesIndexes;
    TArray<TMap<FVoronoiEdge*, int32>> EdgesIndexes;
    TArray<TMap<FVoronoiVertex*, int32>> VertexesIndexes;

    int32 SurfaceNumber = Surfaces.Num();
    Ar << SurfaceNumber;

	SurfacesIndexes.Reserve(SurfaceNumber);
    FacesIndexes.Init(TMap<FVoronoiFace*, int32>(), SurfaceNumber);
    EdgesIndexes.Init(TMap<FVoronoiEdge*, int32>(), SurfaceNumber);
    VertexesIndexes.Init(TMap<FVoronoiVertex*, int32>(), SurfaceNumber);

    if (Ar.IsLoading())
        Surfaces.Reset(SurfaceNumber);

    for (int32 i = 0; i < SurfaceNumber; ++i)
    {
        if (Ar.IsLoading()) Surfaces.Add(MakeUnique<FVoronoiSurface>());
        if (Ar.IsSaving())  SurfacesIndexes.Add(Surfaces[i], i);

        int32 FacesNumber = Surfaces[i]->Faces.Num();
        int32 EdgesNumber = Surfaces[i]->Edges.Num();
        int32 VertexesNumber = Surfaces[i]->Vertexes.Num();

        Ar << FacesNumber << EdgesNumber << VertexesNumber;

        if (Ar.IsLoading())
        {
            Surfaces[i]->Faces.Reserve(FacesNumber);
            Surfaces[i]->Edges.Reserve(EdgesNumber);
            Surfaces[i]->Vertexes.Reserve(VertexesNumber);
        }

        EdgesIndexes[i].Reserve(EdgesNumber);
        VertexesIndexes[i].Reserve(VertexesNumber);
        FacesIndexes[i].Reserve(FacesNumber);

        for (int32 j = 0; j < FacesNumber; ++j)
        {
            if (Ar.IsLoading()) Surfaces[i]->Faces.Add(MakeUnique<FVoronoiFace>());
            if (Ar.IsSaving())  FacesIndexes[i].Add(Surfaces[i]->Faces[j], j);
        }

        for (int32 j = 0; j < EdgesNumber; ++j)
        {
            if (Ar.IsLoading()) Surfaces[i]->Edges.Add(MakeUnique<FVoronoiEdge>());
            if (Ar.IsSaving())  EdgesIndexes[i].Add(Surfaces[i]->Edges[j], j);
        }

        for (int32 j = 0; j < VertexesNumber; ++j)
        {
            if (Ar.IsLoading()) Surfaces[i]->Vertexes.Add(MakeUnique<FVoronoiVertex>());
            if (Ar.IsSaving())  VertexesIndexes[i].Add(Surfaces[i]->Vertexes[j], j);
        }
    }

    for (int32 i = 0; i < SurfaceNumber; ++i)
    {
        Ar << Surfaces[i]->Borders;

        for (int32 j = 0, FacesNumber = Surfaces[i]->Faces.Num(); j < FacesNumber; ++j)
        {
            int32 EdgeID = Ar.IsSaving() ? EdgesIndexes[i][Surfaces[i]->Faces[j]->Edge] : 0;
            int32 LinksNumber = Surfaces[i]->Faces[j]->Links.Num();

            Ar << Surfaces[i]->Faces[j]->Location << EdgeID << LinksNumber;

            if (Ar.IsLoading())
            {
                Surfaces[i]->Faces[j]->Edge = Surfaces[i]->Edges[EdgeID];
                Surfaces[i]->Faces[j]->Links.Reserve(LinksNumber);
                Surfaces[i]->Faces[j]->Surface = Surfaces[i];
            }
            
            for (int32 k = 0; k < LinksNumber; ++k)
            {
                int32 SurfaceID = Ar.IsSaving() ? SurfacesIndexes[Surfaces[i]->Faces[j]->Links[k].Face->Surface] : 0;
                int32 FaceID = Ar.IsSaving() ? FacesIndexes[SurfaceID][Surfaces[i]->Faces[j]->Links[k].Face] : 0;

                uint8 bJumpRequired = Ar.IsSaving() ? Surfaces[i]->Faces[j]->Links[k].bJumpRequired : 0;
                Ar << SurfaceID << FaceID << bJumpRequired;

                if (Ar.IsLoading())
                    Surfaces[i]->Faces[j]->Links.Emplace(Surfaces[SurfaceID]->Faces[FaceID], bJumpRequired);
            }

            Ar.Serialize(&Surfaces[i]->Faces[j]->Flags, sizeof(FVoronoiFlags));
            Ar.Serialize(&Surfaces[i]->Faces[j]->TacticalProperties, sizeof(FVoronoiTacticalProperties));
        }

        for (int32 j = 0, EdgesNumber = Surfaces[i]->Edges.Num(); j < EdgesNumber; ++j)
        {
            int32 LeftFaceID = Ar.IsSaving() && Surfaces[i]->Edges[j]->LeftFace ? FacesIndexes[i][Surfaces[i]->Edges[j]->LeftFace] : -1;
            int32 RightFaceID = Ar.IsSaving() && Surfaces[i]->Edges[j]->RightFace ? FacesIndexes[i][Surfaces[i]->Edges[j]->RightFace] : -1;

            int32 BeginID = Ar.IsSaving() ? VertexesIndexes[i][Surfaces[i]->Edges[j]->FirstVertex] : 0;
            int32 EndID = Ar.IsSaving() ? VertexesIndexes[i][Surfaces[i]->Edges[j]->LastVertex] : 0;

            int32 NextID = Ar.IsSaving() ? EdgesIndexes[i][Surfaces[i]->Edges[j]->NextEdge] : 0;
            int32 PreviousID = Ar.IsSaving() ? EdgesIndexes[i][Surfaces[i]->Edges[j]->PreviousEdge] : 0;

            Ar << LeftFaceID << RightFaceID << BeginID << EndID << NextID << PreviousID;

            if (Ar.IsLoading())
            {
                Surfaces[i]->Edges[j]->LeftFace = LeftFaceID != -1 ? (FVoronoiFace*)Surfaces[i]->Faces[LeftFaceID] : nullptr;
                Surfaces[i]->Edges[j]->RightFace = RightFaceID != -1 ? (FVoronoiFace*)Surfaces[i]->Faces[RightFaceID] : nullptr;

                Surfaces[i]->Edges[j]->FirstVertex = Surfaces[i]->Vertexes[BeginID];
                Surfaces[i]->Edges[j]->LastVertex = Surfaces[i]->Vertexes[EndID];

                Surfaces[i]->Edges[j]->NextEdge = Surfaces[i]->Edges[NextID];
                Surfaces[i]->Edges[j]->PreviousEdge = Surfaces[i]->Edges[PreviousID];
            }
        }

        for (int32 j = 0, VertexesNumber = Surfaces[i]->Vertexes.Num(); j < VertexesNumber; ++j)
        {
            int32 EdgeID = Ar.IsSaving() ? EdgesIndexes[i][Surfaces[i]->Vertexes[j]->Edge] : 0;
            Ar << Surfaces[i]->Vertexes[j]->Location << EdgeID;

            if (Ar.IsLoading())
                Surfaces[i]->Vertexes[j]->Edge = Surfaces[i]->Edges[EdgeID];
        }

        if (Ar.IsLoading())
            Surfaces[i]->QuadTree = FVoronoiQuadTree::ConstructFromSequence(Surfaces[i]->Faces);
    }
}
