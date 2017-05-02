// By Polyakov Pavel

#pragma once

#include "BezierCurve.h"
#include "Navigation/PathFollowingComponent.h"
#include "BotPathFollowingComponent.generated.h"

struct FVoronoiFace;

/** PathFollowing component that moves along splines instead of straight lines */
UCLASS(HideCategories = (Variable, Sockets, Tags, Activation, Events))
class PRACTICEPROJECT_API UBotPathFollowingComponent : public UPathFollowingComponent
{
    GENERATED_BODY()

    /** Some parameters on how a Bezier curve is built */
    UPROPERTY(EditDefaultsOnly, Category = BotPathFollowingConfig)
    FBezierCurveBuildParams BezierCurveBuildParams;

    const FVoronoiFace* LastFaceStandOn;
    FVector CurrentMoveDirection;

    TUniquePtr<FSpline> Spline;

protected:
    virtual void OnPathUpdated() override;
    virtual void FollowPathSegment(float DeltaTime) override;

    virtual FVector GetMoveFocus(bool bAllowStrafe) const override;
};
