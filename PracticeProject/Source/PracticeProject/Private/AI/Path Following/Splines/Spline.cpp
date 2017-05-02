// By Polyakov Pavel

#include "PracticeProject.h"
#include "Spline.h"

/** FSpline */

FVector FSpline::GetNextTargetLocation(const FVector& CurrentLocation, double PossibleDistance)
{
    static const int32 BinSearchIterations = 24;
    static const double Delta = 0.01;

    const double PossibleDistanceSquared = PossibleDistance * PossibleDistance;
    FVector TargetLocation;

    while (true)
    {
        double l = CurrentParameter, r = 1.0;
        for (int32 i = 0; i < BinSearchIterations; ++i)
        {
            CurrentParameter = (l + r) / 2;
            TargetLocation = GetValue(CurrentPiece, CurrentParameter);

            const FVector NearTargetLocation = GetValue(CurrentPiece, FMath::Min(1., CurrentParameter + Delta));
            const double Dist1 = FVector::DistSquaredXY(TargetLocation, CurrentLocation);
            const double Dist2 = FVector::DistSquaredXY(NearTargetLocation, CurrentLocation);

            if (Dist1 < PossibleDistanceSquared || Dist2 < Dist1)
                l = CurrentParameter;
            else
                r = CurrentParameter;
        }

        if (CurrentParameter < 0.999 || CurrentPiece == GetPieceNumber() - 1)
            break;

        CurrentParameter = 0.;
        ++CurrentPiece;
    }

    return TargetLocation;
}
