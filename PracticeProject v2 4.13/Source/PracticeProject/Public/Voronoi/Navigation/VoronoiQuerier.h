// By Polyakov Pavel

#pragma once

#include "VoronoiGraph.h"
#include "VoronoiQuerier.generated.h"

/** Class needed to support Cast<IVoronoiQuerier>(Object) */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UVoronoiQuerier : public UInterface
{
    GENERATED_UINTERFACE_BODY()
};

/** Smth that is able to make customized queries to VoronoiNavData */
class IVoronoiQuerier
{
    GENERATED_IINTERFACE_BODY()

    /** Get a penalty value for rotation used in pathfinding */
    virtual float GetPenaltyForRotation() const { check(0); return 0; }

    /** Get a penalty mutiplier for crouching on the way */
    virtual float GetPenaltyMultiplierForCrouch() const { check(0); return 0; }

    /** Get a penalty value for jump */
    virtual float GetPenaltyForJump() const { check(0); return 0; }

    /** Calculate additional penalty for movement between faces (negative value means no way) */
    virtual float GetAdditionalPenalty(FVoronoiFace *From, FVoronoiFace *To) const { return 0; }

    /** Get a rotation a querier has at the moment he is doing a request */
    virtual FVector2D GetInitialRotation() const { return FVector2D(0, 0); }

    /** Whether customizing queries feature is on */
    virtual bool IsVoronoiQuerier() const { return true; }
};
