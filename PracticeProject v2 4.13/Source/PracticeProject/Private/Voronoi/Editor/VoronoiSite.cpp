// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiSite.h"
#include "VoronoiNavData.h"
#include "VoronoiPropertiesManager.h"
#include "VoronoiRenderingComponent.h"

AVoronoiSite::AVoronoiSite()
{
    Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
    Sphere->ShapeColor = FColor(0, 255, 0);
    Sphere->SetHiddenInGame(false);
    Sphere->SetSphereRadius(10);

    Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sphere->SetMobility(EComponentMobility::Static);

    Sphere->bCastDynamicShadow = false;
    Sphere->bCastStaticShadow = false;

    RootComponent = Sphere;

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    BaseCost = 1;
    BaseEnterCost = 0;
    bNoWay = false;
}

void AVoronoiSite::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
    Super::TickActor(DeltaTime, TickType, ThisTickFunction);

    const UWorld* World = GetWorld();
    if (!World || World->IsPendingKill() || !World->GetNavigationSystem())
        return;

    auto VoronoiData = UVoronoiPropertiesManager::GetVoronoiNavData(World);
    if (!VoronoiData)
        return;

    bool bShowNavigation = UVoronoiRenderingComponent::IsNavigationShowFlagSet(World) && VoronoiData->GetDrawingOptions().bDrawSites && VoronoiData->IsDrawingEnabled();
    if (Sphere->bVisible != bShowNavigation)
        Sphere->SetVisibility(bShowNavigation);
}

#ifdef WITH_EDITOR
void AVoronoiSite::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);

    const UWorld* World = GetWorld();
    if (!World || !World->GetNavigationSystem() || World->IsGameWorld())
        return;

    auto VoronoiData = UVoronoiPropertiesManager::GetVoronoiNavData(World);
    if (VoronoiData && VoronoiData->IsLiveGenerated())
        VoronoiData->RebuildAll();
}

void AVoronoiSite::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const UWorld* World = GetWorld();
    if (!World || !World->GetNavigationSystem() || World->IsGameWorld())
        return;

    auto VoronoiData = UVoronoiPropertiesManager::GetVoronoiNavData(World);
    if (VoronoiData && VoronoiData->IsLiveGenerated())
        VoronoiData->RebuildAll();
}
#endif
