// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiSurface.h"
#include "VoronoiNavData.h"
#include "VoronoiPropertiesManager.h"

AVoronoiSurface::AVoronoiSurface()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent->SetMobility(EComponentMobility::Static);
}

#ifdef WITH_EDITOR
void AVoronoiSurface::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);

    const UWorld* World = GetWorld();
    if (!World || !World->GetNavigationSystem() || World->IsGameWorld())
        return;

    auto VoronoiData = UVoronoiPropertiesManager::GetVoronoiNavData(World);
    if (VoronoiData && VoronoiData->IsLiveGenerated())
        VoronoiData->RebuildAll();
}

void AVoronoiSurface::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const UWorld* World = GetWorld();
    if (!World || World->IsPendingKill() || !World->GetNavigationSystem() || World->IsGameWorld())
        return;

    auto VoronoiData = UVoronoiPropertiesManager::GetVoronoiNavData(World);
    if (VoronoiData && VoronoiData->IsLiveGenerated())
        VoronoiData->RebuildAll();
}
#endif
