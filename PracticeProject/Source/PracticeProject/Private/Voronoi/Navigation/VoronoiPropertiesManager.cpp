// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiNavData.h"

#include "VoronoiPropertiesManager.h"

AVoronoiNavData* UVoronoiPropertiesManager::GetVoronoiNavData(const UObject* WorldContextObject)
{
    if (UWorld* World = WorldContextObject->GetWorld())
        if (auto Iter = TActorIterator<AVoronoiNavData>(World))
            return *Iter;

    return nullptr;
}

bool UVoronoiPropertiesManager::GetVoronoiPropertiesForLocation(const UObject* WorldContextObject, const FVector& Location, float &BaseCost, float &BaseEnterCost, bool &bNoWay)
{
    if (const AVoronoiNavData* VoronoiData = GetVoronoiNavData(WorldContextObject))
    {
        if (const FVoronoiFace *Face = VoronoiData->GetFaceByPoint(Location))
        {
            BaseCost = Face->Properties.BaseCost;
            BaseEnterCost = Face->Properties.BaseEnterCost;
            bNoWay = Face->Properties.bNoWay;

            return true;
        }
    }
    
    return false;
}

bool UVoronoiPropertiesManager::SetVoronoiPropertiesForLocation(const UObject* WorldContextObject, const FVector& Location, float BaseCost, float BaseEnterCost, bool bNoWay)
{
    if (AVoronoiNavData* VoronoiData = GetVoronoiNavData(WorldContextObject))
    {
        if (FVoronoiFace *Face = VoronoiData->GetFaceByPoint(Location))
        {
            Face->Properties.BaseCost = FMath::Max(1.f, BaseCost);
            Face->Properties.BaseEnterCost = FMath::Max(0.f, BaseEnterCost);
            Face->Properties.bNoWay = bNoWay;

            VoronoiData->MarkPathsAsNeedingUpdate();

            return true;
        }
    }

    return false;
}

bool UVoronoiPropertiesManager::SetVoronoiPropertiesForArea(const UObject* WorldContextObject, const FVector& Origin, float Radius, float BaseCost, float BaseEnterCost, bool bNoWay)
{
    BaseCost = FMath::Max(1.f, BaseCost);
    BaseEnterCost = FMath::Max(0.f, BaseEnterCost);

    if (AVoronoiNavData* VoronoiData = GetVoronoiNavData(WorldContextObject))
    {
        if (FVoronoiFace *Face = VoronoiData->GetFaceByPoint(Origin))
        {
            for (FVoronoiFace *i : VoronoiData->GetReachableFacesInRadius(Origin, Radius))
            {
                i->Properties.BaseCost = FMath::Max(1.f, BaseCost);
                i->Properties.BaseEnterCost = FMath::Max(0.f, BaseEnterCost);
                i->Properties.bNoWay = bNoWay;
            }

            VoronoiData->MarkPathsAsNeedingUpdate();

            return true;
        }
    }

    return false;
}

bool UVoronoiPropertiesManager::IsLocationNavigable(const UObject* WorldContextObject, const FVector& Location)
{
    if (const AVoronoiNavData* VoronoiData = GetVoronoiNavData(WorldContextObject))
        if (const FVoronoiFace *Face = VoronoiData->GetFaceByPoint(Location))
            return Face->IsNavigable();

    return false;
}
