// By Polyakov Pavel

#pragma once

#include "GameFramework/Actor.h"
#include "VoronoiSite.generated.h"

/** Site that will be used by FVoronoiGeneratorTask to create a Voronoi diagram */
UCLASS(NotBlueprintable, HideCategories = (Input, Rendering, Actor))
class PRACTICEPROJECT_API AVoronoiSite : public AActor
{
    GENERATED_BODY()

    USphereComponent *Sphere;

    /** Cost for moving in a Face defined by this site */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Voronoi, meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
    float BaseCost;

    /** Cost for entering a Face defined by this site */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Voronoi, meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
    float BaseEnterCost;

    /** Whether entering a Face defined by this site is not allowed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Voronoi, meta = (AllowPrivateAccess = "true"))
    bool bNoWay;

public:
    AVoronoiSite();
    virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

#ifdef WITH_EDITOR
    /** Overriden to initiate rebuild of Voronoi graph */
    virtual void PostEditMove(bool bFinished);

    /** Overriden to initiate rebuild of Voronoi graph */
    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

    FORCEINLINE float GetBaseCost() const { return BaseCost; }
    FORCEINLINE float GetBaseEnterCost() const { return BaseEnterCost; }
    FORCEINLINE bool  IsNoWay() const { return bNoWay; }
};
