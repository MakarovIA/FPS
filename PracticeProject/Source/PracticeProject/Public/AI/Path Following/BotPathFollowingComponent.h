// By Polyakov Pavel

#pragma once

#include "BezierCurve.h"
#include "Navigation/PathFollowingComponent.h"
#include "BotPathFollowingComponent.generated.h"

class FVoronoiFace;

/** PathFollowing component that moves along splines instead of straight lines */
UCLASS(HideCategories = (Variable, Sockets, Tags, Activation, Events))
class PRACTICEPROJECT_API UBotPathFollowingComponent : public UPathFollowingComponent
{
    GENERATED_BODY()

    /** Some parameters on how a Bezier curve is built */
    UPROPERTY(EditDefaultsOnly, Category = BotPathFollowingConfig)
    FBezierCurveBuildParams BezierCurveBuildParams;

    /** A spline bot is moving along */
    TUniquePtr<FSpline> Spline;

    /** Last face bot was standing on */
    FVoronoiFace* LastFaceStandOn;

protected:
    virtual void OnPathUpdated() override;
    virtual void FollowPathSegment(float DeltaTime) override;

    virtual FVector GetMoveFocus(bool bAllowStrafe) const override;

    /** Direction of our last move */
    FVector CurrentMoveDirection;

public:
    UBotPathFollowingComponent() {}
};
