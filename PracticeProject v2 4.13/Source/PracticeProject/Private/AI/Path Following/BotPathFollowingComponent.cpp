// By Polyakov Pavel

#include "PracticeProject.h"
#include "BotPathFollowingComponent.h"
#include "VoronoiNavData.h"

#include "PathRenderer.h"

/** UBotPathFollowingComponent */

void UBotPathFollowingComponent::OnPathUpdated()
{
    if (Path.IsValid() && MovementComp != nullptr)
        Spline = FBezierCurveBuilder::BuildBezierCurve(Path->GetPathPoints(), CurrentMoveDirection, BezierCurveBuildParams, MovementComp->NavAgentProps, GetWorld());

    LastFaceStandOn = nullptr;
}

void UBotPathFollowingComponent::FollowPathSegment(float DeltaTime)
{
    if (MovementComp == nullptr || !Path.IsValid() || !Spline.IsValid())
    {
        Super::FollowPathSegment(DeltaTime);
        return;
    }

    auto CharMoveComp = Cast<UCharacterMovementComponent>(MovementComp);
    if (CharMoveComp == nullptr)
        return;

    // -----------------------------------------------------------------------------------------------
    // Manage movement state
    // -----------------------------------------------------------------------------------------------

    const FVector CurrentLocation = CharMoveComp->GetActorFeetLocation();
    const float MaxSpeed = CharMoveComp->IsCrouching() ? CharMoveComp->MaxWalkSpeedCrouched : CharMoveComp->MaxWalkSpeed;

    if (const auto VPath = Path->CastPath<FVoronoiNavigationPath>())
    {
        if (const auto VoronoiNavData = static_cast<AVoronoiNavData*>(VPath->GetNavigationDataUsed()))
        {
            FVoronoiFace *Face = VoronoiNavData->GetFaceByPoint(CurrentLocation);
            if (Face != nullptr && Face->IsNavigable())
            {
                CharMoveComp->bWantsToCrouch = Face->GetFlags().bCrouchedOnly;

                if (CharMoveComp->MovementMode == EMovementMode::MOVE_Walking)
                    LastFaceStandOn = Face;

                if (VPath->JumpPositions.Contains(Face))
                {
                    if (CharMoveComp->MovementMode == EMovementMode::MOVE_Walking)
                        CharMoveComp->DoJump(false);
                    else if (LastFaceStandOn != Face)
                    {
                        CharMoveComp->Velocity.X = 0;
                        CharMoveComp->Velocity.Y = 0;
                        UpdateMoveFocus();
                        return;
                    }
                }
            }
        }
    }

    // -----------------------------------------------------------------------------------------------
    // Manage movement direction
    // -----------------------------------------------------------------------------------------------

    const double PossibleDistance = MaxSpeed * DeltaTime;
    const FVector CurrentTarget = Spline->GetNextTargetLocation(CurrentLocation, PossibleDistance);

    CurrentMoveDirection = CurrentTarget - CurrentLocation;
    CurrentMoveDirection.Z = 0;
    CurrentMoveDirection = CurrentMoveDirection.GetSafeNormal();

    if (CharMoveComp->IsFalling())
    {
        CharMoveComp->Velocity.X = CurrentMoveDirection.X * MaxSpeed;
        CharMoveComp->Velocity.Y = CurrentMoveDirection.Y * MaxSpeed;
    }
    else
        CharMoveComp->RequestDirectMove(CurrentMoveDirection * MaxSpeed, true);

    UpdateMoveFocus();
}

FVector UBotPathFollowingComponent::GetMoveFocus(bool bAllowStrafe) const
{
    if (!Spline.IsValid())
        return Super::GetMoveFocus(bAllowStrafe);

    if (bAllowStrafe && DestinationActor.IsValid())
        return DestinationActor->GetActorLocation();
    else 
        if (MovementComp != nullptr)
            return MovementComp->GetActorLocation() + (CurrentMoveDirection * 20.0f);

    return FVector::ZeroVector;
}
