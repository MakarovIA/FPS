// By Polyakov Pavel

#pragma once

#include "GameFramework/Actor.h"
#include "VoronoiSurface.generated.h"

/** A container for Voronoi sites */
UCLASS(NotBlueprintable, HideCategories = (Input, Rendering, Actor))
class PRACTICEPROJECT_API AVoronoiSurface : public AActor
{
    GENERATED_BODY()

    /** Z-coordinate of a surface */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi, meta = (AllowPrivateAccess = "true"))
    FVector2D Size;
    
public:
    AVoronoiSurface();

#ifdef WITH_EDITOR
    /** Overriden to initiate rebuild of Voronoi graph */
    virtual void PostEditMove(bool bFinished);

    /** Overriden to initiate rebuild of Voronoi graph */
    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

    /** Get Z-coordinate */
    FORCEINLINE const FVector2D& GetSize() const { return Size; }
};
