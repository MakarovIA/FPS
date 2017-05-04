// By Polyakov Pavel

#pragma once

#include "VoronoiGraph.h"
#include "VoronoiQuerier.generated.h"

/** Voronoi querier parameters for pathfinding */
USTRUCT()
struct PRACTICEPROJECT_API FVoronoiQuerierParameters
{
    GENERATED_USTRUCT_BODY()

    /** Penalty value for rotation used in pathfinding */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1000.0"))
    float PenaltyForRotation;

    /** Penalty mutiplier for crouching on the way */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1.0", ClampMax = "100.0"))
    float PenaltyMultiplierForCrouch;

    /** Penalty value for jump */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "10000.0"))
    float PenaltyForJump;

    FORCEINLINE FVoronoiQuerierParameters()
        : PenaltyForRotation(200), PenaltyMultiplierForCrouch(1), PenaltyForJump(0) {}
};

/** Class needed to support Cast<IVoronoiQuerier>(Object) */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UVoronoiQuerier : public UInterface
{
    GENERATED_UINTERFACE_BODY()
};

/** Something able to make customized queries to VoronoiNavData */
class PRACTICEPROJECT_API IVoronoiQuerier
{
    GENERATED_IINTERFACE_BODY()

    /** Get penalty modification parameters for pathfinding */
    virtual const FVoronoiQuerierParameters& GetVoronoiQuerierParameters() const { check(0); static FVoronoiQuerierParameters Dummy; return Dummy; }

    /** Calculate additional penalty for movement between faces (negative value means no way) */
    virtual float GetAdditionalPenalty(const FVoronoiFace *From, const FVoronoiFace *To) const { return 0; }

    /** Get a rotation a querier has at the moment he is doing a request */
    virtual FVector2D GetInitialRotation() const { return FVector2D(0, 0); }

    /** Whether customizing queries feature is on */
    virtual bool IsVoronoiQuerier() const { return false; }
};
