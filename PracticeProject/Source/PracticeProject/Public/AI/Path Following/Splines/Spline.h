// By Polyakov Pavel

#pragma once

/** Spline to move along */
struct PRACTICEPROJECT_API FSpline
{
	int32 CurrentPiece = 0;
    double CurrentParameter = 0;

    virtual FVector GetValue(int32 InPiece, double InParameter) const { check(0); return FVector(); }
    virtual int32 GetPieceNumber() const { check(0);  return 0; }

    FVector GetNextTargetLocation(const FVector& CurrentLocation, double PossibleDistance);
};
